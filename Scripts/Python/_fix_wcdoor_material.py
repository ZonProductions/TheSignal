import unreal

# Find the regular office door material to use instead of the WC sign material.
# We want the same look as a normal office door — just no toilet sign.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

# First, find what material normal (non-WC) doors use
print("=== Finding normal door material ===")
normal_door_mat = None
for a in all_actors:
    class_name = a.get_class().get_name()
    label = a.get_actor_label()
    # Look for regular office doors (not WC)
    if 'Door' in class_name and 'WC' not in class_name and 'WC' not in label:
        smcs = a.get_components_by_class(unreal.StaticMeshComponent)
        for smc in smcs:
            if smc.static_mesh and smc.static_mesh.get_name() == 'SM_DoorOffice':
                mat = smc.get_material(0)
                if mat and 'WC' not in mat.get_name() and 'wc' not in mat.get_name():
                    normal_door_mat = mat
                    print(f'  Found normal door: {label} — Mat: {mat.get_name()} ({mat.get_path_name()})')
                    break
    if normal_door_mat:
        break

if not normal_door_mat:
    # Use MI_DoorBW — plain door without signage
    normal_door_mat = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_DoorBW')
    if normal_door_mat:
        print(f'  Using MI_DoorBW (plain door)')

if not normal_door_mat:
    print('ERROR: Could not find a normal door material to swap to.')
else:
    print(f'\nUsing material: {normal_door_mat.get_name()}')

    # Swap all WC doors to use the normal door material
    count = 0
    for a in all_actors:
        class_name = a.get_class().get_name()
        if 'BP_WCDoor' not in class_name:
            continue
        smcs = a.get_components_by_class(unreal.StaticMeshComponent)
        for smc in smcs:
            if smc.static_mesh and smc.static_mesh.get_name() == 'SM_DoorOffice':
                old_mat = smc.get_material(0)
                old_name = old_mat.get_name() if old_mat else 'None'
                if 'WC' in old_name or 'wc' in old_name:
                    smc.set_material(0, normal_door_mat)
                    print(f'  Swapped: {a.get_actor_label()} — {old_name} → {normal_door_mat.get_name()}')
                    count += 1

    print(f'\nSwapped {count} WC door materials')
    subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsys_level.save_all_dirty_levels()
    print('Level saved.')
