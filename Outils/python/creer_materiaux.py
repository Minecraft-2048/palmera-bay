"""Crée les matériaux de la ville : surfaces, route, façades, vitres, métaux, lumières."""
import sys

import unreal

sys.path.insert(0, r"E:\Minecraft\Claude Unreal Engine\PalmeraBay\Outils\python")
from mat_outils import Graphe  # noqa: E402

MP = unreal.MaterialProperty

HLSL_SURFACE = """
float m = Pluie * saturate(0.3 + 1.1 * Bruit.r) * saturate(NormaleZ * 2.0);
Rugosite = lerp(ORM.g, ORM.g * 0.4, m);
Occlusion = ORM.r;
Metal = ORM.b;
float salete = 0.86 + 0.26 * Bruit.g;
return Couleur * Teinte * salete * lerp(1.0, 0.62, m);
"""


def surface(nom, tex, echelle, teinte=(1, 1, 1), instance_teinte=False):
    """Matériau standard : couleur, normale, occlusion/rugosité/métal, assombri par la pluie."""
    g = Graphe(nom)
    uv = g.uv(echelle)
    c = g.texture(f"T_{tex}_C", uv)
    n = g.texture(f"T_{tex}_N", uv, normale=True)
    o = g.texture(f"T_{tex}_ORM", uv, masque=True)
    p = g.monde()
    masque = g.n(unreal.MaterialExpressionComponentMask, r=True, g=True, b=False, a=False)
    g.lier(p, masque)
    uv_b = g.n(unreal.MaterialExpressionMultiply, const_b=0.0173)
    g.lier(masque, uv_b, "A")
    b = g.texture("T_Bruit", uv_b, masque=True)
    if instance_teinte:
        t = g.n(unreal.MaterialExpressionAppendVector)
        tr = g.instance(0, 1.0)
        tv = g.instance(1, 1.0)
        tb = g.instance(2, 1.0)
        t2 = g.n(unreal.MaterialExpressionAppendVector)
        g.lier(tr, t2, "A")
        g.lier(tv, t2, "B")
        g.lier(t2, t, "A")
        g.lier(tb, t, "B")
    else:
        t = g.constante(*teinte)
    nz = g.n(unreal.MaterialExpressionVertexNormalWS)
    masque_z = g.n(unreal.MaterialExpressionComponentMask, r=False, g=False, b=True, a=False)
    g.lier(nz, masque_z)
    pl = g.collection("Pluie")
    k = g.custom(HLSL_SURFACE, {"Couleur": (c, "RGB"), "ORM": (o, "RGB"), "Bruit": (b, "RGB"), "Teinte": t,
                                "Pluie": pl, "NormaleZ": masque_z},
                 "float3", {"Rugosite": "float", "Occlusion": "float", "Metal": "float"}, "Surface")
    g.sortie(k, MP.MP_BASE_COLOR)
    g.sortie(n, MP.MP_NORMAL, "RGB")
    g.sortie(k, MP.MP_ROUGHNESS, "Rugosite")
    g.sortie(k, MP.MP_AMBIENT_OCCLUSION, "Occlusion")
    g.sortie(k, MP.MP_METALLIC, "Metal")
    return g.fin()


HLSL_ROUTE = """
float2 P = PM.xy;
float w = max(Dist * 0.0000035, 0.012);
float2 g = P / 120.0;
float2 d = (g - round(g)) * 120.0;
float2 a = abs(d);
#define TRAIT(x, l) smoothstep((l) + w, (l) - w, abs(x))
#define ENTRE(x, b0, b1) (smoothstep((b0) - w, (b0) + w, (x)) * smoothstep((b1) + w, (b1) - w, (x)))
float blanc = 0.0, jaune = 0.0;
float surNS = step(a.x, 7.0) * step(7.0, a.y);
float surEO = step(a.y, 7.0) * step(7.0, a.x);
/* Rues nord-sud */
jaune += surNS * TRAIT(a.x - 0.14, 0.055) * ENTRE(a.y, 12.9, 200.0);
blanc += surNS * TRAIT(a.x - 3.5, 0.06) * ENTRE(frac(P.y / 9.0), 0.0, 0.34) * ENTRE(a.y, 15.0, 200.0);
blanc += surNS * TRAIT(a.x - 6.72, 0.06) * ENTRE(a.y, 12.0, 200.0);
blanc += surNS * ENTRE(a.y, 12.2, 12.7) * step(0.0, d.x * d.y) * ENTRE(a.x, 0.25, 6.8);
blanc += surNS * ENTRE(a.y, 8.0, 11.8) * ENTRE(frac(P.x / 1.1), 0.0, 0.5) * ENTRE(a.x, 0.0, 6.7);
/* Rues est-ouest : dans ce sens, la voie de droite change de côté */
jaune += surEO * TRAIT(a.y - 0.14, 0.055) * ENTRE(a.x, 12.9, 200.0);
blanc += surEO * TRAIT(a.y - 3.5, 0.06) * ENTRE(frac(P.x / 9.0), 0.0, 0.34) * ENTRE(a.x, 15.0, 200.0);
blanc += surEO * TRAIT(a.y - 6.72, 0.06) * ENTRE(a.x, 12.0, 200.0);
blanc += surEO * ENTRE(a.x, 12.2, 12.7) * step(d.x * d.y, 0.0) * ENTRE(a.y, 0.25, 6.8);
blanc += surEO * ENTRE(a.x, 8.0, 11.8) * ENTRE(frac(P.y / 1.1), 0.0, 0.5) * ENTRE(a.y, 0.0, 6.7);
float usure = smoothstep(0.22, 0.62, Bruit.g * 0.7 + Grain * 0.6 + Bruit.r * 0.25);
float masque = saturate(blanc + jaune) * usure;
/* Bandes de roulement : la chaussée est plus lisse et plus sombre sous les pneus */
float v = abs(frac((surNS > 0.5 ? a.x : a.y) / 3.5) - 0.5) * 3.5;
float trace = exp(-pow((v - 0.85) / 0.38, 2.0)) * saturate(surNS + surEO);
float3 c = Couleur * (0.8 + 0.42 * Bruit.r) * lerp(1.0, 0.8, trace);
float3 peinture = lerp(float3(0.75, 0.75, 0.72), float3(0.78, 0.5, 0.06), saturate(jaune));
c = lerp(c, peinture, masque * 0.92);
float rug = lerp(ORM.g - trace * 0.14, 0.6, masque);
Occlusion = ORM.r;
/* Pluie : la chaussée mouillée devient miroir, les flaques remplissent les creux */
float flaque = smoothstep(0.5, 0.6, Bruit.r + Bruit.b * 0.35 - 0.12) * Pluie;
float mouille = saturate(Pluie * 1.4);
Rugosite = lerp(rug, rug * 0.42, mouille);
Rugosite = lerp(Rugosite, 0.04, flaque);
Plat = saturate(masque * 0.7 + flaque);
return c * lerp(1.0, 0.55, mouille) * lerp(1.0, 0.75, flaque);
"""


def route():
    g = Graphe("M_Route")
    p = g.monde()
    masque = g.n(unreal.MaterialExpressionComponentMask, r=True, g=True, b=False, a=False)
    g.lier(p, masque)
    uv = g.n(unreal.MaterialExpressionMultiply, const_b=0.25)
    g.lier(masque, uv, "A")
    c = g.texture("T_Asphalte_C", uv)
    n = g.texture("T_Asphalte_N", uv, normale=True)
    o = g.texture("T_Asphalte_ORM", uv, masque=True)
    uv_b = g.n(unreal.MaterialExpressionMultiply, const_b=0.0173)
    g.lier(masque, uv_b, "A")
    b = g.texture("T_Bruit", uv_b, masque=True)
    uv_g = g.n(unreal.MaterialExpressionMultiply, const_b=0.33)
    g.lier(masque, uv_g, "A")
    grain = g.texture("T_Bruit", uv_g, masque=True)
    dist = g.n(unreal.MaterialExpressionPixelDepth)
    k = g.custom(HLSL_ROUTE, {"PM": p, "Dist": dist, "Couleur": (c, "RGB"), "ORM": (o, "RGB"), "Bruit": (b, "RGB"),
                              "Grain": (grain, "G"), "Pluie": g.collection("Pluie")},
                 "float3", {"Rugosite": "float", "Occlusion": "float", "Plat": "float"}, "Route")
    plat = g.constante(0, 0, 1)
    nf = g.n(unreal.MaterialExpressionLinearInterpolate)
    g.lier(n, nf, "A", "RGB")
    g.lier(plat, nf, "B")
    g.lier(k, nf, "Alpha", "Plat")
    g.sortie(k, MP.MP_BASE_COLOR)
    g.sortie(nf, MP.MP_NORMAL)
    g.sortie(k, MP.MP_ROUGHNESS, "Rugosite")
    g.sortie(k, MP.MP_AMBIENT_OCCLUSION, "Occlusion")
    return g.fin()


HLSL_VITRE = """
float2 id = floor(UV * Module);
float2 f = frac(UV * Module);
float h1 = frac(sin(dot(id + Graine * 91.7, float2(12.9898, 78.233))) * 43758.5453);
float h2 = frac(h1 * 57.31 + 0.13);
float h3 = frac(h1 * 91.73 + 0.37);
float h4 = frac(h1 * 23.17 + 0.71);
float3 ro = float3(f * 2.0 - 1.0, 0.0);
float3 rd = -normalize(V);
rd.z = min(rd.z, -0.02);
float prof = 1.3 + 1.1 * h2;
float3 inv = 1.0 / rd;
float3 tp = float3((sign(rd.x) - ro.x) * inv.x, (sign(rd.y) - ro.y) * inv.y, (-prof - ro.z) * inv.z);
float t = min(tp.x, min(tp.y, tp.z));
float3 p = ro + rd * t;
float3 mur = lerp(float3(0.62, 0.55, 0.45), float3(0.5, 0.58, 0.62), h2) * (0.75 + 0.5 * h3);
float3 c;
if (t == tp.z) {
    /* Mur du fond : un meuble sombre et un tableau */
    c = mur;
    c *= 1.0 - 0.6 * step(abs(p.x - (h3 - 0.5)), 0.35) * step(p.y, -0.3 + h4 * 0.2);
    c = lerp(c, float3(h4, h2, h1) * 0.8, step(abs(p.x + 0.5 * h1 - 0.2), 0.16) * step(abs(p.y - 0.3), 0.2));
} else if (t == tp.y) {
    c = rd.y < 0.0 ? float3(0.24, 0.17, 0.12) * (0.7 + 0.3 * h4) : float3(0.82, 0.8, 0.76);
} else {
    c = mur * 0.82;
}
if (Boutique > 0.5 && t == tp.z) {
    /* Rayonnages de la boutique : des rangées de produits colorés */
    float2 q = float2(p.x * 3.0 + h1 * 9.0, p.y * 2.5);
    float2 ci = floor(q);
    float2 cf = frac(q);
    float hc = frac(sin(dot(ci, float2(41.3, 289.1))) * 43758.5453);
    float3 produit = lerp(float3(0.8, 0.75, 0.6), float3(frac(hc * 7.0), frac(hc * 13.0), frac(hc * 23.0)), 0.75);
    c = lerp(float3(0.12, 0.1, 0.09), produit, step(cf.y, 0.72) * step(0.08, cf.x) * step(cf.x, 0.92));
}
/* Lumière du plafonnier : plus forte au centre, atténuée vers le fond */
float lampe = 0.42 + 0.9 * saturate(0.5 + 0.5 * p.y) * saturate(1.0 - length(p.xz + float2(0.0, 0.6 * prof)) * 0.42);
float3 teinte = h1 < 0.55 ? float3(1.0, 0.76, 0.5) : (h1 < 0.86 ? float3(1.0, 0.92, 0.78) : float3(0.72, 0.84, 1.0));
/* Rideaux ou stores : cachent le haut de certaines fenêtres */
float rideau = step(0.42, h2) * step(1.0 - (0.15 + 0.6 * h3), f.y);
c = lerp(c * lampe, float3(0.92, 0.82, 0.68) * 0.55, rideau);
/* Allumage progressif au crépuscule ; toutes les pièces ne sont pas occupées */
float allume = smoothstep(h3 * 0.75, h3 * 0.75 + 0.1, Nuit) * step(Occupe, h4);
float jour = saturate(1.0 - Nuit * 1.6);
Pieces = c * (allume * Force + jour * Jour);
return Pieces;
"""

HLSL_FACADE = """
/* Salissures : coulures verticales sous les rebords, pied de mur plus sombre, usure générale */
float coulure = smoothstep(0.42, 0.78, Coulures.r) * smoothstep(0.3, 0.7, Coulures.b);
float pied = saturate(1.0 - Z / 5.0);
float salete = saturate(coulure * 0.34 + pied * pied * 0.26 + (Bruit.b - 0.5) * 0.28);
float m = Pluie * saturate(0.45 + Bruit.r);
Rugosite = lerp(ORM.g, 0.42, m);
Occlusion = ORM.r;
float3 c = Couleur * Teinte * (0.84 + 0.3 * Bruit.g);
c = lerp(c, c * float3(0.52, 0.5, 0.46), salete);
return c * lerp(1.0, 0.68, m);
"""


def facade():
    """Enduit teinté par instance, vieilli par des coulures calculées sur la hauteur."""
    g = Graphe("M_Facade")
    uv = g.uv(1.0)
    c = g.texture("T_Enduit_C", uv)
    n = g.texture("T_Enduit_N", uv, normale=True)
    o = g.texture("T_Enduit_ORM", uv, masque=True)
    p = g.monde()
    xy = g.n(unreal.MaterialExpressionComponentMask, r=True, g=True, b=False, a=False)
    g.lier(p, xy)
    uv_b = g.n(unreal.MaterialExpressionMultiply, const_b=0.0173)
    g.lier(xy, uv_b, "A")
    b = g.texture("T_Bruit", uv_b, masque=True)
    z = g.n(unreal.MaterialExpressionComponentMask, r=False, g=False, b=True, a=False)
    g.lier(p, z)
    coulures_uv = g.custom("return float2((P.x + P.y) * 0.11, P.z * 0.012);", {"P": p}, "float2", None, "UV coulures")
    co = g.texture("T_Bruit", coulures_uv, masque=True)
    teinte = g.n(unreal.MaterialExpressionAppendVector)
    t2 = g.n(unreal.MaterialExpressionAppendVector)
    g.lier(g.instance(0, 1.0), t2, "A")
    g.lier(g.instance(1, 1.0), t2, "B")
    g.lier(t2, teinte, "A")
    g.lier(g.instance(2, 1.0), teinte, "B")
    k = g.custom(HLSL_FACADE, {"Couleur": (c, "RGB"), "ORM": (o, "RGB"), "Bruit": (b, "RGB"), "Coulures": (co, "RGB"), "Teinte": teinte, "Z": z,
                               "Pluie": g.collection("Pluie")}, "float3", {"Rugosite": "float", "Occlusion": "float"}, "Facade")
    g.sortie(k, MP.MP_BASE_COLOR)
    g.sortie(n, MP.MP_NORMAL, "RGB")
    g.sortie(k, MP.MP_ROUGHNESS, "Rugosite")
    g.sortie(k, MP.MP_AMBIENT_OCCLUSION, "Occlusion")
    return g.fin()


HLSL_ALLEGE = """
/* Sur les tours, le bas de chaque étage est une bande sombre et mate : c'est elle qui dessine les étages de loin */
float b = Bandes > 0.5 ? step(frac(UV.y), 0.24) : 0.0;
Metal2 = lerp(Metal, 0.25, b);
Rugosite2 = lerp(Rugosite, 0.32, b);
return Couleur * lerp(1.0, 0.3, b);
"""


def vitre(nom, module=(1.0, 1.0), force=6.0, occupe=0.42, metal=0.55, couleur=(0.22, 0.24, 0.27), rugosite=0.05, jour=200.0, boutique=0.0, bandes=0.0):
    """Vitre opaque et réfléchissante : on voit derrière une fausse pièce en perspective."""
    g = Graphe(nom)
    cam = g.n(unreal.MaterialExpressionCameraVectorWS)
    vue = g.n(unreal.MaterialExpressionTransform, transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,
              transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
    g.lier(cam, vue)
    k = g.custom(HLSL_VITRE, {"UV": g.uv(), "V": vue, "Module": g.constante(*module), "Graine": g.instance(6, 0.5),
                              "Nuit": g.collection("Nuit"), "Force": g.constante(force), "Occupe": g.constante(occupe), "Jour": g.constante(jour),
                              "Boutique": g.constante(boutique)},
                 "float3", {"Pieces": "float3"}, "Pieces")
    a = g.custom(HLSL_ALLEGE, {"UV": g.uv(), "Bandes": g.constante(bandes), "Couleur": g.constante(*couleur), "Metal": g.constante(metal), "Rugosite": g.constante(rugosite)},
                 "float3", {"Metal2": "float", "Rugosite2": "float"}, "Allege")
    g.sortie(a, MP.MP_BASE_COLOR)
    g.sortie(a, MP.MP_METALLIC, "Metal2")
    g.sortie(a, MP.MP_ROUGHNESS, "Rugosite2")
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


HLSL_COLLINE = """
/* Roche sur les pentes fortes, maquis ailleurs, mélangés par un bruit très large pour éviter toute répétition */
float pente = saturate((0.95 - NZ) * 5.0);
float maquis = saturate((1.0 - pente) * 1.3 + (B.r - 0.5) * 1.6 + (B2.g - 0.5) * 0.9);
float3 vert = lerp(float3(0.024, 0.046, 0.016), float3(0.08, 0.1, 0.036), B2.b);
float3 roche = lerp(C1, C2, 0.5) * (0.62 + 0.55 * B.g);
return lerp(roche, vert, maquis * 0.9);
"""


def colline():
    g = Graphe("M_Colline")
    p = g.monde()
    xy = g.n(unreal.MaterialExpressionComponentMask, r=True, g=True, b=False, a=False)
    g.lier(p, xy)

    def echelle(k):
        m = g.n(unreal.MaterialExpressionMultiply, const_b=k)
        g.lier(xy, m, "A")
        return m

    c1 = g.texture("T_Colline_C", echelle(0.0041))
    c2 = g.texture("T_Colline_C", echelle(0.0197))
    n = g.texture("T_Colline_N", echelle(0.0197), normale=True)
    b = g.texture("T_Bruit", echelle(0.00083), masque=True)
    b2 = g.texture("T_Bruit", echelle(0.0061), masque=True)
    nz = g.n(unreal.MaterialExpressionComponentMask, r=False, g=False, b=True, a=False)
    g.lier(g.n(unreal.MaterialExpressionVertexNormalWS), nz)
    k = g.custom(HLSL_COLLINE, {"C1": (c1, "RGB"), "C2": (c2, "RGB"), "B": (b, "RGB"), "B2": (b2, "RGB"), "NZ": nz}, "float3", None, "Colline")
    g.sortie(k, MP.MP_BASE_COLOR)
    g.sortie(n, MP.MP_NORMAL, "RGB")
    g.sortie(g.constante(0.9), MP.MP_ROUGHNESS)
    return g.fin()


def uni(nom, couleur, rugosite, metal=0.0):
    g = Graphe(nom)
    g.sortie(g.constante(*couleur), MP.MP_BASE_COLOR)
    g.sortie(g.constante(rugosite), MP.MP_ROUGHNESS)
    g.sortie(g.constante(metal), MP.MP_METALLIC)
    return g.fin()


HLSL_NEON = """
float pulse = 1.0;
return Couleur * (1.2 + Force * smoothstep(0.1, 0.8, Nuit)) * pulse;
"""


def teinte_instance(g, i0):
    t2 = g.n(unreal.MaterialExpressionAppendVector)
    g.lier(g.instance(i0, 1.0), t2, "A")
    g.lier(g.instance(i0 + 1, 0.5), t2, "B")
    t = g.n(unreal.MaterialExpressionAppendVector)
    g.lier(t2, t, "A")
    g.lier(g.instance(i0 + 2, 0.3), t, "B")
    return t


def neon(nom="M_Neon", force=42.0):
    g = Graphe(nom)
    t = teinte_instance(g, 3)
    k = g.custom(HLSL_NEON, {"Couleur": t, "Nuit": g.collection("Nuit"), "Force": g.constante(force)}, "float3", None, "Neon")
    g.sortie(g.constante(0.02, 0.02, 0.02), MP.MP_BASE_COLOR)
    g.sortie(g.constante(0.3), MP.MP_ROUGHNESS)
    g.sortie(k, MP.MP_EMISSIVE_COLOR)
    return g.fin()


def auvent():
    g = Graphe("M_Auvent", two_sided=True)
    t = teinte_instance(g, 3)
    u = g.n(unreal.MaterialExpressionComponentMask, r=True, g=False, b=False, a=False)
    g.lier(g.uv(), u)
    k = g.custom("float r = step(frac(U * 1.25), 0.5);\nreturn Couleur * lerp(1.0, 0.72, r) * (1.0 - 0.3 * Pluie);",
                 {"Couleur": t, "U": u, "Pluie": g.collection("Pluie")}, "float3", None, "Rayures")
    g.sortie(k, MP.MP_BASE_COLOR)
    g.sortie(g.constante(0.75), MP.MP_ROUGHNESS)
    return g.fin()


if __name__ == "__main__":
    surface("M_Trottoir", "Trottoir", 0.33)
    surface("M_Sable", "Sable", 0.25)
    surface("M_Herbe", "Herbe", 0.33)
    colline()
    surface("M_Beton", "Beton", 1.0, (0.86, 0.85, 0.82))
    facade()
    surface("M_Brique", "Brique", 1.0)
    surface("M_Toit", "Gravier", 1.0)
    surface("M_Tuile", "Tuile", 1.0)
    surface("M_Tole", "Tole", 1.0, instance_teinte=True)
    surface("M_Bois", "Bois", 1.0)
    surface("M_Ecorce", "Ecorce", 1.0)
    route()
    vitre("M_Vitre")
    vitre("M_VitreTour", (0.25, 1.0), 4.5, 0.5, 0.85, (0.2, 0.3, 0.35), 0.05, 90.0, 0.0, 1.0)
    vitre("M_Vitrine", (1.0, 1.0), 9.0, 0.1, 0.3, (0.1, 0.11, 0.12), 0.04, 170.0, 1.0)
    uni("M_Cadre", (0.03, 0.032, 0.035), 0.42, 0.9)
    uni("M_Metal", (0.42, 0.43, 0.44), 0.48, 0.95)
    uni("M_Enseigne", (0.05, 0.06, 0.07), 0.5, 0.2)
    neon()
    auvent()
    unreal.log("PALMERA matériaux terminés")
