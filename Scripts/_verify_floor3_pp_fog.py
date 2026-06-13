import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
print("LEVEL:", w.get_name() if w else None)
aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = aus.get_all_level_actors()

ppvs = [a for a in actors if isinstance(a, unreal.PostProcessVolume)]
fogs = [a for a in actors if isinstance(a, unreal.ExponentialHeightFog)]
print("PPV count:", len(ppvs), "labels:", [a.get_actor_label() for a in ppvs])
print("FOG count:", len(fogs), "labels:", [a.get_actor_label() for a in fogs])

new_ppv = next((a for a in ppvs if a.get_actor_label() == "PP_HorrorTone"), None)
new_fog = next((a for a in fogs if a.get_actor_label() == "HeightFog_HorrorTone"), None)

if new_ppv:
    s = new_ppv.get_editor_property("settings")
    print("PPV OK | unbound:", new_ppv.get_editor_property("unbound"),
          "| exposure_bias:", s.get_editor_property("auto_exposure_bias"),
          "| vignette:", s.get_editor_property("vignette_intensity"),
          "| film_grain:", s.get_editor_property("film_grain_intensity"))
else:
    print("PPV MISSING")

if new_fog:
    fc = new_fog.get_editor_property("component")
    print("FOG OK | density:", fc.get_editor_property("fog_density"),
          "| falloff:", fc.get_editor_property("fog_height_falloff"),
          "| inscatter:", fc.get_editor_property("fog_inscattering_luminance"))
else:
    print("FOG MISSING")

dirty = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
print("DIRTY_MAPS:", dirty)
