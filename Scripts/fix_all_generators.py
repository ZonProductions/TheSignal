import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()

print("=== FIXING ALL GENERATOR/POWER MESH COLLISION ===")
fixed_meshes = set()
for a in actors:
    label = a.get_actor_label()
    if 'generator' not in label.lower() and 'power' not in label.lower():
        continue
    if not isinstance(a, unreal.StaticMeshActor):
        continue

    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    for c in comps:
        mesh = c.static_mesh
        if not mesh or mesh.get_path_name() in fixed_meshes:
            continue

        body = mesh.get_editor_property('body_setup')
        if body:
            old_flag = body.get_editor_property('collision_trace_flag')
            if old_flag == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
                print(f"  {label}: {mesh.get_name()} — already fixed")
                fixed_meshes.add(mesh.get_path_name())
                continue
            body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            new_flag = body.get_editor_property('collision_trace_flag')
            print(f"  {label}: {mesh.get_name()}")
            print(f"    {old_flag} -> {new_flag}")
            unreal.EditorAssetLibrary.save_asset(mesh.get_path_name())
            print(f"    Saved")
            fixed_meshes.add(mesh.get_path_name())

print(f"\nProcessed {len(fixed_meshes)} unique mesh asset(s)")
