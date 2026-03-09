import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = subsys.get_all_level_actors()

# Find the Floor 1 stairs location to define stairwell area
stairs = None
for a in actors:
    if a.get_actor_label() == "SM_Steps":
        stairs = a
        break

if stairs:
    loc = stairs.get_actor_location()
    print(f"Floor 1 stairs at: {loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}")

    # Search for actors near the stairwell (within 500 UU horizontal)
    nearby = []
    for a in actors:
        al = a.get_actor_location()
        dx = abs(al.x - loc.x)
        dy = abs(al.y - loc.y)
        if dx < 800 and dy < 800 and al.z >= 0 and al.z < 600:
            label = a.get_actor_label()
            # Focus on floors, doors, tiles
            if any(k in label.lower() for k in ["floor", "door", "tile", "step", "stair", "landing", "platform", "cube"]):
                nearby.append((label, al.x, al.y, al.z, a.get_class().get_name()))

    nearby.sort(key=lambda x: (x[3], x[0]))
    print(f"\nNearby floor/door/stair actors (F1-F2 range, within 800 UU):")
    for label, x, y, z, cls in nearby:
        print(f"  {label}: ({x:.1f}, {y:.1f}, {z:.1f}) [{cls}]")
else:
    print("SM_Steps not found")
