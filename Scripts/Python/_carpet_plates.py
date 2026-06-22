import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

print("=== floor-3 horizontal plates with a Carpet material ===")
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    c = cs[0]
    sm = c.get_editor_property("static_mesh")
    if not sm: continue
    org, ext = a.get_actor_bounds(False)
    if ext.z > 80: continue
    for i in range(c.get_num_materials()):
        m = c.get_material(i)
        if m and "carpet" in m.get_name().lower():
            s = a.get_actor_scale3d()
            print("  %-24s mesh=%-16s mat=%-12s z=%.0f foot=(%.0f x %.0f) scale=(%.2f,%.2f)" % (
                a.get_actor_label(), sm.get_name(), m.get_name(), org.z,
                ext.x*2, ext.y*2, s.x, s.y))
            break
