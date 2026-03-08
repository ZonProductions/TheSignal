"""Measure floor height by finding Z bounds of structural actors."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Skip non-geometry actors
SKIP_CLASSES = ['AtmosphericFog', 'SkyLight', 'DirectionalLight', 'ExponentialHeightFog',
                'PostProcessVolume', 'LightmassImportanceVolume', 'SphereReflectionCapture',
                'BoxReflectionCapture', 'Note', 'PlayerStart', 'NavMeshBoundsVolume',
                'BlockingVolume', 'BP_Sky', 'VolumetricCloud', 'SkyAtmosphere']

min_z = float('inf')
max_z = float('-inf')

for actor in all_actors:
    class_name = actor.get_class().get_name()
    if any(skip in class_name for skip in SKIP_CLASSES):
        continue
    loc = actor.get_actor_location()
    if loc.z < -1000:  # Skip anything deep underground
        continue
    if loc.z < min_z:
        min_z = loc.z
    if loc.z > max_z:
        max_z = loc.z

print(f"Geometry Z range: {min_z:.0f} to {max_z:.0f}")
print(f"Height span: {max_z - min_z:.0f} UU")

# Show Z distribution to find natural floor/ceiling
z_vals = []
for actor in all_actors:
    class_name = actor.get_class().get_name()
    if any(skip in class_name for skip in SKIP_CLASSES):
        continue
    loc = actor.get_actor_location()
    if -100 < loc.z < 2000:
        z_vals.append(loc.z)

# Histogram of Z values in 50 UU bins
bins = {}
for z in z_vals:
    b = int(z // 50) * 50
    bins[b] = bins.get(b, 0) + 1

print("\n--- Z distribution (50 UU bins) ---")
for b in sorted(bins.keys()):
    bar = '#' * min(bins[b], 60)
    print(f"  Z {b:5d}: {bins[b]:4d} {bar}")

# Show highest structural actors
print("\n--- Highest 20 actors ---")
high_actors = []
for actor in all_actors:
    class_name = actor.get_class().get_name()
    if any(skip in class_name for skip in SKIP_CLASSES):
        continue
    loc = actor.get_actor_location()
    if loc.z > -1000:
        high_actors.append((loc.z, actor.get_actor_label(), class_name))

high_actors.sort(reverse=True)
for z, name, cls in high_actors[:20]:
    print(f"  Z={z:7.0f}: {name} ({cls})")
