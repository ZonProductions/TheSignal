"""
Spawn AZP_InteractDoor triggers for all doors in the current level.

Identifies door actors, determines Rotate vs Slide mode, spawns a trigger
at each door's location with DoorActor linked.

Door panels come in pairs (door + frame) at the same location.
We only create triggers for the panel (the part that moves), not the frame.
"""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# First: delete any existing door triggers from previous runs
all_actors = eas.get_all_level_actors()
old_triggers = [a for a in all_actors if 'ZP_InteractDoor' in a.get_class().get_name() or 'DoorTrigger' in a.get_actor_label()]
if old_triggers:
    print(f"Removing {len(old_triggers)} old door triggers...")
    for t in old_triggers:
        eas.destroy_actor(t)

all_actors = eas.get_all_level_actors()

# Collect all door actors
doors = []
for actor in all_actors:
    name = actor.get_actor_label()
    class_name = actor.get_class().get_name()
    if 'door' in name.lower() or 'door' in class_name.lower():
        doors.append((name, class_name, actor))

print(f"Found {len(doors)} door-related actors")

# Classification rules:
# Slide doors: glass doors, elevator doors, auto doors
# Rotate doors: office doors, WC doors, exit doors
# Skip: indoor_unit (AC units, not doors), door frames (paired meshes)

SLIDE_KEYWORDS = ['glass', 'elevator', 'aotu', 'auto']
SKIP_KEYWORDS = ['indoor_unit', 'elevator']

# For office_door pairs: polySurface48-54 are frames, polySurface55-63 are panels (or vice versa)
# Actually they come in pairs at the same location. We'll track locations to avoid double-spawning.
# We pick one per location.

spawned_locations = {}  # (x,y) -> trigger actor, to avoid duplicates at same spot

# Load the AZP_InteractDoor class
door_trigger_class = unreal.load_class(None, '/Script/TheSignal.ZP_InteractDoor')
if not door_trigger_class:
    raise RuntimeError("Could not load AZP_InteractDoor class!")

spawned_count = 0
skipped = []

for name, class_name, actor in doors:
    # Skip non-door items
    if any(kw in name.lower() for kw in SKIP_KEYWORDS):
        skipped.append(f"{name} (skip keyword)")
        continue

    # Skip SM_FrameTallDoor (it's a frame, not a panel)
    if 'frame' in name.lower():
        skipped.append(f"{name} (frame)")
        continue

    loc = actor.get_actor_location()
    loc_key = (round(loc.x), round(loc.y))

    # Skip if we already spawned a trigger at this location (paired meshes)
    if loc_key in spawned_locations:
        skipped.append(f"{name} (duplicate location with {spawned_locations[loc_key]})")
        continue

    # Determine mode
    is_slide = any(kw in name.lower() for kw in SLIDE_KEYWORDS)

    # Spawn trigger at door location
    trigger = eas.spawn_actor_from_class(
        door_trigger_class,
        unreal.Vector(loc.x, loc.y, loc.z + 50)  # Slightly above floor for overlap
    )

    if not trigger:
        print(f"  FAILED to spawn trigger for {name}")
        continue

    # Set properties
    trigger.set_editor_property('DoorActor', actor)

    if is_slide:
        trigger.set_editor_property('OpenMode', unreal.ZP_InteractDoorMode.SLIDE)
        trigger.set_editor_property('SlideOffset', unreal.Vector(0, 150, 0))
    else:
        trigger.set_editor_property('OpenMode', unreal.ZP_InteractDoorMode.ROTATE)
        trigger.set_editor_property('OpenAngle', 90.0)

    trigger.set_editor_property('InterpSpeed', 4.0)

    # Label it clearly
    mode_str = "Slide" if is_slide else "Rotate"
    trigger.set_actor_label(f"DoorTrigger_{name}_{mode_str}")

    spawned_locations[loc_key] = name
    spawned_count += 1
    print(f"  Spawned: DoorTrigger_{name}_{mode_str} at ({loc.x:.0f}, {loc.y:.0f})")

print(f"\n=== Spawned {spawned_count} door triggers ===")
if skipped:
    print(f"Skipped {len(skipped)}:")
    for s in skipped:
        print(f"  {s}")
