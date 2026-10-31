"""Premier essai : vérifie que l'éditeur démarre et que Python voit les outils dont on a besoin."""
import unreal

unreal.log(f"PALMERA version moteur = {unreal.SystemLibrary.get_engine_version()}")
unreal.log(f"PALMERA dossier projet = {unreal.Paths.project_dir()}")
for nom in ("MaterialEditingLibrary", "EditorAssetLibrary", "AssetImportTask", "MaterialExpressionCustom",
            "InterchangeManager", "LevelEditorSubsystem", "EditorActorSubsystem"):
    unreal.log(f"PALMERA {nom} : {'oui' if hasattr(unreal, nom) else 'NON'}")
