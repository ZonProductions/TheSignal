import unreal

subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in subsystem.get_all_level_actors():
    name = actor.get_name()
    if "Patrol" in name or "Crawler" in name or "Monster" in name:
        print(f"=== {name} (class={actor.get_class().get_name()}) ===")
        for comp in actor.get_components_by_class(unreal.ActorComponent):
            print(f"  {comp.get_name()} — {comp.get_class().get_name()}")
        break
