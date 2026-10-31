"""Textures, matériaux et instances des humains dans Unreal, puis attribution aux emplacements de chaque modèle."""
import json
import os
import sys
from pathlib import Path

import unreal

sys.path.insert(0, "E:/Minecraft/Claude Unreal Engine/PalmeraBay/Outils/python")
from mat_outils import Graphe, assets, mel, outils  # noqa: E402

MP = unreal.MaterialProperty
ICI = Path("E:/Minecraft/Claude Unreal Engine/PalmeraBay/Outils/humains")
ART = Path("E:/Minecraft/Claude Unreal Engine/PalmeraBay/Art/Humains/Textures")
DOSSIER = "/Game/Humains"
TEXTURES = f"{DOSSIER}/Textures"
INSTANCES = f"{DOSSIER}/Materiaux"
PLAT = "T_Plat_normale"
ETAPES = os.environ.get("PALMERA_ETAPES", "textures,materiaux,instances")
HUMAINS = [n for n in os.environ.get("PALMERA_HUMAINS", "").split(",") if n]

HLSL_CHEVEUX = """
float l = dot(T, float3(0.3, 0.59, 0.11));
return Couleur * clamp(0.12 + 0.95 * l / max(Moyenne, 0.01), 0.0, 1.9);
"""

HLSL_TISSU = """
float haut = smoothstep(93.0, 97.0, P.z);
float force = lerp(ForceBas, ForceHaut, haut);
float3 teinte = lerp(Bas, Haut, haut);
float l = dot(T, float3(0.3, 0.59, 0.11));
return lerp(T, teinte * saturate(0.55 + 0.45 * l / max(Moyenne, 0.01)), force);
"""


def importer_textures(fichiers):
    taches = []
    for nom, normale in fichiers.items():
        if assets.does_asset_exist(f"{TEXTURES}/{nom}"):
            continue
        tache = unreal.AssetImportTask()
        tache.set_editor_property("filename", str(ART / f"{nom}.png"))
        tache.set_editor_property("destination_path", TEXTURES)
        tache.set_editor_property("destination_name", nom)
        tache.set_editor_property("automated", True)
        tache.set_editor_property("replace_existing", True)
        tache.set_editor_property("save", False)
        taches.append(tache)
    outils.import_asset_tasks(taches)
    for nom, normale in fichiers.items():
        t = assets.load_asset(f"{TEXTURES}/{nom}")
        if normale:
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            t.set_editor_property("srgb", False)
            t.set_editor_property("flip_green_channel", True)
            t.set_editor_property("max_texture_size", 1024)
        else:
            # Unreal prend parfois une texture très bleue pour une carte de relief
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
            t.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
            t.set_editor_property("srgb", True)
            t.set_editor_property("max_texture_size", 2048)
        assets.save_asset(f"{TEXTURES}/{nom}", only_if_is_dirty=False)
    unreal.log(f"PALMERA textures : {len(fichiers)}")


def parametre(g, nom, defaut, normale=False):
    e = g.n(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name=nom, texture=assets.load_asset(f"{TEXTURES}/{defaut}"))
    if normale:
        e.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    return e


def materiau_peau(defaut):
    g = Graphe("M_Peau", dossier=DOSSIER, nanite=False, instances=False)
    g.m.set_editor_property("used_with_skeletal_mesh", True)
    g.m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE_PROFILE)
    t = parametre(g, "Diffuse", defaut)
    g.sortie(t, MP.MP_BASE_COLOR, "RGB")
    g.sortie(g.constante(0.5), MP.MP_ROUGHNESS)
    g.sortie(g.constante(0.42), MP.MP_SPECULAR)
    g.sortie(g.constante(0.9), MP.MP_OPACITY)
    return g.fin()


def materiau_oeil(defaut):
    g = Graphe("M_Oeil", dossier=DOSSIER, nanite=False, instances=False)
    g.m.set_editor_property("used_with_skeletal_mesh", True)
    t = parametre(g, "Diffuse", defaut)
    g.sortie(t, MP.MP_BASE_COLOR, "RGB")
    g.sortie(g.constante(0.08), MP.MP_ROUGHNESS)
    g.sortie(g.constante(0.6), MP.MP_SPECULAR)
    return g.fin()


def materiau_poil(defaut):
    """Sourcils et cils : des poils dessinés sur de petites surfaces, le reste est découpé."""
    g = Graphe("M_Poil", dossier=DOSSIER, nanite=False, instances=False, two_sided=True)
    g.m.set_editor_property("used_with_skeletal_mesh", True)
    g.m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    g.m.set_editor_property("opacity_mask_clip_value", 0.42)
    t = parametre(g, "Diffuse", defaut)
    teinte = g.n(unreal.MaterialExpressionVectorParameter, parameter_name="Couleur", default_value=unreal.LinearColor(0.02, 0.015, 0.012, 1.0))
    g.sortie(teinte, MP.MP_BASE_COLOR, "RGB")
    g.sortie(t, MP.MP_OPACITY_MASK, "A")
    g.sortie(g.constante(0.7), MP.MP_ROUGHNESS)
    return g.fin()


def materiau_cheveux(defaut, normale):
    g = Graphe("M_Meches", dossier=DOSSIER, nanite=False, instances=False, two_sided=True)
    g.m.set_editor_property("used_with_skeletal_mesh", True)
    g.m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    g.m.set_editor_property("opacity_mask_clip_value", 0.35)
    t = parametre(g, "Diffuse", defaut)
    couleur = g.n(unreal.MaterialExpressionVectorParameter, parameter_name="Couleur", default_value=unreal.LinearColor(0.05, 0.03, 0.02, 1.0))
    moyenne = g.n(unreal.MaterialExpressionScalarParameter, parameter_name="Moyenne", default_value=0.2)
    k = g.custom(HLSL_CHEVEUX, {"T": (t, "RGB"), "Couleur": (couleur, "RGB"), "Moyenne": moyenne}, "float3", None, "Cheveux")
    g.sortie(k, MP.MP_BASE_COLOR)
    g.sortie(t, MP.MP_OPACITY_MASK, "A")
    g.sortie(g.constante(0.66), MP.MP_ROUGHNESS)
    g.sortie(g.constante(0.28), MP.MP_SPECULAR)
    if normale:
        g.sortie(parametre(g, "Normale", normale, True), MP.MP_NORMAL, "RGB")
    return g.fin()


def materiau_tissu(defaut, normale):
    """Tissu ou cuir : garde la texture, et peut en changer la couleur séparément en haut et en bas du corps."""
    g = Graphe("M_Vetement", dossier=DOSSIER, nanite=False, instances=False, two_sided=True)
    g.m.set_editor_property("used_with_skeletal_mesh", True)
    t = parametre(g, "Diffuse", defaut)
    pose = g.n(unreal.MaterialExpressionPreSkinnedPosition)
    p = g.n(unreal.MaterialExpressionVertexInterpolator)
    g.lier(pose, p, "VS")
    haut = g.n(unreal.MaterialExpressionVectorParameter, parameter_name="Haut", default_value=unreal.LinearColor(1, 1, 1, 1))
    bas = g.n(unreal.MaterialExpressionVectorParameter, parameter_name="Bas", default_value=unreal.LinearColor(1, 1, 1, 1))
    entrees = {"T": (t, "RGB"), "P": p, "Haut": (haut, "RGB"), "Bas": (bas, "RGB")}
    for nom, valeur in (("ForceHaut", 0.0), ("ForceBas", 0.0), ("Moyenne", 0.2)):
        entrees[nom] = g.n(unreal.MaterialExpressionScalarParameter, parameter_name=nom, default_value=valeur)
    k = g.custom(HLSL_TISSU, entrees, "float3", None, "Tissu")
    g.sortie(k, MP.MP_BASE_COLOR)
    g.sortie(parametre(g, "Normale", normale, True), MP.MP_NORMAL, "RGB")
    g.sortie(g.constante(0.78), MP.MP_ROUGHNESS)
    g.sortie(g.constante(0.3), MP.MP_SPECULAR)
    return g.fin()


def lineaire(rvb):
    """Couleur sRGB 0-255 vers une couleur linéaire."""
    return unreal.LinearColor(*[(c / 255.0) ** 2.2 for c in rvb], 1.0)


def instance(nom, parent, textures=None, vecteurs=None, scalaires=None):
    chemin = f"{INSTANCES}/{nom}"
    if assets.does_asset_exist(chemin):
        mi = assets.load_asset(chemin)
    else:
        mi = outils.create_asset(nom, INSTANCES, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for cle, valeur in (textures or {}).items():
        mel.set_material_instance_texture_parameter_value(mi, cle, assets.load_asset(f"{TEXTURES}/{valeur}"))
    for cle, valeur in (vecteurs or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, cle, valeur)
    for cle, valeur in (scalaires or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, cle, valeur)
    assets.save_asset(chemin)
    return mi


infos = json.loads((ICI / "textures.json").read_text(encoding="utf-8"))
casting = json.loads((ICI / "casting.json").read_text(encoding="utf-8"))
assets.make_directory(DOSSIER)
assets.make_directory(TEXTURES)
assets.make_directory(INSTANCES)

if "textures" in ETAPES:
    fichiers = {PLAT: True}
    for entree in infos.values():
        fichiers[entree["diffuse"]] = False
        if "normale" in entree:
            fichiers[entree["normale"]] = True
    importer_textures(fichiers)

if "materiaux" in ETAPES:
    valeurs = list(infos.items())
    peau = next(e for c, e in valeurs if c.startswith("peau:"))
    oeil = next(e for c, e in valeurs if c.startswith("yeux:"))
    poil = next(e for c, e in valeurs if c.startswith("sourcils:"))
    cheveux = next(e for c, e in valeurs if c.startswith("cheveux:"))
    tissu = next(e for c, e in valeurs if c.startswith("vetement:"))
    materiau_peau(peau["diffuse"])
    materiau_oeil(oeil["diffuse"])
    materiau_poil(poil["diffuse"])
    materiau_cheveux(cheveux["diffuse"], cheveux.get("normale"))
    materiau_tissu(tissu["diffuse"], tissu["normale"])

if "instances" in ETAPES:
    m_peau = assets.load_asset(f"{DOSSIER}/M_Peau")
    m_oeil = assets.load_asset(f"{DOSSIER}/M_Oeil")
    m_poil = assets.load_asset(f"{DOSSIER}/M_Poil")
    m_meches = assets.load_asset(f"{DOSSIER}/M_Meches")
    m_tissu = assets.load_asset(f"{DOSSIER}/M_Vetement")
    for fiche in casting:
        nom = fiche["nom"]
        if HUMAINS and nom not in HUMAINS:
            continue
        modele = assets.load_asset(f"{DOSSIER}/SKM_{nom}")
        if not modele:
            unreal.log_warning(f"PALMERA modèle absent : {nom}")
            continue
        poils = lineaire(fiche.get("couleur_cheveux", [24, 18, 14]))
        poils = unreal.LinearColor(min(poils.r, 0.08), min(poils.g, 0.07), min(poils.b, 0.07), 1.0)
        i = {}
        i["peau"] = instance(f"MI_Peau_{fiche['peau']}", m_peau, {"Diffuse": infos[f"peau:{fiche['peau']}"]["diffuse"]})
        i["yeux"] = instance(f"MI_Yeux_{fiche['yeux']}", m_oeil, {"Diffuse": infos[f"yeux:{fiche['yeux']}"]["diffuse"]})
        i["sourcils"] = instance(f"MI_Sourcils_{nom}", m_poil, {"Diffuse": infos[f"sourcils:{fiche['sourcils']}"]["diffuse"]}, {"Couleur": poils})
        i["cils"] = instance(f"MI_Cils_{nom}", m_poil, {"Diffuse": infos[f"cils:{fiche['cils']}"]["diffuse"]}, {"Couleur": unreal.LinearColor(0.01, 0.008, 0.008, 1.0)})
        if fiche["cheveux"]:
            e = infos[f"cheveux:{fiche['cheveux']}"]
            textures = {"Diffuse": e["diffuse"]}
            if "normale" in e:
                textures["Normale"] = e["normale"]
            i["cheveux"] = instance(f"MI_Cheveux_{nom}", m_meches, textures, {"Couleur": lineaire(fiche["couleur_cheveux"])}, {"Moyenne": e["moyenne"]})
        e = infos[f"vetement:{fiche['vetement']}"]
        i["vetement"] = instance(f"MI_Vetement_{nom}", m_tissu, {"Diffuse": e["diffuse"], "Normale": e.get("normale", PLAT)},
                                 {"Haut": lineaire(fiche.get("haut", [255, 255, 255])), "Bas": lineaire(fiche.get("bas", [255, 255, 255]))},
                                 {"ForceHaut": fiche.get("force_haut", 0.0), "ForceBas": fiche.get("force_bas", 0.0), "Moyenne": e["moyenne"]})
        e = infos[f"chaussures:{fiche['chaussures']}"]
        i["chaussures"] = instance(f"MI_Chaussures_{fiche['chaussures']}", m_tissu, {"Diffuse": e["diffuse"], "Normale": e.get("normale", PLAT)}, None, {"Moyenne": e["moyenne"]})
        # Parcourir la liste donne des copies : il faut la reconstruire puis la réécrire
        emplacements = []
        for em in modele.get_editor_property("materials"):
            slot = str(em.get_editor_property("material_slot_name"))
            if slot in i:
                em.set_editor_property("material_interface", i[slot])
            else:
                unreal.log_warning(f"PALMERA emplacement inconnu {nom} : {slot}")
            emplacements.append(em)
        modele.set_editor_property("materials", emplacements)
        assets.save_asset(f"{DOSSIER}/SKM_{nom}", only_if_is_dirty=False)
        unreal.log(f"PALMERA habillé : {nom}")
if "lods" in ETAPES:
    # Versions allégées pour la distance : le moteur choisit seul selon la taille à l'écran
    sous_systeme = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    for fiche in casting:
        nom = fiche["nom"]
        if HUMAINS and nom not in HUMAINS:
            continue
        modele = assets.load_asset(f"{DOSSIER}/SKM_{nom}")
        fait = sous_systeme.regenerate_lod(modele, 4, True, False)
        assets.save_asset(f"{DOSSIER}/SKM_{nom}", only_if_is_dirty=False)
        unreal.log(f"PALMERA LOD {nom} : {fait}, {sous_systeme.get_lod_count(modele)} niveaux, {[sous_systeme.get_num_verts(modele, i) for i in range(sous_systeme.get_lod_count(modele))]}")
unreal.log("PALMERA humains prêts")
