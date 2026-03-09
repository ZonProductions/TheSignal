"""Query all actors on Floor 2 to understand naming patterns for furniture vs structural."""
import unreal
from collections import Counter

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# F2 Z range: ~450 to ~780
F2_MIN_Z = 450
F2_MAX_Z = 780

f2_actors = []
for a in all_actors:
    loc = a.get_actor_location()
    if loc.z >= F2_MIN_Z and loc.z <= F2_MAX_Z:
        if 'ZP_InteractDoor' in a.get_class().get_name():
            continue
        if 'ZP_DoorSign' in a.get_class().get_name():
            continue
        f2_actors.append(a)

print(f"=== F2 actors (Z {F2_MIN_Z}-{F2_MAX_Z}): {len(f2_actors)} ===")

# Count by class
class_counts = Counter()
for a in f2_actors:
    class_counts[a.get_class().get_name()] += 1
print(f"\nBy class:")
for cls, count in class_counts.most_common():
    print(f"  {cls}: {count}")

# Show all unique base names (strip F2_ prefix and trailing numbers)
import re
base_names = Counter()
for a in f2_actors:
    label = a.get_actor_label()
    # Strip F2_ prefix
    name = re.sub(r'^F\d+_', '', label)
    # Strip trailing numbers
    name = re.sub(r'\d+$', '', name)
    base_names[name] += 1

print(f"\nUnique base name patterns ({len(base_names)}):")
for name, count in base_names.most_common():
    print(f"  {name}: {count}")
