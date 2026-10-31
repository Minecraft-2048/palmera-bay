"""Compare le squelette d'un humain importé à celui du jeu : un écart de rotation ferait déraper les animations."""
import math
import sys
from pathlib import Path

SORTIE = Path(r"E:\UnrealTools\MakeHuman\sortie")


def lire(fichier):
    os_ = {}
    for ligne in fichier.read_text(encoding="utf-8").splitlines():
        nom, parent, *v = ligne.split()
        v = [float(x) for x in v]
        os_[nom] = (parent, v[:3], v[3:])
    return os_


def comparer(nom, base):
    a, b = lire(SORTIE / f"verif_SKM_{nom}.txt"), lire(SORTIE / f"os_{base}.txt")
    manquants = sorted(set(b) - set(a))
    ecart_t, ecart_r, pire = 0.0, 0.0, ""
    for os_, (parent, t, q) in a.items():
        if os_ not in b:
            continue
        t2, q2 = b[os_][1], b[os_][2]
        dt = math.dist(t, t2)
        produit = min(abs(sum(x * y for x, y in zip(q, q2))), 1.0)
        dr = math.degrees(2 * math.acos(produit))
        if dr > ecart_r:
            ecart_r, pire = dr, os_
        ecart_t = max(ecart_t, dt)
    print(f"{nom} : {len(a)} os, manquants {manquants[:6]}, ecart maxi {ecart_t:.3f} cm, rotation maxi {ecart_r:.3f} degres ({pire})")


for argument in sys.argv[1:]:
    nom, base = argument.split(":")
    comparer(nom, base)
