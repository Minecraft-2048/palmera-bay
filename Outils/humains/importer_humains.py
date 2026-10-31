"""Importe les humains exportés par Blender sur le squelette d'Epic, puis relève leur squelette pour le comparer."""
import os
from pathlib import Path

import unreal

DOSSIER = Path(r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Art\Humains")
DESTINATION = "/Game/Humains"
NOMS = [n for n in os.environ.get("PALMERA_HUMAINS", "").split(",") if n]

unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
squelette = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")

taches = []
for fichier in sorted(DOSSIER.glob("SKM_*.fbx")):
    if NOMS and fichier.stem[4:] not in NOMS:
        continue
    interface = unreal.FbxImportUI()
    interface.set_editor_property("import_mesh", True)
    interface.set_editor_property("import_as_skeletal", True)
    interface.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    interface.set_editor_property("import_animations", False)
    interface.set_editor_property("import_materials", False)
    interface.set_editor_property("import_textures", False)
    interface.set_editor_property("create_physics_asset", False)
    interface.set_editor_property("automated_import_should_detect_type", False)
    interface.set_editor_property("skeleton", squelette)
    donnees = interface.skeletal_mesh_import_data
    donnees.set_editor_property("convert_scene", False)
    donnees.set_editor_property("force_front_x_axis", False)
    donnees.set_editor_property("convert_scene_unit", False)
    donnees.set_editor_property("import_uniform_scale", 1.0)
    donnees.set_editor_property("import_morph_targets", False)
    donnees.set_editor_property("update_skeleton_reference_pose", False)
    donnees.set_editor_property("use_t0_as_ref_pose", False)
    tache = unreal.AssetImportTask()
    tache.set_editor_property("filename", str(fichier))
    tache.set_editor_property("destination_path", DESTINATION)
    tache.set_editor_property("destination_name", fichier.stem)
    tache.set_editor_property("automated", True)
    tache.set_editor_property("replace_existing", True)
    tache.set_editor_property("save", True)
    tache.set_editor_property("options", interface)
    taches.append(tache)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(taches)

for tache in taches:
    nom = tache.get_editor_property("destination_name")
    chemin = f"{DESTINATION}/{nom}"
    modele = unreal.load_asset(chemin)
    if not modele:
        unreal.log_error(f"PALMERA import rate {nom}")
        continue
    unreal.log(f"PALMERA importe {nom} : squelette {modele.skeleton.get_name() if modele.skeleton else 'aucun'}, {[m.material_slot_name for m in modele.materials]}")
    unreal.PalmeraGameMode.relever_squelette(f"{chemin}.{nom}", f"E:/UnrealTools/MakeHuman/sortie/verif_{nom}.txt")
unreal.log("PALMERA import fini")
