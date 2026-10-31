"""Importe les sons de Art/Sons dans /Game/Sons et règle les boucles."""
import glob
import os

import unreal

SOURCE = r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Art\Sons"
DEST = "/Game/Sons"
assets = unreal.EditorAssetLibrary
BOUCLES = ("S_MoteurBas", "S_MoteurMoyen", "S_MoteurHaut", "S_Vent", "S_Crissement", "S_Sirene", "S_Vagues", "S_Ville", "S_Pluie",
           "S_Radio1", "S_Radio2", "S_Radio3")

fichiers = sorted(glob.glob(os.path.join(SOURCE, "*.wav")))
taches = []
for f in fichiers:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", f)
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    taches.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(taches)

for f in fichiers:
    nom = os.path.splitext(os.path.basename(f))[0]
    chemin = f"{DEST}/{nom}"
    s = assets.load_asset(chemin)
    if s is None:
        unreal.log_warning(f"PALMERA son non importé : {nom}")
        continue
    if nom in BOUCLES:
        s.set_editor_property("looping", True)
    assets.save_asset(chemin)
unreal.log(f"PALMERA sons importés : {len(fichiers)}")
