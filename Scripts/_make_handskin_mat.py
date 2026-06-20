import unreal

pkg = '/Game/Core/Materials'
mle = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

base_obj = pkg + '/M_HandSkin.M_HandSkin'
inst_obj = pkg + '/MI_HandSkin.MI_HandSkin'

# Clean recreate
for p in (inst_obj, base_obj):
    if unreal.EditorAssetLibrary.does_asset_exist(p):
        unreal.EditorAssetLibrary.delete_asset(p)

# Base material: flat skin (UV-independent), opaque, default lit.
mat = tools.create_asset('M_HandSkin', pkg, unreal.Material, unreal.MaterialFactoryNew())

col = mle.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -100)
col.set_editor_property('parameter_name', 'SkinColor')
# Default ~ CCMH skin tone (sRGB ~ 222,172,142). LinearColor takes 0-1 sRGB-ish; tune in instance.
col.set_editor_property('default_value', unreal.LinearColor(0.78, 0.55, 0.42, 1.0))
mle.connect_material_property(col, '', unreal.MaterialProperty.MP_BASE_COLOR)

rough = mle.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 120)
rough.set_editor_property('parameter_name', 'Roughness')
rough.set_editor_property('default_value', 0.55)
mle.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)

mle.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)

# Instance so the dev can tune SkinColor/Roughness live.
inst = tools.create_asset('MI_HandSkin', pkg, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
mle.set_material_instance_parent(inst, mat)
unreal.EditorAssetLibrary.save_loaded_asset(inst, only_if_is_dirty=False)

print("M_HandSkin exists:", unreal.EditorAssetLibrary.does_asset_exist(base_obj))
print("MI_HandSkin exists:", unreal.EditorAssetLibrary.does_asset_exist(inst_obj))
