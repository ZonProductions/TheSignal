import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Find elevator and stairwell actors to determine their X,Y positions
print("=== Elevator-related actors (all floors) ===")
for a in eas.get_all_level_actors():
    n = a.get_name()
    if any(k in n for k in ["Elevator", "elevator", "Lift", "lift"]):
        loc = a.get_actor_location()
        print(f"  {n} [{a.get_class().get_name()}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")

print("\n=== Stairwell-related actors (all floors) ===")
for a in eas.get_all_level_actors():
    n = a.get_name()
    if any(k in n.lower() for k in ["stair", "step", "railing", "handrail"]):
        loc = a.get_actor_location()
        if loc.z < 100:  # Just show F1 to avoid duplicates
            print(f"  {n} [{a.get_class().get_name()}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")

# Also show ladder positions as potential stairwell indicators
print("\n=== Ladder actors ===")
for a in eas.get_all_level_actors():
    if "Ladder" in a.get_class().get_name() or "ladder" in a.get_name().lower():
        loc = a.get_actor_location()
        print(f"  {n} at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")

# Show the X,Y spread of all vert/horz actors on F1 to understand the floor plan extent
print("\n=== Floor plan extent (F1, Z~0) ===")
min_x, max_x, min_y, max_y = 99999, -99999, 99999, -99999
for a in eas.get_all_level_actors():
    n = a.get_name().lower()
    if ("vert" in n or "horz" in n) and abs(a.get_actor_location().z) < 100:
        loc = a.get_actor_location()
        min_x = min(min_x, loc.x)
        max_x = max(max_x, loc.x)
        min_y = min(min_y, loc.y)
        max_y = max(max_y, loc.y)
print(f"  X: {min_x:.0f} to {max_x:.0f}")
print(f"  Y: {min_y:.0f} to {max_y:.0f}")
