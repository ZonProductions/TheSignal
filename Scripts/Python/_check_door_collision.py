import unreal

for path in [
    "/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_DoorExit",
    "/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_DoorExitFrame",
    "/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_DoorOffice",
    "/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_DoorOfficeFrame",
]:
    sm = unreal.load_asset(path)
    if not sm:
        print(f"{path}: NOT FOUND")
        continue
    name = path.split("/")[-1]
    body = sm.get_editor_property("body_setup")
    flag = body.get_editor_property("collision_trace_flag")
    agg = body.get_editor_property("agg_geom")
    convex = agg.get_editor_property("convex_elems")
    box = agg.get_editor_property("box_elems")
    sphere = agg.get_editor_property("sphere_elems")
    print(f"{name}: flag={flag}, convex={len(convex)}, box={len(box)}, sphere={len(sphere)}")
