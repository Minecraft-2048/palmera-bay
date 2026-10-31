"""Compare les réglages des textures de vêtements : une couleur fausse vient souvent de là."""
import unreal

for nom in ("male_worksuit01_diffuse", "male_casualsuit05_diffuse", "male_casualsuit06_diffuse", "young_lightskinned_female_diffuse"):
    t = unreal.load_asset(f"/Game/Humains/Textures/{nom}")
    if not t:
        unreal.log(f"PALMERA {nom} : ABSENTE")
        continue
    unreal.log(f"PALMERA {nom} : {t.blueprint_get_size_x()}x{t.blueprint_get_size_y()} compression={t.get_editor_property('compression_settings')} srgb={t.get_editor_property('srgb')} vert={t.get_editor_property('flip_green_channel')} groupe={t.get_editor_property('lod_group')}")
for nom in ("MI_Vetement_Nina", "MI_Vetement_Sam"):
    mi = unreal.load_asset(f"/Game/Humains/Materiaux/{nom}")
    mel = unreal.MaterialEditingLibrary
    unreal.log(f"PALMERA {nom} : diffuse={mel.get_material_instance_texture_parameter_value(mi, 'Diffuse').get_name()} haut={mel.get_material_instance_vector_parameter_value(mi, 'Haut')} bas={mel.get_material_instance_vector_parameter_value(mi, 'Bas')} fh={mel.get_material_instance_scalar_parameter_value(mi, 'ForceHaut')} fb={mel.get_material_instance_scalar_parameter_value(mi, 'ForceBas')} moy={mel.get_material_instance_scalar_parameter_value(mi, 'Moyenne')}")
