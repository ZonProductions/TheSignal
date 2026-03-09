import unreal

# Find static mesh actors that look like light fixtures (ceiling panels, tubes, etc.)
subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

# Count meshes with "light" in the name
light_meshes = {}
for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    mesh_name = smc.static_mesh.get_name()
    if 'light' in mesh_name.lower() or 'lamp' in mesh_name.lower() or 'ceil' in mesh_name.lower() or 'fluor' in mesh_name.lower() or 'panel' in mesh_name.lower():
        if mesh_name not in light_meshes:
            light_meshes[mesh_name] = []
        light_meshes[mesh_name].append(a.get_actor_label())

for mesh_name, actors in sorted(light_meshes.items(), key=lambda x: -len(x[1])):
    print(f"{mesh_name}: {len(actors)} instances")
    for a in actors[:3]:
        print(f"    {a}")
    if len(actors) > 3:
        print(f"    ... and {len(actors)-3} more")

total = sum(len(v) for v in light_meshes.values())
print(f"\nTotal light fixture meshes: {total}")
