import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = next(a for a in aus.get_all_level_actors()
           if a.get_class().get_name() == "Ultra_Dynamic_Sky_C")

def cap():
    for c in uds.get_components_by_class(unreal.SkyLightComponent):
        if c.get_editor_property("source_type") == unreal.SkyLightSourceType.SLS_CAPTURED_SCENE:
            return round(c.get_editor_property("intensity"), 3)
    return None

print("BEFORE | ToD:", uds.get_editor_property("Time of Day"),
      "| skylight:", cap(), "| sunZ:", round(uds.call_method("Sun Height", (False,)), 3),
      "| moonLux:", round(uds.call_method("Unfiltered Moon Light Intensity"), 3))

# Force a transform change -> editor re-runs the construction script (full rebuild)
loc = uds.get_actor_location()
uds.set_editor_property("Time of Day", 200.0)
uds.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + 25.0), False, False)
uds.set_actor_location(loc, False, False)
uds.call_method("Recapture Sky Light")

print("AFTER  | ToD:", uds.get_editor_property("Time of Day"),
      "| skylight:", cap(), "| sunZ:", round(uds.call_method("Sun Height", (False,)), 3),
      "| moonLux:", round(uds.call_method("Unfiltered Moon Light Intensity"), 3))
print("(not saved — checking if reconstruction pushed night values)")
