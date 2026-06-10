"""Spawn a single AZP_InteractDoor trigger for F3_Exit_door_pCube166.

This door near the F3 player start was never given a trigger by the bulk
spawn_door_triggers.py run, so it could not be opened. Matches the working
sibling exit doors (Exit_door_pCube164 family): Rotate mode, 90 deg, speed 4.
"""
import unreal

TARGET = 'F3_Exit_door_pCube166'

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()

door = None
for a in actors:
    if a.get_actor_label() == TARGET:
        door = a
        break

if not door:
    raise RuntimeError(f"{TARGET} not found in level")

# Guard: don't create a duplicate trigger if one already links this door.
for a in actors:
    if 'InteractDoor' in a.get_class().get_name():
        try:
            da = a.get_editor_property('DoorActor')
        except Exception:
            da = None
        if da == door:
            raise RuntimeError(f"Trigger already exists for {TARGET}: {a.get_actor_label()}")

loc = door.get_actor_location()

door_trigger_class = unreal.load_class(None, '/Script/TheSignal.ZP_InteractDoor')
if not door_trigger_class:
    raise RuntimeError("Could not load AZP_InteractDoor class!")

trigger = eas.spawn_actor_from_class(
    door_trigger_class,
    unreal.Vector(loc.x, loc.y, loc.z + 50)
)
if not trigger:
    raise RuntimeError("Failed to spawn trigger")

trigger.set_editor_property('DoorActor', door)
trigger.set_editor_property('OpenMode', unreal.ZP_InteractDoorMode.ROTATE)
trigger.set_editor_property('OpenAngle', 90.0)
trigger.set_editor_property('InterpSpeed', 4.0)
trigger.set_actor_label(f"DoorTrigger_{TARGET}_Rotate")

# Verify
da = trigger.get_editor_property('DoorActor')
tl = trigger.get_actor_location()
print(f"Spawned {trigger.get_actor_label()} at ({tl.x:.0f},{tl.y:.0f},{tl.z:.0f}) -> DoorActor={da.get_actor_label() if da else 'NONE'}, mode=ROTATE, angle=90")

# Persist the level edit
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.save_current_level()
print("Saved current level.")
