import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

wall_meshes = {"SM_Cube","SM_Column","SM_Cylinder","SM_WcWall","SM_Securitywall",
    "SM_FrameTall","SM_FrameTallDoor","SM_LastLineEndWall","SM_CenterRoomsinnerWall",
    "SM_ElevatorWall","SM_Elevator","SM_Room2SideGlass","SM_SecuritySilling",
    "SM_SillingTile","SM_SillingCompoDark","SM_ThinBoxHorizen","SM_ThinBoxVertical",
    "SM_ThinBoxHorizenDouble","SM_ThinBoxVerticalDouble","SM_WebPartitionFrame",
    "SM_PartitionWorkSpace","SM_WorkStation_Partition","SM_Fence","SM_Steps",
    "SM_Antena","SM_DoorOfficeFrame","SM_DoorExitFrame","SM_AutoDoorBase"}

FLOOR_LEVEL_Z = 1987

idx = 0
for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    if loc.z < 1900 or loc.z > 2300:
        continue
    cn = a.get_class().get_name()
    if cn != "StaticMeshActor":
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if not sm:
        continue
    mn = sm.get_name()

    is_wall = False
    if mn == "SM_Cube":
        s = a.get_actor_scale3d()
        if abs(s.x-1.0)<0.1 and abs(s.y-1.0)<0.1 and s.z<0.5 and abs(loc.z-FLOOR_LEVEL_Z)<20:
            continue
        if abs(s.x)<0.05 and abs(s.y)<0.05:
            continue
        if s.z<0.3 and loc.z > FLOOR_LEVEL_Z + 150:
            continue
        r = a.get_actor_rotation()
        if abs(r.pitch) > 10 and abs(r.roll) < 10:
            continue
        is_wall = True
    elif mn in wall_meshes:
        is_wall = True

    if is_wall:
        if idx == 658:
            s = a.get_actor_scale3d()
            r = a.get_actor_rotation()
            print(f"Wall #{idx}: {a.get_name()} [{mn}]")
            print(f"  Loc: ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")
            print(f"  Scale: ({s.x:.3f},{s.y:.3f},{s.z:.3f})")
            print(f"  Rotation: roll={r.roll:.1f} pitch={r.pitch:.1f} yaw={r.yaw:.1f}")
        idx += 1
