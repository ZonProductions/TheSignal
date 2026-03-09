import unreal

# Restore WC door materials on instances by re-applying from the correct MI.
# BP_WCDoor01 = MI_DoorWCmen, BP_WCDoor02 = MI_DoorWCwomen

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

mi_men = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_DoorWCmen')
mi_women = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_DoorWCwomen')

print(f'MI_DoorWCmen: {mi_men}')
print(f'MI_DoorWCwomen: {mi_women}')

count = 0
for a in all_actors:
    class_name = a.get_class().get_name()
    if 'BP_WCDoor' not in class_name:
        continue
    label = a.get_actor_label()
    smcs = a.get_components_by_class(unreal.StaticMeshComponent)
    for smc in smcs:
        if smc.static_mesh and smc.static_mesh.get_name() == 'SM_DoorOffice':
            current_mat = smc.get_material(0)
            current_name = current_mat.get_name() if current_mat else 'None'
            if 'WCDoor01' in class_name:
                smc.set_material(0, mi_men)
                print(f'  {label}: {current_name} → MI_DoorWCmen')
            elif 'WCDoor02' in class_name:
                smc.set_material(0, mi_women)
                print(f'  {label}: {current_name} → MI_DoorWCwomen')
            count += 1

print(f'\nRestored {count} WC door instance materials')
subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsys_level.save_all_dirty_levels()
print('Level saved.')
