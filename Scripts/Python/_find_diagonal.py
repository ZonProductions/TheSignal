import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

wall_meshes = {"SM_Cube","SM_Column","SM_Cylinder","SM_WcWall","SM_Securitywall",
    "SM_FrameTall","SM_FrameTallDoor","SM_LastLineEndWall","SM_CenterRoomsinnerWall",
    "SM_ElevatorWall","SM_Elevator","SM_RoomManagerA","SM_RoomManagerB",
    "SM_ConferenceSecretaryRoom","SM_Room2SideGlass","SM_SecuritySilling",
    "SM_ThinBoxHorizen","SM_ThinBoxVertical","SM_Fence","SM_WindowWall"}

print("=== F5 actors with diagonal rotation (yaw not 0/90/180/270) ===\n")
for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    if loc.z < 1900 or loc.z > 2300:
        continue
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if not sm:
        continue
    mn = sm.get_name()
    if mn not in wall_meshes:
        continue

    yaw = a.get_actor_rotation().yaw % 360
    # Check if yaw is NOT aligned to 0/90/180/270 (within 5 degrees)
    aligned = any(abs(yaw - angle) < 5 or abs(yaw - angle - 360) < 5 for angle in [0, 90, 180, 270])
    if not aligned:
        s = a.get_actor_scale3d()
        print(f"  {a.get_name()} [{mn}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) yaw={yaw:.1f} scale=({s.x:.1f},{s.y:.1f},{s.z:.1f})")

# Also check what's near the elevator area more broadly
print("\n=== All wall meshes near elevator (X 800-1600, Y -1500 to -1100) ===\n")
for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    if loc.z < 1900 or loc.z > 2300:
        continue
    if not (800 < loc.x < 1600 and -1500 < loc.y < -1100):
        continue
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if not sm:
        continue
    mn = sm.get_name()
    if mn in wall_meshes or "Room" in mn or "Silling" in mn:
        yaw = a.get_actor_rotation().yaw
        s = a.get_actor_scale3d()
        print(f"  {a.get_name()} [{mn}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) yaw={yaw:.0f} scale=({s.x:.1f},{s.y:.1f},{s.z:.1f})")
