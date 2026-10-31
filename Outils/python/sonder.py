"""Affiche la position et l'orientation des os de la tête du mannequin, et les limites du corps."""
import unreal

assets = unreal.EditorAssetLibrary
sq = assets.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")
pose = unreal.AnimPoseExtensions.get_reference_pose(sq)
for nom in ("head", "neck_02", "spine_05"):
    t = unreal.AnimPoseExtensions.get_bone_pose(pose, nom, unreal.AnimPoseSpaces.WORLD)
    p, r = t.translation, t.rotation.rotator()
    unreal.log(f"PALMERA os {nom} pos=({p.x:.2f},{p.y:.2f},{p.z:.2f}) rot=(roulis {r.roll:.2f}, tangage {r.pitch:.2f}, lacet {r.yaw:.2f})")
for chemin in ("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple", "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"):
    m = assets.load_asset(chemin)
    b = m.get_bounds()
    unreal.log(f"PALMERA corps {chemin.rsplit('/', 1)[1]} centre=({b.origin.x:.1f},{b.origin.y:.1f},{b.origin.z:.1f}) demi=({b.box_extent.x:.1f},{b.box_extent.y:.1f},{b.box_extent.z:.1f})")
