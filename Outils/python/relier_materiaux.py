"""Relie chaque emplacement de matériau des maillages importés au matériau de la ville."""
import os

import unreal

assets = unreal.EditorAssetLibrary
MATERIAUX = {
    "facade": "M_Facade", "beton": "M_Beton", "brique": "M_Brique", "tole": "M_Tole", "vitre": "M_Vitre",
    "vitre_tour": "M_VitreTour", "vitrine": "M_Vitrine", "cadre": "M_Cadre", "metal": "M_Metal", "toit": "M_Toit",
    "tuile": "M_Tuile", "auvent": "M_Auvent", "enseigne": "M_Enseigne", "neon": "M_Neon", "bois": "M_Bois",
    "ecorce": "M_Ecorce", "trottoir": "M_Trottoir", "herbe": "M_Herbe", "sable": "M_Sable",
    "ocean": "M_Ocean", "palme": "M_Palme", "lampe": "M_Lampe", "feu": "M_Feu", "piscine": "M_Piscine", "colline": "M_Colline",
    "pluie": "M_Pluie", "peinture": "M_Peinture", "vitre_auto": "M_VitreAuto", "plastique": "M_Plastique", "chrome": "M_Chrome",
    "feu_avant": "M_FeuAvant", "feu_arriere": "M_FeuArriere", "pneu": "M_Pneu", "tissu": "M_Tissu", "peint": "M_Peint", "lumineux": "M_Lumineux", "ecran": "M_EcranCine", "peinture_garee": "M_PeintureGaree",
}

nb = 0
for chemin in assets.list_assets(os.environ.get("PALMERA_DOSSIER", "/Game/Modeles"), recursive=True, include_folder=False):
    chemin = chemin.split(".")[0]
    if "/StaticMeshes/" not in chemin:
        continue
    sm = assets.load_asset(chemin)
    if not isinstance(sm, unreal.StaticMesh):
        continue
    for i, s in enumerate(sm.get_editor_property("static_materials")):
        cle = str(s.get_editor_property("material_slot_name")).lower()
        mat = MATERIAUX.get(cle)
        if mat is None:
            unreal.log_warning(f"PALMERA emplacement inconnu {cle} dans {chemin}")
            continue
        sm.set_material(i, assets.load_asset(f"/Game/Materiaux/{mat}"))
    assets.save_asset(chemin)
    nb += 1
verif = assets.load_asset("/Game/Modeles/Mobilier/SM_Lampadaire/StaticMeshes/SM_Lampadaire")
unreal.log(f"PALMERA vérification : {[str(verif.get_material(i).get_name()) for i in range(len(verif.get_editor_property('static_materials')))]}")
unreal.log(f"PALMERA maillages reliés : {nb}")
