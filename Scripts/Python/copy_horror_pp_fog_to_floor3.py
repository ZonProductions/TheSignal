"""Copy the PostProcessVolume + ExponentialHeightFog from Lvl_Horror into
Building1_3rdFloor, preserving every setting. Run via MCP Python endpoint.

Flow: capture (on Lvl_Horror) -> discard pending change -> switch level ->
spawn + apply -> save. Dev chose to DISCARD the unsaved Lvl_Horror change."""
import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

world = unreal.EditorLevelLibrary.get_editor_world()
assert world.get_name() == "Lvl_Horror", "Expected Lvl_Horror open, got %s" % world.get_name()

actors = aus.get_all_level_actors()
ppv = next(a for a in actors if isinstance(a, unreal.PostProcessVolume))
fog = next(a for a in actors if isinstance(a, unreal.ExponentialHeightFog))

# --- Capture PPV (whole settings struct = all 13 overrides, exact) ---
ppv_settings = ppv.get_editor_property("settings")   # value copy, survives switch
ppv_props = {
    "unbound": ppv.get_editor_property("unbound"),
    "priority": ppv.get_editor_property("priority"),
    "blend_weight": ppv.get_editor_property("blend_weight"),
    "enabled": ppv.get_editor_property("enabled"),
}

# --- Capture fog component (copy whatever property names resolve) ---
fc = fog.get_editor_property("component")
fog_loc = fog.get_actor_location()
fog_rot = fog.get_actor_rotation()
fog_candidates = [
    "fog_density", "fog_height_falloff", "fog_inscattering_luminance",
    "fog_max_opacity", "start_distance", "fog_cutoff_distance",
    "directional_inscattering_exponent", "directional_inscattering_start_distance",
    "directional_inscattering_luminance", "directional_inscattering_color",
    "volumetric_fog", "enable_volumetric_fog",
    "volumetric_fog_scattering_distribution", "volumetric_fog_albedo",
    "volumetric_fog_extinction_scale", "volumetric_fog_distance",
    "volumetric_fog_static_lighting_scattering_intensity",
    "volumetric_fog_emissive", "second_fog_data",
]
fog_vals = {}
for name in fog_candidates:
    try:
        fog_vals[name] = fc.get_editor_property(name)
    except Exception:
        pass
print("CAPTURED fog props:", sorted(fog_vals.keys()))

# --- Discard pending Lvl_Horror change, then switch level ---
# Dev chose DISCARD. load_level() switches without saving (drops unsaved edits).
print("Switching to Building1_3rdFloor (discarding unsaved Lvl_Horror edit)...")
les.load_level("/Game/Building1_3rdFloor")

world2 = unreal.EditorLevelLibrary.get_editor_world()
assert world2.get_name() == "Building1_3rdFloor", "Switch failed, on %s" % world2.get_name()

# --- Spawn + apply ---
new_ppv = aus.spawn_actor_from_class(unreal.PostProcessVolume,
                                     unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
new_ppv.set_actor_label("PP_HorrorTone")
new_ppv.set_editor_property("settings", ppv_settings)
for k, v in ppv_props.items():
    new_ppv.set_editor_property(k, v)

new_fog = aus.spawn_actor_from_class(unreal.ExponentialHeightFog, fog_loc, fog_rot)
new_fog.set_actor_label("HeightFog_HorrorTone")
nfc = new_fog.get_editor_property("component")
applied = []
for k, v in fog_vals.items():
    try:
        nfc.set_editor_property(k, v)
        applied.append(k)
    except Exception as e:
        print("FOG set fail", k, e)
print("APPLIED fog props:", sorted(applied))

# --- Save ---
saved = les.save_current_level()
print("PPV:", new_ppv.get_actor_label(), "| FOG:", new_fog.get_actor_label(),
      "| SAVED:", saved)
