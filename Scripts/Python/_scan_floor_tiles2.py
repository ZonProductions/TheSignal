import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Search for actors with "vert" or "horz" in the name
matches = []
for a in eas.get_all_level_actors():
    n = a.get_name()
    if "vert" in n.lower() or "horz" in n.lower() or "hori" in n.lower():
        loc = a.get_actor_location()
        matches.append((n, loc.z, a.get_class().get_name()))

print(f"Found {len(matches)} actors with vert/horz in name\n")

# Group by Z level
from collections import defaultdict
by_floor = defaultdict(list)
for name, z, cls in matches:
    # Round Z to nearest 500 to group by floor
    floor_z = round(z / 500) * 500
    by_floor[floor_z].append(name)

for z in sorted(by_floor):
    names = by_floor[z]
    print(f"Z ~{z}: {len(names)} actors")
    for n in names[:5]:
        print(f"  {n}")
    if len(names) > 5:
        print(f"  ... and {len(names)-5} more")
    print()

# Also check FloorTile pattern
ft_count = 0
for a in eas.get_all_level_actors():
    if "FloorTile" in a.get_name():
        ft_count += 1
if ft_count:
    print(f"\nAlso found {ft_count} 'FloorTile' actors")

# Check for any f4 or F4 pattern
f4_count = 0
f4_samples = []
for a in eas.get_all_level_actors():
    n = a.get_name()
    if "_f4_" in n.lower() or n.lower().startswith("f4"):
        f4_count += 1
        if len(f4_samples) < 5:
            f4_samples.append(n)
if f4_count:
    print(f"\nFound {f4_count} actors with 'f4' pattern")
    for s in f4_samples:
        print(f"  {s}")
