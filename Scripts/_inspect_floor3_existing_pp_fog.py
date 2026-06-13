import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = aus.get_all_level_actors()

print("=== PostProcessVolumes ===")
for a in actors:
    if isinstance(a, unreal.PostProcessVolume):
        s = a.get_editor_property("settings")
        ov = [p for p in dir(s) if p.startswith("override_") and s.get_editor_property(p)]
        print(" %s | hidden:%s unbound:%s enabled:%s priority:%s blend:%s | #overrides:%d"
              % (a.get_actor_label(),
                 a.is_temporarily_hidden_in_editor(),
                 a.get_editor_property("unbound"),
                 a.get_editor_property("enabled"),
                 a.get_editor_property("priority"),
                 a.get_editor_property("blend_weight"),
                 len(ov)))

print("=== Fog actors (ExponentialHeightFog) ===")
for a in actors:
    if isinstance(a, unreal.ExponentialHeightFog):
        fc = a.get_editor_property("component")
        print(" %s | hidden:%s density:%s falloff:%s inscatter:%s"
              % (a.get_actor_label(),
                 a.is_temporarily_hidden_in_editor(),
                 fc.get_editor_property("fog_density"),
                 fc.get_editor_property("fog_height_falloff"),
                 fc.get_editor_property("fog_inscattering_luminance")))

# Any Ultra Dynamic Sky / Weather actor that owns its own fog?
print("=== Sky/Weather actors ===")
for a in actors:
    cn = a.get_class().get_name()
    if "UltraDynamic" in cn or "Sky" in cn or "Weather" in cn:
        print(" %s | class:%s hidden:%s" % (a.get_actor_label(), cn, a.is_temporarily_hidden_in_editor()))
