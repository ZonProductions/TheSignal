import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
uds = next(a for a in aus.get_all_level_actors()
           if a.get_class().get_name() == "Ultra_Dynamic_Sky_C")

def comp_skylight():
    for c in uds.get_components_by_class(unreal.SkyLightComponent):
        if c.get_editor_property("source_type") == unreal.SkyLightSourceType.SLS_CAPTURED_SCENE:
            return round(c.get_editor_property("intensity"), 3)
    return None

print("BEFORE component skylight:", comp_skylight(), "| NightBrightness:",
      uds.get_editor_property("Night Brightness"))

uds.set_editor_property("Time of Day", 200.0)
uds.set_editor_property("Night Brightness", 0.1)

# Try to force UDS to rebuild its cache and push to components
for fn in ["Hard Reset Cache", "Update Active Variables", "Update Common Derivatives",
           "Update High Priority Properties", "Update Low Priority Properties",
           "Update Dynamic Sky Light Multiplier", "Recapture Sky Light"]:
    try:
        uds.call_method(fn)
    except Exception as e:
        print("FAIL", fn, str(e)[:50])

les.editor_set_viewport_realtime(True)
les.editor_invalidate_viewports()
print("AFTER  component skylight:", comp_skylight(), "| NightBrightness:",
      uds.get_editor_property("Night Brightness"))
print("(not saved)")
