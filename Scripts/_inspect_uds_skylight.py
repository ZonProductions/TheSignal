import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = aus.get_all_level_actors()

uds = next((a for a in actors if a.get_class().get_name() == "Ultra_Dynamic_Sky_C"), None)
print("UDS:", uds.get_actor_label() if uds else None)

# SkyLight component state
for a in actors:
    for c in a.get_components_by_class(unreal.SkyLightComponent):
        print("SKYLIGHT on %s | intensity:%s realtime:%s mobility:%s source_type:%s"
              % (a.get_actor_label(), c.get_editor_property("intensity"),
                 c.get_editor_property("real_time_capture"),
                 c.get_editor_property("mobility"),
                 c.get_editor_property("source_type")))

# SkyAtmosphere actors (conflict check)
for a in actors:
    if isinstance(a, unreal.SkyAtmosphere):
        print("SKYATMOSPHERE:", a.get_actor_label(), "| hidden:", a.is_temporarily_hidden_in_editor())

# UDS callable BP functions (look for update / recapture)
if uds:
    fns = []
    for f in dir(uds):
        lf = f.lower()
        if any(k in lf for k in ("update", "recaptur", "refresh", "sky", "light", "capture", "time")):
            fns.append(f)
    print("UDS candidate functions:", fns)
    # UDS properties of interest
    for p in ["Time of Day", "Use Custom Skylight Recapture Settings",
              "Capture Sky every frame in editor", "Realtime Capture",
              "Sync Background Color With Fog", "Use Volumetric Clouds"]:
        try:
            print("  UDS prop", p, "=", uds.get_editor_property(p))
        except Exception:
            pass
