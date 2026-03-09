"""Create a simple dark material for door sign backgrounds."""
import unreal

MAT_PATH = "/Game/TheSignal/Materials"
MAT_NAME = "M_DoorSignDark"
full = f"{MAT_PATH}/{MAT_NAME}"

if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_factory = unreal.MaterialFactoryNew()
mat = asset_tools.create_asset(MAT_NAME, MAT_PATH, unreal.Material, mat_factory)

if mat:
    # Dark constant color → BaseColor
    color_node = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant3Vector, -300, 0)
    color_node.set_editor_property("constant", unreal.LinearColor(0.02, 0.02, 0.02, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        color_node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    # High roughness
    rough_node = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -300, 200)
    rough_node.set_editor_property("r", 0.9)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)

    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(full)
    print(f"Created {MAT_NAME}")
else:
    print("Failed to create material")
