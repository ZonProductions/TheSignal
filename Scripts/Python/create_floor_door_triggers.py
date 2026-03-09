"""
Create ZP_InteractDoor triggers for floors 2-5 by replicating F1 triggers.
Building 1 has identical floors stacked at 500 UU intervals.
F1 already has 16 triggers — this script creates matching triggers on F2-F5.
"""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

FLOOR_HEIGHT = 500

# Get existing ZP_InteractDoor triggers (all should be on F1)
triggers = [a for a in all_actors if 'ZP_InteractDoor' in a.get_class().get_name()]
print(f"Found {len(triggers)} existing triggers")

# Check which floors already have triggers to avoid duplicates
existing_trigger_floors = set()
for t in triggers:
    tz = t.get_actor_location().z
    floor = round(tz / FLOOR_HEIGHT) + 1
    existing_trigger_floors.add(floor)
print(f"Triggers exist on floors: {sorted(existing_trigger_floors)}")

# Only process F1 triggers as templates
f1_triggers = [t for t in triggers if round(t.get_actor_location().z / FLOOR_HEIGHT) + 1 == 1]
print(f"F1 template triggers: {len(f1_triggers)}")

created = 0
warnings = []

for trigger in f1_triggers:
    door_actor = trigger.get_editor_property('door_actor')
    if not door_actor:
        warnings.append(f"Trigger {trigger.get_actor_label()} has no DoorActor — skipped")
        continue

    trigger_loc = trigger.get_actor_location()
    trigger_rot = trigger.get_actor_rotation()
    door_loc = door_actor.get_actor_location()

    # Get trigger config
    open_mode = trigger.get_editor_property('open_mode')
    open_angle = trigger.get_editor_property('open_angle')
    slide_offset = trigger.get_editor_property('slide_offset')
    interp_speed = trigger.get_editor_property('interp_speed')

    # Get door class + mesh for matching on other floors
    door_class_name = door_actor.get_class().get_name()
    door_mesh = None
    if door_class_name == 'StaticMeshActor':
        smc = door_actor.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            door_mesh = smc.get_editor_property('static_mesh')

    mode_str = "Slide" if "SLIDE" in str(open_mode).upper() else "Rotate"
    print(f"\nTemplate: {trigger.get_actor_label()} -> {door_actor.get_actor_label()} ({mode_str})")

    for floor_num in [2, 3, 4, 5]:
        if floor_num in existing_trigger_floors:
            # Check if this specific door already has a trigger on this floor
            # For now, skip entire floor if any triggers exist there
            pass  # We'll create anyway if floor isn't in existing set

        z_offset = (floor_num - 1) * FLOOR_HEIGHT
        expected_door_z = door_loc.z + z_offset

        # Find matching door actor on this floor
        target_door = None
        best_dist = 999999

        for a in all_actors:
            # Must be same class
            if a.get_class().get_name() != door_class_name:
                continue
            # Skip F1 actors
            a_loc = a.get_actor_location()

            # XY must match within 5 UU
            if abs(a_loc.x - door_loc.x) > 5 or abs(a_loc.y - door_loc.y) > 5:
                continue
            # Z must be near expected floor
            if abs(a_loc.z - expected_door_z) > 10:
                continue

            # For StaticMeshActor, verify same mesh
            if door_mesh and door_class_name == 'StaticMeshActor':
                a_smc = a.get_component_by_class(unreal.StaticMeshComponent)
                if a_smc:
                    a_mesh = a_smc.get_editor_property('static_mesh')
                    if a_mesh != door_mesh:
                        continue
                else:
                    continue

            dist = abs(a_loc.z - expected_door_z)
            if dist < best_dist:
                best_dist = dist
                target_door = a

        if not target_door:
            warnings.append(f"No match for {door_actor.get_actor_label()} on F{floor_num}")
            continue

        # Spawn new trigger
        new_loc = unreal.Vector(
            trigger_loc.x,
            trigger_loc.y,
            trigger_loc.z + z_offset
        )
        new_rot = unreal.Rotator(
            roll=trigger_rot.roll,
            pitch=trigger_rot.pitch,
            yaw=trigger_rot.yaw
        )

        new_trigger = eas.spawn_actor_from_class(
            unreal.ZP_InteractDoor,
            new_loc,
            new_rot
        )

        if not new_trigger:
            warnings.append(f"FAILED to spawn trigger for F{floor_num} {target_door.get_actor_label()}")
            continue

        # Copy all properties
        new_trigger.set_editor_property('door_actor', target_door)
        new_trigger.set_editor_property('open_mode', open_mode)
        new_trigger.set_editor_property('open_angle', open_angle)
        new_trigger.set_editor_property('slide_offset', slide_offset)
        new_trigger.set_editor_property('interp_speed', interp_speed)

        # Label: DoorTrigger_F{N}_{DoorLabel}_{Mode}
        label = f"DoorTrigger_F{floor_num}_{target_door.get_actor_label()}_{mode_str}"
        new_trigger.set_actor_label(label)

        created += 1
        print(f"  Created: {label}")

print(f"\n=== DONE: {created} triggers created ===")
if warnings:
    print(f"\n=== WARNINGS ({len(warnings)}) ===")
    for w in warnings:
        print(f"  {w}")
