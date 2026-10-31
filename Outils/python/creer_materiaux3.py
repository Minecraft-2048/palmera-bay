"""Matériaux des voitures de tous les jours et des personnages."""
import sys

import unreal

sys.path.insert(0, r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Outils\python")
from mat_outils import Graphe  # noqa: E402

MP = unreal.MaterialProperty



def peinture():
    """Peinture de carrosserie à vernis : la couleur vient de chaque voiture."""
    g = Graphe("M_Peinture")
    t = g.n(unreal.MaterialExpressionVectorParameter, parameter_name="Paint Tint", default_value=unreal.LinearColor(0.6, 0.6, 0.62, 1.0))
    g.sortie(t, MP.MP_BASE_COLOR, "RGB")
    g.sortie(g.constante(0.45), MP.MP_METALLIC)
    g.sortie(g.constante(0.2), MP.MP_ROUGHNESS)
    return g.fin()


def uni(nom, couleur, rugosite, metal=0.0):
    g = Graphe(nom)
    g.sortie(g.constante(*couleur), MP.MP_BASE_COLOR)
    g.sortie(g.constante(rugosite), MP.MP_ROUGHNESS)
    g.sortie(g.constante(metal), MP.MP_METALLIC)
    return g.fin()


def feu(nom, couleur, code):
    """Optique de voiture : éteinte le jour, allumée par les phares, plus forte au freinage pour l'arrière."""
    g = Graphe(nom)
    e = g.n(unreal.MaterialExpressionScalarParameter, parameter_name="EmissivePower", default_value=0.0)
    f = g.n(unreal.MaterialExpressionScalarParameter, parameter_name="Frein", default_value=0.0)
    k = g.custom(code, {"Phares": e, "Frein": f}, "float3", None, "Feu")
    g.sortie(g.constante(*couleur), MP.MP_BASE_COLOR)
    g.sortie(g.constante(0.12), MP.MP_ROUGHNESS)
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


HLSL_ENSEIGNE_UV = """
float cellule = floor(UV.x);
float h = frac(sin(cellule * 12.9898 + Graine * 78.233) * 43758.5453);
float rang = floor(h * 32.0);
float col = floor(rang / 16.0);
float lig = rang - col * 16.0;
return float2((col + frac(UV.x)) / 2.0, (lig + saturate(UV.y)) / 16.0);
"""


def enseigne():
    """Nom de boutique pris sur la planche : couleur de la façade le jour, néon la nuit."""
    g = Graphe("M_Enseigne")
    t2 = g.n(unreal.MaterialExpressionAppendVector)
    g.lier(g.instance(3, 1.0), t2, "A")
    g.lier(g.instance(4, 0.5), t2, "B")
    accent = g.n(unreal.MaterialExpressionAppendVector)
    g.lier(t2, accent, "A")
    g.lier(g.instance(5, 0.3), accent, "B")
    uv = g.custom(HLSL_ENSEIGNE_UV, {"UV": g.uv(), "Graine": g.instance(6, 0.5)}, "float2", None, "Case")
    tex = g.texture("T_Enseignes", uv, masque=True)
    k = g.custom("Base = lerp(float3(0.02, 0.022, 0.026), Accent, Masque);\nreturn Accent * Masque * (0.4 + Nuit * 16.0);",
                 {"Accent": accent, "Masque": (tex, "R"), "Nuit": g.collection("Nuit")}, "float3", {"Base": "float3"}, "Neon")
    g.sortie(k, MP.MP_BASE_COLOR, "Base")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    g.sortie(g.constante(0.4), MP.MP_ROUGHNESS)
    return g.fin()


def peinture_garee():
    """Même peinture que les voitures, teintée par instance pour les voitures à l'arrêt."""
    g = Graphe("M_PeintureGaree")
    t2 = g.n(unreal.MaterialExpressionAppendVector)
    g.lier(g.instance(0, 0.6), t2, "A")
    g.lier(g.instance(1, 0.6), t2, "B")
    t = g.n(unreal.MaterialExpressionAppendVector)
    g.lier(t2, t, "A")
    g.lier(g.instance(2, 0.62), t, "B")
    g.sortie(t, MP.MP_BASE_COLOR)
    g.sortie(g.constante(0.45), MP.MP_METALLIC)
    g.sortie(g.constante(0.2), MP.MP_ROUGHNESS)
    return g.fin()


def peint():
    """Surface peinte ou meuble : la couleur vient de chaque sommet, avec un léger grain."""
    g = Graphe("M_Peint", nanite=True, instances=True)
    couleur = g.n(unreal.MaterialExpressionVertexColor)
    k = g.custom("float n = frac(sin(dot(floor(P.xy * 40.0), float2(12.9898, 78.233))) * 43758.5453);\nreturn C * (0.96 + 0.08 * n);",
                 {"C": couleur, "P": g.monde()}, "float3", None, "Peint")
    g.sortie(k, MP.MP_BASE_COLOR)
    g.sortie(g.constante(0.55), MP.MP_ROUGHNESS)
    return g.fin()


def lumineux():
    """Néons, écrans de bornes, plafonniers : la couleur du sommet éclaire d'elle-même."""
    g = Graphe("M_Lumineux", nanite=True, instances=True)
    g.m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    couleur = g.n(unreal.MaterialExpressionVertexColor)
    k = g.custom("return C * 14.0;", {"C": couleur}, "float3", None, "Lumineux")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


def ecran_cine():
    """L'écran du cinéma : des images qui changent doucement, comme une bande-annonce vue de loin."""
    g = Graphe("M_EcranCine", nanite=True, instances=True)
    g.m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    k = g.custom("float s = floor(T / 3.0);\nfloat2 u = UV * float2(2.0, 1.0);\nfloat3 a = 0.5 + 0.5 * cos(float3(0.0, 2.1, 4.2) + s * 1.7 + u.x * 2.0 + sin(u.y * 3.0 + T * 0.3));\nfloat v = smoothstep(0.0, 0.25, frac(T / 3.0)) * smoothstep(1.0, 0.8, frac(T / 3.0));\nreturn a * v * 16.0;",
                 {"T": g.n(unreal.MaterialExpressionTime), "UV": g.uv(0.2)}, "float3", None, "Ecran")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


if __name__ == "__main__":
    peinture()
    uni("M_VitreAuto", (0.05, 0.06, 0.065), 0.04, 0.85)
    uni("M_Plastique", (0.012, 0.012, 0.014), 0.62)
    uni("M_Chrome", (0.82, 0.83, 0.85), 0.16, 1.0)
    uni("M_Pneu", (0.012, 0.012, 0.012), 0.86)
    feu("M_FeuAvant", (0.72, 0.74, 0.76), "return float3(1.0, 0.95, 0.85) * Phares * 6.0;")
    feu("M_FeuArriere", (0.22, 0.008, 0.01), "return float3(1.0, 0.03, 0.02) * (Phares * 1.2 + Frein * 300.0);")
    peint()
    lumineux()
    ecran_cine()
    enseigne()
    peinture_garee()
    uni("M_Tissu", (0.5, 0.4, 0.24), 0.92)
    unreal.log("PALMERA matériaux terminés")
