"""Find all door-related actors in the current level."""
import unreal

eas = unreal.EditorActorSubsystem()
all_actors = eas.get_all_level_actors()

doors = [a for a in all_actors if "door" in a.get_actor_label().lower() or "Door" in a.get_class().get_name()]

print(f"=== Door actors ({len(doors)} total) ===")
for d in sorted(doors, key=lambda a: a.get_actor_label()):
    loc = d.get_actor_location()
    cls = d.get_class().get_name()
    label = d.get_actor_label()
    print(f"  {cls:30s} | {label:45s} | ({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f})")

# Specifically check for InteractDoor triggers
interact_doors = [a for a in all_actors if "InteractDoor" in a.get_class().get_name()]
print(f"\n=== ZP_InteractDoor triggers ({len(interact_doors)}) ===")
for d in interact_doors:
    label = d.get_actor_label()
    loc = d.get_actor_location()
    # Try to get DoorActor reference
    try:
        door_ref = d.get_editor_property("door_actor")
        ref_label = door_ref.get_actor_label() if door_ref else "NONE"
    except:
        ref_label = "??"
    print(f"  {label:40s} | DoorActor={ref_label:30s} | ({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f})")
