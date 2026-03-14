import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Find all door-related actors on F1-F4, excluding elevator doors and exit doors (stairwell area)
# Elevator is at X~1050-1440, Y~-1317
# Stairwell is at X~750, Y~-1468

door_meshes = {"SM_DoorOffice", "SM_DoorOfficeFrame"}
door_bps = {"BP_WCDoor01_C", "BP_WCDoor02_C"}

# Also find InteractDoor triggers that reference office doors
static_doors = []
bp_doors = []
interact_doors = []

for a in eas.get_all_level_actors():
    z = a.get_actor_location().z
    if z > 1900:  # Skip F5
        continue

    cn = a.get_class().get_name()

    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if comps:
            sm = comps[0].get_editor_property("static_mesh")
            if sm and sm.get_name() in door_meshes:
                loc = a.get_actor_location()
                static_doors.append((a.get_name(), sm.get_name(), loc.x, loc.y, loc.z))

    elif cn in door_bps:
        loc = a.get_actor_location()
        bp_doors.append((a.get_name(), cn, loc.x, loc.y, loc.z))

    elif cn == "ZP_InteractDoor":
        loc = a.get_actor_location()
        da = a.get_editor_property("DoorActor")
        da_name = da.get_name() if da else "None"
        interact_doors.append((a.get_name(), da_name, loc.x, loc.y, loc.z))

print(f"=== F1-F4 Office Door Meshes ({len(static_doors)}) ===")
for name, mesh, x, y, z in static_doors:
    print(f"  {name} [{mesh}] at ({x:.0f},{y:.0f},{z:.0f})")

print(f"\n=== F1-F4 Door Blueprints ({len(bp_doors)}) ===")
for name, cls, x, y, z in bp_doors:
    print(f"  {name} [{cls}] at ({x:.0f},{y:.0f},{z:.0f})")

print(f"\n=== F1-F4 InteractDoor Triggers ({len(interact_doors)}) ===")
for name, da, x, y, z in interact_doors:
    print(f"  {name} -> {da} at ({x:.0f},{y:.0f},{z:.0f})")

print(f"\nTotal door-related actors: {len(static_doors) + len(bp_doors) + len(interact_doors)}")
