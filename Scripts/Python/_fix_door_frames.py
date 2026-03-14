import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()

fixed = 0
for a in actors:
    name = a.get_name()
    # Door frame meshes from BigCompany pack
    if "polySurface" not in name and "DoorFrame" not in name and "DoorExitFrame" not in name:
        continue

    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    for c in comps:
        sm = c.get_editor_property("static_mesh")
        if sm is None:
            continue

        body = sm.get_editor_property("body_setup")
        if body is None:
            continue

        current = body.get_editor_property("collision_trace_flag")
        if str(current) != "CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE":
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            fixed += 1
            print(f"Fixed: {name} mesh={sm.get_name()} → complex-as-simple")

print(f"\nTotal fixed: {fixed}")
