import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

counts = {}
samples = {}
stairwell_elevator = {}

for a in eas.get_all_level_actors():
    n = a.get_name().lower()

    # Match floor prefixes f1_ through f5_
    floor = None
    for i in range(1, 6):
        if n.startswith(f"f{i}_"):
            floor = f"F{i}"
            break

    if not floor:
        continue

    counts[floor] = counts.get(floor, 0) + 1

    # Check if it's stairwell or elevator related
    is_special = any(k in n for k in ["stair", "elevator", "elev", "lift"])
    if is_special:
        stairwell_elevator[floor] = stairwell_elevator.get(floor, 0) + 1

    if floor not in samples:
        samples[floor] = []
    if len(samples[floor]) < 8:
        tag = " [STAIR/ELEV]" if is_special else ""
        samples[floor].append(a.get_name() + tag)

print("=== Floor Tile Actors ===\n")
for k in sorted(counts):
    special = stairwell_elevator.get(k, 0)
    deletable = counts[k] - special
    print(f"{k}: {counts[k]} total, {special} stairwell/elevator, {deletable} deletable")
    for s in samples[k]:
        print(f"  {s}")
    print()

# Show what naming patterns exist
print("=== Unique suffixes (first 20) ===")
suffixes = set()
for a in eas.get_all_level_actors():
    n = a.get_name().lower()
    if n.startswith("f4_"):
        # Get the part after f4_
        suffix = n[3:]
        base = suffix.split("_")[0] if "_" in suffix else suffix
        suffixes.add(base)
for s in sorted(suffixes)[:20]:
    print(f"  {s}")
