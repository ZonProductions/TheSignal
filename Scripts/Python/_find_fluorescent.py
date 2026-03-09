import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

# Find the fluorescent actor and its mesh
for a in all_actors:
    label = a.get_actor_label()
    if 'florosent' in label.lower() or 'fluorescent' in label.lower():
        if isinstance(a, unreal.StaticMeshActor):
            smc = a.static_mesh_component
            mesh = smc.static_mesh
            loc = a.get_actor_location()
            rot = a.get_actor_rotation()
            scale = a.get_actor_scale3d()
            print(f"{label}:")
            print(f"  Mesh: {mesh.get_path_name() if mesh else 'None'}")
            print(f"  Location: {loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}")
            print(f"  Rotation: {rot.roll:.1f}, {rot.pitch:.1f}, {rot.yaw:.1f}")
            print(f"  Scale: {scale.x:.2f}, {scale.y:.2f}, {scale.z:.2f}")
            # Materials
            for i in range(smc.get_num_materials()):
                mat = smc.get_material(i)
                print(f"  Mat[{i}]: {mat.get_name() if mat else 'None'}")

# Also find remaining light fixtures
print("\n--- Remaining light fixtures ---")
remaining = {}
for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    mesh_name = smc.static_mesh.get_name()
    if any(kw in mesh_name.lower() for kw in ['light', 'lamp']):
        if mesh_name not in remaining:
            remaining[mesh_name] = 0
        remaining[mesh_name] += 1

for name, count in sorted(remaining.items(), key=lambda x: -x[1]):
    print(f"  {name}: {count}")
print(f"Total remaining: {sum(remaining.values())}")
