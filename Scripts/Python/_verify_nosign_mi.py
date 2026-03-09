import unreal

mi = unreal.load_asset('/Game/TheSignal/Materials/MI_WCPackB_NoSign')
print(f'MI: {mi.get_name()}')
tps = mi.get_editor_property('texture_parameter_values')
for tp in tps:
    pinfo = tp.get_editor_property('parameter_info')
    pname = pinfo.get_editor_property('name')
    pval = tp.get_editor_property('parameter_value')
    print(f'  {pname}: {pval.get_name() if pval else "None"} ({pval.get_path_name() if pval else "N/A"})')

# The texture might not have actually changed. Force-set it again.
tex_a = unreal.load_asset('/Game/office_BigCompanyArchViz/Textures/T_WCPackA_BaseColor')
print(f'\nT_WCPackA_BaseColor: {tex_a.get_name() if tex_a else "NOT FOUND"}')

# Try setting via set_editor_property on the MI directly
unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(mi, 'Color', tex_a)
unreal.MaterialEditingLibrary.update_material_instance(mi)
unreal.EditorAssetLibrary.save_asset('/Game/TheSignal/Materials/MI_WCPackB_NoSign')
print('Force-set Color param via MaterialEditingLibrary, saved.')

# Verify
tps2 = mi.get_editor_property('texture_parameter_values')
for tp in tps2:
    pinfo = tp.get_editor_property('parameter_info')
    pname = pinfo.get_editor_property('name')
    pval = tp.get_editor_property('parameter_value')
    print(f'  {pname}: {pval.get_name() if pval else "None"}')
