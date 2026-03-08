"""Set all door StaticMeshComponents to Movable mobility."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

count = 0
for actor in all_actors:
    name = actor.get_actor_label()
    if 'door' in name.lower() and 'Trigger' not in name and 'indoor' not in name.lower():
        comps = actor.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps:
            c.set_mobility(unreal.ComponentMobility.MOVABLE)
            count += 1

print(f"Set {count} mesh components to Movable")
