import unreal

ues = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = ues.get_all_level_actors()

targets = [a for a in actors if "ScytheerPath" in a.get_actor_label()
           or "ClimbPath" in a.get_actor_label()
           or isinstance(a, unreal.ZP_ScytheerClimbPath)]

print("Found", len(targets), "path actor(s)")
for a in targets:
    label = a.get_actor_label()
    cls = type(a).__name__
    print("\n=== %s  (class %s) ===" % (label, cls))
    print("  actor loc:", a.get_actor_location())
    spline = a.get_component_by_class(unreal.SplineComponent)
    if not spline:
        print("  NO SplineComponent")
        continue
    n = spline.get_number_of_spline_points()
    print("  spline points:", n, " closed:", spline.is_closed_loop())
    for i in range(n):
        wl = spline.get_location_at_spline_point(i, unreal.SplineCoordinateSpace.WORLD)
        pt = spline.get_spline_point_type(i)
        print("    [%d] world=(%.0f, %.0f, %.0f)  type=%s" % (i, wl.x, wl.y, wl.z, pt))
    try:
        normals = a.get_editor_property("PerPointWallNormals")
        print("  PerPointWallNormals (%d):" % len(normals))
        for i, nrm in enumerate(normals):
            print("    [%d] (%.2f, %.2f, %.2f)" % (i, nrm.x, nrm.y, nrm.z))
    except Exception as e:
        print("  PerPointWallNormals read failed:", e)
