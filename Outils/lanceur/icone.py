"""Icône de Palmera Racing : un damier d'arrivée sur fond rouge."""
from pathlib import Path

from PIL import Image, ImageDraw

cote = 256
im = Image.new("RGBA", (cote, cote), (0, 0, 0, 0))
d = ImageDraw.Draw(im)
d.rounded_rectangle((8, 8, cote - 8, cote - 8), 44, fill=(205, 30, 36, 255))
case = 28
for i in range(6):
    for j in range(5):
        x, y = 44 + i * case, 56 + j * case
        d.rectangle((x, y, x + case, y + case), fill=(250, 250, 248, 255) if (i + j) % 2 == 0 else (18, 18, 22, 255))
im.save(Path(__file__).with_name("PalmeraRacing.ico"), sizes=[(256, 256), (64, 64), (48, 48), (32, 32), (16, 16)])
