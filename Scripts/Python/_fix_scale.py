import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

mi = unreal.load_asset('/Game/TheSignal/Materials/MI_StallDoorCover')
mel.set_material_instance_scalar_parameter_value(mi, 'Texture_Scale', 0.01)
mel.update_material_instance(mi)
eal.save_asset('/Game/TheSignal/Materials/MI_StallDoorCover')
print('Texture_Scale set to 0.01')
