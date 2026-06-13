"""Apply Lvl_Horror look to Building1_3rdFloor (dev decisions):
  PPV  -> NEW layered volume PP_HorrorTone (priority 0, all 13 overrides).
  Fog  -> RETUNE existing NightFog: density/falloff/volumetric from Lvl_Horror,
          but KEEP NightFog's dark-blue inscatter colour.
Reads source directly from Lvl_Horror external-actor packages (no level switch)."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

w = ues.get_editor_world()
assert w and w.get_name() == "Building1_3rdFloor", "Wrong level: %s" % (w.get_name() if w else None)

# --- Load source actors from Lvl_Horror external-actor packages ---
ar = unreal.AssetRegistryHelpers.get_asset_registry()
assets = ar.get_assets_by_path("/Game/__ExternalActors__/Variant_Horror/Lvl_Horror",
                               recursive=True, include_only_on_disk_assets=False)
src_ppv = src_fog = None
for ad in assets:
    cls = str(ad.asset_class_path.asset_name)
    if cls == "PostProcessVolume":
        src_ppv = ad.get_asset()
    elif cls == "ExponentialHeightFog":
        src_fog = ad.get_asset()
assert src_ppv and src_fog, "Source actors not found"

# --- PPV: new layered volume ---
new_ppv = aus.spawn_actor_from_class(unreal.PostProcessVolume,
                                     unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
new_ppv.set_actor_label("PP_HorrorTone")
new_ppv.set_editor_property("settings", src_ppv.get_editor_property("settings"))
new_ppv.set_editor_property("unbound", True)
new_ppv.set_editor_property("priority", 0.0)
new_ppv.set_editor_property("blend_weight", 1.0)
new_ppv.set_editor_property("enabled", True)
print("Created PP_HorrorTone (priority 0, unbound). Overrides copied from source.")

# --- Fog: retune NightFog, keep its inscatter colour ---
night = next((a for a in aus.get_all_level_actors()
              if isinstance(a, unreal.ExponentialHeightFog) and a.get_actor_label() == "NightFog"), None)
assert night, "NightFog not found"
sfc = src_fog.get_editor_property("component")
nfc = night.get_editor_property("component")
# Structural + volumetric only (NOT inscatter colours -> keep NightFog blue)
copy_props = ["fog_density", "fog_height_falloff", "fog_max_opacity", "start_distance",
              "fog_cutoff_distance", "enable_volumetric_fog",
              "volumetric_fog_scattering_distribution", "volumetric_fog_extinction_scale",
              "volumetric_fog_distance"]
for p in copy_props:
    nfc.set_editor_property(p, sfc.get_editor_property(p))
night.modify()
nfc.mark_render_state_dirty()
print("Retuned NightFog density=%s falloff=%s (inscatter kept: %s)" % (
    nfc.get_editor_property("fog_density"),
    nfc.get_editor_property("fog_height_falloff"),
    nfc.get_editor_property("fog_inscattering_luminance")))

# --- Save ---
saved = les.save_current_level()
print("SAVED:", saved)
