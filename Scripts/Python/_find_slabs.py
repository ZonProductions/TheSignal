import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# horizontal slabs = large in X&Y, thin in Z -> floor/ceiling plates. Cluster their center Z.
slabz = {}
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs:
        continue
    sm = cs[0].get_editor_property("static_mesh")
    if not sm:
        continue
    org, ext = a.get_actor_bounds(False)
    if ext.x > 300 and ext.y > 300 and ext.z < 60:   # broad, flat
        b = int(round(org.z / 10.0) * 10)
        slabz.setdefault(b, []).append(sm.get_name())

print("=== horizontal slab center-Z clusters (floor/ceiling plates) ===")
for b in sorted(slabz):
    names = slabz[b]
    from collections import Counter
    top = Counter(names).most_common(3)
    print("  z=%5d  count=%-3d  %s" % (b, len(names), top))
