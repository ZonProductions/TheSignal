import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAT = "/Game/HorrorLight/M_VoidCube"

sm = unreal.load_asset("/Game/HorrorLight/SM_VoidCube")
print("SM_VoidCube exists:", sm is not None, "| mat before:", sm.get_material(0))
print("M_VoidCube exists before:", EAL.does_asset_exist(MAT))

if EAL.does_asset_exist(MAT):
    print("delete old:", EAL.delete_asset(MAT))

at = unreal.AssetToolsHelpers.get_asset_tools()
mat = at.create_asset("M_VoidCube", "/Game/HorrorLight", unreal.Material, unreal.MaterialFactoryNew())
print("created M_VoidCube:", mat is not None)
assert mat is not None, "create still failed - asset may still be locked"

mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -50)
color.set_editor_property("parameter_name", "GlowColor")
color.set_editor_property("default_value", unreal.LinearColor(0.02, 0.02, 0.02, 1.0))
strength = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 150)
strength.set_editor_property("parameter_name", "GlowStrength")
strength.set_editor_property("default_value", 1.0)
mult = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 0)
MEL.connect_material_expressions(color, "", mult, "A")
MEL.connect_material_expressions(strength, "", mult, "B")
MEL.connect_material_property(mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
EAL.save_asset(MAT)

sm.set_material(0, mat)
EAL.save_asset("/Game/HorrorLight/SM_VoidCube")
print("DONE | SM mat after:", sm.get_material(0).get_name(), "| GlowColor (0.02,0.02,0.02) neutral")
