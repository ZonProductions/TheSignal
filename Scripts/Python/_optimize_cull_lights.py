"""Cull archviz light bloat. Keep ~10 lights per floor, delete the rest."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Collect all lights by floor
floor_lights = {0: [], 1: [], 2: [], 3: [], 4: []}

for a in eas.get_all_level_actors():
    lc = a.get_component_by_class(unreal.LightComponent)
    if not lc:
        continue

    label = a.get_actor_label()
    cls = a.get_class().get_name()
    loc = a.get_actor_location()
    z = loc.z
    floor_idx = max(0, min(4, int(z / 500.0)))

    # Get light type and intensity
    intensity = lc.get_editor_property('intensity')
    atten = 0
    try:
        atten = lc.get_editor_property('attenuation_radius')
    except:
        pass

    floor_lights[floor_idx].append({
        'actor': a,
        'label': label,
        'cls': cls,
        'x': loc.x, 'y': loc.y, 'z': z,
        'intensity': intensity,
        'atten': atten,
        'floor': floor_idx
    })

for f in range(5):
    print(f"Floor {f+1}: {len(floor_lights[f])} lights")

# Strategy: keep the brightest/largest lights per floor, delete the rest
# Keep up to 10 per floor = 50 total
KEEP_PER_FLOOR = 10

total_deleted = 0
total_kept = 0

for f in range(5):
    lights = floor_lights[f]
    if len(lights) <= KEEP_PER_FLOOR:
        total_kept += len(lights)
        print(f"  Floor {f+1}: keeping all {len(lights)} (under limit)")
        continue

    # Sort by intensity * attenuation (biggest impact lights first)
    lights.sort(key=lambda l: l['intensity'] * max(l['atten'], 1), reverse=True)

    kept = 0
    deleted = 0
    for i, light in enumerate(lights):
        if i < KEEP_PER_FLOOR:
            kept += 1
        else:
            light['actor'].destroy_actor()
            deleted += 1

    total_kept += kept
    total_deleted += deleted
    print(f"  Floor {f+1}: kept {kept}, deleted {deleted}")

print(f"\nTotal: kept {total_kept} lights, deleted {total_deleted}")
