import unreal

# Find PPV_GlobalDarkness and disable Lumen GI entirely.
# The "swimming pool" effect IS Lumen's temporal probe recalculation.
# Turning off GI method = direct light only = no swimming.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

ppv = None
for a in all_actors:
    if a.get_actor_label() == 'PPV_GlobalDarkness':
        ppv = a
        break

if not ppv:
    print("PPV_GlobalDarkness not found!")
else:
    settings = ppv.settings

    # Disable Lumen GI — switch to None
    settings.set_editor_property('override_dynamic_global_illumination_method', True)
    settings.set_editor_property('dynamic_global_illumination_method', unreal.DynamicGlobalIlluminationMethod.NONE)

    # Disable Lumen Reflections — switch to Screen Space
    settings.set_editor_property('override_reflection_method', True)
    settings.set_editor_property('reflection_method', unreal.ReflectionMethod.SCREEN_SPACE)

    # Indirect lighting zero (belt and suspenders)
    settings.set_editor_property('override_indirect_lighting_intensity', True)
    settings.set_editor_property('indirect_lighting_intensity', 0.0)

    ppv.settings = settings

    print("PPV_GlobalDarkness updated:")
    print("  DynamicGlobalIlluminationType = None (Lumen GI OFF)")
    print("  ReflectionMethod = Screen Space (Lumen Reflections OFF)")
    print("  IndirectLightingIntensity = 0")

    subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsys_level.save_all_dirty_levels()
    print("Level saved.")
