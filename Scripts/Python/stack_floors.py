"""Stack the current office floor 4 more times to create a 5-story building.

Duplicates all geometry actors with Z offset per floor.
Floor 1 = existing (Z=0), Floor 2-5 = duplicated copies.

Rooftop elements (Z > 460) are only included on the top floor (Floor 5).
Intermediate floors get interior-only copies to prevent clipping.
"""
import unreal

FLOOR_HEIGHT = 500  # UU per floor
NUM_NEW_FLOORS = 4  # 4 more copies = 5 total
ROOF_CUTOFF_Z = 460  # Actors above this Z are "rooftop" elements

# Actor classes to SKIP (system/environment actors that shouldn't be per-floor)
SKIP_CLASSES = {
    'DirectionalLight', 'SkyLight', 'SkyAtmosphere', 'VolumetricCloud',
    'ExponentialHeightFog', 'AtmosphericFog', 'PostProcessVolume',
    'LightmassImportanceVolume', 'NavMeshBoundsVolume', 'BlockingVolume',
    'PlayerStart', 'GameModeBase', 'WorldSettings', 'AbstractNavData',
    'NavigationData', 'RecastNavMesh', 'LevelBounds',
    'SphereReflectionCapture', 'BoxReflectionCapture', 'PlanarReflection',
    'Note', 'LevelScriptActor', 'WorldDataLayers', 'WorldPartitionMiniMap',
    'BP_Sky_Sphere_C',
}

# Skip our custom gameplay actors
SKIP_PREFIXES = ['ZP_', 'DoorTrigger', 'BP_PatrolCreature', 'BP_3D_Grid']

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# --- STEP 1: Delete existing floors 2-5 ---
print("=== Deleting existing floors 2-5 ===")
all_actors = eas.get_all_level_actors()
to_delete = [a for a in all_actors if a.get_actor_label().startswith(('F2_', 'F3_', 'F4_', 'F5_'))]
if to_delete:
    print(f"Deleting {len(to_delete)} actors from floors 2-5...")
    for a in to_delete:
        eas.destroy_actor(a)
    print("Deleted.")
else:
    print("No existing floor 2-5 actors found.")

# --- STEP 2: Collect Floor 1 actors ---
all_actors = eas.get_all_level_actors()

interior_actors = []  # Below roof cutoff — duplicated for ALL floors
rooftop_actors = []   # Above roof cutoff — only duplicated for top floor

for actor in all_actors:
    class_name = actor.get_class().get_name()
    label = actor.get_actor_label()

    # Skip system actors
    if class_name in SKIP_CLASSES or class_name.rstrip('0123456789') in SKIP_CLASSES:
        continue

    # Skip our gameplay actors
    if any(label.startswith(p) for p in SKIP_PREFIXES):
        continue
    if any(class_name.startswith(p) for p in ['ZP_', 'AZP_']):
        continue

    z = actor.get_actor_location().z
    if z > ROOF_CUTOFF_Z:
        rooftop_actors.append(actor)
    else:
        interior_actors.append(actor)

print(f"\nInterior actors (Z <= {ROOF_CUTOFF_Z}): {len(interior_actors)}")
print(f"Rooftop actors (Z > {ROOF_CUTOFF_Z}): {len(rooftop_actors)}")
print(f"Floor height: {FLOOR_HEIGHT} UU")

# --- STEP 3: Duplicate ---
total_created = 0

for floor_idx in range(1, NUM_NEW_FLOORS + 1):
    z_offset = FLOOR_HEIGHT * floor_idx
    floor_num = floor_idx + 1
    is_top_floor = (floor_idx == NUM_NEW_FLOORS)

    print(f"\n--- Floor {floor_num} (Z offset: +{z_offset}) ---")

    # Interior actors for all floors
    offset = unreal.Vector(0, 0, z_offset)
    new_actors = eas.duplicate_actors(interior_actors, offset=offset)
    count = len(new_actors) if new_actors else 0

    # Rooftop actors only for top floor
    if is_top_floor and rooftop_actors:
        roof_new = eas.duplicate_actors(rooftop_actors, offset=offset)
        roof_count = len(roof_new) if roof_new else 0
        print(f"  + {roof_count} rooftop actors (top floor only)")
        if roof_new:
            for a in roof_new:
                a.set_actor_label(f"F{floor_num}_{a.get_actor_label()}")
            count += roof_count

    if new_actors:
        for a in new_actors:
            a.set_actor_label(f"F{floor_num}_{a.get_actor_label()}")

    print(f"  Duplicated {count} actors")
    total_created += count

print(f"\n=== Done! Created {total_created} actors across {NUM_NEW_FLOORS} new floors ===")
print(f"Rooftop elements only on Floor {NUM_NEW_FLOORS + 1} (top floor)")
print(f"Floor 1 untouched.")
