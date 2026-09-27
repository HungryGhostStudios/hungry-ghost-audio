"""Create the complete corresponding-source archive, including pinned JUCE."""
import argparse, hashlib, json, subprocess, zipfile
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('--juce',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
def tracked(folder,additional=False):
 command=['git','-C',str(folder),'ls-files','-z']
 if additional:command+=['--cached','--others','--exclude-standard']
 return sorted(set(subprocess.check_output(command).decode().strip('\0').split('\0')))
juce_version=subprocess.check_output(['git','-C',str(args.juce),'describe','--tags','--exact-match'],text=True).strip()
if juce_version!='9.0.2':raise SystemExit('Expected official JUCE 9.0.2 tag')
args.output.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(args.output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
 for name in tracked(root,True):
  file=root/name
  if file.is_file() and file.suffix.lower() not in ('.pem','.key','.pfx') and not name.startswith('releases/'):
   archive.write(file,'HungryGhostSuite/'+name)
 for name in tracked(args.juce):
  file=args.juce/name
  if file.is_file():archive.write(file,'HungryGhostSuite/vendor/JUCE/'+name)
 archive.writestr('HungryGhostSuite/SOURCE-RELEASE.txt','Hungry Ghost Audio suite 0.1.0. Complete AGPLv3 corresponding source with official JUCE 9.0.2. CMake automatically uses vendor/JUCE when present. See README.md and LICENSE.\n')
print('Full corresponding source archive created with JUCE '+juce_version,flush=True)
