import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAT = "/Game/HorrorLight/M_VoidCube"

sm = unreal.load_asset("/Game/HorrorLight/SM_VoidCube")
sm.set_material(0, None)          # unhook before delete so the ref is clean
if EAL.does_asset_exist(MAT):
    EAL.delete_asset(MAT)

at = unreal.AssetToolsHelpers.get_asset_tools()
mat = at.create_asset("M_VoidCube", "/Game/HorrorLight", unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -50)
color.set_editor_property("parameter_name", "GlowColor")
color.set_editor_property("default_value", unreal.LinearColor(0.02, 0.02, 0.02, 1.0))  # neutral near-black glow
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
print("M_VoidCube rebuilt neutral (0.02,0.02,0.02) + reassigned to SM_VoidCube")
