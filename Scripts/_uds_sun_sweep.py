import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = next(a for a in aus.get_all_level_actors()
           if a.get_class().get_name() == "Ultra_Dynamic_Sky_C")

print("Sweeping Time of Day -> live Sun Height (uncached):")
for tod in [0, 200, 400, 600, 900, 1200, 1500, 1800, 2100, 2300]:
    uds.set_editor_property("Time of Day", float(tod))
    z = uds.call_method("Sun Height", (False,))
    day = uds.call_method("Is it Daytime?")
    print("  ToD %4d -> sunZ %7.3f  daytime=%s" % (tod, z, day))

# Location / sun-position config that determines where the sun is at a given time
for p in ["Use Sun Position Calculation Plugin", "Latitude", "Longitude",
          "Time Zone", "North Yaw", "Day", "Month", "Year",
          "Solar Time vs Local Time"]:
    try:
        print("CFG", p, "=", uds.get_editor_property(p))
    except Exception:
        pass

uds.set_editor_property("Time of Day", 200.0)  # leave at 2 AM
