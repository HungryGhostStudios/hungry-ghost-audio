"""Package only binaries whose exact hashes passed the release validator."""
import argparse, hashlib, json, zipfile
from pathlib import Path
from release_metadata import catalogue, suite_version

parser=argparse.ArgumentParser()
parser.add_argument('--validation',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--work',type=Path,required=True)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
products=catalogue(root)
version=suite_version(root)
results=json.loads(args.validation.read_text())
verified={r['id']:r for r in results if r.get('passed') and r.get('strictness',0)>=5}
if set(verified)!=set(p['id'] for p in products):raise SystemExit('All 50 plugins must pass before packaging')
args.output.mkdir(parents=True,exist_ok=True);args.work.mkdir(parents=True,exist_ok=True)
bundle_entries=[];manifest=[]
for product in products:
    result=verified[product['id']];binary=Path(result['binary'])
    if result.get('version')!=product['version']:raise SystemExit('Validation version differs from current catalogue: '+product['id'])
    if hashlib.sha256(binary.read_bytes()).hexdigest()!=result['sha256']:raise SystemExit('Binary changed after validation: '+product['id'])
    bundle=binary.parents[2]
    entries=[(file,'VST3/'+bundle.name+'/'+file.relative_to(bundle).as_posix()) for file in sorted(bundle.rglob('*')) if file.is_file()]
    bundle_entries+=entries
    manifest.append(dict(id=product['id'],name=product['name'],version=product['version'],bundle=bundle.name,binarySHA256=result['sha256'],validator='pluginval 1.0.4',strictness=5,seed=2130))

docs=[(root/'LICENSE','LICENSE.txt'),(root/'NOTICE','NOTICE.txt'),(root/'Docs'/'ThirdPartyNotices.txt','ThirdPartyNotices.txt'),(root/'Docs'/'UserGuide.md','UserGuide.md')]
def package(path,entries):
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        for file,name in entries+docs:archive.write(file,name)
        archive.writestr('SOURCE.txt',f'Complete corresponding source: https://github.com/HungryGhostStudios/hungry-ghost-audio/releases/tag/v{version}\nThe source archive includes the pinned JUCE framework and editable interface assets. AGPL-3.0-or-later.\n')
package(args.output/f'HungryGhostSuite-{version}-Windows-VST3.zip',bundle_entries)
for product in products:
    name=verified[product['id']]['name']+'.vst3'
    package(args.output/('HungryGhost-'+product['name']+'-'+product['version']+'-Windows-VST3.zip'),[(file,path) for file,path in bundle_entries if path.startswith('VST3/'+name+'/')])
(args.work/'installer-hashes.tsv').write_text(''.join(path+'\t'+hashlib.sha256(file.read_bytes()).hexdigest()+'\n' for file,path in bundle_entries),encoding='utf-8')
(args.output/'manifest.json').write_text(json.dumps(dict(suiteVersion=version,platform='Windows x64',format='VST3',products=manifest),indent=2)+'\n')
print('Packaged 50 individual downloads and complete suite; every binary matches its validation hash.',flush=True)
