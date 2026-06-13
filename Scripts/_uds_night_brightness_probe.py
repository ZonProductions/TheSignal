import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = next(a for a in aus.get_all_level_actors()
           if a.get_class().get_name() == "Ultra_Dynamic_Sky_C")

# UDS night-related properties
for p in ["Night Brightness", "Overall Intensity", "Moon Light Intensity",
          "Sky Light Intensity", "Use Sun Position Calculation Plugin"]:
    try:
        print("PROP", p, "=", uds.get_editor_property(p))
    except Exception:
        pass

print("--- brightness vs sun depth (live computed) ---")
for tod in [0, 100, 200, 300, 600]:
    uds.set_editor_property("Time of Day", float(tod))
    z = uds.call_method("Sun Height", (False,))
    try:
        nb = uds.call_method("Combined Night Brightness")
    except Exception as e:
        nb = "err:" + str(e)[:30]
    try:
        out = uds.call_method("Get Current Sky Light Color and Intensity")
        # returns [Intensity, Color]
        si = out[0] if isinstance(out, (list, tuple)) else out
    except Exception as e:
        si = "err:" + str(e)[:30]
    print("  ToD %4d sunZ %6.3f | CombinedNightBrightness %s | SkyLightIntensity %s"
          % (tod, z, nb, si))

uds.set_editor_property("Time of Day", 200.0)
