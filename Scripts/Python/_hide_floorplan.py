import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Meshes to delete on F1-F4
delete_meshes = {
    "SM_ThinBoxVertical", "SM_ThinBoxHorizen",
    "SM_ConferenceSecretaryRoom", "SM_PlayRoomFloorSilling",
    "SM_DoorOffice", "SM_DoorOfficeFrame",
}

# KEEP zones (bounding boxes) — actors inside these are preserved
keep_zones = [
    # Elevator/stairwell: X 700-1500, Y -1500 to -1200
    (700, 1500, -1500, -1200),
    # Bathrooms: X -3350 to -2950, Y -1250 to -1100
    (-3350, -2950, -1250, -1100),
]

def in_any_keep_zone(x, y):
    for x_min, x_max, y_min, y_max in keep_zones:
        if x_min <= x <= x_max and y_min <= y <= y_max:
            return True
    return False

hidden = 0
hidden_doors = 0
hidden_triggers = 0

for a in eas.get_all_level_actors():
    z = a.get_actor_location().z
    if z > 1900:  # Skip F5
        continue

    cn = a.get_class().get_name()
    loc = a.get_actor_location()
    in_keep = in_any_keep_zone(loc.x, loc.y)

    # Never touch bathroom doors
    if cn in ("BP_WCDoor01_C", "BP_WCDoor02_C"):
        continue

    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps:
            continue
        sm = comps[0].get_editor_property("static_mesh")
        if not sm or sm.get_name() not in delete_meshes:
            continue

        # Keep doors in keep zones
        if in_keep and sm.get_name() in ("SM_DoorOffice", "SM_DoorOfficeFrame"):
            continue

        a.set_is_temporarily_hidden_in_editor(True)
        comps[0].set_visibility(False)
        if sm.get_name() in ("SM_DoorOffice", "SM_DoorOfficeFrame"):
            hidden_doors += 1
        else:
            hidden += 1

    elif cn == "ZP_InteractDoor":
        if in_keep:
            continue
        da = a.get_editor_property("DoorActor")
        if not da:
            continue
        # Check if DoorActor is an office door
        da_comps = da.get_components_by_class(unreal.StaticMeshComponent)
        is_office_door = False
        if da_comps:
            da_sm = da_comps[0].get_editor_property("static_mesh")
            if da_sm and da_sm.get_name() in ("SM_DoorOffice", "SM_DoorOfficeFrame"):
                is_office_door = True
        if is_office_door:
            a.set_is_temporarily_hidden_in_editor(True)
            hidden_triggers += 1

print(f"Hidden on F1-F4 (F5 untouched):")
print(f"  Floor plan lines + rooms: {hidden}")
print(f"  Office doors: {hidden_doors}")
print(f"  InteractDoor triggers: {hidden_triggers}")
print(f"  Total: {hidden + hidden_doors + hidden_triggers}")
print(f"\nKept zones: elevator/stairwell + bathrooms")
print(f"Bathroom doors (BP_WCDoor) always kept")
