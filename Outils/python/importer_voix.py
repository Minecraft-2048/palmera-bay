"""Importe les voix enregistrées dans /Game/Voix."""
import glob
import os

import unreal

SOURCE = r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Art\Voix"
fichiers = sorted(glob.glob(os.path.join(SOURCE, "*.wav")))
taches = []
for f in fichiers:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", f)
    t.set_editor_property("destination_path", "/Game/Voix")
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    taches.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(taches)
unreal.log(f"PALMERA voix importées : {len(unreal.EditorAssetLibrary.list_assets('/Game/Voix', recursive=False))}")
