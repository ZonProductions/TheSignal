"""Delete FloorTile actors in the stairwell area for floors 2-4.
Keeps F1 (ground) and F5 (top) stairwell floors intact."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Stairwell XY bounds (around Exit_door at X~939, Y~-1324)
STAIR_X_MIN = 800
STAIR_X_MAX = 1050  # Stop before elevator at X~1049
STAIR_Y_MIN = -1500
STAIR_Y_MAX = -1150

# F2-F4 Z range (skip F1 ground and F5 top)
Z_MIN = 400   # Above F1
Z_MAX = 1950  # Below F5

to_delete = []
for a in all_actors:
    label = a.get_actor_label()
    if 'FloorTile' not in label:
        continue
    loc = a.get_actor_location()
    if (STAIR_X_MIN <= loc.x <= STAIR_X_MAX and
        STAIR_Y_MIN <= loc.y <= STAIR_Y_MAX and
        Z_MIN <= loc.z <= Z_MAX):
        to_delete.append(a)
        print(f"  DELETE: {label} at Z={loc.z:.0f} ({loc.x:.0f}, {loc.y:.0f})")

print(f"\nDeleting {len(to_delete)} stairwell floor tiles (F2-F4)...")

deleted = 0
for a in to_delete:
    try:
        eas.destroy_actor(a)
        deleted += 1
    except:
        pass

print(f"Done: {deleted} tiles removed")
