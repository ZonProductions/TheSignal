import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

print("=== Actors with 'toilet' in label or mesh name ===")
found = set()
for a in all_actors:
    label = a.get_actor_label()
    if 'toilet' not in label.lower() and 'wc' not in label.lower() and 'restroom' not in label.lower() and 'bathroom' not in label.lower():
        continue
    if isinstance(a, unreal.StaticMeshActor):
        smc = a.static_mesh_component
        if smc and smc.static_mesh:
            mesh_name = smc.static_mesh.get_name()
            mesh_path = smc.static_mesh.get_path_name()
            # Get materials
            num_mats = smc.get_num_materials()
            mat_info = []
            for i in range(num_mats):
                mat = smc.get_material(i)
                mat_info.append(mat.get_name() if mat else 'None')
            key = f'{mesh_name}|{"|".join(mat_info)}'
            if key not in found:
                found.add(key)
                print(f'  [{a.get_class().get_name()}] {label}')
                print(f'    Mesh: {mesh_name} ({mesh_path})')
                for i, m in enumerate(mat_info):
                    print(f'    Mat[{i}]: {m}')
    else:
        print(f'  [{a.get_class().get_name()}] {label}')

# Also search for door meshes with toilet-related materials
print("\n=== Door meshes with toilet/wc materials ===")
for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    mesh_name = smc.static_mesh.get_name()
    if 'door' not in mesh_name.lower():
        continue
    num_mats = smc.get_num_materials()
    for i in range(num_mats):
        mat = smc.get_material(i)
        if mat:
            mat_name = mat.get_name()
            if 'toilet' in mat_name.lower() or 'wc' in mat_name.lower() or 'sign' in mat_name.lower():
                print(f'  {a.get_actor_label()} — Mesh: {mesh_name}, Mat[{i}]: {mat_name}')
