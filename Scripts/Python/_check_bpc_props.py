import unreal

subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in subsystem.get_all_level_actors():
    if "Patrol" in actor.get_name():
        for comp in actor.get_components_by_class(unreal.ActorComponent):
            if "BPC_Climbing" in comp.get_name():
                print(f"Found: {comp.get_name()} ({comp.get_class().get_name()})")
                # Try common property names
                for prop_name in ["Enabled", "enabled", "bEnabled", "Is Enabled",
                                  "Trace Channel", "TraceChannel", "Trace_Channel",
                                  "Ground Trace Size", "GroundTraceSize",
                                  "ReattachmentDelay", "Jump Strength",
                                  "Capsule Component", "CapsuleComponent"]:
                    try:
                        val = comp.get_editor_property(prop_name)
                        print(f"  {prop_name} = {val}")
                    except:
                        pass
