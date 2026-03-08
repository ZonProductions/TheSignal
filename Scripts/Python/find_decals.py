"""Find all decal actors in the current level."""
import unreal

eas = unreal.EditorActorSubsystem()
all_actors = eas.get_all_level_actors()

decals = []
for actor in all_actors:
    name = actor.get_actor_label()
    class_name = actor.get_class().get_name()
    if 'decal' in class_name.lower() or 'decal' in name.lower():
        loc = actor.get_actor_location()
        decals.append((name, class_name, loc))

print(f"Found {len(decals)} decal actors:")
for name, cls, loc in decals:
    print(f"  {name} ({cls}) at ({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f})")
