from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[1]
for original,folder in [('AFTER','Reverb'),('FERAL','Feral')]:
 source=root.parent/original/'Source'
 dest=root/'Source'/'Originals'/folder
 dest.mkdir(parents=True,exist_ok=True)
 for path in source.iterdir():
  if path.suffix in ('.h','.cpp'):
   text=path.read_text(encoding='utf-8')
   if path.name=='PluginProcessor.cpp':
    start=text.index('juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()')
    text=text[:start]+'#ifndef HG_SUITE_BUILD\n'+text[start:]+'\n#endif\n'
   (dest/path.name).write_text(text,encoding='utf-8')
 # Editor references UI/GhostTheme relative to its own folder.
 (dest/'UI').mkdir(exist_ok=True)
 for path in (source/'UI').glob('*.h'):
  (dest/'UI'/path.name).write_text('#pragma once\n#include "../../../UI/'+path.name+'"\n')
 print('Imported',folder,'without changing its host state or IDs.')
