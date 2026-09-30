"""Validate every catalogue VST3 and record hashes of the exact tested binaries."""
import argparse, concurrent.futures, hashlib, json, re, subprocess, time
from pathlib import Path
from release_metadata import catalogue as read_catalogue

parser = argparse.ArgumentParser()
parser.add_argument('--build', required=True, type=Path)
parser.add_argument('--validator', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
parser.add_argument('--workers', type=int, default=3)
args = parser.parse_args()
catalogue = read_catalogue()
args.output.mkdir(parents=True, exist_ok=True)

def validate(product):
    name = 'Hungry Ghost' if product['id'] == 'reverb' else 'Hungry Ghost ' + product['name']
    bundle = args.build.resolve() / (product['name'] + '_artefacts') / 'Release' / 'VST3' / (name + '.vst3')
    binary = bundle / 'Contents' / 'x86_64-win' / (name + '.vst3')
    if not binary.is_file():
        return dict(id=product['id'], passed=False, error='Missing binary')
    module_info = bundle / 'Contents/Resources/moduleinfo.json'
    # JUCE's manifest permits trailing commas, so only read its version field.
    found = re.search(r'"Version"\s*:\s*"([^"]+)"', module_info.read_text(encoding='utf-8')) if module_info.is_file() else None
    if not found or found.group(1) != product['version']:
        return dict(id=product['id'], passed=False, error='Built VST3 version differs from catalogue')
    before = hashlib.sha256(binary.read_bytes()).hexdigest()
    started = time.monotonic()
    try:
        with (args.output / (product['id'] + '.log')).open('w', encoding='utf-8') as log:
            run = subprocess.run([str(args.validator.resolve()), '--strictness-level', '5',
                '--random-seed', '2130', '--skip-gui-tests', '--timeout-ms', '60000',
                '--validate', str(bundle)], stdout=log, stderr=subprocess.STDOUT,
                timeout=600, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        after = hashlib.sha256(binary.read_bytes()).hexdigest()
        completed = (args.output / (product['id'] + '.log')).read_text(encoding='utf-8', errors='replace').strip().endswith('SUCCESS')
        return dict(id=product['id'], name=name, version=product['version'],
            binary=str(binary), sha256=after, passed=run.returncode == 0 and before == after and completed,
            exitCode=run.returncode, seconds=round(time.monotonic()-started, 2), strictness=5,
            seed=2130, guiTests=False)
    except subprocess.TimeoutExpired:
        return dict(id=product['id'], passed=False, error='Validation timeout')

results=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
    for result in pool.map(validate, catalogue):
        results.append(result)
        (args.output / 'results.json').write_text(json.dumps(results, indent=2)+'\n')
        print(('PASS' if result['passed'] else 'FAIL') + ': ' + result['id'], flush=True)
failed = [r['id'] for r in results if not r['passed']]
print(f'{len(results)-len(failed)}/{len(catalogue)} passed; failures: {failed}', flush=True)
raise SystemExit(bool(failed))
