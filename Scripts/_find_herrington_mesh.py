import unreal

found = unreal.EditorAssetLibrary.list_assets('/Game/KINEMATION', recursive=True, include_folder=False)
hits = [a for a in found if ('herrington' in a.lower() or '11-87' in a.lower())
        and ('/Animations/' not in a)]
unreal.log(f'--- non-animation Herrington assets: {len(hits)}')
for h in hits:
    unreal.log(f'  {h}')

# Inspect the Kinemation skeletal mesh's materials if found
for h in hits:
    name = h.split('.')[-1]
    if name.startswith(('SK_', 'SKM_')):
        mesh = unreal.load_asset(h)
        if isinstance(mesh, unreal.SkeletalMesh):
            unreal.log(f'=== {name} materials:')
            for m in mesh.materials:
                iface = m.get_editor_property('material_interface')
                slot = m.get_editor_property('material_slot_name')
                unreal.log(f'  slot {slot}: {iface.get_path_name() if iface else "NONE"}')
