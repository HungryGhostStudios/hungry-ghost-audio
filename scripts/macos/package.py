"""Build Apple's selectable installer from validated universal plugin bundles."""
import argparse, hashlib, json, plistlib, shutil, subprocess, sys, tempfile, xml.etree.ElementTree as ET
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from release_metadata import suite_version

p = argparse.ArgumentParser()
p.add_argument('--stage', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--installer-identity')
p.add_argument('--application-identity')
p.add_argument('--notary-profile')
p.add_argument('--notary-keychain', type=Path)
p.add_argument('--preview', action='store_true')
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
if not a.preview and not all([a.installer_identity, a.application_identity, a.notary_profile]):
    p.error('A public installer requires Application/Installer identities and a notary profile')
manifest = json.loads((a.stage / 'manifest.json').read_text(encoding='utf-8'))
assert len(manifest['products']) == 100 and all(r['passed'] for r in manifest['products'])
assert manifest['suiteVersion'] == suite_version(root), 'Staged release version differs from source'
a.output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='hungryghost-mac-') as scratch:
    scratch = Path(scratch)
    packages = scratch / 'packages'; packages.mkdir()
    resources = scratch / 'resources'; resources.mkdir()
    shutil.copy2(root / 'Design/Brand/hungry-ghost-mark.png', resources / 'mark.png')
    (resources / 'welcome.html').write_text('''<!doctype html><html><head><meta charset="utf-8"><style>
      body{font:14px -apple-system,Helvetica,sans-serif;color:#16231e;padding:24px}img{width:64px;float:left;margin-right:18px}
      h1{font-size:24px;margin-top:10px}p{line-height:1.5}h2{font-size:17px}</style></head><body>
      <img src="mark.png"><h1>Hungry Ghost Audio</h1><p>50 effects. One common language.</p><h2>Before you install</h2>
      <p>Close your audio apps. Use Customize to choose your plugins and VST3 / Audio Unit formats.</p>
      <p>Universal plugins run natively on Apple Silicon and Intel, on macOS 11 or later. Audio Units support Logic Pro;
      VST3 supports REAPER and other compatible hosts. Your existing purchase includes both platforms.</p>
      <p>Plugins are installed for all users in /Library/Audio/Plug-Ins. Existing project settings and licence files are preserved.</p>
      <p>Full AGPLv3 source and support: https://hungryghostaudio.com</p></body></html>''', encoding='utf-8')
    dist = ET.Element('installer-gui-script', {'minSpecVersion': '2'})
    ET.SubElement(dist, 'title').text = 'Hungry Ghost Audio'
    ET.SubElement(dist, 'welcome', {'file': 'welcome.html', 'mime-type': 'text/html'})
    ET.SubElement(dist, 'options', {'customize': 'always', 'require-scripts': 'false',
                                 'hostArchitectures': 'arm64,x86_64', 'rootVolumeOnly': 'true'})
    domains = ET.SubElement(dist, 'domains', {'enable_localSystem': 'true', 'enable_currentUserHome': 'false', 'enable_anywhere': 'false'})
    volume = ET.SubElement(dist, 'volume-check')
    ET.SubElement(ET.SubElement(volume, 'allowed-os-versions'), 'os-version', {'min': '11.0'})
    outline = ET.SubElement(dist, 'choices-outline')
    top = ET.SubElement(outline, 'line', {'choice': 'suite'})
    ET.SubElement(dist, 'choice', {'id': 'suite', 'title': 'Hungry Ghost Audio Suite', 'visible': 'false', 'start_selected': 'true'})
    for fmt in ['VST3', 'AU']:
        branch = ET.SubElement(top, 'line', {'choice': fmt})
        ET.SubElement(dist, 'choice', {'id': fmt, 'title': 'VST3' if fmt == 'VST3' else 'Audio Units (Logic Pro)', 'start_selected': 'true'})
        for record in [r for r in manifest['products'] if r['format'] == fmt]:
            folder = 'VST3' if fmt == 'VST3' else 'Components'
            original_bundle = Path(record['bundle'])
            bundle = a.stage / 'Library/Audio/Plug-Ins' / folder / original_bundle.name
            with (bundle / 'Contents/Info.plist').open('rb') as f: info = plistlib.load(f)
            binary = bundle / 'Contents/MacOS' / info['CFBundleExecutable']
            assert hashlib.sha256(binary.read_bytes()).hexdigest() == record['sha256'], record['id']
            subprocess.run(['codesign', '--verify', '--strict', str(bundle)], check=True)
            if not a.preview:
                signature = subprocess.run(['codesign', '-d', '--verbose=4', str(bundle)], capture_output=True, text=True, check=True).stderr
                assert a.application_identity in signature and 'runtime' in signature, record['id']
            choice_id = record['id'] + '.' + fmt.lower()
            package_id = 'audio.hungryghost.' + choice_id
            package_name = choice_id + '.pkg'
            payload = scratch / choice_id / 'root'
            payload.mkdir(parents=True)
            shutil.copytree(bundle, payload / bundle.name, symlinks=True)
            components = scratch / choice_id / 'components.plist'
            subprocess.run(['pkgbuild', '--analyze', '--root', str(payload), str(components)], check=True)
            with components.open('rb') as f: entries = plistlib.load(f)
            for entry in entries:
                entry.update(BundleIsRelocatable=False, BundleHasStrictIdentifier=True, BundleOverwriteAction='upgrade')
            with components.open('wb') as f: plistlib.dump(entries, f)
            subprocess.run(['pkgbuild', '--root', str(payload), '--component-plist', str(components),
                            '--identifier', package_id, '--version', record['version'],
                            '--install-location', '/Library/Audio/Plug-Ins/' + folder, str(packages / package_name)], check=True)
            # The component package now owns the payload. Do not retain 100
            # duplicate staging trees alongside the original universal bundles.
            assert payload.parent.resolve().parent == scratch.resolve()
            shutil.rmtree(payload.parent)
            ET.SubElement(branch, 'line', {'choice': choice_id})
            choice = ET.SubElement(dist, 'choice', {'id': choice_id, 'title': record['name'], 'start_selected': 'true'})
            ET.SubElement(choice, 'pkg-ref', {'id': package_id})
            ET.SubElement(dist, 'pkg-ref', {'id': package_id, 'version': record['version']}).text = package_name
    distribution = scratch / 'distribution.xml'
    ET.ElementTree(dist).write(distribution, encoding='utf-8', xml_declaration=True)
    name = 'HungryGhostSuite-' + manifest['suiteVersion'] + '-macOS-Universal' + ('-Preview' if a.preview else '') + '.pkg'
    package = a.output.resolve() / name
    command = ['productbuild', '--distribution', str(distribution), '--resources', str(resources),
               '--package-path', str(packages)]
    if not a.preview: command += ['--sign', a.installer_identity, '--timestamp']
    subprocess.run(command + [str(package)], check=True)
    if not a.preview:
        subprocess.run(['pkgutil', '--check-signature', str(package)], check=True)
        command = ['xcrun', 'notarytool', 'submit', str(package), '--keychain-profile', a.notary_profile,
                   '--wait', '--timeout', '30m', '--output-format', 'json']
        if a.notary_keychain: command += ['--keychain', str(a.notary_keychain.resolve())]
        notarization = subprocess.check_output(command, text=True)
        result = json.loads(notarization)
        (a.output / 'notarization.json').write_text(notarization, encoding='utf-8')
        assert result.get('status') == 'Accepted', result.get('status')
        subprocess.run(['xcrun', 'stapler', 'staple', str(package)], check=True)
        subprocess.run(['xcrun', 'stapler', 'validate', str(package)], check=True)
        subprocess.run(['spctl', '--assess', '--type', 'install', '--verbose=4', str(package)], check=True)
    shutil.copy2(distribution, a.output / 'macOS-distribution.xml')
    manifest['signing'] = 'unsigned preview' if a.preview else 'Developer ID signed, notarized and stapled'
    manifest['installerSHA256'] = hashlib.sha256(package.read_bytes()).hexdigest()
    (a.output / 'macOS-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print('Created ' + str(package), flush=True)
