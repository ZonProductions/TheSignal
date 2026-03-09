"""Check for PostProcessVolumes and their GI/reflection settings."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

for a in eas.get_all_level_actors():
    cls = a.get_class().get_name()
    if 'PostProcess' in cls:
        label = a.get_actor_label()
        loc = a.get_actor_location()
        print(f"PPV: {label} ({cls}) at Z={loc.z:.0f}")
        # Check if it has settings override
        ppc = a.get_component_by_class(unreal.PostProcessComponent)
        if ppc:
            settings = ppc.get_editor_property('settings')
            print(f"  Override GI: {settings.get_editor_property('override_dynamic_global_illumination_method')}")
            print(f"  Override Reflection: {settings.get_editor_property('override_reflection_method')}")
