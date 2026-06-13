import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = next(a for a in aus.get_all_level_actors()
           if a.get_class().get_name() == "Ultra_Dynamic_Sky_C")

def report(tag):
    sky = None
    dls = []
    for c in uds.get_components_by_class(unreal.SkyLightComponent):
        if c.get_editor_property("source_type") == unreal.SkyLightSourceType.SLS_CAPTURED_SCENE:
            sky = c.get_editor_property("intensity")
    for c in uds.get_components_by_class(unreal.DirectionalLightComponent):
        dls.append((c.get_name(), round(c.get_editor_property("intensity"), 3),
                    str(c.get_editor_property("visible")),
                    str(c.get_world_rotation())))
    print(tag, "| skylight:", sky, "| sunZ:", uds.call_method("Sun Height", (False,)),
          "| sunLux:", uds.call_method("Current Sun Light Intensity"))
    for d in dls:
        print("    DIR", d)

report("BEFORE")
# UDS's canonical instant editor refresh
for fn in ["Hard Reset Cache", "Editor Instant Update Active Properties",
           "Update Directional Light Rotations", "Update Dynamic Sky Light Multiplier",
           "Recapture Sky Light"]:
    try:
        uds.call_method(fn)
        print("called:", fn)
    except Exception as e:
        print("FAIL:", fn, str(e)[:60])
report("AFTER")
