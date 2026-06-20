import unreal

pkg_path = '/Game/Core/Materials'
mat_name = 'M_Invisible'
obj_path = pkg_path + '/' + mat_name + '.' + mat_name

tools = unreal.AssetToolsHelpers.get_asset_tools()
mle = unreal.MaterialEditingLibrary

# Recreate cleanly so we know its graph is correct.
if unreal.EditorAssetLibrary.does_asset_exist(obj_path):
    unreal.EditorAssetLibrary.delete_asset(obj_path)

mf = unreal.MaterialFactoryNew()
mat = tools.create_asset(mat_name, pkg_path, unreal.Material, mf)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
const = mle.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 0)
const.set_editor_property('r', 0.0)
ok = mle.connect_material_property(const, '', unreal.MaterialProperty.MP_OPACITY_MASK)
mle.recompile_material(mat)

# Robust save: save the loaded asset object, then verify on disk.
saved = unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
exists = unreal.EditorAssetLibrary.does_asset_exist(obj_path)
print("connect_opacity_mask:", ok, "| save_loaded_asset:", saved, "| does_asset_exist:", exists)
