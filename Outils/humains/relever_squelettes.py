"""Écrit le squelette de référence de Manny et Quinn pour le script d'ajustement dans Blender."""
import unreal

for nom in ("Manny", "Quinn"):
    unreal.PalmeraGameMode.relever_squelette(f"/Game/Characters/Mannequins/Meshes/SKM_{nom}_Simple.SKM_{nom}_Simple", f"E:/UnrealTools/MakeHuman/sortie/os_{nom}.txt")
unreal.log("PALMERA squelettes releves")
