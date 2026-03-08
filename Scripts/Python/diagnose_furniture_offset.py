"""Find furniture items and compare F1 vs F2 positions relative to their floor surface."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

FLOOR_HEIGHT = 500

# Sort actors by Z into floor buckets
floor1 = []  # Z < 450
floor2 = []  # 450 < Z < 950

for a in all_actors:
    z = a.get_actor_location().z
    label = a.get_actor_label()
    if -200 < z < 450:
        floor1.append(a)
    elif 300 < z < 950:
        floor2.append(a)

print(f"Floor 1 actors (Z -200 to 450): {len(floor1)}")
print(f"Floor 2 actors (Z 300 to 950): {len(floor2)}")

# Show a sample of floor 2 actors with their Z
print("\n=== Floor 2 sample (sorted by Z) ===")
f2_sorted = sorted(floor2, key=lambda a: a.get_actor_location().z)
for a in f2_sorted[:30]:
    loc = a.get_actor_location()
    cls = a.get_class().get_name()
    print(f"  Z={loc.z:7.1f}: {a.get_actor_label()} ({cls})")

# Find the floor surface mesh on Floor 2
print("\n=== Floor 2 floor/ground meshes ===")
for a in floor2:
    label = a.get_actor_label().lower()
    if 'floor' in label or 'ground' in label or 'guest' in label:
        loc = a.get_actor_location()
        print(f"  {a.get_actor_label()}: Z={loc.z:.1f}")

# Look for duplicated names to match F1->F2
print("\n=== Finding F1->F2 pairs by name ===")
f1_by_name = {}
for a in floor1:
    f1_by_name[a.get_actor_label()] = a

# F2 actors often get a number suffix when duplicated
pairs_found = 0
for a in floor2:
    label = a.get_actor_label()
    # Try stripping trailing numbers to find original
    base = label.rstrip('0123456789')
    if base and base.endswith('_'):
        base = base[:-1]
    if label in f1_by_name or base in f1_by_name:
        orig_name = label if label in f1_by_name else base
        f1_z = f1_by_name[orig_name].get_actor_location().z
        f2_z = a.get_actor_location().z
        diff = f2_z - f1_z
        if pairs_found < 20:
            print(f"  {orig_name}: F1 Z={f1_z:.1f}, F2 Z={f2_z:.1f}, diff={diff:.1f}")
        pairs_found += 1

print(f"\nTotal pairs found: {pairs_found}")
