"""Render the original geometric HG stamp consistently for web and Windows."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Design'/'Brand'
OUT.mkdir(parents=True,exist_ok=True)
H=[(92,104),(140,104),(140,228),(216,228),(216,104),(264,104),(264,408),(216,408),(216,276),(140,276),(140,408),(92,408)]
G=[(326,104),(420,104),(420,152),(338,152),(338,360),(372,360),(372,278),(352,278),(352,230),(420,230),(420,408),(326,408),(290,372),(290,140)]
paths=[' '.join(f'{x},{y}' for x,y in p) for p in (H,G)]
svg='<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><title>Hungry Ghost Audio</title><rect width="512" height="512" rx="64" fill="#101714"/>'+''.join(f'<polygon points="{p}" fill="#e9e7dc"/>' for p in paths)+'</svg>\n'
(OUT/'hungry-ghost-mark.svg').write_text(svg)
im=Image.new('RGBA',(2048,2048),'#101714');d=ImageDraw.Draw(im)
for p in (H,G):d.polygon([(x*4,y*4) for x,y in p],fill='#e9e7dc')
im=im.resize((512,512),Image.Resampling.LANCZOS)
im.save(OUT/'hungry-ghost-mark.png')
im.save(OUT/'hungry-ghost.ico',sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
public=ROOT/'site'/'public'
(public/'favicon.svg').write_text(svg)
im.save(public/'favicon.ico',sizes=[(16,16),(32,32),(48,48)])
im.resize((180,180),Image.Resampling.LANCZOS).save(public/'apple-touch-icon.png')
(public/'assets'/'hungry-ghost-mark.svg').write_text(svg)
print('Created SVG master, 512px mark, Windows icon, favicon and touch icon')
