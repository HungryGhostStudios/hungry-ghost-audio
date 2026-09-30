"""Stage the exact Mac binaries which passed the native host validators."""
import argparse, hashlib, json, shutil, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from release_metadata import catalogue, suite_version
p = argparse.ArgumentParser()
p.add_argument('--validation', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
records = json.loads(a.validation.read_text(encoding='utf-8'))
assert len(records) == 100 and all(r['passed'] for r in records)
assert len({(r['id'], r['format']) for r in records}) == 100
versions = {product['id']: product['version'] for product in catalogue()}
for record in records:
    assert record['version'] == versions[record['id']], 'Validation version differs from catalogue: ' + record['id']
    assert hashlib.sha256(Path(record['binary']).read_bytes()).hexdigest() == record['sha256']
    source = Path(record['bundle'])
    folder = 'VST3' if record['format'] == 'VST3' else 'Components'
    destination = a.output / 'Library/Audio/Plug-Ins' / folder / source.name
    shutil.copytree(source, destination, dirs_exist_ok=True, symlinks=True)
manifest = dict(suiteVersion=suite_version(), platform='macOS 11+, Apple Silicon and Intel',
                formats=['VST3', 'AU'], signing='validated build; distribution signing checked during packaging', products=records)
a.output.mkdir(parents=True, exist_ok=True)
(a.output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
print('Staged all 100 validated universal bundles.', flush=True)
