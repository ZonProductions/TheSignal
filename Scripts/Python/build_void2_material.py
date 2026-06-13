"""Fresh material M_Void2 (neutral near-black glow) to replace the navy
M_VoidCube on SM_VoidCube. M_VoidCube is left untouched (it's reference-locked).
Naming: M_ prefix per project convention; 'void2' per dev request."""
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAT = "/Game/HorrorLight/M_Void2"

if EAL.does_asset_exist(MAT):
    EAL.delete_asset(MAT)
at = unreal.AssetToolsHelpers.get_asset_tools()
mat = at.create_asset("M_Void2", "/Game/HorrorLight", unreal.Material, unreal.MaterialFactoryNew())
assert mat is not None, "create failed"
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -50)
color.set_editor_property("parameter_name", "GlowColor")
color.set_editor_property("default_value", unreal.LinearColor(0.02, 0.02, 0.02, 1.0))  # neutral near-black
strength = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 150)
strength.set_editor_property("parameter_name", "GlowStrength")
strength.set_editor_property("default_value", 1.0)
mult = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 0)
MEL.connect_material_expressions(color, "", mult, "A")
MEL.connect_material_expressions(strength, "", mult, "B")
MEL.connect_material_property(mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
EAL.save_asset(MAT)

sm = unreal.load_asset("/Game/HorrorLight/SM_VoidCube")
sm.set_material(0, mat)
EAL.save_asset("/Game/HorrorLight/SM_VoidCube")
print("M_Void2 created (neutral glow) + assigned to SM_VoidCube. SM mat now:",
      sm.get_material(0).get_name())
