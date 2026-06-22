import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
FLOOR_Z = 987
shown = 0
would_skip = 0
total = 0
for a in eas.get_all_level_actors():
    if "wallbrick" not in a.get_actor_label().lower():
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    sm = comps[0].get_editor_property("static_mesh") if comps else None
    if not sm or sm.get_name() != "SM_Cube":
        continue
    total += 1
    s = a.get_actor_scale3d()
    r = a.get_actor_rotation()
    z = a.get_actor_location().z
    # replicate the scan's SM_Cube skip filters
    skip = ""
    if abs(s.x-1.0)<0.1 and abs(s.y-1.0)<0.1 and s.z<0.5 and abs(z-FLOOR_Z)<20: skip = "floor-tile"
    elif abs(s.x)<0.05 and abs(s.y)<0.05: skip = "tiny"
    elif s.z<0.3 and z > FLOOR_Z + 150: skip = "thin-ceiling(skip3)"
    elif abs(r.pitch) > 10 and abs(r.roll) < 10: skip = "tilted(skip4)"
    if skip: would_skip += 1
    if shown < 18:
        print("  %-22s s=(%.2f,%.2f,%.2f) rot=(R%.0f P%.0f Y%.0f) z=%.0f  %s" %
              (a.get_actor_label(), s.x, s.y, s.z, r.roll, r.pitch, r.yaw, z, "SKIP:"+skip if skip else "ok"))
        shown += 1
print("F3 brick(SM_Cube) actors: %d total, %d would be SKIPPED by current filters" % (total, would_skip))
