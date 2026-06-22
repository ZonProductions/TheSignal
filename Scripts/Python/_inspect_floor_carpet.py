import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Floor 3 walkable floor sits near z~694 (F3 window bottom). Find large horizontal meshes there.
print("=== floor-3 floor candidates (horizontal, z 600..760) ===")
seen = {}
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs:
        continue
    c = cs[0]
    sm = c.get_editor_property("static_mesh")
    if not sm:
        continue
    org, ext = a.get_actor_bounds(False)
    if ext.z > 80:       # not a slab
        continue
    if not (600 <= org.z <= 770):
        continue
    if ext.x < 150 and ext.y < 150:
        continue
    mats = [ (c.get_material(i).get_name() if c.get_material(i) else "none") for i in range(c.get_num_materials()) ]
    key = (sm.get_name(), tuple(mats))
    seen[key] = seen.get(key, 0) + 1
    if seen[key] <= 2:
        print("  %-26s mesh=%-22s z=%.0f footprint=(%.0f x %.0f) scale=%s mats=%s" % (
            a.get_actor_label(), sm.get_name(), org.z, ext.x*2, ext.y*2,
            (round(a.get_actor_scale3d().x,2), round(a.get_actor_scale3d().y,2), round(a.get_actor_scale3d().z,2)), mats))
print("\nfloor candidate tally (mesh, mats) -> count:")
for k, v in sorted(seen.items(), key=lambda kv: -kv[1]):
    print("  %3d  %s" % (v, k))
