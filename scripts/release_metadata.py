"""Read release versions from CMake, and reject stale catalogue metadata."""
import argparse
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def suite_version(root=ROOT):
    source = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    match = re.search(r'project\(HungryGhostSuite\s+VERSION\s+(\d+\.\d+\.\d+)\b', source)
    if not match:
        raise ValueError('Missing suite version in CMakeLists.txt')
    return match.group(1)


def product_version(product_id, root=ROOT):
    if product_id != 'reverb':
        return suite_version(root)
    source = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    match = re.search(r'set\(HG_REVERB_VERSION\s+(\d+\.\d+\.\d+)\s*\)', source)
    if not match:
        raise ValueError('Missing REVERB version in CMakeLists.txt')
    return match.group(1)


def catalogue(root=ROOT):
    products = json.loads((root / 'catalogue.json').read_text(encoding='utf-8'))
    if len(products) != 50 or len({p['id'] for p in products}) != 50:
        raise ValueError('Expected all 50 unique catalogue products')
    for product in products:
        if product['version'] != product_version(product['id'], root):
            raise ValueError('Catalogue/CMake version mismatch: ' + product['id'])
    return products


def verify_native_source(revision, root=ROOT):
    """Allow validation/gallery metadata edits while pinning all build inputs."""
    if not re.fullmatch(r'[0-9a-f]{40}', revision):
        raise ValueError('Expected a complete validated Git commit SHA')
    git = ['git', '-C', str(root)]
    subprocess.run(git + ['fetch', '--depth=1', 'origin', revision], check=True)
    native_paths = ['CMakeLists.txt', 'Products.cmake', 'Source', 'Tests', 'Assets',
                    'scripts/release_metadata.py']
    subprocess.run(git + ['diff', '--exit-code', revision, 'HEAD', '--', *native_paths], check=True)
    previous = json.loads(subprocess.check_output(git + ['show', revision + ':catalogue.json'], text=True))
    current = json.loads((root / 'catalogue.json').read_text(encoding='utf-8'))
    # Preserve array ordering and every other field, including IDs, versions,
    # controls, descriptions, engine kinds and prices.
    def build_fields(rows):
        return [{key: value for key, value in row.items() if key not in {'status', 'image'}} for row in rows]
    if build_fields(previous) != build_fields(current):
        raise ValueError('Catalogue build fields differ from validated source')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if args.check:
        products = catalogue()
        public = json.loads((ROOT / 'site/public/catalogue.json').read_text(encoding='utf-8'))
        if {(p['id'], p['version']) for p in public} != {(p['id'], p['version']) for p in products}:
            raise SystemExit('Storefront/catalogue version mismatch')
        print(f'Suite {suite_version()}: 50 native and storefront versions agree.')
    else:
        print(suite_version())
