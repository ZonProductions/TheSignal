import unreal, json

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = aus.get_all_level_actors()
ppv = next((a for a in actors if isinstance(a, unreal.PostProcessVolume)), None)
fog = next((a for a in actors if isinstance(a, unreal.ExponentialHeightFog)), None)
print("PPV:", ppv.get_name() if ppv else None, "| FOG:", fog.get_name() if fog else None)

# --- PostProcessVolume volume-level props ---
print("PPV bUnbound:", ppv.get_editor_property("unbound"))
print("PPV Priority:", ppv.get_editor_property("priority"))
print("PPV BlendWeight:", ppv.get_editor_property("blend_weight"))
print("PPV bEnabled:", ppv.get_editor_property("enabled"))

s = ppv.get_editor_property("settings")
# Report which override flags are ON so we know what the volume actually changes
on = []
for p in dir(s):
    if p.startswith("override_"):
        try:
            if s.get_editor_property(p):
                on.append(p)
        except Exception:
            pass
print("PPV OVERRIDES ON (%d):" % len(on))
for o in on:
    print("   ", o)

# A few key 'darkening' values for the report
for f in ["auto_exposure_method","auto_exposure_bias","auto_exposure_min_brightness",
          "auto_exposure_max_brightness","exposure_metering_mode","color_saturation",
          "color_contrast","film_grain_intensity","vignette_intensity",
          "scene_color_tint","white_temp"]:
    try:
        print("   val", f, "=", s.get_editor_property(f))
    except Exception:
        pass

# --- ExponentialHeightFog component props ---
fc = fog.get_editor_property("component")
for f in ["fog_density","fog_height_falloff","fog_inscattering_luminance","fog_max_opacity",
          "start_distance","fog_cutoff_distance","directional_inscattering_exponent",
          "directional_inscattering_color","volumetric_fog","volumetric_fog_scattering_distribution",
          "volumetric_fog_albedo","volumetric_fog_extinction_scale","volumetric_fog_distance"]:
    try:
        print("FOG", f, "=", fc.get_editor_property(f))
    except Exception as e:
        print("FOG", f, "ERR", e)
