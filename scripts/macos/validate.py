"""Check both slices and validate all VST3/Audio Unit bundles on the running Mac."""
import argparse, concurrent.futures, hashlib, json, os, platform, plistlib, shutil, subprocess, sys, threading, time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from release_metadata import catalogue, bundle_name

p = argparse.ArgumentParser()
p.add_argument('--build', type=Path)
p.add_argument('--stage', type=Path)
p.add_argument('--validator', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--workers', type=int, default=2)
p.add_argument('--system-au-install', action='store_true', help='Use the system Components folder on disposable GitHub CI Macs only')
a = p.parse_args()
if bool(a.build) == bool(a.stage): p.error('Supply either --build or --stage')
if a.system_au_install and os.environ.get('GITHUB_ACTIONS') != 'true':
    p.error('--system-au-install is restricted to disposable GitHub Actions runners')
root = Path(__file__).resolve().parents[2]
products = catalogue(root)
a.output.mkdir(parents=True, exist_ok=True)
inventory = []
for product in products:
    name = bundle_name(product)
    for fmt, ext in [('VST3', 'vst3'), ('AU', 'component')]:
        bundle = ((a.build / (product['name'] + '_artefacts') / 'Release' / fmt) if a.build
                  else (a.stage / 'Library/Audio/Plug-Ins' / ('VST3' if fmt == 'VST3' else 'Components'))) / (name + '.' + ext)
        bundle = bundle.resolve()
        with (bundle / 'Contents/Info.plist').open('rb') as f: info = plistlib.load(f)
        assert info.get('CFBundleShortVersionString') == product['version'], (product['id'], fmt, 'Built version differs from catalogue')
        binary = bundle / 'Contents/MacOS' / info['CFBundleExecutable']
        arches = subprocess.check_output(['lipo', '-archs', str(binary)], text=True).split()
        assert set(arches) == {'arm64', 'x86_64'}, (product['id'], fmt, arches)
        subprocess.run(['codesign', '--verify', '--strict', str(bundle)], check=True)
        record = dict(id=product['id'], name=product['name'], version=product['version'], format=fmt,
                      bundle=str(bundle), binary=str(binary), sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                      architectures=arches, testedArchitecture=platform.machine())
        if fmt == 'AU':
            component = info['AudioComponents'][0]
            assert component['manufacturer'] == 'Aftr'
            record['audioComponent'] = {k: component[k] for k in ('type', 'subtype', 'manufacturer')}
            destination = (Path('/Library/Audio/Plug-Ins/Components') if a.system_au_install
                           else Path.home() / 'Library/Audio/Plug-Ins/Components') / bundle.name
            if a.system_au_install:
                subprocess.run(['sudo', 'ditto', str(bundle), str(destination)], check=True)
            else:
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copytree(bundle, destination, dirs_exist_ok=True, symlinks=True)
            installed = destination / 'Contents/MacOS' / info['CFBundleExecutable']
            assert hashlib.sha256(installed.read_bytes()).hexdigest() == record['sha256']
            subprocess.run(['codesign', '--verify', '--strict', str(destination)], check=True)
        inventory.append(record)

# auval resolves component identifiers through Apple's registrar, rather than a
# bundle path. Wait for all installed components to appear before testing DSP.
if os.environ.get('GITHUB_ACTIONS') == 'true':
    subprocess.run(['sudo', 'killall', '-9', 'AudioComponentRegistrar'], capture_output=True)
expected = {tuple(r['audioComponent'][k] for k in ('type', 'subtype', 'manufacturer'))
            for r in inventory if r['format'] == 'AU'}
assert len(expected) == len(products), 'Audio Unit identifiers must be unique'
for attempt in range(6):
    scan = subprocess.run(['auval', '-a'], capture_output=True, text=True, timeout=120)
    (a.output / f'au-registry-{attempt}.log').write_text(scan.stdout + scan.stderr, encoding='utf-8')
    discovered = {tuple(line.split()[:3]) for line in scan.stdout.splitlines() if len(line.split()) >= 3}
    missing = expected - discovered
    if not missing:
        break
    time.sleep(2)
assert not missing, 'Audio Units missing from Apple registrar: ' + repr(sorted(missing))
au_lock = threading.Lock()

def validate(record):
    record = dict(record)
    log = a.output / (record['id'] + '-' + record['format'] + '.log')
    if record['format'] == 'VST3':
        command = [str(a.validator.resolve()), '--strictness-level', '5', '--random-seed', '2130',
                   '--skip-gui-tests', '--timeout-ms', '60000', '--validate', record['bundle']]
        record.update(validator='pluginval 1.0.4', strictness=5, seed=2130)
    else:
        c = record['audioComponent']
        command = ['auval', '-v', c['type'], c['subtype'], c['manufacturer']]
        record.update(validator='Apple auval')
    try:
        with log.open('w', encoding='utf-8') as f:
            if record['format'] == 'AU':
                with au_lock:
                    run = subprocess.run(command, stdout=f, stderr=subprocess.STDOUT, timeout=600)
            else:
                run = subprocess.run(command, stdout=f, stderr=subprocess.STDOUT, timeout=600)
        record['exitCode'] = run.returncode
        record['passed'] = run.returncode == 0 and hashlib.sha256(Path(record['binary']).read_bytes()).hexdigest() == record['sha256']
        if record['format'] == 'AU': record['passed'] &= 'AU VALIDATION SUCCEEDED' in log.read_text(encoding='utf-8', errors='replace')
    except subprocess.TimeoutExpired:
        record.update(passed=False, error='Validation timed out')
    return record

results = []
with concurrent.futures.ThreadPoolExecutor(max_workers=a.workers) as pool:
    for record in pool.map(validate, inventory):
        results.append(record)
        (a.output / 'results.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
        print(('PASS' if record['passed'] else 'FAIL') + ': ' + record['id'] + ' / ' + record['format'], flush=True)
assert len(results) == 2*len(products)
failed = [r for r in results if not r['passed']]
print(f'{2*len(products)-len(failed)}/{2*len(products)} passed on {platform.machine()}', flush=True)
raise SystemExit(bool(failed))
