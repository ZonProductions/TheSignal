import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# The patio/wing transition is where the two buildings meet — around Y=1898 (mirror edge)
# SM_OutsideFloor is at Y=1378 in original building
# Find all SM_Column actors and group by Y position to find the patio area

# First, count columns per Y zone
columns_by_y = {}
all_columns = []

for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if not sm or sm.get_name() != "SM_Column":
        continue

    loc = a.get_actor_location()
    y_bucket = round(loc.y / 100) * 100
    if y_bucket not in columns_by_y:
        columns_by_y[y_bucket] = []
    columns_by_y[y_bucket].append((a.get_name(), loc.x, loc.y, loc.z))
    all_columns.append((a, loc.x, loc.y, loc.z))

print(f"Total SM_Column actors: {len(all_columns)}\n")
print("=== Columns grouped by Y zone ===")
for y in sorted(columns_by_y):
    count = len(columns_by_y[y])
    # Check if any were removed from F5 (Z~2000) — user said they removed some from top floor
    f5_count = sum(1 for _, _, _, z in columns_by_y[y] if 1900 < z < 2200)
    print(f"  Y~{y}: {count} columns (F5: {f5_count})")

# Find Y zones near the patio (OutsideFloor at Y=1378, mirror at Y=1898)
# The patio columns would be in the Y range between the buildings
print("\n=== Patio area columns (Y 1200-2600) ===")
patio_columns = [(a, x, y, z) for a, x, y, z in all_columns if 1200 < y < 2600]
print(f"Count: {len(patio_columns)}")

# Compare F5 vs other floors to find which ones user removed
by_floor = {}
for a, x, y, z in patio_columns:
    floor = round(z / 500)
    by_floor[floor] = by_floor.get(floor, 0) + 1

print("\nPatio columns per floor:")
for f in sorted(by_floor):
    print(f"  Z~{f*500}: {by_floor[f]}")
