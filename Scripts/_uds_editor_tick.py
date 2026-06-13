import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = next(a for a in aus.get_all_level_actors()
           if a.get_class().get_name() == "Ultra_Dynamic_Sky_C")

def cap():
    for c in uds.get_components_by_class(unreal.SkyLightComponent):
        if c.get_editor_property("source_type") == unreal.SkyLightSourceType.SLS_CAPTURED_SCENE:
            return round(c.get_editor_property("intensity"), 3)
    return None

cam_loc, cam_rot = ues.get_level_viewport_camera_info()
print("cam:", cam_loc, cam_rot)
print("BEFORE | skylight:", cap(), "| sunZ:", round(uds.call_method("Sun Height", (False,)), 3))

uds.set_editor_property("Time of Day", 200.0)
for fn in ["Start Editor Tick Handler", "Start Active Timers"]:
    try:
        uds.call_method(fn); print("called:", fn)
    except Exception as e:
        print("FAIL", fn, str(e)[:50])

# Drive UDS update directly, several ticks (it converges over frames)
for i in range(30):
    try:
        uds.call_method("Editor Tick", (cam_loc, cam_rot, False))
    except Exception as e:
        print("Editor Tick FAIL:", str(e)[:80]); break

uds.call_method("Recapture Sky Light")
print("AFTER  | skylight:", cap(), "| sunZ:", round(uds.call_method("Sun Height", (False,)), 3))
