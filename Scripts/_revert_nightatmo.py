import unreal
aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atmo = next(a for a in aus.get_all_level_actors() if isinstance(a, unreal.SkyAtmosphere))
ac = atmo.get_component_by_class(unreal.SkyAtmosphereComponent)
ac.set_editor_property("visible", True)
print("NightSkyAtmosphere re-enabled:", ac.get_editor_property("visible"))
