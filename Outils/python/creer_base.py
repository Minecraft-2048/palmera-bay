"""Crée la carte vide et la collection de paramètres partagés par les matériaux."""
import unreal

outils = unreal.AssetToolsHelpers.get_asset_tools()
assets = unreal.EditorAssetLibrary

if not assets.does_asset_exist("/Game/Materiaux/MPC_Palmera"):
    mpc = outils.create_asset("MPC_Palmera", "/Game/Materiaux", unreal.MaterialParameterCollection,
                              unreal.MaterialParameterCollectionFactoryNew())
    valeurs = []
    for nom in ("Nuit", "Pluie", "TempsFeux"):
        p = unreal.CollectionScalarParameter()
        p.set_editor_property("parameter_name", nom)
        p.set_editor_property("default_value", 0.0)
        valeurs.append(p)
    mpc.set_editor_property("scalar_parameters", valeurs)
    assets.save_asset("/Game/Materiaux/MPC_Palmera")
    unreal.log("PALMERA collection créée")

niveaux = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not assets.does_asset_exist("/Game/Cartes/Palmera"):
    ok = niveaux.new_level("/Game/Cartes/Palmera", False)
    niveaux.save_current_level()
    unreal.log(f"PALMERA carte créée : {ok}")
unreal.log("PALMERA base prête")
