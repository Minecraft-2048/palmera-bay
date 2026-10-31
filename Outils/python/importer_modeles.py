"""Importe les .glb d'un dossier : Nanite activé, emplacements de matériaux reliés à nos matériaux.

Le dossier source est choisi par la variable PALMERA_MODELES : « Immeubles:/Game/Modeles/Immeubles[:nom1,nom2] ».
"""
import glob
import os

import unreal

BASE = r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Art\Modeles"
assets = unreal.EditorAssetLibrary

MATERIAUX = {
    "facade": "M_Facade", "beton": "M_Beton", "brique": "M_Brique", "tole": "M_Tole", "vitre": "M_Vitre",
    "vitre_tour": "M_VitreTour", "vitrine": "M_Vitrine", "cadre": "M_Cadre", "metal": "M_Metal", "toit": "M_Toit",
    "tuile": "M_Tuile", "auvent": "M_Auvent", "enseigne": "M_Enseigne", "neon": "M_Neon", "bois": "M_Bois",
    "ecorce": "M_Ecorce", "trottoir": "M_Trottoir", "herbe": "M_Herbe", "sable": "M_Sable",
    "ocean": "M_Ocean", "palme": "M_Palme", "lampe": "M_Lampe", "feu": "M_Feu", "piscine": "M_Piscine", "colline": "M_Colline",
    "pluie": "M_Pluie", "peinture": "M_Peinture", "vitre_auto": "M_VitreAuto", "plastique": "M_Plastique", "chrome": "M_Chrome",
    "feu_avant": "M_FeuAvant", "feu_arriere": "M_FeuArriere", "pneu": "M_Pneu", "tissu": "M_Tissu", "planche": "M_Bois", "peint": "M_Peint", "lumineux": "M_Lumineux", "ecran": "M_EcranCine", "peinture_garee": "M_PeintureGaree",
}
SANS_NANITE = ("SM_Ocean", "SM_Pluie")


def importer(fichier, dest):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", fichier)
    t.set_editor_property("destination_path", dest)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    return list(t.get_editor_property("imported_object_paths"))


def regler(chemin, nanite=True):
    sm = assets.load_asset(chemin)
    if not isinstance(sm, unreal.StaticMesh):
        return False
    for i, s in enumerate(sm.get_editor_property("static_materials")):
        cle = str(s.get_editor_property("material_slot_name")).lower()
        mat = MATERIAUX.get(cle)
        if mat is None:
            unreal.log_warning(f"PALMERA emplacement inconnu {cle} dans {chemin}")
            continue
        sm.set_material(i, assets.load_asset(f"/Game/Materiaux/{mat}"))
    reglage = sm.get_editor_property("nanite_settings")
    if bool(reglage.get_editor_property("enabled")) != nanite:
        reglage.set_editor_property("enabled", nanite)
        sm.set_editor_property("nanite_settings", reglage)
    assets.save_asset(chemin)
    return True


spec = os.environ.get("PALMERA_MODELES", "Immeubles:/Game/Modeles/Immeubles").split(":")
dossier, dest = spec[0], spec[1]
noms = spec[2].split(",") if len(spec) > 2 else None
fichiers = sorted(glob.glob(os.path.join(BASE, dossier, "*.glb")))
if noms:
    fichiers = [f for f in fichiers if os.path.splitext(os.path.basename(f))[0] in noms]

for f in fichiers:
    objets = importer(f, dest)
    trouve = False
    for o in objets:
        chemin = str(o).split(".")[0]
        if "/Materials/" not in chemin and regler(chemin, chemin.rsplit("/", 1)[1] not in SANS_NANITE):
            sm = assets.load_asset(chemin)
            trouve = True
            unreal.log(f"PALMERA réglé {chemin} emplacements={len(sm.get_editor_property('static_materials'))}")
            # Interchange crée une copie de chaque matériau : on n'en a pas besoin
            dossier_mat = chemin.rsplit("/", 1)[0] + "/Materials"
            if assets.does_directory_exist(dossier_mat):
                assets.delete_directory(dossier_mat)
    if not trouve:
        unreal.log_warning(f"PALMERA aucun maillage importé depuis {f}")

unreal.log("PALMERA contenu : " + ", ".join(assets.list_assets(dest, recursive=True, include_folder=False)[:6]))
unreal.log("PALMERA import terminé")
