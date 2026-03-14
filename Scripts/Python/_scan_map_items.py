"""Scan the current level for all interactable items and their positions/collision."""
import unreal

ea = unreal.EditorLevelLibrary
actors = ea.get_all_level_actors()

# Categories to track
pickups = []
containers = []
interactables = []
lockers = []
elevators = []

for a in actors:
    name = a.get_name()
    cls = a.get_class().get_name()
    loc = a.get_actor_location()
    pos = f"({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f})"

    if 'ItemPickup' in cls:
        pickups.append((name, cls, pos))
    elif 'LootLocker' in cls:
        lockers.append((name, cls, pos))
    elif 'ItemContainer' in cls or 'Briefcase' in cls:
        containers.append((name, cls, pos))
    elif 'Interactable' in cls or 'Ladder' in cls or 'Door' in cls or 'SavePoint' in cls or 'MapPickup' in cls:
        interactables.append((name, cls, pos))
    elif 'Elevator' in name or 'elevator' in name.lower():
        elevators.append((name, cls, pos))

print("\n=== LEVEL ITEM SCAN ===")
print(f"\n--- PICKUPS ({len(pickups)}) ---")
for n, c, p in pickups:
    print(f"  {n} [{c}] at {p}")

print(f"\n--- LOOT LOCKERS ({len(lockers)}) ---")
for n, c, p in lockers:
    print(f"  {n} [{c}] at {p}")

print(f"\n--- CONTAINERS ({len(containers)}) ---")
for n, c, p in containers:
    print(f"  {n} [{c}] at {p}")

print(f"\n--- INTERACTABLES ({len(interactables)}) ---")
for n, c, p in interactables:
    print(f"  {n} [{c}] at {p}")

print(f"\n--- ELEVATOR ACTORS ({len(elevators)}) ---")
for n, c, p in elevators:
    print(f"  {n} [{c}] at {p}")

# Also find SM_Elevator static meshes
sm_elevators = []
for a in actors:
    if a.get_class().get_name() == 'StaticMeshActor':
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps:
            sm = c.static_mesh
            if sm and 'levator' in sm.get_name():
                loc = a.get_actor_location()
                sm_elevators.append((a.get_name(), sm.get_name(), f"({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f})"))

print(f"\n--- SM_ELEVATOR MESHES ({len(sm_elevators)}) ---")
for n, sm, p in sm_elevators:
    print(f"  {n} mesh={sm} at {p}")

print("\n=== END SCAN ===")
