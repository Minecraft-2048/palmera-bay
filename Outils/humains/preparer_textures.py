"""Copie les textures MakeHuman utilisées par le casting et note ce qu'Unreal doit en savoir (fichiers, luminosité moyenne)."""
import json
import re
import shutil
from pathlib import Path

from PIL import Image

ICI = Path(__file__).parent
DONNEES = Path(r"E:\UnrealTools\MakeHuman\donnees")
ART = Path(r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Art\Humains\Textures")
ART.mkdir(parents=True, exist_ok=True)


def lire_mhmat(chemin):
    infos = {}
    for ligne in chemin.read_text(encoding="utf-8", errors="ignore").splitlines():
        m = re.match(r"(diffuseTexture|normalmapTexture)\s+(\S+)", ligne)
        if m:
            infos[m.group(1)] = chemin.parent / m.group(2)
    return infos


def luminosite(chemin):
    """Luminosité moyenne des pixels visibles, pour recolorer sans perdre le relief de la texture."""
    im = Image.open(chemin).convert("RGBA").resize((128, 128))
    somme, nombre = 0.0, 0
    for r, g, b, a in im.getdata():
        if a > 128:
            somme += ((0.3 * r + 0.59 * g + 0.11 * b) / 255.0) ** 2.2
            nombre += 1
    return round(somme / max(nombre, 1), 4)


resultat = {}


def ajouter(sorte, nom, mhmat):
    cle = f"{sorte}:{nom}"
    if cle in resultat:
        return
    infos = lire_mhmat(mhmat)
    entree = {"diffuse": infos["diffuseTexture"].stem, "moyenne": luminosite(infos["diffuseTexture"])}
    shutil.copy2(infos["diffuseTexture"], ART / infos["diffuseTexture"].name)
    voisine = infos["diffuseTexture"].with_name(infos["diffuseTexture"].stem.replace("_diffuse", "") + "_normal.png")
    if "normalmapTexture" not in infos and voisine.exists():
        infos["normalmapTexture"] = voisine
    if "normalmapTexture" in infos:
        entree["normale"] = infos["normalmapTexture"].stem
        shutil.copy2(infos["normalmapTexture"], ART / infos["normalmapTexture"].name)
    resultat[cle] = entree


for fiche in json.loads((ICI / "casting.json").read_text(encoding="utf-8")):
    ajouter("peau", fiche["peau"], DONNEES / "skins" / fiche["peau"] / f"{fiche['peau']}.mhmat")
    ajouter("yeux", fiche["yeux"], DONNEES / "eyes" / "materials" / f"{fiche['yeux']}.mhmat")
    ajouter("sourcils", fiche["sourcils"], DONNEES / "eyebrows" / fiche["sourcils"] / f"{fiche['sourcils']}.mhmat")
    ajouter("cils", fiche["cils"], DONNEES / "eyelashes" / fiche["cils"] / f"{fiche['cils']}.mhmat")
    if fiche["cheveux"]:
        ajouter("cheveux", fiche["cheveux"], DONNEES / "hair" / fiche["cheveux"] / f"{fiche['cheveux']}.mhmat")
    ajouter("vetement", fiche["vetement"], DONNEES / "clothes" / fiche["vetement"] / f"{fiche['vetement']}.mhmat")
    ajouter("chaussures", fiche["chaussures"], DONNEES / "clothes" / fiche["chaussures"] / f"{fiche['chaussures']}.mhmat")

Image.new("RGB", (4, 4), (128, 128, 255)).save(ART / "T_Plat_normale.png")
(ICI / "textures.json").write_text(json.dumps(resultat, indent=1), encoding="utf-8")
print(len(resultat), "entrees,", len(list(ART.iterdir())), "fichiers")
