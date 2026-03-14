import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# ConferenceSecretaryRoom on F5 at (-305,-441,2001)
room_x, room_y, room_z = -305, -441, 2001

print(f"=== Actors within 600 UU of ConferenceSecretaryRoom F5 ===\n")

for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    dx = abs(loc.x - room_x)
    dy = abs(loc.y - room_y)
    dz = abs(loc.z - room_z)
    if dx > 600 or dy > 600 or dz > 100:
        continue

    cn = a.get_class().get_name()
    # Skip floor tiles, cylinders, cubes, basic geometry
    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if comps:
            sm = comps[0].get_editor_property("static_mesh")
            mesh_name = sm.get_name() if sm else "None"
            # Only show doors, rooms, and BP-related meshes
            if any(k in mesh_name for k in ["Door", "door", "Room", "room", "Conference", "Auto", "Glass"]):
                dist = ((loc.x - room_x)**2 + (loc.y - room_y)**2)**0.5
                print(f"{a.get_name()} [{cn}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) dist={dist:.0f}")
                print(f"  mesh: {mesh_name}")
        continue

    # Show all non-StaticMeshActor actors (BPs, triggers, etc.)
    dist = ((loc.x - room_x)**2 + (loc.y - room_y)**2)**0.5
    print(f"{a.get_name()} [{cn}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) dist={dist:.0f}")

# Also search globally for BigCompany door BPs
print("\n=== BigCompany Door Blueprint classes on F5 ===\n")
for a in eas.get_all_level_actors():
    cn = a.get_class().get_name()
    loc = a.get_actor_location()
    if abs(loc.z - room_z) > 100:
        continue
    if any(k in cn for k in ["Door", "door", "Glass", "WCDoor", "AutoDoor"]):
        print(f"{a.get_name()} [{cn}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")
