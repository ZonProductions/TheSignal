import unreal

# Find ALL door-related actors inside the bathrooms — looking for toilet stall doors
# with "toilet" text/icon on them.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

print("=== All actors with 'door' or 'stall' or 'toilet' in mesh name ===")
seen = set()
for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    mesh_name = smc.static_mesh.get_name()
    label = a.get_actor_label()
    if any(kw in mesh_name.lower() for kw in ['door', 'stall', 'toilet', 'wc', 'cabin']):
        num_mats = smc.get_num_materials()
        mat_names = []
        for i in range(num_mats):
            mat = smc.get_material(i)
            mat_names.append(mat.get_name() if mat else 'None')
        key = f'{mesh_name}|{"|".join(mat_names)}'
        if key not in seen:
            seen.add(key)
            print(f'  {label} — Mesh: {mesh_name}')
            for i, mn in enumerate(mat_names):
                print(f'    Mat[{i}]: {mn}')

# Also check BP actors with 'door' in class name (excluding WCDoor which we know)
print("\n=== BP door actors (non-WCDoor) ===")
bp_seen = set()
for a in all_actors:
    class_name = a.get_class().get_name()
    if 'Door' not in class_name or 'WCDoor' in class_name or 'InteractDoor' in class_name:
        continue
    if class_name not in bp_seen:
        bp_seen.add(class_name)
        label = a.get_actor_label()
        print(f'  [{class_name}] {label}')
        smcs = a.get_components_by_class(unreal.StaticMeshComponent)
        for smc in smcs:
            mesh = smc.static_mesh
            if mesh:
                num_mats = smc.get_num_materials()
                for i in range(num_mats):
                    mat = smc.get_material(i)
                    print(f'    {smc.get_name()} — Mesh: {mesh.get_name()}, Mat[{i}]: {mat.get_name() if mat else "None"}')
