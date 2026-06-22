import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 1) Actors whose LABEL hints at brick/wall -> what mesh do they use?
print("=== actors with 'brick' or 'wall' in their LABEL (label | mesh | z) ===")
seen_meshes = {}
shown = 0
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    if "brick" not in lbl.lower() and "wall" not in lbl.lower():
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    sm = comps[0].get_editor_property("static_mesh") if comps else None
    mname = sm.get_name() if sm else "(no mesh)"
    seen_meshes[mname] = seen_meshes.get(mname, 0) + 1
    if shown < 25:
        print("  %-32s | %-24s | z=%.0f" % (lbl, mname, a.get_actor_location().z))
        shown += 1
print("mesh-name tally for brick/wall-labelled actors:", seen_meshes)

# 2) Most common meshes among actors at floor-3 Z (900..1200) — reveals the interior geometry
print("\n=== top meshes at floor-3 Z [900..1200] ===")
tally = {}
for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    z = a.get_actor_location().z
    if not (900 <= z <= 1200):
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    sm = comps[0].get_editor_property("static_mesh") if comps else None
    if not sm:
        continue
    mn = sm.get_name()
    tally[mn] = tally.get(mn, 0) + 1
for mn, c in sorted(tally.items(), key=lambda kv: -kv[1])[:25]:
    print("  %4d  %s" % (c, mn))
