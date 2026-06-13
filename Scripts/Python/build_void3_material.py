"""M_Void3 = blackhole-black: pure black body, faint Fresnel rim (event-horizon
edge) + a tiny base glow so the cubes stay visible. Unlit. Assigned to
SM_VoidCube. Tunable params: RimColor, BaseGlow."""
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAT = "/Game/HorrorLight/M_Void3"

if EAL.does_asset_exist(MAT):
    EAL.delete_asset(MAT)
at = unreal.AssetToolsHelpers.get_asset_tools()
mat = at.create_asset("M_Void3", "/Game/HorrorLight", unreal.Material, unreal.MaterialFactoryNew())
assert mat is not None
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

fres = MEL.create_material_expression(mat, unreal.MaterialExpressionFresnel, -700, -100)
fres.set_editor_property("exponent", 3.0)
fres.set_editor_property("base_reflect_fraction", 0.0)
rim = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, 80)
rim.set_editor_property("parameter_name", "RimColor")
rim.set_editor_property("default_value", unreal.LinearColor(0.04, 0.04, 0.06, 1.0))  # faint cool edge
base = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -450, 220)
base.set_editor_property("parameter_name", "BaseGlow")
base.set_editor_property("default_value", unreal.LinearColor(0.004, 0.004, 0.004, 1.0))  # near-pure-black

mult = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -450, -20)
MEL.connect_material_expressions(fres, "", mult, "A")
MEL.connect_material_expressions(rim, "", mult, "B")
add = MEL.create_material_expression(mat, unreal.MaterialExpressionAdd, -220, 60)
MEL.connect_material_expressions(mult, "", add, "A")
MEL.connect_material_expressions(base, "", add, "B")
MEL.connect_material_property(add, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
EAL.save_asset(MAT)

sm = unreal.load_asset("/Game/HorrorLight/SM_VoidCube")
sm.set_material(0, mat)
EAL.save_asset("/Game/HorrorLight/SM_VoidCube")
print("M_Void3 (blackhole black + faint rim) assigned. SM mat now:",
      sm.get_material(0).get_name())
