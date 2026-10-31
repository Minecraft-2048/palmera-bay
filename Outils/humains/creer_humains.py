"""Crée les humains du jeu avec MakeHuman et les ajuste sur le squelette de Manny ou Quinn.

Le squelette du jeu reste celui d'Epic : toutes les animations continuent de marcher. Chaque os du rig
« game_engine » de MakeHuman est amené sur l'os correspondant du jeu, le corps suit, puis le maillage
est rattaché au vrai squelette et exporté en FBX. Usage : blender.ps1 -Script creer_humains.py -Arguments "Sam,Nina"
"""
import addon_utils
import importlib
import json
import sys
import traceback
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector

ICI = Path(__file__).parent
SORTIE = Path(r"E:\UnrealTools\MakeHuman\sortie")
FBX = Path(r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Art\Humains")

addon_utils.enable("bl_ext.user_default.mpfb", default_set=True, persistent=False)


def service(module, classe):
    return getattr(importlib.import_module(f"bl_ext.user_default.mpfb.services.{module}"), classe)


HumanService = service("humanservice", "HumanService")
AssetService = service("assetservice", "AssetService")

# Quel os du jeu reçoit les poids de chaque os de MakeHuman (le reste porte le même nom)
ROLES = {"spine_01": "spine_02", "spine_02": "spine_03", "spine_03": "spine_04"}
# Le haut du buste et du cou passe en douceur sur la vertèbre suivante (hauteurs en centimètres)
PARTAGES = {"spine_04": ("spine_05", 126.0, 145.0), "neck_01": ("neck_02", 153.0, 158.0)}
SUITE = {"pelvis": "spine_01", "spine_01": "spine_02", "spine_02": "spine_03", "spine_03": "neck_01", "neck_01": "head"}
for c in "lr":
    SUITE.update({f"clavicle_{c}": f"upperarm_{c}", f"upperarm_{c}": f"lowerarm_{c}", f"lowerarm_{c}": f"hand_{c}", f"hand_{c}": f"middle_01_{c}",
                  f"thigh_{c}": f"calf_{c}", f"calf_{c}": f"foot_{c}", f"foot_{c}": f"ball_{c}"})
    for doigt in ("thumb", "index", "middle", "ring", "pinky"):
        SUITE.update({f"{doigt}_01_{c}": f"{doigt}_02_{c}", f"{doigt}_02_{c}": f"{doigt}_03_{c}"})


def lire_squelette(base):
    """Squelette du jeu, passé du repère d'Unreal à celui de Blender : Y inversé, en mètres."""
    os_jeu = []
    for ligne in (SORTIE / f"os_{base}.txt").read_text(encoding="utf-8").splitlines():
        nom, parent, x, y, z, qx, qy, qz, qw = ligne.split()
        position = Vector((float(x), -float(y), float(z))) * 0.01
        rotation = Quaternion((float(qw), -float(qx), float(qy), -float(qz)))
        os_jeu.append((nom, None if parent == "-" else parent, position, rotation))
    return os_jeu


def creer_armature(os_jeu):
    """Reconstruit le squelette du jeu en centimètres, avec l'orientation exacte de chaque os."""
    donnees = bpy.data.armatures.new("Armature")
    objet = bpy.data.objects.new("Armature", donnees)
    bpy.context.collection.objects.link(objet)
    bpy.context.view_layer.objects.active = objet
    objet.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    for nom, parent, position, rotation in os_jeu:
        os_ = donnees.edit_bones.new(nom)
        os_.head = (0, 0, 0)
        os_.tail = (0, 5, 0)
        os_.matrix = Matrix.LocRotScale(position * 100.0, rotation, None)
    for nom, parent, _, _ in os_jeu:
        if parent:
            donnees.edit_bones[nom].parent = donnees.edit_bones[parent]
    bpy.ops.object.mode_set(mode="OBJECT")
    return objet


def matrices_ajustement(rig, cibles, echelle):
    """Pour chaque os de MakeHuman, la transformation qui l'amène sur l'os du jeu."""
    tetes = {b.name: rig.matrix_world @ b.head_local for b in rig.data.bones}
    queues = {b.name: rig.matrix_world @ b.tail_local for b in rig.data.bones}
    resultat = {}

    def cible(nom):
        return cibles[ROLES.get(nom, nom)]

    for nom in tetes:
        if nom == "Root":
            continue
        depart = tetes[nom]
        arrivee = cible(nom)
        if nom in SUITE:
            u = tetes[SUITE[nom]] - depart
            v = cible(SUITE[nom]) - arrivee
        else:
            # Bout de chaîne (dernière phalange, tête, orteils) : on garde sa direction, à l'échelle
            parent = rig.data.bones[nom].parent
            u = queues[nom] - depart
            if parent and parent.name in SUITE and SUITE[parent.name] == nom:
                sens_mh = (tetes[nom] - tetes[parent.name]).normalized()
                sens_jeu = (cible(nom) - cible(parent.name)).normalized()
                v = sens_mh.rotation_difference(sens_jeu) @ u * echelle
            else:
                v = u * echelle
        axe = u.normalized()
        rotation = axe.rotation_difference(v.normalized()).to_matrix()
        s = v.length / max(u.length, 1e-6)
        # Longueur ajustée le long de l'os, largeur à l'échelle du corps entier
        projection = Matrix(((axe.x * axe.x, axe.x * axe.y, axe.x * axe.z), (axe.y * axe.x, axe.y * axe.y, axe.y * axe.z), (axe.z * axe.x, axe.z * axe.y, axe.z * axe.z)))
        etirement = Matrix.Identity(3) * echelle + projection * (s - echelle)
        m = Matrix.Translation(arrivee) @ (rotation @ etirement).to_4x4() @ Matrix.Translation(-depart)
        resultat[nom] = np.array(m)
    return resultat


def cuire(objet, garder_subdivision):
    """Fige formes et masques (les parties cachées sous les vêtements disparaissent vraiment)."""
    bpy.ops.object.select_all(action="DESELECT")
    objet.select_set(True)
    bpy.context.view_layer.objects.active = objet
    if objet.data.shape_keys:
        bpy.ops.object.shape_key_remove(all=True, apply_mix=True)
    for m in list(objet.modifiers):
        if m.type == "ARMATURE":
            objet.modifiers.remove(m)
        elif m.type == "SUBSURF" and not garder_subdivision:
            objet.modifiers.remove(m)
        elif m.type == "SUBSURF":
            m.levels = 1
            bpy.ops.object.modifier_apply(modifier=m.name)
        elif m.type == "MASK" and m.vertex_group == "body" and m.invert_vertex_group:
            objet.modifiers.remove(m)
        else:
            bpy.ops.object.modifier_apply(modifier=m.name)


def ajuster(objet, matrices):
    """Déforme le maillage avec les poids de MakeHuman, puis les rattache aux os du jeu."""
    n = len(objet.data.vertices)
    co = np.empty(n * 3)
    objet.data.vertices.foreach_get("co", co)
    co = np.c_[co.reshape(-1, 3), np.ones(n)]
    co = (np.array(objet.matrix_world) @ co.T).T
    noms = {g.index: g.name for g in objet.vertex_groups}
    somme = np.zeros((n, 3))
    total = np.zeros(n)
    for v in objet.data.vertices:
        for g in v.groups:
            nom = noms[g.group]
            if nom in matrices and g.weight > 0.0:
                somme[v.index] += g.weight * (matrices[nom][:3] @ co[v.index])
                total[v.index] += g.weight
    orphelins = int((total <= 0.0).sum())
    if orphelins:
        somme[total <= 0.0] = (matrices["pelvis"][:3] @ co[total <= 0.0].T).T
        total[total <= 0.0] = 1.0
    objet.data.vertices.foreach_set("co", (somme / total[:, None] * 100.0).ravel())
    objet.matrix_world = Matrix.Identity(4)
    objet.parent = None
    objet.data.update()
    for g in list(objet.vertex_groups):
        if g.name in matrices:
            g.name = ROLES.get(g.name, g.name)
        else:
            objet.vertex_groups.remove(g)
    for bas, (haut, z0, z1) in PARTAGES.items():
        groupe = objet.vertex_groups.get(bas)
        if groupe is None:
            continue
        suivant = objet.vertex_groups.new(name=haut)
        for v in objet.data.vertices:
            for g in v.groups:
                if g.group == groupe.index:
                    t = min(max((v.co.z - z0) / (z1 - z0), 0.0), 1.0)
                    t = t * t * (3.0 - 2.0 * t)
                    poids = g.weight
                    suivant.add([v.index], poids * t, "REPLACE")
                    groupe.add([v.index], poids * (1.0 - t), "REPLACE")
                    break
    return orphelins


def materiau(nom):
    return bpy.data.materials.get(nom) or bpy.data.materials.new(nom)


def creer(fiche):
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    bpy.context.scene.unit_settings.scale_length = 1.0
    print("PALMERA creation", fiche["nom"])
    macro = {"gender": 0.5, "age": 0.5, "muscle": 0.5, "weight": 0.5, "proportions": 0.5, "height": 0.5, "cupsize": 0.5, "firmness": 0.5,
             "race": {"caucasian": 1.0, "asian": 0.0, "african": 0.0}}
    macro.update(fiche["macro"])
    corps = HumanService.create_human(macro_detail_dict=macro)
    HumanService.set_character_skin(AssetService.find_asset_absolute_path(fiche["peau"] + ".mhmat", asset_subdir="skins"), corps, skin_type="GAMEENGINE")
    HumanService.add_builtin_rig(corps, "game_engine")
    rig = corps.parent

    parties = [("eyes", "low-poly", "Eyes", "yeux", True), ("eyebrows", fiche["sourcils"], "Eyebrows", "sourcils", False), ("eyelashes", fiche["cils"], "Eyelashes", "cils", False)]
    if fiche.get("cheveux"):
        parties.append(("hair", fiche["cheveux"], "Hair", "cheveux", False))
    parties.append(("clothes", fiche["vetement"], "Clothes", "vetement", True))
    parties.append(("clothes", fiche["chaussures"], "Clothes", "chaussures", True))
    objets = [(corps, "peau", False)]
    for sous, fichier, sorte, role, subdivision in parties:
        chemin = AssetService.find_asset_absolute_path(fichier + ".mhclo", asset_subdir=sous)
        objet = HumanService.add_mhclo_asset(chemin, corps, asset_type=sorte, material_type="GAMEENGINE")
        objets.append((objet, role, subdivision))

    os_jeu = lire_squelette(fiche["base"])
    cibles = {nom: position for nom, _, position, _ in os_jeu}
    echelle = cibles["head"].z / (rig.matrix_world @ rig.data.bones["head"].head_local).z
    matrices = matrices_ajustement(rig, cibles, echelle)
    if fiche.get("rapport"):
        print("PALMERA os MakeHuman", [(b.name, b.parent.name if b.parent else "-", tuple(round(x, 3) for x in rig.matrix_world @ b.head_local)) for b in rig.data.bones])

    orphelins = 0
    for objet, role, subdivision in objets:
        cuire(objet, subdivision and len(objet.data.vertices) < 5000)
        orphelins += ajuster(objet, matrices)
        objet.data.materials.clear()
        objet.data.materials.append(materiau(role))
        for rang, couche in enumerate(list(objet.data.uv_layers)):
            if rang == 0:
                couche.name = "UVMap"
            else:
                objet.data.uv_layers.remove(couche)
    bpy.data.objects.remove(rig)

    bpy.context.scene.unit_settings.scale_length = 0.01
    armature = creer_armature(os_jeu)
    bpy.ops.object.select_all(action="DESELECT")
    for objet, _, _ in objets:
        objet.select_set(True)
    bpy.context.view_layer.objects.active = corps
    bpy.ops.object.join()
    corps.data.polygons.foreach_set("use_smooth", [True] * len(corps.data.polygons))
    corps.data.update()
    corps.name = "SKM_" + fiche["nom"]
    corps.data.name = corps.name
    corps.parent = armature
    modificateur = corps.modifiers.new("Armature", "ARMATURE")
    modificateur.object = armature

    bpy.ops.object.select_all(action="DESELECT")
    corps.select_set(True)
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature
    FBX.mkdir(parents=True, exist_ok=True)
    chemin = FBX / f"SKM_{fiche['nom']}.fbx"
    bpy.ops.export_scene.fbx(filepath=str(chemin), use_selection=True, object_types={"ARMATURE", "MESH"}, add_leaf_bones=False, bake_anim=False,
                             primary_bone_axis="Y", secondary_bone_axis="X", axis_forward="Y", axis_up="Z", mesh_smooth_type="FACE", use_armature_deform_only=False,
                             apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE", global_scale=1.0)
    triangles = sum(len(p.vertices) - 2 for p in corps.data.polygons)
    print("PALMERA fini", fiche["nom"], len(corps.data.vertices), "sommets", triangles, "triangles", orphelins, "orphelins", [m.name for m in corps.data.materials], chemin.stat().st_size // 1024, "Ko")


try:
    arguments = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    voulus = set(arguments[0].split(",")) if arguments and arguments[0] else None
    for fiche in json.loads((ICI / "casting.json").read_text(encoding="utf-8")):
        if voulus is None or fiche["nom"] in voulus:
            creer(fiche)
except Exception:
    traceback.print_exc()
    print("PALMERA ECHEC")
sys.stdout.flush()
