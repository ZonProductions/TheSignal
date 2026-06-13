import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = aus.get_all_level_actors()

def comp_of(actor, cls):
    for c in actor.get_components_by_class(cls):
        return c
    return None

print("=== Lights ===")
for a in actors:
    sl = comp_of(a, unreal.SkyLightComponent)
    if sl:
        print(" SKYLIGHT actor:%s | intensity:%s mobility:%s realtime:%s hidden:%s"
              % (a.get_actor_label(), sl.get_editor_property("intensity"),
                 sl.get_editor_property("mobility"),
                 sl.get_editor_property("real_time_capture"),
                 a.is_temporarily_hidden_in_editor()))
    dl = comp_of(a, unreal.DirectionalLightComponent)
    if dl:
        print(" DIRECTIONAL actor:%s | intensity:%s mobility:%s hidden:%s"
              % (a.get_actor_label(), dl.get_editor_property("intensity"),
                 dl.get_editor_property("mobility"), a.is_temporarily_hidden_in_editor()))

print("=== PPV_GlobalDarkness overrides ===")
for a in actors:
    if isinstance(a, unreal.PostProcessVolume) and a.get_actor_label() == "PPV_GlobalDarkness":
        s = a.get_editor_property("settings")
        for p in dir(s):
            if p.startswith("override_") and s.get_editor_property(p):
                val = p[len("override_"):]
                try:
                    print("  ", val, "=", s.get_editor_property(val))
                except Exception:
                    print("  ", val, "(on)")

print("=== UDS brightness-ish props ===")
for a in actors:
    if a.get_class().get_name() == "Ultra_Dynamic_Sky_C":
        for p in ["Time of Day", "Overall Intensity", "Cloud Lighting", "Skylight Intensity",
                  "Sky Brightness", "Night Brightness", "Moon Light Intensity",
                  "Directional Light Intensity"]:
            try:
                print("  UDS", p, "=", a.get_editor_property(p))
            except Exception:
                pass
