import unreal

mi = unreal.load_asset('/Game/Core/Meshes/MI_Herrington_11-87_Police')
if mi:
    parent = mi.get_editor_property('parent')
    unreal.log(f'MI parent: {parent.get_path_name() if parent else "NONE"}')
    for tp in mi.texture_parameter_values:
        info = tp.get_editor_property('parameter_info')
        val = tp.get_editor_property('parameter_value')
        unreal.log(f'  tex param {info.get_editor_property("name")}: {val.get_path_name() if val else "NONE"}')
else:
    unreal.log_warning('MI not found')

found = unreal.EditorAssetLibrary.list_assets('/Game/KINEMATION', recursive=True, include_folder=False)
hits = [a for a in found if 'herrington' in a.lower() or '11-87' in a.lower() or '11_87' in a.lower()]
unreal.log(f'--- KINEMATION Herrington assets: {len(hits)}')
for h in hits[:25]:
    unreal.log(f'  {h}')
