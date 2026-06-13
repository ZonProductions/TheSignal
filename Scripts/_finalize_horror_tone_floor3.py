import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = aus.get_all_level_actors()

# Guard against duplicate PP_HorrorTone from any re-run
ppts = [a for a in actors if a.get_actor_label() == "PP_HorrorTone"]
print("PP_HorrorTone count:", len(ppts))
if ppts:
    s = ppts[0].get_editor_property("settings")
    print("  exposure_bias:", s.get_editor_property("auto_exposure_bias"),
          "| vignette:", s.get_editor_property("vignette_intensity"),
          "| enabled:", ppts[0].get_editor_property("enabled"),
          "| priority:", ppts[0].get_editor_property("priority"))

night = next((a for a in actors if isinstance(a, unreal.ExponentialHeightFog)
              and a.get_actor_label() == "NightFog"), None)
if night:
    fc = night.get_editor_property("component")
    print("NightFog density:", fc.get_editor_property("fog_density"),
          "| falloff:", fc.get_editor_property("fog_height_falloff"),
          "| inscatter:", fc.get_editor_property("fog_inscattering_luminance"))

saved = les.save_current_level()
print("SAVED:", saved)
print("DIRTY_MAPS:", [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()])
