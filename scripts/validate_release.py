"""Validate every catalogue VST3 and record hashes of the exact tested binaries."""
import argparse, concurrent.futures, ctypes, hashlib, json, os, re, subprocess, time
from pathlib import Path
from release_metadata import catalogue as read_catalogue, bundle_name

def windows_product_version(binary):
    """Read the PE ProductVersion string without loading executable code."""
    if os.name != 'nt':
        raise ValueError('Windows PE version preflight requires Windows')
    from ctypes import wintypes
    api = ctypes.WinDLL('version', use_last_error=True)
    api.GetFileVersionInfoSizeW.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(wintypes.DWORD)]
    api.GetFileVersionInfoSizeW.restype = wintypes.DWORD
    api.GetFileVersionInfoW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, wintypes.LPVOID]
    api.GetFileVersionInfoW.restype = wintypes.BOOL
    api.VerQueryValueW.argtypes = [wintypes.LPCVOID, wintypes.LPCWSTR,
                                 ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(wintypes.UINT)]
    api.VerQueryValueW.restype = wintypes.BOOL
    path = str(binary.resolve())
    unused = wintypes.DWORD()
    size = api.GetFileVersionInfoSizeW(path, ctypes.byref(unused))
    if not size:
        raise ValueError('Missing or unreadable Windows PE version resource')
    resource = ctypes.create_string_buffer(size)
    if not api.GetFileVersionInfoW(path, 0, size, resource):
        raise ValueError('Cannot read Windows PE version resource')

    def query(key):
        pointer, length = ctypes.c_void_p(), wintypes.UINT()
        if not api.VerQueryValueW(resource, key, ctypes.byref(pointer), ctypes.byref(length)):
            return None, 0
        return pointer.value, length.value

    pointer, length = query('\\VarFileInfo\\Translation')
    if not pointer or length < 4 or length % 4:
        raise ValueError('Missing Windows PE version string-table translation')
    translations = ctypes.cast(pointer, ctypes.POINTER(wintypes.WORD))
    versions = set()
    for offset in range(0, length // 2, 2):
        language, codepage = translations[offset], translations[offset + 1]
        pointer, count = query(f'\\StringFileInfo\\{language:04x}{codepage:04x}\\ProductVersion')
        if not pointer or not count:
            raise ValueError('Missing Windows PE ProductVersion string')
        versions.add(ctypes.wstring_at(pointer, count).rstrip('\0').strip())
    if len(versions) != 1 or not next(iter(versions)):
        raise ValueError('Missing or inconsistent Windows PE ProductVersion strings')
    return next(iter(versions))


def preflight_versions(binary, module_info, expected):
    # A fresh moduleinfo.json can coexist with a stale JUCE generated RC file.
    # Both version surfaces must match before pluginval can certify the binary.
    found = re.search(r'"Version"\s*:\s*"([^"]+)"', module_info.read_text(encoding='utf-8')) if module_info.is_file() else None
    if not found or found.group(1) != expected:
        raise ValueError('Built VST3 moduleinfo version differs from catalogue')
    actual = windows_product_version(binary)
    if actual != expected:
        raise ValueError(f'Windows PE ProductVersion {actual!r} differs from catalogue {expected!r}. '
                         'Use a fresh build directory or regenerate JUCE generated RC resources '
                         'from current Info.txt and relink all affected plugins; then revalidate.')
    return actual

def validate(product, args):
    name = bundle_name(product)
    bundle = args.build.resolve() / (product['name'] + '_artefacts') / 'Release' / 'VST3' / (name + '.vst3')
    binary = bundle / 'Contents' / 'x86_64-win' / (name + '.vst3')
    if not binary.is_file():
        return dict(id=product['id'], passed=False, error='Missing binary')
    # Include the metadata preflight in the same unchanged-bytes check as pluginval.
    before = hashlib.sha256(binary.read_bytes()).hexdigest()
    module_info = bundle / 'Contents/Resources/moduleinfo.json'
    # JUCE's manifest permits trailing commas; preflight reads its version field only.
    try:
        pe_version = preflight_versions(binary, module_info, product['version'])
    except (OSError, ValueError) as error:
        return dict(id=product['id'], passed=False, error=str(error))
    started = time.monotonic()
    try:
        with (args.output / (product['id'] + '.log')).open('w', encoding='utf-8') as log:
            run = subprocess.run([str(args.validator.resolve()), '--strictness-level', '5',
                '--random-seed', '2130', '--skip-gui-tests', '--timeout-ms', '60000',
                '--validate', str(bundle)], stdout=log, stderr=subprocess.STDOUT,
                timeout=600, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        after = hashlib.sha256(binary.read_bytes()).hexdigest()
        completed = (args.output / (product['id'] + '.log')).read_text(encoding='utf-8', errors='replace').strip().endswith('SUCCESS')
        return dict(id=product['id'], name=name, version=product['version'], peProductVersion=pe_version,
            binary=str(binary), sha256=after, passed=run.returncode == 0 and before == after and completed,
            exitCode=run.returncode, seconds=round(time.monotonic()-started, 2), strictness=5,
            seed=2130, guiTests=False)
    except subprocess.TimeoutExpired:
        return dict(id=product['id'], passed=False, error='Validation timeout')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', required=True, type=Path)
    parser.add_argument('--validator', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--workers', type=int, default=3)
    args = parser.parse_args()
    catalogue = read_catalogue()
    args.output.mkdir(parents=True, exist_ok=True)
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        for result in pool.map(lambda product: validate(product, args), catalogue):
            results.append(result)
            (args.output / 'results.json').write_text(json.dumps(results, indent=2)+'\n')
            detail = ': ' + result['error'] if result.get('error') else ''
            print(('PASS' if result['passed'] else 'FAIL') + ': ' + result['id'] + detail, flush=True)
    failed = [r['id'] for r in results if not r['passed']]
    print(f'{len(results)-len(failed)}/{len(catalogue)} passed; failures: {failed}', flush=True)
    return bool(failed)


if __name__ == '__main__':
    raise SystemExit(main())
