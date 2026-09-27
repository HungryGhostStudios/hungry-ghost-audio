"""Pack original Blender frames into the lossless JUCE texture atlas."""
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parent
atlas = Image.new('RGBA', (256 * 12, 256 * 8))
for index in range(96):
    with Image.open(root / 'frames' / f'dial-{index:03d}.png') as frame:
        assert frame.size == (256, 256)
        atlas.paste(frame, ((index % 12) * 256, (index // 12) * 256))
target = root.parent.parent / 'Assets' / 'monolith-dial.png'
atlas.save(target, optimize=True)
print(f'{target}: {target.stat().st_size:,} bytes / 96 frames')
