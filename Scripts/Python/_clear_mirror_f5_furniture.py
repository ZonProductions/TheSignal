import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Structural — KEEP always
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
    "SM_ThinBoxVertical", "SM_ThinBoxHorizen",  # already deleted but just in case
    "SM_DoorOffice", "SM_DoorOfficeFrame",  # already deleted
}

# Bathroom fixtures — KEEP in bathroom zone
bathroom_meshes = {
    "SM_ToiletKiosk", "SM_ToiletUnitA", "SM_ToiletUnitB",
    "SM_StandToilet", "SM_WashBasinA", "SM_WashBasinB",
    "SM_HandDryer", "SM_Dryer", "SM_Soap", "SM_ToiletTissu",
    "SM_REcycleToiletPaper", "SM_WetSign", "SM_Shower",
    "SM_Towel", "SM_Bucket", "SM_Broom", "SM_Dustpan",
}

# BP classes to KEEP
keep_bp_classes = {
    "BP_ElevatorDoors_C", "BP_GlassDoors1_C",
    "BP_WCDoor01_C", "BP_WCDoor02_C",
    "GroupActor", "SphereReflectionCapture",
}

# Bathroom keep zone (mirrored)
bath_x_min, bath_x_max = 508, 908
bath_y_min, bath_y_max = 4896, 5046

def in_bathroom(x, y):
    return bath_x_min <= x <= bath_x_max and bath_y_min <= y <= bath_y_max

to_delete = []

for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    if loc.y <= 1898 or loc.z <= 1900:
        continue

    cn = a.get_class().get_name()

    if cn in keep_bp_classes:
        continue

    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps:
            continue
        sm = comps[0].get_editor_property("static_mesh")
        if not sm:
            continue
        mn = sm.get_name()

        if mn in keep_meshes:
            continue
        if mn in bathroom_meshes and in_bathroom(loc.x, loc.y):
            continue

        to_delete.append(a)

    elif cn == "ZP_InteractDoor":
        # Keep InteractDoors that reference exit/elevator doors
        da = a.get_editor_property("DoorActor")
        if da:
            da_comps = da.get_components_by_class(unreal.StaticMeshComponent)
            if da_comps:
                da_sm = da_comps[0].get_editor_property("static_mesh")
                if da_sm and da_sm.get_name() in ("SM_DoorExit", "SM_DoorExitFrame", "SM_AutoDoorBase", "SM_AutoDoorLeft"):
                    continue
        to_delete.append(a)

    else:
        # Delete all other BPs (item pickups, loot lockers, signs, music, etc.)
        to_delete.append(a)

print(f"Deleting {len(to_delete)} furniture/prop actors from mirrored F5...")
for a in to_delete:
    eas.destroy_actor(a)
print(f"Done. Deleted {len(to_delete)} actors.")
