import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# Create MI_StallDoorCover — same textures as kiosk door but with high
# Texture_Scale so the sign shrinks to invisible, showing only door surface pattern
mi_path = '/Game/TheSignal/Materials/MI_StallDoorCover'
if eal.does_asset_exist(mi_path):
    eal.delete_asset(mi_path)

# Duplicate MI_WCPackB (has correct Color, Normal, RAM textures)
eal.duplicate_asset('/Game/office_BigCompanyArchViz/Materials/MI_WCPackB', mi_path)
mi = unreal.load_asset(mi_path)
print(f'Created: {mi_path}')

# Set Texture_Scale high — sign becomes tiny, door surface pattern dominates
mel.set_material_instance_scalar_parameter_value(mi, 'Texture_Scale', 6.0)
mel.update_material_instance(mi)
eal.save_asset(mi_path)
print('Set Texture_Scale=6, saved')
print('Done — set this on BP_SignCover CoverMesh material slot')
