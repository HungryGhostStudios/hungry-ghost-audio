"""Validate every catalogue VST3 and record hashes of the exact tested binaries."""
import argparse, concurrent.futures, hashlib, json, subprocess, time
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--build', required=True, type=Path)
parser.add_argument('--validator', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
parser.add_argument('--workers', type=int, default=3)
args = parser.parse_args()
catalogue = json.loads((Path(__file__).resolve().parents[1] / 'catalogue.json').read_text())
args.output.mkdir(parents=True, exist_ok=True)

def validate(product):
    name = 'Hungry Ghost' if product['id'] == 'reverb' else 'Hungry Ghost ' + product['name']
    bundle = args.build.resolve() / (product['name'] + '_artefacts') / 'Release' / 'VST3' / (name + '.vst3')
    binary = bundle / 'Contents' / 'x86_64-win' / (name + '.vst3')
    if not binary.is_file():
        return dict(id=product['id'], passed=False, error='Missing binary')
    before = hashlib.sha256(binary.read_bytes()).hexdigest()
    started = time.monotonic()
    try:
        with (args.output / (product['id'] + '.log')).open('w', encoding='utf-8') as log:
            run = subprocess.run([str(args.validator.resolve()), '--strictness-level', '5',
                '--random-seed', '2130', '--skip-gui-tests', '--timeout-ms', '60000',
                '--validate', str(bundle)], stdout=log, stderr=subprocess.STDOUT,
                timeout=600, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        after = hashlib.sha256(binary.read_bytes()).hexdigest()
        return dict(id=product['id'], name=name, version=product['version'],
            binary=str(binary), sha256=after, passed=run.returncode == 0 and before == after,
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
