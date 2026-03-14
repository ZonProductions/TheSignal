import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

delete_meshes = {
    "SM_ThinBoxVertical", "SM_ThinBoxHorizen",
    "SM_ConferenceSecretaryRoom", "SM_PlayRoomFloorSilling",
    "SM_DoorOffice", "SM_DoorOfficeFrame",
}

# Keep zones for the MIRRORED building (original zones transformed)
# Mirror: new_x = -2442 - old_x, new_y = 3796 - old_y
# Original elevator (X 700-1500, Y -1500 to -1200) → mirrored:
# Original bathrooms (X -3350 to -2950, Y -1250 to -1100) → mirrored:
keep_zones = [
    (-3942, -3142, 4996, 5296),  # Mirrored elevator/stairwell
    (508, 908, 4896, 5046),       # Mirrored bathrooms
]

def in_any_keep_zone(x, y):
    for x_min, x_max, y_min, y_max in keep_zones:
        if x_min <= x <= x_max and y_min <= y <= y_max:
            return True
    return False

to_delete = []

for a in eas.get_all_level_actors():
    loc = a.get_actor_location()

    # Only mirrored building F5: Y > 1898 AND Z > 1900
    if loc.y <= 1898 or loc.z <= 1900:
        continue

    cn = a.get_class().get_name()
    in_keep = in_any_keep_zone(loc.x, loc.y)

    if cn in ("BP_WCDoor01_C", "BP_WCDoor02_C"):
        continue  # Never touch bathroom doors

    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps:
            continue
        sm = comps[0].get_editor_property("static_mesh")
        if not sm or sm.get_name() not in delete_meshes:
            continue
        if in_keep and sm.get_name() in ("SM_DoorOffice", "SM_DoorOfficeFrame"):
            continue
        to_delete.append(a)

    elif cn == "ZP_InteractDoor":
        if in_keep:
            continue
        da = a.get_editor_property("DoorActor")
        if not da:
            continue
        da_comps = da.get_components_by_class(unreal.StaticMeshComponent)
        if da_comps:
            da_sm = da_comps[0].get_editor_property("static_mesh")
            if da_sm and da_sm.get_name() in ("SM_DoorOffice", "SM_DoorOfficeFrame"):
                to_delete.append(a)

print(f"Deleting {len(to_delete)} actors on mirrored building F5...")
for a in to_delete:
    eas.destroy_actor(a)
print(f"Done. Deleted {len(to_delete)} actors.")
