"""Pack 24 Blender-lit rotations without rotating their baked lighting."""
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parent
frames=[root/'reel-frames'/f'reel-{i:02d}.png' for i in range(24)]
atlas=Image.new('RGBA',(384*6,384*4))
for i,path in enumerate(frames):
    image=Image.open(path).convert('RGBA')
    assert image.size==(384,384)
    atlas.paste(image,(i%6*384,i//6*384))
atlas.save(root.parents[1]/'Assets'/'reel-spool.png')
