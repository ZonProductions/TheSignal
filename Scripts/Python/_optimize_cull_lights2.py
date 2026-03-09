"""Find and cull lights. Try multiple detection methods."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Find lights by class name
lights = []
for a in eas.get_all_level_actors():
    cls = a.get_class().get_name()
    if 'Light' in cls and 'Controller' not in cls and 'Importance' not in cls and 'Lightmass' not in cls:
        loc = a.get_actor_location()
        z = loc.z
        floor_idx = max(0, min(4, int(z / 500.0)))

        # Try to get intensity
        intensity = 0
        for comp_cls in [unreal.SpotLightComponent, unreal.RectLightComponent, unreal.PointLightComponent, unreal.DirectionalLightComponent]:
            lc = a.get_component_by_class(comp_cls)
            if lc:
                try:
                    intensity = lc.get_editor_property('intensity')
                except:
                    pass
                break

        lights.append({
            'actor': a,
            'label': a.get_actor_label(),
            'cls': cls,
            'z': z,
            'floor': floor_idx,
            'intensity': intensity,
        })

print(f"Found {len(lights)} light actors")

# Count by type
type_counts = {}
for l in lights:
    type_counts[l['cls']] = type_counts.get(l['cls'], 0) + 1
for cls, count in sorted(type_counts.items(), key=lambda x: -x[1]):
    print(f"  {cls}: {count}")

# Count by floor
for f in range(5):
    count = sum(1 for l in lights if l['floor'] == f)
    print(f"Floor {f+1}: {count} lights")

# Keep top 10 per floor by intensity, delete rest
KEEP_PER_FLOOR = 10
total_deleted = 0
total_kept = 0

for f in range(5):
    floor = [l for l in lights if l['floor'] == f]
    if len(floor) <= KEEP_PER_FLOOR:
        total_kept += len(floor)
        continue

    floor.sort(key=lambda l: l['intensity'], reverse=True)
    for i, l in enumerate(floor):
        if i < KEEP_PER_FLOOR:
            total_kept += 1
        else:
            l['actor'].destroy_actor()
            total_deleted += 1

    print(f"  Floor {f+1}: kept {KEEP_PER_FLOOR}, deleted {len(floor) - KEEP_PER_FLOOR}")

print(f"\nKept {total_kept}, deleted {total_deleted}")
