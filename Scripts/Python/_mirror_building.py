import unreal
import math

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Step 1: Find the OutsideFloor outer edge (max Y of building)
# OutsideFloor at Y=1378, world size Y=711, so outer edge ~Y=2089
# Building Y range: -1796 to 1898
# We'll use the building center X as the mirror axis

# Find building center X (mirror axis)
all_actors = eas.get_all_level_actors()
skip_classes = {"SkyAtmosphere", "ExponentialHeightFog", "PostProcessVolume",
                "DirectionalLight", "SkyLight", "AbstractNavData",
                "NavigationData", "WorldSettings", "GameModeBase",
                "Brush", "PlayerStart"}

actors_to_copy = []
min_x, max_x = 99999, -99999
max_y = -99999

for a in all_actors:
    cn = a.get_class().get_name()
    if cn in skip_classes:
        continue
    # Skip any actors that are clearly world-level, not building
    if "NavMesh" in cn or "WorldSettings" in cn:
        continue

    loc = a.get_actor_location()
    min_x = min(min_x, loc.x)
    max_x = max(max_x, loc.x)
    max_y = max(max_y, loc.y)
    actors_to_copy.append(a)

center_x = (min_x + max_x) / 2.0

# The mirror point in Y: OutsideFloor outer edge
# We want the copy's OutsideFloor to touch the original's
# Mirror Y = max_y (outer edge), copy placed on the other side
mirror_y = max_y

print(f"Building: X {min_x:.0f} to {max_x:.0f}, center_x={center_x:.0f}")
print(f"Mirror Y edge: {mirror_y:.0f}")
print(f"Actors to duplicate: {len(actors_to_copy)}")

# Step 2: Duplicate and transform
# For 180° rotation around Z:
#   new_x = 2*center_x - old_x  (mirror around center X)
#   new_y = 2*mirror_y - old_y  (mirror around outer Y edge)
#   new_yaw = old_yaw + 180

duplicated = 0
errors = 0

for a in actors_to_copy:
    try:
        old_loc = a.get_actor_location()
        old_rot = a.get_actor_rotation()
        old_scale = a.get_actor_scale3d()

        # Calculate mirrored position
        new_x = 2.0 * center_x - old_loc.x
        new_y = 2.0 * mirror_y - old_loc.y
        new_z = old_loc.z

        new_loc = unreal.Vector(new_x, new_y, new_z)
        new_rot = unreal.Rotator(roll=old_rot.roll, pitch=old_rot.pitch, yaw=old_rot.yaw + 180.0)

        # Duplicate
        new_actors = eas.duplicate_actors([a])
        if new_actors and len(new_actors) > 0:
            new_a = new_actors[0]
            new_a.set_actor_location(new_loc, False, False)
            new_a.set_actor_rotation(new_rot, False)
            duplicated += 1
        else:
            errors += 1
    except Exception as e:
        errors += 1

print(f"\nDuplicated: {duplicated}")
print(f"Errors: {errors}")
print(f"Total actors now: {len(eas.get_all_level_actors())}")
