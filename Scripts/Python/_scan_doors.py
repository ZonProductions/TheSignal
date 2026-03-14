import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()
print(f"Total actors: {len(actors)}")

doors = []
interact_doors = []
lockers = []
containers = []
pickups = []

for a in actors:
    name = a.get_name()
    cls = a.get_class().get_name()
    loc = a.get_actor_location()
    pos = f"({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})"

    if "InteractDoor" in cls or "ZP_InteractDoor" in cls:
        interact_doors.append((name, cls, pos))
    elif "Door" in name or "door" in name.lower():
        doors.append((name, cls, pos))

    if "LootLocker" in cls:
        lockers.append((name, cls, pos))
    elif "ItemContainer" in cls or "Briefcase" in cls:
        containers.append((name, cls, pos))
    elif "ItemPickup" in cls:
        pickups.append((name, cls, pos))

print(f"\n--- ZP_InteractDoor actors ({len(interact_doors)}) ---")
for n,c,p in interact_doors:
    print(f"  {n} [{c}] at {p}")

print(f"\n--- Other Door-named actors ({len(doors)}) ---")
for n,c,p in sorted(doors, key=lambda x: x[2]):
    print(f"  {n} [{c}] at {p}")

print(f"\n--- Loot Lockers ({len(lockers)}) ---")
for n,c,p in lockers:
    print(f"  {n} [{c}] at {p}")

print(f"\n--- Containers ({len(containers)}) ---")
for n,c,p in containers:
    print(f"  {n} [{c}] at {p}")

print(f"\n--- Pickups ({len(pickups)}) ---")
for n,c,p in pickups:
    print(f"  {n} [{c}] at {p}")
