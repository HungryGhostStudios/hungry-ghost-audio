"""Require successful Intel and Apple Silicon validation of identical binaries."""
import argparse, hashlib, json, plistlib
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--stage', type=Path, required=True)
p.add_argument('--intel', type=Path, required=True)
p.add_argument('--silicon', type=Path, required=True)
a = p.parse_args()
maps = []
for file, arch in [(a.intel, 'x86_64'), (a.silicon, 'arm64')]:
    records = json.loads(file.read_text(encoding='utf-8'))
    assert len(records) == 100 and all(r['passed'] and r['testedArchitecture'] == arch for r in records)
    by_key = {(r['id'], r['format']): r for r in records}
    assert len(by_key) == 100
    maps.append(by_key)
assert maps[0].keys() == maps[1].keys()
manifest = json.loads((a.stage / 'manifest.json').read_text(encoding='utf-8'))
for record in manifest['products']:
    key = (record['id'], record['format'])
    assert maps[0][key]['sha256'] == maps[1][key]['sha256'] == record['sha256']
    folder = 'VST3' if record['format'] == 'VST3' else 'Components'
    bundle = a.stage / 'Library/Audio/Plug-Ins' / folder / Path(record['bundle']).name
    with (bundle / 'Contents/Info.plist').open('rb') as f: info = plistlib.load(f)
    assert hashlib.sha256((bundle / 'Contents/MacOS' / info['CFBundleExecutable']).read_bytes()).hexdigest() == record['sha256']
print('All 100 exact universal bundles passed on Intel and Apple Silicon.')
