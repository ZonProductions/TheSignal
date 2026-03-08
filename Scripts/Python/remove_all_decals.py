"""Remove all DecalActors from the current level."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

decals = [a for a in all_actors if a.get_class().get_name() == 'DecalActor']
print(f"Found {len(decals)} DecalActors to remove.")

removed = 0
for actor in decals:
    name = actor.get_actor_label()
    eas.destroy_actor(actor)
    removed += 1
    print(f"  Removed: {name}")

print(f"\n=== Removed {removed} DecalActors ===")
