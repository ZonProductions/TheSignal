import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
duct = {}
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    sm = cs[0].get_editor_property("static_mesh")
    if not sm: continue
    n = sm.get_name().lower()
    if "duct" in n or "vent" in n or "siling" in n or "ceiling" in n:
        org, ext = a.get_actor_bounds(False)
        key = sm.get_name()
        b = int(org.z // 50) * 50
        duct.setdefault(key, {}).setdefault(b, 0)
        duct[key][b] += 1
for k in sorted(duct):
    print(k, "centerZ buckets(50):", dict(sorted(duct[k].items())))
