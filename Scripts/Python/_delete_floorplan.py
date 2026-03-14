import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

delete_meshes = {
    "SM_ThinBoxVertical", "SM_ThinBoxHorizen",
    "SM_ConferenceSecretaryRoom", "SM_PlayRoomFloorSilling",
    "SM_DoorOffice", "SM_DoorOfficeFrame",
}

keep_zones = [
    (700, 1500, -1500, -1200),    # Elevator/stairwell
    (-3350, -2950, -1250, -1100), # Bathrooms
]

def in_any_keep_zone(x, y):
    for x_min, x_max, y_min, y_max in keep_zones:
        if x_min <= x <= x_max and y_min <= y <= y_max:
            return True
    return False

to_delete = []

for a in eas.get_all_level_actors():
    z = a.get_actor_location().z
    if z > 1900:
        continue

    cn = a.get_class().get_name()
    loc = a.get_actor_location()
    in_keep = in_any_keep_zone(loc.x, loc.y)

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

print(f"Deleting {len(to_delete)} actors on F1-F4...")
for a in to_delete:
    eas.destroy_actor(a)
print(f"Done. Deleted {len(to_delete)} actors.")
