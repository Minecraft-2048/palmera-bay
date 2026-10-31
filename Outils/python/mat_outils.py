"""Aide à la construction de matériaux Unreal depuis Python."""
import unreal

mel = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary
outils = unreal.AssetToolsHelpers.get_asset_tools()

DOSSIER = "/Game/Materiaux"
SORTIES = {
    "float": unreal.CustomMaterialOutputType.CMOT_FLOAT1,
    "float2": unreal.CustomMaterialOutputType.CMOT_FLOAT2,
    "float3": unreal.CustomMaterialOutputType.CMOT_FLOAT3,
    "float4": unreal.CustomMaterialOutputType.CMOT_FLOAT4,
}


class Graphe:
    """Un matériau en cours de construction : recrée les nœuds, garde les références au matériau."""

    def __init__(self, nom, dossier=DOSSIER, nanite=True, instances=True, **proprietes):
        self.chemin = f"{dossier}/{nom}"
        if assets.does_asset_exist(self.chemin):
            self.m = assets.load_asset(self.chemin)
            mel.delete_all_material_expressions(self.m)
        else:
            self.m = outils.create_asset(nom, dossier, unreal.Material, unreal.MaterialFactoryNew())
        for cle, valeur in proprietes.items():
            self.m.set_editor_property(cle, valeur)
        if nanite:
            self.m.set_editor_property("used_with_nanite", True)
        if instances:
            self.m.set_editor_property("used_with_instanced_static_meshes", True)
        self.x = -1500
        self.y = 0

    def n(self, classe, **proprietes):
        e = mel.create_material_expression(self.m, classe, self.x, self.y)
        self.y += 150
        if self.y > 1500:
            self.y = 0
            self.x += 320
        for cle, valeur in proprietes.items():
            e.set_editor_property(cle, valeur)
        return e

    def lier(self, de, vers, entree="", sortie=""):
        ok = mel.connect_material_expressions(de, sortie, vers, entree)
        if not ok:
            unreal.log_warning(f"PALMERA lien refusé dans {self.chemin} : {sortie} -> {entree}")

    def texture(self, nom, uv=None, normale=False, masque=False):
        """Échantillonne /Game/Textures/<nom>. masque : données linéaires (ORM, bruit)."""
        t = assets.load_asset(f"/Game/Textures/{nom}")
        if t is None:
            raise RuntimeError(f"texture absente : {nom}")
        e = self.n(unreal.MaterialExpressionTextureSample, texture=t)
        if normale:
            e.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif masque:
            e.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        if uv is not None:
            self.lier(uv, e, "UVs")
        return e

    def uv(self, echelle=1.0, index=0):
        return self.n(unreal.MaterialExpressionTextureCoordinate, coordinate_index=index, u_tiling=echelle, v_tiling=echelle)

    def constante(self, *valeurs):
        if len(valeurs) == 1:
            return self.n(unreal.MaterialExpressionConstant, r=valeurs[0])
        if len(valeurs) == 2:
            return self.n(unreal.MaterialExpressionConstant2Vector, r=valeurs[0], g=valeurs[1])
        return self.n(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(valeurs[0], valeurs[1], valeurs[2], 1))

    def collection(self, nom):
        mpc = assets.load_asset(f"{DOSSIER}/MPC_Palmera")
        return self.n(unreal.MaterialExpressionCollectionParameter, collection=mpc, parameter_name=nom)

    def instance(self, index, defaut=0.0):
        return self.n(unreal.MaterialExpressionPerInstanceCustomData, data_index=index, const_default_value=defaut)

    def monde(self):
        """Position du pixel dans le monde, en mètres."""
        p = self.n(unreal.MaterialExpressionWorldPosition)
        d = self.n(unreal.MaterialExpressionDivide, const_b=100.0)
        self.lier(p, d, "A")
        return d

    def custom(self, code, entrees, sortie="float3", sorties=None, description="HLSL"):
        """entrees : {nom : expression ou (expression, sortie)}. sorties : {nom : type} en plus du retour."""
        c = self.n(unreal.MaterialExpressionCustom, code=code, output_type=SORTIES[sortie], description=description)
        liste = []
        for nom in entrees:
            i = unreal.CustomInput()
            i.set_editor_property("input_name", nom)
            liste.append(i)
        c.set_editor_property("inputs", liste)
        if sorties:
            extra = []
            for nom, type_ in sorties.items():
                o = unreal.CustomOutput()
                o.set_editor_property("output_name", nom)
                o.set_editor_property("output_type", SORTIES[type_])
                extra.append(o)
            c.set_editor_property("additional_outputs", extra)
        for nom, valeur in entrees.items():
            e, s = valeur if isinstance(valeur, tuple) else (valeur, "")
            self.lier(e, c, nom, s)
        return c

    def sortie(self, expression, propriete, nom=""):
        ok = mel.connect_material_property(expression, nom, propriete)
        if not ok:
            unreal.log_warning(f"PALMERA sortie refusée dans {self.chemin} : {nom} -> {propriete}")

    def fin(self):
        mel.layout_material_expressions(self.m)
        mel.recompile_material(self.m)
        assets.save_asset(self.chemin)
        unreal.log(f"PALMERA matériau prêt : {self.chemin}")
        return self.m
