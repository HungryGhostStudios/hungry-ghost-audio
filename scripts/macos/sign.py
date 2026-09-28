"""Import CI certificates into an ephemeral keychain and sign plugin bundles."""
import argparse, base64, hashlib, json, os, plistlib, secrets, subprocess
from pathlib import Path

def run(command, private=False, **kwargs):
    # CalledProcessError includes every command argument, including P12 and
    # keychain passwords. Keep credential failures out of public CI logs.
    if private:
        kwargs['stdout'] = subprocess.DEVNULL
        kwargs['stderr'] = subprocess.DEVNULL
    result = subprocess.run(command, **kwargs)
    if result.returncode:
        raise RuntimeError('Signing operation failed: ' + Path(command[0]).name + ' ' + command[1]
                           + ' (exit ' + str(result.returncode) + ')')
    return result

p = argparse.ArgumentParser()
p.add_argument('--stage', type=Path, required=True)
p.add_argument('--keychain', type=Path, required=True)
p.add_argument('--scratch', type=Path, required=True)
p.add_argument('--import-only', action='store_true')
a = p.parse_args()
required = ['MACOS_APPLICATION_P12', 'MACOS_INSTALLER_P12', 'MACOS_CERTIFICATE_PASSWORD',
            'MACOS_APPLICATION_IDENTITY', 'MACOS_INSTALLER_IDENTITY']
assert all(os.environ.get(k) for k in required), 'Missing Apple certificate secrets'
assert os.environ['MACOS_APPLICATION_IDENTITY'].startswith('Developer ID Application:')
assert os.environ['MACOS_INSTALLER_IDENTITY'].startswith('Developer ID Installer:')
a.scratch.mkdir(parents=True, exist_ok=True)
password = secrets.token_hex(24)
keychain = a.keychain.resolve()
run(['security', 'create-keychain', '-p', password, str(keychain)], private=True)
run(['security', 'set-keychain-settings', '-lut', '21600', str(keychain)], private=True)
run(['security', 'unlock-keychain', '-p', password, str(keychain)], private=True)
run(['security', 'list-keychains', '-d', 'user', '-s', str(keychain)], private=True)
for variable in ['MACOS_APPLICATION_P12', 'MACOS_INSTALLER_P12']:
    file = a.scratch / (variable + '.p12')
    file.write_bytes(base64.b64decode(os.environ[variable], validate=True)); file.chmod(0o600)
    try:
        certificate_password = (os.environ.get('MACOS_INSTALLER_CERTIFICATE_PASSWORD')
                                if variable == 'MACOS_INSTALLER_P12' else None) or os.environ['MACOS_CERTIFICATE_PASSWORD']
        run(['security', 'import', str(file), '-k', str(keychain), '-P', certificate_password,
             '-T', '/usr/bin/codesign', '-T', '/usr/bin/productbuild', '-T', '/usr/bin/productsign'], private=True)
    finally:
        file.unlink()
run(['security', 'set-key-partition-list', '-S', 'apple-tool:,apple:,codesign:', '-s', '-k', password, str(keychain)], private=True)
if a.import_only:
    print('Imported certificates into the temporary CI keychain.')
    raise SystemExit(0)
count = 0
for folder, extension in [('VST3', '*.vst3'), ('Components', '*.component')]:
    for bundle in (a.stage / 'Library/Audio/Plug-Ins' / folder).glob(extension):
        subprocess.run(['codesign', '--force', '--sign', os.environ['MACOS_APPLICATION_IDENTITY'], '--keychain', str(keychain),
                        '--timestamp', '--options', 'runtime', str(bundle)], check=True)
        subprocess.run(['codesign', '--verify', '--strict', str(bundle)], check=True)
        with (bundle / 'Contents/Info.plist').open('rb') as f: info = plistlib.load(f)
        binary = bundle / 'Contents/MacOS' / info['CFBundleExecutable']
        assert set(subprocess.check_output(['lipo', '-archs', str(binary)], text=True).split()) == {'arm64', 'x86_64'}
        count += 1
assert count == 100, count
print('Imported certificates into the temporary CI keychain and signed all universal bundles.')
