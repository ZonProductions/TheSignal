"""Fix 'windowed rooms look like daylight': set UDS Time of Day to 2 AM and
re-run its construction script so the sun/sky/skylight recompute for night
(setting the property alone does NOT trigger UDS's rebuild). Verify via
Is it Daytime? / Sun Height / captured skylight intensity, then save."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

w = ues.get_editor_world()
assert w and w.get_name() == "Building1_3rdFloor", "Wrong level: %s" % (w.get_name() if w else None)

uds = next((a for a in aus.get_all_level_actors()
            if a.get_class().get_name() == "Ultra_Dynamic_Sky_C"), None)
assert uds, "UDS not found"

def captured_intensity():
    for c in uds.get_components_by_class(unreal.SkyLightComponent):
        if c.get_editor_property("source_type") == unreal.SkyLightSourceType.SLS_CAPTURED_SCENE:
            return c.get_editor_property("intensity")
    return None

print("BEFORE: ToD=%s daytime=%s sunZ=%s skylight=%s" % (
    uds.get_editor_property("Time of Day"),
    uds.call_method("Is it Daytime?"),
    uds.call_method("Sun Height", (False,)),
    captured_intensity()))

uds.set_editor_property("Time of Day", 200.0)        # 2:00 AM
uds.call_method("UserConstructionScript")            # full UDS rebuild for new time
uds.call_method("Recapture Sky Light")               # refresh ambient from rebuilt sky

print("AFTER:  ToD=%s daytime=%s sunZ=%s skylight=%s" % (
    uds.get_editor_property("Time of Day"),
    uds.call_method("Is it Daytime?"),
    uds.call_method("Sun Height", (False,)),
    captured_intensity()))

saved = les.save_current_level()
print("SAVED:", saved)
