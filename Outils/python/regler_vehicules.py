"""Autorise les matériaux des voitures et du mannequin sur les maillages Nanite et animés, et les marque pour la suite."""
import unreal

assets = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
nb = 0
for dossier in ("/Game/Vehicles", "/Game/Characters"):
    for chemin in assets.list_assets(dossier, recursive=True, include_folder=False):
        chemin = chemin.split(".")[0]
        a = assets.find_asset_data(chemin)
        if str(a.asset_class_path.asset_name) != "Material":
            continue
        m = assets.load_asset(chemin)
        m.set_editor_property("used_with_nanite", True)
        m.set_editor_property("used_with_instanced_static_meshes", True)
        m.set_editor_property("used_with_skeletal_mesh", True)
        mel.recompile_material(m)
        assets.save_asset(chemin)
        nb += 1
        unreal.log(f"PALMERA réglé {chemin}")
unreal.log(f"PALMERA matériaux de véhicules réglés : {nb}")

for chemin in ("/Game/Vehicles/SportsCar/Materials/M_SportsCar_Lights", "/Game/Vehicles/OffroadCar/Materials/MI_OffroadCar_Lights"):
    m = assets.load_asset(chemin)
    if m:
        s = [str(n) for n in mel.get_scalar_parameter_names(m)]
        v = [str(n) for n in mel.get_vector_parameter_names(m)]
        unreal.log(f"PALMERA feux {chemin.rsplit('/', 1)[1]} scalaires={s} vecteurs={v}")
