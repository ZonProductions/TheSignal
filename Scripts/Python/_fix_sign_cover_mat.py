import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# Create MI_SignCover as a child of M_ProbBase (same parent as the kiosk material)
# Use engine white texture for Color — gives us same shading as door, no sign
mi_path = '/Game/TheSignal/Materials/MI_SignCover'
mi = unreal.load_asset(mi_path)
if mi:
    eal.delete_asset(mi_path)

# Duplicate MI_WCPackB so we get same params, then swap Color texture to plain white
src = '/Game/office_BigCompanyArchViz/Materials/MI_WCPackB'
eal.duplicate_asset(src, mi_path)
mi = unreal.load_asset(mi_path)
print(f'Duplicated MI_WCPackB -> {mi_path}')

# Use engine white texture as Color (removes the sign)
white_tex = unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture')
if white_tex:
    mel.set_material_instance_texture_parameter_value(mi, 'Color', white_tex)
    mel.update_material_instance(mi)
    eal.save_asset(mi_path)
    print(f'Set Color to WhiteSquareTexture, saved')
else:
    print('ERROR: WhiteSquareTexture not found')

# Delete the old flat M_SignCover material
if eal.does_asset_exist('/Game/TheSignal/Materials/M_SignCover'):
    eal.delete_asset('/Game/TheSignal/Materials/M_SignCover')
    print('Deleted old M_SignCover')

print('Done')
