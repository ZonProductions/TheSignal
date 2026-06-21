import unreal

ues = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = ues.get_all_level_actors()

target = None
for a in actors:
    if a.get_actor_label() == "ZP_ScytheerClimbPath2":
        target = a
        break

if not target:
    print("ZP_ScytheerClimbPath2 NOT FOUND")
else:
    print("Before:")
    for i, n in enumerate(target.get_editor_property("PerPointWallNormals")):
        print("   [%d] (%.2f, %.2f, %.2f)" % (i, n.x, n.y, n.z))

    target.snap_wall_normals_to_geometry()

    print("After:")
    spline = target.get_component_by_class(unreal.SplineComponent)
    for i, n in enumerate(target.get_editor_property("PerPointWallNormals")):
        wl = spline.get_location_at_spline_point(i, unreal.SplineCoordinateSpace.WORLD)
        print("   [%d] pos=(%.0f,%.0f,%.0f)  normal=(%.2f, %.2f, %.2f)" % (i, wl.x, wl.y, wl.z, n.x, n.y, n.z))

    # Persist the level edit.
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.save_current_level()
    print("SAVED LEVEL")
