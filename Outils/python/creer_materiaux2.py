"""Crée les matériaux restants : océan, palmes, lampes, feux tricolores, piscine."""
import sys

import unreal

sys.path.insert(0, r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Outils\python")
from mat_outils import Graphe  # noqa: E402

MP = unreal.MaterialProperty

VAGUES = """
#define VAGUE(dx, dy, L, A, V) { float k = 6.28318 / (L); float w = sqrt(9.81 * k) * (V); float ph = k * ((dx) * p.x + (dy) * p.y) - w * T; \\
    float c = cos(ph), s = sin(ph); float a = (A) * prof; o.z += a * c; o.xy -= float2(dx, dy) * a * 0.75 * s; pente -= float2(dx, dy) * k * a * s; }
"""

HLSL_HOULE = VAGUES + """
float2 p = PM.xy;
float2 pente = 0.0;
float3 o = 0.0;
float D = -975.0 - p.x;
float fond = 0.12 - 0.019 * D - 0.000012 * D * D;
float hauteur = -0.6 - fond;
float prof = saturate((hauteur - 0.2) / 3.5) * (1.0 - smoothstep(2500.0, 3300.0, length(p)));
if (p.x > -1000.0) prof = 0.35 * (1.0 - smoothstep(2500.0, 3300.0, length(p)));
VAGUE(0.93, 0.37, 74.0, 0.42, 0.9)
VAGUE(0.99, -0.14, 41.0, 0.27, 0.95)
VAGUE(0.8, 0.6, 23.0, 0.14, 1.0)
VAGUE(0.6, -0.8, 17.0, 0.08, 1.0)
/* Le bord de l'eau monte et descend sur le sable */
float rivage = saturate(1.0 - hauteur / 2.0) * step(p.x, -990.0);
o.z += rivage * (0.13 * sin(T * 0.55 + p.y * 0.023 + 2.0 * sin(p.y * 0.007)) + 0.05 * sin(T * 1.1 + p.y * 0.05));
return o * 100.0;
"""

HLSL_SURFACE_EAU = VAGUES + """
float2 p = PM.xy;
float2 pente = 0.0;
float3 o = 0.0;
float D = -975.0 - p.x;
float fond = 0.12 - 0.019 * D - 0.000012 * D * D;
float hauteur = -0.6 - fond;
float prof = saturate((hauteur - 0.2) / 3.5) * (1.0 - smoothstep(2500.0, 3300.0, length(p)));
if (p.x > -1000.0) prof = 0.35 * (1.0 - smoothstep(2500.0, 3300.0, length(p)));
VAGUE(0.93, 0.37, 74.0, 0.42, 0.9)
VAGUE(0.99, -0.14, 41.0, 0.27, 0.95)
VAGUE(0.8, 0.6, 23.0, 0.14, 1.0)
VAGUE(0.6, -0.8, 17.0, 0.08, 1.0)
float loinC = saturate(Dist / 90000.0);
float2 detail = (NA.xy * 0.55 + NB.xy * 0.4) * lerp(1.0, 0.06, loinC) * (0.55 + 0.45 * Pluie);
float2 n2 = -pente + detail * 0.42;
Normale = normalize(float3(n2.x, n2.y, 1.0));
/* Écume : lignes qui arrivent sur la plage, déchirées par le bruit */
float rive = saturate(1.0 - hauteur / 1.35) * step(p.x, -990.0);
float vagues = sin(hauteur * 5.5 + T * 0.7 + Bruit.r * 5.0 + sin(p.y * 0.02) * 1.5);
float ecume = saturate(smoothstep(0.1, 0.95, vagues) * rive * 1.4);
float bord = smoothstep(0.32, 0.02, hauteur + (Bruit.b - 0.5) * 0.3 + 0.05 * sin(T * 0.55 + p.y * 0.023));
ecume = saturate(ecume + bord * step(p.x, -990.0));
ecume *= smoothstep(0.26, 0.6, Bruit.g + Bruit.b * 0.6);
/* Moutons au large, sur les crêtes */
float crete = saturate((length(pente) - 0.17) * 9.0) * smoothstep(0.55, 0.8, Bruit.g);
Ecume = saturate(ecume + crete * 0.5);
return Ecume;
"""


def ocean():
    # La sortie d'eau survit à l'effacement des nœuds : sans matériau neuf, elle serait en double
    assets = unreal.EditorAssetLibrary
    if assets.does_asset_exist("/Game/Materiaux/M_Ocean"):
        assets.delete_asset("/Game/Materiaux/M_Ocean")
    g = Graphe("M_Ocean", nanite=False, instances=False, tangent_space_normal=False)
    g.m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    p = g.monde()
    t = g.n(unreal.MaterialExpressionTime)
    xy = g.n(unreal.MaterialExpressionComponentMask, r=True, g=True, b=False, a=False)
    g.lier(p, xy)
    def defile(echelle, vx, vy):
        m = g.n(unreal.MaterialExpressionMultiply, const_b=echelle)
        g.lier(xy, m, "A")
        a = g.n(unreal.MaterialExpressionPanner, speed_x=vx, speed_y=vy)
        g.lier(m, a, "Coordinate")
        g.lier(t, a, "Time")
        return a
    na = g.texture("T_Eau_A_N", defile(0.031, 0.011, 0.004), normale=True)
    nb = g.texture("T_Eau_B_N", defile(0.11, -0.012, 0.016), normale=True)
    b = g.texture("T_Bruit", defile(0.043, 0.006, 0.002), masque=True)
    dist = g.n(unreal.MaterialExpressionPixelDepth)
    k = g.custom(HLSL_SURFACE_EAU, {"PM": p, "T": t, "NA": (na, "RGB"), "NB": (nb, "RGB"), "Bruit": (b, "RGB"), "Dist": dist,
                                    "Pluie": g.collection("Pluie")},
                 "float", {"Normale": "float3", "Ecume": "float"}, "Surface de l'eau")
    wpo = g.custom(HLSL_HOULE, {"PM": p, "T": t}, "float3", None, "Houle")
    g.sortie(wpo, MP.MP_WORLD_POSITION_OFFSET)
    g.sortie(k, MP.MP_NORMAL, "Normale")
    blanc = g.constante(0.85, 0.88, 0.88)
    nul = g.constante(0.0, 0.0, 0.0)
    coul = g.n(unreal.MaterialExpressionLinearInterpolate)
    g.lier(nul, coul, "A")
    g.lier(blanc, coul, "B")
    g.lier(k, coul, "Alpha", "Ecume")
    g.sortie(coul, MP.MP_BASE_COLOR)
    rug = g.n(unreal.MaterialExpressionLinearInterpolate, const_a=0.035, const_b=0.75)
    g.lier(k, rug, "Alpha", "Ecume")
    g.sortie(rug, MP.MP_ROUGHNESS)
    g.sortie(g.constante(0.5), MP.MP_SPECULAR)
    g.sortie(k, MP.MP_OPACITY, "Ecume")
    sortie = g.n(unreal.MaterialExpressionSingleLayerWaterMaterialOutput)
    g.lier(g.constante(0.012, 0.055, 0.07), sortie, "ScatteringCoefficients")
    g.lier(g.constante(0.42, 0.09, 0.05), sortie, "AbsorptionCoefficients")
    g.lier(g.constante(0.4), sortie, "PhaseG")
    mat = g.fin()
    mer = "/Game/Modeles/Sol/SM_Ocean/StaticMeshes/SM_Ocean"
    sm = assets.load_asset(mer)
    if sm:
        sm.set_material(0, mat)
        assets.save_asset(mer)
    return mat


HLSL_PALME = """
float t = UV.y;
float bout = saturate((UV.x - 0.5) * 2.0);
float3 c = lerp(float3(0.035, 0.105, 0.022), float3(0.07, 0.16, 0.032), Hasard);
c = lerp(c, float3(0.2, 0.235, 0.055), saturate(bout * 0.55 + t * t * 0.5));
Sous = c * float3(1.7, 2.3, 0.9);
return c * (1.0 - 0.28 * Pluie);
"""

HLSL_VENT = """
float h = saturate((PL.z - 800.0) / 700.0);
float bras = saturate(length(PL.xy) / 450.0);
float phase = PW.x * 0.0011 + PW.y * 0.0009;
float rafale = sin(T * 0.9 + phase) * 0.6 + sin(T * 2.3 + phase * 3.0) * 0.3;
float3 v = float3(0.8, 0.5, 0.0) * rafale * 10.0 * h;
v.z += sin(T * 3.1 + phase * 6.0 + UV.y * 8.0) * 7.0 * bras * bras;
return v;
"""


def palme():
    msm = unreal.MaterialShadingModel
    feuillage = next(getattr(msm, n) for n in dir(msm) if "FOLIAGE" in n.upper())
    g = Graphe("M_Palme", shading_model=feuillage, two_sided=True)
    uv = g.uv()
    hasard = g.n(unreal.MaterialExpressionPerInstanceRandom)
    k = g.custom(HLSL_PALME, {"UV": uv, "Hasard": hasard, "Pluie": g.collection("Pluie")}, "float3", {"Sous": "float3"}, "Palme")
    g.sortie(k, MP.MP_BASE_COLOR)
    g.sortie(k, MP.MP_SUBSURFACE_COLOR, "Sous")
    g.sortie(g.constante(0.55), MP.MP_ROUGHNESS)
    pw = g.n(unreal.MaterialExpressionWorldPosition)
    pl = g.n(unreal.MaterialExpressionLocalPosition)
    wpo = g.custom(HLSL_VENT, {"PW": pw, "PL": pl, "T": g.n(unreal.MaterialExpressionTime), "UV": uv}, "float3", None, "Vent")
    g.sortie(wpo, MP.MP_WORLD_POSITION_OFFSET)
    return g.fin()


def lampe():
    g = Graphe("M_Lampe")
    k = g.custom("return float3(1.0, 0.8, 0.56) * (2.0 + Nuit * 900.0);", {"Nuit": g.collection("Nuit")}, "float3", None, "Lampe")
    g.sortie(g.constante(0.5, 0.45, 0.35), MP.MP_BASE_COLOR)
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


HLSL_FEU = """
float t = fmod(Temps + Axe * 12.0, 24.0);
float etat = t < 10.0 ? 0.0 : (t < 12.0 ? 1.0 : 2.0);
float l = min(floor(UV.y * 3.0), 2.0);
float3 c = l < 0.5 ? float3(0.01, 1.0, 0.2) : (l < 1.5 ? float3(1.0, 0.42, 0.01) : float3(1.0, 0.012, 0.01));
float2 cu = float2(UV.x, frac(UV.y * 3.0)) - 0.5;
float rond = smoothstep(0.46, 0.42, length(cu));
float allume = abs(l - etat) < 0.5 ? 1.0 : 0.0;
Teinte = lerp(float3(0.012, 0.012, 0.012), c * 0.12, rond);
return c * allume * rond * (1500.0 * saturate(1.0 - Nuit) + 160.0);
"""


def feu():
    g = Graphe("M_Feu")
    k = g.custom(HLSL_FEU, {"UV": g.uv(), "Temps": g.collection("TempsFeux"), "Axe": g.instance(0, 0.0), "Nuit": g.collection("Nuit")},
                 "float3", {"Teinte": "float3"}, "Feu")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    g.sortie(k, MP.MP_BASE_COLOR, "Teinte")
    g.sortie(g.constante(0.25), MP.MP_ROUGHNESS)
    return g.fin()


def piscine():
    g = Graphe("M_Piscine")
    t = g.n(unreal.MaterialExpressionTime)
    a = g.n(unreal.MaterialExpressionPanner, speed_x=0.03, speed_y=0.02)
    g.lier(g.uv(1.0), a, "Coordinate")
    g.lier(t, a, "Time")
    n = g.texture("T_Eau_B_N", a, normale=True)
    k = g.custom("return float3(0.02, 0.5, 0.62) * Nuit * 12.0;", {"Nuit": g.collection("Nuit")}, "float3", None, "Piscine")
    g.sortie(g.constante(0.03, 0.42, 0.55), MP.MP_BASE_COLOR)
    g.sortie(n, MP.MP_NORMAL, "RGB")
    g.sortie(g.constante(0.04), MP.MP_ROUGHNESS)
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


HLSL_CHUTE = """
float z = frac(UV.y * 7.0 - T * 0.34) * 44.0 - 22.0;
float3 droite = normalize(cross(Vue, float3(0.0, 0.0, 1.0)));
return float3(0.0, 0.0, z * 100.0) + droite * (UV.x - 0.5) * 0.9;
"""


def pluie():
    """Bâtonnets de pluie : tombent sans fin autour de la caméra."""
    g = Graphe("M_Pluie", nanite=False, instances=False, two_sided=True)
    g.m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    g.m.set_editor_property("enable_responsive_aa", True)
    g.m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    vue = g.n(unreal.MaterialExpressionSubtract)
    g.lier(g.n(unreal.MaterialExpressionCameraPositionWS), vue, "A")
    g.lier(g.n(unreal.MaterialExpressionWorldPosition), vue, "B")
    wpo = g.custom(HLSL_CHUTE, {"UV": g.uv(), "T": g.n(unreal.MaterialExpressionTime), "Vue": vue}, "float3", None, "Chute")
    g.sortie(wpo, MP.MP_WORLD_POSITION_OFFSET)
    k = g.custom("return float3(0.75, 0.82, 0.95) * Pluie * lerp(190.0, 2.2, Nuit);", {"Pluie": g.collection("Pluie"), "Nuit": g.collection("Nuit")}, "float3", None, "Eclat")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


HLSL_ETOILES = """
float3 d = -normalize(Vue);
float3 p = d * 170.0;
float3 i = floor(p);
float3 f = frac(p) - 0.5;
float h = frac(sin(dot(i, float3(127.1, 311.7, 74.7))) * 43758.5453);
float h2 = frac(h * 91.7);
float etoile = step(0.9915, h) * smoothstep(0.34, 0.02, length(f)) * (0.35 + 3.2 * h2 * h2 * h2);
float haut = saturate(d.z);
float lueur = 0.34 * pow(1.0 - haut, 3.0) + 0.1;
float3 fond = float3(0.1, 0.17, 0.38) * lueur;
return (fond + etoile * float3(0.9, 0.95, 1.0) * saturate(d.z * 6.0)) * Nuit;
"""


def ciel():
    """Le ciel est une grande sphère : l'atmosphère du moteur, plus les étoiles et le bleu de la nuit."""
    g = Graphe("M_Ciel", nanite=False, instances=False, two_sided=True)
    g.m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    g.m.set_editor_property("is_sky", True)
    atmo = g.n(unreal.MaterialExpressionSkyAtmosphereViewLuminance)
    d0 = g.n(unreal.MaterialExpressionSkyAtmosphereLightDiskLuminance, light_index=0)
    d1 = g.n(unreal.MaterialExpressionSkyAtmosphereLightDiskLuminance, light_index=1)
    et = g.custom(HLSL_ETOILES, {"Vue": g.n(unreal.MaterialExpressionCameraVectorWS), "Nuit": g.collection("Nuit")}, "float3", None, "Etoiles")
    k = g.custom("return A + D0 + D1 + E;", {"A": atmo, "D0": d0, "D1": d1, "E": et}, "float3", None, "Ciel")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


def gyrophare():
    """Rampe de police : moitié rouge, moitié bleue, en alternance."""
    g = Graphe("M_Gyrophare", nanite=False, instances=False)
    k = g.custom("float a = frac(T * 3.0) < 0.5 ? 1.0 : 0.0;\nreturn (P.y < 0.0 ? float3(1.0, 0.02, 0.02) * a : float3(0.03, 0.12, 1.0) * (1.0 - a)) * 1800.0 + 2.0;",
                 {"T": g.n(unreal.MaterialExpressionTime), "P": g.n(unreal.MaterialExpressionLocalPosition)}, "float3", None, "Gyrophare")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    g.sortie(g.constante(0.02, 0.02, 0.02), MP.MP_BASE_COLOR)
    return g.fin()


def fumee():
    """Bouffée de fumée : disque doux dont l'opacité et la teinte viennent de chaque instance."""
    g = Graphe("M_Fumee", nanite=False, instances=True, two_sided=True)
    g.m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    g.m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    uv = g.uv()
    k = g.custom("float2 d = UV - 0.5;\nfloat r = saturate(1.0 - length(d) * 2.0);\nOpacite = r * r * Force;\nreturn float3(Gris, Gris, Gris * 1.03) * lerp(5200.0, 8.0, Nuit);",
                 {"UV": uv, "Force": g.instance(0, 0.5), "Gris": g.instance(1, 0.5), "Nuit": g.collection("Nuit")}, "float3", {"Opacite": "float"}, "Fumee")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    g.sortie(k, MP.MP_OPACITY, "Opacite")
    return g.fin()


def balise():
    """Colonne de lumière des points de mission, visible de jour comme de nuit."""
    g = Graphe("M_Balise", nanite=False, instances=False, two_sided=True)
    g.m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    g.m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    c = g.n(unreal.MaterialExpressionVectorParameter, parameter_name="Couleur", default_value=unreal.LinearColor(1.0, 0.7, 0.2, 1.0))
    z = g.n(unreal.MaterialExpressionLocalPosition)
    k = g.custom("float h = saturate(P.z / 100.0 + 0.5);\nfloat loin = saturate((Dist - 1500.0) / 5000.0);\nreturn Couleur.rgb * pow(1.0 - h, 2.2) * lerp(2400.0, 18.0, Nuit) * loin * loin;",
                 {"Couleur": c, "P": z, "Nuit": g.collection("Nuit"), "Dist": g.n(unreal.MaterialExpressionPixelDepth)}, "float3", None, "Balise")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


if __name__ == "__main__":
    ocean()
    palme()
    lampe()
    feu()
    piscine()
    balise()
    pluie()
    gyrophare()
    ciel()
    fumee()
    unreal.log("PALMERA matériaux terminés")
