"""Réduit des photos du jeu en JPEG léger pour les regarder sans alourdir la conversation."""
import sys
from pathlib import Path

from PIL import Image

SOURCE = Path(r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Saved\Photos")
SORTIE = Path(sys.argv[1])
for nom in sys.argv[2:]:
    im = Image.open(SOURCE / f"{nom}.png").convert("RGB")
    im.thumbnail((1500, 1500), Image.LANCZOS)
    im.save(SORTIE / f"{nom}.jpg", quality=80)
    print(nom, im.size)
