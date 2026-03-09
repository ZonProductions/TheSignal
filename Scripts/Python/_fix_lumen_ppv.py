"""Fix Lumen splotches by overriding GI method to None via the existing PostProcessVolume."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

for a in eas.get_all_level_actors():
    if a.get_class().get_name() == 'PostProcessVolume':
        settings = a.get_editor_property('settings')

        # Override GI method to None (0)
        settings.set_editor_property('override_dynamic_global_illumination_method', True)
        settings.set_editor_property('dynamic_global_illumination_method', unreal.DynamicGlobalIlluminationMethod.NONE)

        # Override reflection method to Screen Space (not Lumen)
        settings.set_editor_property('override_reflection_method', True)
        settings.set_editor_property('reflection_method', unreal.ReflectionMethod.SCREEN_SPACE)

        # Apply back
        a.set_editor_property('settings', settings)

        print(f"PostProcessVolume '{a.get_actor_label()}': GI=None, Reflections=ScreenSpace")
        print(f"  Unbound: {a.get_editor_property('unbound')}")

        # Verify
        s2 = a.get_editor_property('settings')
        print(f"  Verify GI override: {s2.get_editor_property('override_dynamic_global_illumination_method')}")
        print(f"  Verify GI method: {s2.get_editor_property('dynamic_global_illumination_method')}")
        print(f"  Verify Refl override: {s2.get_editor_property('override_reflection_method')}")
        print(f"  Verify Refl method: {s2.get_editor_property('reflection_method')}")
        break
