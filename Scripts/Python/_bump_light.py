import unreal

# Bump overall brightness by ~10% via auto-exposure bias on PPV_GlobalDarkness.
# Lumen GI stays OFF (no swimming pool). This just brightens the camera exposure.

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

    # Subtle indirect fill — 0.1 was way too much (daylight), try 0.02
    settings.set_editor_property('override_indirect_lighting_intensity', True)
    settings.set_editor_property('indirect_lighting_intensity', 0.0025)

    ppv.settings = settings

    print("PPV_GlobalDarkness brightness adjusted:")
    print("  AutoExposureBias = -0.3 (was -0.5, small nudge)")
    print("  IndirectLightingIntensity = 0.0")

    subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsys_level.save_all_dirty_levels()
    print("Level saved.")
