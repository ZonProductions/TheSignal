import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# Use a new name to avoid delete+create timing issues
mat_path = '/Game/TheSignal/Materials/M_StallDoor'
if eal.does_asset_exist(mat_path):
    eal.delete_asset(mat_path)
    unreal.SystemLibrary.delay(None, 0.1)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.MaterialFactoryNew()
mat = asset_tools.create_asset('M_StallDoor', '/Game/TheSignal/Materials', unreal.Material, factory)
print(f'Created mat: {mat}')

if mat:
    # Light warm grey to approximate kiosk door color
    color_node = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -200, 0)
    print(f'Color node: {color_node}')
    if color_node:
        color_node.set_editor_property('constant', unreal.LinearColor(r=0.85, g=0.84, b=0.82, a=1.0))
        mel.connect_material_property(color_node, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)

    rough_node = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 200)
    if rough_node:
        rough_node.set_editor_property('r', 0.7)
        mel.connect_material_property(rough_node, '', unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(mat)
    eal.save_asset(mat_path)
    print(f'Saved {mat_path}')
