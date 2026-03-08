"""Assign each floor's actors to a named Layer using the Layers subsystem.

Lights and system actors go into a 'Global' layer that stays always visible.
Geometry/furniture goes into Floor_1 through Floor_5.
"""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ls = unreal.get_editor_subsystem(unreal.LayersSubsystem)
all_actors = eas.get_all_level_actors()

# Remove old layer assignments first
for a in all_actors:
    ls.disassociate_actor_from_layers(a)

# Delete old layers
for i in range(1, 6):
    ls.delete_layer(f"Floor_{i}")

# Create layers
ls.create_layer("Global")
for i in range(1, 6):
    ls.create_layer(f"Floor_{i}")

# System/lighting classes that should always be visible
GLOBAL_CLASSES = {
    'DirectionalLight', 'SkyLight', 'SkyAtmosphere', 'VolumetricCloud',
    'ExponentialHeightFog', 'AtmosphericFog', 'PostProcessVolume',
    'LightmassImportanceVolume', 'NavMeshBoundsVolume', 'BlockingVolume',
    'PlayerStart', 'WorldSettings', 'AbstractNavData', 'NavigationData',
    'RecastNavMesh', 'LevelBounds', 'Note', 'LevelScriptActor',
    'WorldDataLayers', 'WorldPartitionMiniMap', 'BP_Sky_Sphere_C',
    'SphereReflectionCapture', 'BoxReflectionCapture', 'PlanarReflection',
    'PointLight', 'SpotLight', 'RectLight',
}

# Light-related name patterns (catches numbered variants like PointLight2)
LIGHT_KEYWORDS = ['light', 'lamp', 'florosent', 'Light']

counts = {"Global": 0, 1: 0, 2: 0, 3: 0, 4: 0, 5: 0}

for a in all_actors:
    label = a.get_actor_label()
    class_name = a.get_class().get_name()
    base_class = class_name.rstrip('0123456789_')

    # System actors and lights -> Global (always visible)
    is_global = (
        base_class in GLOBAL_CLASSES or
        class_name in GLOBAL_CLASSES or
        any(kw in class_name for kw in ['Light', 'Fog', 'Sky', 'Volume', 'Reflection'])
    )

    if is_global:
        ls.add_actor_to_layer(a, "Global")
        counts["Global"] += 1
        continue

    # Floor assignment by prefix
    if label.startswith('F2_'):
        floor = 2
    elif label.startswith('F3_'):
        floor = 3
    elif label.startswith('F4_'):
        floor = 4
    elif label.startswith('F5_'):
        floor = 5
    else:
        floor = 1

    ls.add_actor_to_layer(a, f"Floor_{floor}")
    counts[floor] += 1

# All visible
ls.make_all_layers_visible()
ls.editor_refresh_layer_browser()

for k, c in sorted(counts.items(), key=lambda x: str(x[0])):
    print(f"{'Global' if k == 'Global' else f'Floor_{k}'}: {c} actors")

print("\nDone! Global layer = lights/system (keep on). Floor layers = geometry (toggle freely).")
