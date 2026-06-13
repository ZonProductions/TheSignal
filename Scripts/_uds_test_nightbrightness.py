import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = next(a for a in aus.get_all_level_actors()
           if a.get_class().get_name() == "Ultra_Dynamic_Sky_C")

uds.set_editor_property("Time of Day", 200.0)
orig = uds.get_editor_property("Night Brightness")
print("Night Brightness original:", orig)
print("--- computed skylight intensity vs Night Brightness (at 2 AM) ---")
for nb in [1.0, 0.5, 0.25, 0.1, 0.05]:
    uds.set_editor_property("Night Brightness", float(nb))
    out = uds.call_method("Get Current Sky Light Color and Intensity")
    si = out[0] if isinstance(out, (list, tuple)) else out
    moon = uds.call_method("Unfiltered Moon Light Intensity")
    print("  NightBrightness %4.2f -> skylight %s | moonLux %s" % (nb, si, moon))

uds.set_editor_property("Night Brightness", orig)  # restore until dev picks
print("restored Night Brightness to", uds.get_editor_property("Night Brightness"))
