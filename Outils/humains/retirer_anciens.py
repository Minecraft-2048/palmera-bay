"""Retire les anciennes coupes et tenues peintes sur les mannequins : les humains de /Game/Humains les remplacent."""
import unreal

assets = unreal.EditorAssetLibrary
for chemin in ("/Game/Materiaux/M_Habit", "/Game/Materiaux/M_Cheveux"):
    if assets.does_asset_exist(chemin):
        unreal.log(f"PALMERA retiré {chemin} : {assets.delete_asset(chemin)}")
for s in (1, 2, 3, 4):
    dossier = f"/Game/Modeles/Mobilier/SM_Cheveux_{s}"
    if assets.does_directory_exist(dossier):
        unreal.log(f"PALMERA retiré {dossier} : {assets.delete_directory(dossier)}")
