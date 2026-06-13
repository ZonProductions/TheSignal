import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = aus.get_all_level_actors()

gd = next((a for a in actors if isinstance(a, unreal.PostProcessVolume)
           and a.get_actor_label() == "PPV_GlobalDarkness"), None)
if gd:
    s = gd.get_editor_property("settings")
    print("=== PPV_GlobalDarkness ===")
    for f in ["override_dynamic_global_illumination_method", "dynamic_global_illumination_method",
              "override_reflection_method", "reflection_method",
              "override_indirect_lighting_intensity", "indirect_lighting_intensity",
              "override_lumen_final_gather_quality", "lumen_final_gather_quality",
              "override_lumen_scene_lighting_quality", "lumen_scene_lighting_quality",
              "override_lumen_reflection_quality", "lumen_reflection_quality"]:
        try:
            print("  %s = %s" % (f, s.get_editor_property(f)))
        except Exception as e:
            print("  %s ERR %s" % (f, str(e)[:40]))

# Lumen-relevant cvars (swimming/instability)
def getf(cv):
    try:
        return unreal.SystemLibrary.get_console_variable_float_value(cv)
    except Exception:
        return "n/a"
print("=== cvars ===")
for cv in ["r.Lumen.ScreenProbeGather.Temporal", "r.Lumen.Reflections.Temporal",
           "r.Lumen.ScreenProbeGather.TwoSidedFoliageBackfaceDiffuse",
           "r.Lumen.Reflections.SmoothBias", "r.SSR.Quality",
           "r.Lumen.DiffuseIndirect.Allow", "r.Lumen.ScreenProbeGather.Quality"]:
    print("  %s = %s" % (cv, getf(cv)))

# Flickering lights that can destabilize Lumen -> swimming
hl = [a for a in actors if "HorrorLight" in a.get_class().get_name()]
print("=== BP_HorrorLight count (flicker source):", len(hl))
