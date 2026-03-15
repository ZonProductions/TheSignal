import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Current wall list in the scan script
current_walls = {
    "SM_Cube", "SM_Column", "SM_Cylinder",
    "SM_WindowWall", "SM_WcWall", "SM_Securitywall",
    "SM_FrameTall", "SM_FrameTallDoor", "SM_LastLineEndWall",
    "SM_CenterRoomsinnerWall", "SM_ElevatorWall", "SM_Elevator",
}
current_doors = {
    "SM_DoorOffice", "SM_DoorOfficeFrame",
    "SM_DoorExit", "SM_DoorExitFrame",
    "SM_AutoDoorBase", "SM_AutoDoorLeft", "SM_AutoDoorRight",
    "SM_RoomManagerDoor",
}

meshes = {}
for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    loc = a.get_actor_location()
    if loc.z < 1900 or loc.z > 2300:
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if not sm:
        continue
    mn = sm.get_name()
    # Skip floor tiles
    s = a.get_actor_scale3d()
    if mn == "SM_Cube" and abs(s.x-1.0)<0.1 and abs(s.y-1.0)<0.1 and s.z<0.5:
        continue
    if mn not in current_walls and mn not in current_doors:
        meshes[mn] = meshes.get(mn, 0) + 1

print("=== F5 meshes NOT in wall or door lists ===")
for mn, count in sorted(meshes.items(), key=lambda x: -x[1]):
    print(f"  {count:3d}x {mn}")
