"""Stage the exact Mac binaries which passed the native host validators."""
import argparse, hashlib, json, shutil
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--validation', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
records = json.loads(a.validation.read_text(encoding='utf-8'))
assert len(records) == 100 and all(r['passed'] for r in records)
assert len({(r['id'], r['format']) for r in records}) == 100
for record in records:
    assert hashlib.sha256(Path(record['binary']).read_bytes()).hexdigest() == record['sha256']
    source = Path(record['bundle'])
    folder = 'VST3' if record['format'] == 'VST3' else 'Components'
    destination = a.output / 'Library/Audio/Plug-Ins' / folder / source.name
    shutil.copytree(source, destination, dirs_exist_ok=True, symlinks=True)
manifest = dict(suiteVersion='0.2.0', platform='macOS 11+, Apple Silicon and Intel',
                formats=['VST3', 'AU'], signing='validated build; distribution signing checked during packaging', products=records)
a.output.mkdir(parents=True, exist_ok=True)
(a.output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
print('Staged all 100 validated universal bundles.', flush=True)
