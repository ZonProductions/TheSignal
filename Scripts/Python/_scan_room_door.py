import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# F5 ConferenceSecretaryRoom is at Z~2001
room_loc = None
for a in eas.get_all_level_actors():
    if a.get_name() == "SM_ConferenceSecretaryRoom_26795":
        room_loc = a.get_actor_location()
        print(f"Room: {a.get_name()} at ({room_loc.x:.0f},{room_loc.y:.0f},{room_loc.z:.0f})")
        # Get room mesh bounds
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps:
            sm = c.get_editor_property("static_mesh")
            if sm:
                print(f"  Mesh: {sm.get_name()}")
        break

if not room_loc:
    print("Room not found!")
else:
    # Find all InteractDoors on F5 (Z between 1950 and 2100)
    print("\n=== F5 InteractDoors ===")
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() != "ZP_InteractDoor":
            continue
        loc = a.get_actor_location()
        if abs(loc.z - room_loc.z) > 100:
            continue

        da = a.get_editor_property("DoorActor")
        da_name = da.get_name() if da else "None"
        da_class = da.get_class().get_name() if da else "?"
        da_loc = f"({da.get_actor_location().x:.0f},{da.get_actor_location().y:.0f},{da.get_actor_location().z:.0f})" if da else "?"

        mode = a.get_editor_property("OpenMode")
        angle = a.get_editor_property("OpenAngle")

        # Check if DoorActor name contains ConferenceSecretary or Room
        marker = " <<<" if da and ("Conference" in da_name or "Secretary" in da_name or "Room" in da_name) else ""

        print(f"  {a.get_name()} at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")
        print(f"    DoorActor: {da_name} [{da_class}] at {da_loc} mode={mode} angle={angle}{marker}")

        # Check DoorActor mesh
        if da:
            for c in da.get_components_by_class(unreal.StaticMeshComponent):
                sm = c.get_editor_property("static_mesh")
                if sm:
                    print(f"    DoorActor mesh: {sm.get_name()}")
