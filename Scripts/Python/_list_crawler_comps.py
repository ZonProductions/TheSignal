import unreal

for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if "Patrol" in actor.get_name():
        print(f"=== {actor.get_name()} (class={actor.get_class().get_name()}) ===")
        for comp in actor.get_components_by_class(unreal.ActorComponent):
            print(f"  {comp.get_name()} — {comp.get_class().get_name()}")
