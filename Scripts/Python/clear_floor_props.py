"""
Delete all furniture and props from floors 1-4. Keep floor 5 intact.
Keeps: structural elements, doors, lights, ZP_ actors, floor tiles, columns, walls, pipes, etc.
"""
import unreal
import re
from collections import Counter

DRY_RUN = False  # Set True to preview without deleting

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Floor 5 starts at Z ~1950. Everything below is F1-F4.
F5_MIN_Z = 1950

# ========== KEEP RULES ==========

# Classes to ALWAYS keep
KEEP_CLASSES = {
    'RectLight', 'SpotLight', 'PointLight', 'DirectionalLight',
    'SkyLight', 'ExponentialHeightFog', 'SkyAtmosphere', 'VolumetricCloud',
    'LightmassPortal', 'PostProcessVolume',
    'SphereReflectionCapture', 'BoxReflectionCapture', 'PlanarReflection',
    'GroupActor', 'PlayerStart', 'Note',
    'WorldSettings', 'LevelScriptActor',
    'NavigationSystemV1', 'AbstractNavData', 'RecastNavMesh',
    'NavMeshBoundsVolume', 'LightmassImportanceVolume',
    'CameraActor', 'CineCameraActor',
    'Brush', 'Volume',
    'AudioVolume', 'BlockingVolume', 'TriggerVolume',
    'LevelBounds',
}

# Class name CONTAINS any of these = keep
KEEP_CLASS_CONTAINS = [
    'ZP_',                # All our custom actors
    'BP_ElevatorDoors',   # Elevator door BP
    'BP_GlassDoors',      # Glass door BP
    'BP_WCDoor',          # WC door BP
    'BP_FloorSign',       # Floor sign BPs
]

# Label patterns to KEEP (structural, doors, infrastructure)
# Matched as: label starts with pattern OR stripped label (no F#_ prefix) starts with pattern
KEEP_LABEL_PATTERNS = [
    # Floor/ceiling/walls
    'FloorTile',
    'Column',
    'SM_wallBrick',
    'SM_WindowWall',
    'SM_CenterRooms',
    'SM_WcWall',
    'SM_Securitywall',
    'SM_SecuritySilling',
    'SM_ElevatorWall',
    'SM_LastLine',
    'SM_PlayRoom',
    'SM_Steps',
    'SM_KitchenFloor',
    'SM_ConferenceSecretaryRoom',
    'SM_FrameTall',       # includes SM_FrameTallDoor
    'SM_Fence',
    'base_building',
    'Elevator_final',
    'SM_Elevator',        # elevator doors
    # Structural framing
    'vert',
    'hoiz',
    'out_woodfloor',
    'Cube',               # structural blocks
    'SM_Cube',
    'Cylinder',
    # Doors (mesh actors)
    'office_door',
    'Exit_door',
    'Aotu_door',
    # Infrastructure
    'pipe_line',
    'Pipe_',
    'SM_AirDuct',
    'indoor_unit',
    'out_unit',
    'AirConditioner',
    'Chiller_engine',
    'electrical_enclosure',
]

def should_keep(actor):
    cls_name = actor.get_class().get_name()
    label = actor.get_actor_label()

    # Keep by exact class
    if cls_name in KEEP_CLASSES:
        return True

    # Keep by class contains
    for pattern in KEEP_CLASS_CONTAINS:
        if pattern in cls_name:
            return True

    # Strip floor prefix for label matching (F2_, F3_, etc.)
    stripped = re.sub(r'^F\d+_', '', label)

    # Keep by label pattern
    for pattern in KEEP_LABEL_PATTERNS:
        if label.startswith(pattern) or stripped.startswith(pattern):
            return True

    return False


# ========== PROCESS ==========

to_delete = []
to_keep = []
deleted_names = Counter()
kept_names = Counter()

for a in all_actors:
    loc = a.get_actor_location()

    # Skip Floor 5 entirely
    if loc.z >= F5_MIN_Z:
        continue

    if should_keep(a):
        to_keep.append(a)
        name = re.sub(r'^F\d+_', '', a.get_actor_label())
        name = re.sub(r'\d+$', '', name)
        kept_names[name] += 1
    else:
        to_delete.append(a)
        name = re.sub(r'^F\d+_', '', a.get_actor_label())
        name = re.sub(r'\d+$', '', name)
        deleted_names[name] += 1

print(f"=== {'DRY RUN' if DRY_RUN else 'EXECUTING'} ===")
print(f"Floors 1-4 total actors: {len(to_delete) + len(to_keep)}")
print(f"KEEPING: {len(to_keep)}")
print(f"DELETING: {len(to_delete)}")

print(f"\n--- Top deleted patterns ---")
for name, count in deleted_names.most_common(40):
    print(f"  {name}: {count}")

print(f"\n--- Top kept patterns ---")
for name, count in kept_names.most_common(30):
    print(f"  {name}: {count}")

if not DRY_RUN:
    deleted = 0
    failed = 0
    for a in to_delete:
        try:
            eas.destroy_actor(a)
            deleted += 1
        except:
            failed += 1

    print(f"\n=== RESULT: {deleted} deleted, {failed} failed ===")
else:
    print(f"\n=== DRY RUN — no actors deleted. Set DRY_RUN = False to execute. ===")
