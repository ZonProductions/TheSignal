import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = aus.get_all_level_actors()
uds = next(a for a in actors if a.get_class().get_name() == "Ultra_Dynamic_Sky_C")
atmo = next((a for a in actors if isinstance(a, unreal.SkyAtmosphere)), None)
print("Atmo actor:", atmo.get_actor_label() if atmo else None)

def sky_lum():
    try:
        return uds.call_method("Current Sky Atmosphere Luminance")
    except Exception as e:
        return "err:" + str(e)[:40]

print("BEFORE | atmo_luminance:", sky_lum())

# Disable the SkyAtmosphere COMPONENT (affects editor + game), not just editor-hide
ac = atmo.get_component_by_class(unreal.SkyAtmosphereComponent)
print("atmo component visible before:", ac.get_editor_property("visible"))
ac.set_editor_property("visible", False)
uds.call_method("Recapture Sky Light")
print("atmo component visible after:", ac.get_editor_property("visible"))
print("AFTER  | atmo_luminance:", sky_lum())
print("NOT SAVED — look at the viewport. Windows darker?")
