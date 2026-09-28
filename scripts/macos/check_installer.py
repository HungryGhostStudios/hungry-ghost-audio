"""Exercise Apple's actual installer choices and selected-plugin installation."""
import argparse, hashlib, json, plistlib, subprocess
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--package', type=Path, required=True)
p.add_argument('--manifest', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
manifest = json.loads(a.manifest.read_text(encoding='utf-8'))
package = a.package.resolve()
choices = plistlib.loads(subprocess.check_output(['installer', '-showChoicesXML', '-pkg', str(package)]))
ids = {c['choiceIdentifier'] for c in choices}
required = {r['id'] + '.' + r['format'].lower() for r in manifest['products']}
assert required <= ids and len(required) == 100
changes = [dict(choiceIdentifier=identifier, choiceAttribute='selected', attributeSetting=int(identifier.split('.')[0] in {'crush', 'reel'})) for identifier in sorted(required)]
file = a.output / 'selected-plugins.plist'
with file.open('wb') as f: plistlib.dump(changes, f)
after = subprocess.check_output(['installer', '-showChoicesAfterApplyingChangesXML', str(file), '-pkg', str(package), '-target', '/'])
(a.output / 'applied-choices.plist').write_bytes(after)
selected = {c['choiceIdentifier'] for c in plistlib.loads(after) if c['choiceAttribute'] == 'selected' and c['attributeSetting'] and c['choiceIdentifier'] in required}
assert selected == {'crush.vst3', 'crush.au', 'reel.vst3', 'reel.au'}, selected
subprocess.run(['sudo', 'installer', '-pkg', str(package), '-target', '/', '-applyChoiceChangesXML', str(file)], check=True)
for record in manifest['products']:
    if record['id'] not in {'crush', 'reel'}: continue
    folder = 'VST3' if record['format'] == 'VST3' else 'Components'
    bundle = Path('/Library/Audio/Plug-Ins') / folder / Path(record['bundle']).name
    with (bundle / 'Contents/Info.plist').open('rb') as f: info = plistlib.load(f)
    binary = bundle / 'Contents/MacOS' / info['CFBundleExecutable']
    assert hashlib.sha256(binary.read_bytes()).hexdigest() == record['sha256']
    subprocess.run(['codesign', '--verify', '--strict', str(bundle)], check=True)
print('Apple Installer exposed 100 format/plugin choices and installed exactly the four selected bundles with verified hashes.')
