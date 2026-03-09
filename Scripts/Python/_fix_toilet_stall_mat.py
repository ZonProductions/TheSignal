import unreal

# Swap MI_WCPackB on toilet kiosk doors to a plain material.
# MI_WCPackA exists too — let's check if either has the toilet sign.
# Otherwise use MI_ColorDarkMetallic for a clean solid look.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

# Use MI_WCPackA — might be the version without the sign
new_mat = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_WCPackB')
if not new_mat:
    new_mat = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_ColorDarkMetallic')
print(f'Replacement material: {new_mat.get_name() if new_mat else "NOT FOUND"}')

if not new_mat:
    print('ERROR: No replacement material found')
else:
    count = 0
    for a in all_actors:
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        smc = a.static_mesh_component
        if not smc or not smc.static_mesh:
            continue
        mesh_name = smc.static_mesh.get_name()
        if mesh_name in ('SM_ToiletKiosk', 'SM_ToiletKioskClosed'):
            old_mat = smc.get_material(0)
            old_name = old_mat.get_name() if old_mat else 'None'
            if old_name == 'MI_WCPackA':
                smc.set_material(0, new_mat)
            label = a.get_actor_label()
            print(f'  {label} ({mesh_name}): {old_name} → {new_mat.get_name()}')
            count += 1

    print(f'\nSwapped {count} toilet stall doors')
    if count > 0:
        subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        subsys_level.save_all_dirty_levels()
        print('Level saved.')
