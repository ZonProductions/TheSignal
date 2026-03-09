import unreal

# Add a global (unbound) PostProcessVolume that kills Lumen indirect GI bounce.
# This applies in both editor viewport AND PIE — no more swimming pool caustics.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Spawn the PPV
ppv = subsys.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
ppv.set_actor_label('PPV_GlobalDarkness')

# Make it affect the entire level (unbound)
ppv.set_editor_property('unbound', True)

# High priority so it overrides any other PPVs
ppv.set_editor_property('priority', 10.0)

# Kill indirect GI bounce — eliminates swimming pool effect
settings = ppv.settings
settings.set_editor_property('override_indirect_lighting_intensity', True)
settings.set_editor_property('indirect_lighting_intensity', 0.0)

# Also set here: constrain auto-exposure + kill bloom
settings.set_editor_property('override_auto_exposure_bias', True)
settings.set_editor_property('auto_exposure_bias', -0.5)
settings.set_editor_property('override_bloom_intensity', True)
settings.set_editor_property('bloom_intensity', 0.2)

ppv.settings = settings

print("Created PPV_GlobalDarkness (unbound, priority 10)")
print("  IndirectLightingIntensity = 0 (no Lumen GI bounce)")
print("  AutoExposureBias = -0.5")
print("  BloomIntensity = 0.2")

subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsys_level.save_all_dirty_levels()
print("Level saved.")
