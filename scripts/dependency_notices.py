"""Retain JUCE and bundled third-party licence/copyright notices in downloads."""
import argparse,re
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('--juce',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
root=args.juce.resolve();sections=['Hungry Ghost Audio dependency notices\nJUCE 9.0.2 and bundled dependencies. Some notices cover platform-specific components not used by the Windows VST3 build. Full dependency source is included with our corresponding-source release.']
seen=set()
files=sorted(file for file in (root/'modules').rglob('*') if file.is_file())+[root/'LICENSE.md']
for file in files:
    name=file.name.lower();is_licence=any(token in name for token in ('license','licence','copying','ftl.txt'))
    if is_licence:
        blocks=[file.read_text(encoding='utf-8',errors='replace')]
    elif file.suffix.lower() in ('.h','.hpp','.c','.cpp'):
        # Copyright blocks supplied by the vendors are notices, not generated
        # summaries. Keep their wording, deduplicating repeated exact text.
        text=file.read_text(encoding='utf-8',errors='replace')[:32000]
        blocks=[block for block in re.findall(r'/\*.*?\*/',text,re.S) if re.search(r'copyright|redistribution|permission is hereby|licensed under',block,re.I)]
    else:continue
    for block in blocks:
        key=re.sub(r'\s+',' ',block).strip()
        if key and key not in seen:
            seen.add(key);sections.append('\n--- '+file.relative_to(root).as_posix()+' ---\n'+block.strip())
args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('\n\n'.join(sections)+'\n',encoding='utf-8')
print('Retained '+str(len(seen))+' distinct dependency licence/copyright notices.')
