"""Affiche ce qu'un modèle humain contient vraiment : ses emplacements, leurs matériaux et les textures utilisées."""
import unreal

mel = unreal.MaterialEditingLibrary
modele = unreal.load_asset("/Game/Humains/SKM_Sam")
for em in modele.get_editor_property("materials"):
    mi = em.get_editor_property("material_interface")
    nom = em.get_editor_property("material_slot_name")
    if not mi:
        unreal.log(f"PALMERA {nom} : AUCUN MATERIAU")
        continue
    parent = mi.get_editor_property("parent") if isinstance(mi, unreal.MaterialInstanceConstant) else None
    base = parent or mi
    unreal.log(f"PALMERA {nom} : {mi.get_path_name()} parent={base.get_path_name()} squelette={base.get_editor_property('used_with_skeletal_mesh') if isinstance(base, unreal.Material) else '?'}")
for chemin in ("M_Peau", "M_Oeil", "M_Poil", "M_Meches", "M_Vetement"):
    m = unreal.load_asset(f"/Game/Humains/{chemin}")
    unreal.log(f"PALMERA {chemin} usage squelette={m.get_editor_property('used_with_skeletal_mesh')} modele={m.get_editor_property('shading_model')} expressions={mel.get_num_material_expressions(m)}")
