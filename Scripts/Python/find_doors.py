"""Find all door-related actors in the current level."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

doors = []
for actor in all_actors:
    name = actor.get_actor_label()
    class_name = actor.get_class().get_name()
    if 'door' in name.lower() or 'door' in class_name.lower():
        loc = actor.get_actor_location()
        doors.append((name, class_name, f"({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f})"))

print(f"Found {len(doors)} door-related actors:")
for name, cls, loc in sorted(doors):
    print(f"  {name} [{cls}] at {loc}")
