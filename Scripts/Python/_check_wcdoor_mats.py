import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

# Check BP_WCDoor01 and BP_WCDoor02 components and materials
for a in all_actors:
    label = a.get_actor_label()
    if label not in ('BP_WCDoor01', 'BP_WCDoor02'):
        continue
    print(f'\n=== {label} ({a.get_class().get_name()}) ===')
    # Get all static mesh components
    smcs = a.get_components_by_class(unreal.StaticMeshComponent)
    for smc in smcs:
        mesh = smc.static_mesh
        mesh_name = mesh.get_name() if mesh else 'None'
        mesh_path = mesh.get_path_name() if mesh else 'N/A'
        print(f'  Component: {smc.get_name()}')
        print(f'    Mesh: {mesh_name} ({mesh_path})')
        num_mats = smc.get_num_materials()
        for i in range(num_mats):
            mat = smc.get_material(i)
            if mat:
                print(f'    Mat[{i}]: {mat.get_name()} ({mat.get_path_name()})')
            else:
                print(f'    Mat[{i}]: None')
    break  # Just need one of each type
