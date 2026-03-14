import unreal

# The door is at (939,-1324,~2049). Scan a tight box around the doorway.
door_x, door_y, door_z = 939, -1324, 2049

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

print(f"=== Scanning doorway at ({door_x},{door_y},{door_z}) ===\n")

for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    dx = abs(loc.x - door_x)
    dy = abs(loc.y - door_y)
    dz = abs(loc.z - door_z)

    # Tight box: 150 UU horizontal, 200 UU vertical
    if dx > 150 or dy > 150 or dz > 200:
        continue

    cn = a.get_class().get_name()
    print(f"{a.get_name()} [{cn}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")

    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        ct = c.get_collision_enabled()
        cp = c.get_collision_profile_name()
        extra = ""

        if isinstance(c, unreal.StaticMeshComponent):
            sm = c.get_editor_property("static_mesh")
            if sm:
                extra = f" mesh={sm.get_name()}"
                body = sm.get_editor_property("body_setup")
                if body:
                    flag = body.get_editor_property("collision_trace_flag")
                    extra += f" trace={flag}"
                # Get bounds
                try:
                    bounds = c.get_local_bounds()
                    bmin = bounds[0]
                    bmax = bounds[1]
                    extra += f" bounds=({bmax.x-bmin.x:.0f}x{bmax.y-bmin.y:.0f}x{bmax.z-bmin.z:.0f})"
                except:
                    pass
            else:
                extra = " mesh=None"

        if isinstance(c, unreal.BoxComponent):
            try:
                be = c.get_editor_property("box_extent")
                extra = f" extent=({be.x:.0f},{be.y:.0f},{be.z:.0f})"
            except:
                pass

        blocking = "BLOCKS" if "PHYSICS" in str(ct) else "ok"
        if "BlockAll" in str(cp):
            blocking = "BLOCKS"
        print(f"  {c.get_name()} [{type(c).__name__}] {blocking} collision={ct} profile={cp}{extra}")

    print()
