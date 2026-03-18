import unreal

for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if "Patrol" in actor.get_name():
        for comp in actor.get_components_by_class(unreal.ActorComponent):
            name = comp.get_name()
            cls = comp.get_class().get_name()
            if "Climbing" in name or "BPC" in name:
                print(f"FOUND: {name} (class={cls})")
                # Try to list Blueprint variables via reflection
                try:
                    enabled = comp.get_editor_property("Enabled")
                    print(f"  Enabled = {enabled}")
                except:
                    print(f"  No 'Enabled' property accessible")
                try:
                    trace_ch = comp.get_editor_property("Trace Channel")
                    print(f"  Trace Channel = {trace_ch}")
                except:
                    pass
                try:
                    trace_ch2 = comp.get_editor_property("TraceChannel")
                    print(f"  TraceChannel = {trace_ch2}")
                except:
                    pass
