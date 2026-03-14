import unreal
from collections import defaultdict

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Already deleted
already_gone = {
    "SM_ThinBoxVertical", "SM_ThinBoxHorizen",
    "SM_ConferenceSecretaryRoom", "SM_PlayRoomFloorSilling",
    "SM_DoorOffice", "SM_DoorOfficeFrame",
}

# Structural — KEEP
keep_meshes = {
    "SM_Cube", "SM_Column", "SM_Cylinder",
    "SM_WindowWall", "SM_WcWall", "SM_Securitywall",
    "SM_Woodfloor", "SM_OutsideFloor", "SM_KitchenFloor",
    "SM_FrameTall", "SM_FrameTallDoor", "SM_LastLineEndWall",
    "SM_Steps", "SM_Elevator", "SM_ElevatorWall",
    "SM_ElevatorLeftDoor", "SM_ElevatorRightDoor",
    "SM_DoorExit", "SM_DoorExitFrame",
    "SM_AutoDoorBase", "SM_AutoDoorLeft", "SM_AutoDoorRight",
    "SM_PipeLong", "SM_PipeValve",
    "SM_CenterRoomsinnerWall",
}

mesh_counts = defaultdict(int)

for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    if loc.y <= 1898 or loc.z <= 1900:
        continue

    cn = a.get_class().get_name()
    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps:
            continue
        sm = comps[0].get_editor_property("static_mesh")
        if not sm:
            continue
        mn = sm.get_name()
        if mn in already_gone or mn in keep_meshes:
            continue
        mesh_counts[mn] += 1
    elif cn not in ("GroupActor", "SphereReflectionCapture", "ZP_InteractDoor",
                     "BP_ElevatorDoors_C", "BP_GlassDoors1_C",
                     "BP_WCDoor01_C", "BP_WCDoor02_C"):
        mesh_counts[f"[BP] {cn}"] += 1

print(f"=== Mirrored F5 — remaining non-structural actors ===\n")
total = 0
for mn, count in sorted(mesh_counts.items(), key=lambda x: -x[1]):
    print(f"  {count:4d}x {mn}")
    total += count
print(f"\n  Total to delete: {total}")
