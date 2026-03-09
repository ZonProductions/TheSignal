import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# --- Delete old decal covers ---
all_actors = subsys.get_all_level_actors()
deleted = 0
for a in all_actors:
    if 'KioskSignCover' in a.get_actor_label():
        a.destroy_actor()
        deleted += 1
print(f'Deleted {deleted} old decal covers')

# --- Create flat opaque material matching kiosk door color ---
mat_path = '/Game/TheSignal/Materials/M_SignCover'
mat = unreal.load_asset(mat_path)
if mat:
    eal.delete_asset(mat_path)
    print('Deleted old M_SignCover')

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.MaterialFactoryNew()
mat = asset_tools.create_asset('M_SignCover', '/Game/TheSignal/Materials', unreal.Material, factory)

# DefaultLit, opaque — matches surrounding door color
color_node = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -200, 0)
color_node.set_editor_property('constant', unreal.LinearColor(r=0.75, g=0.75, b=0.75, a=1.0))
mel.connect_material_property(color_node, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)

# Slight roughness to match matte door finish
rough_node = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 200)
rough_node.set_editor_property('r', 0.8)
mel.connect_material_property(rough_node, '', unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(mat)
eal.save_asset(mat_path)
print(f'Created material: {mat_path}')

# Also clean up old M_StallCover decal material
if eal.does_asset_exist('/Game/TheSignal/Materials/M_StallCover'):
    eal.delete_asset('/Game/TheSignal/Materials/M_StallCover')
    print('Deleted old M_StallCover decal material')

# Save level
subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsys_level.save_all_dirty_levels()
print('Done.')
