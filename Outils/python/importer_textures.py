"""Importe les textures de Art/Textures dans /Game/Textures avec les bons réglages."""
import glob
import os

import unreal

SOURCE = r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Art\Textures"
DEST = "/Game/Textures"
assets = unreal.EditorAssetLibrary

fichiers = sorted(glob.glob(os.path.join(SOURCE, "*.png")))
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
    tex = assets.load_asset(chemin)
    if tex is None:
        unreal.log_warning(f"PALMERA texture non importée : {nom}")
        continue
    if nom.endswith("_N"):
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif nom.endswith("_ORM") or nom in ("T_Bruit", "T_Enseignes"):
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    assets.save_asset(chemin)
unreal.log(f"PALMERA textures importées : {len(fichiers)}")
