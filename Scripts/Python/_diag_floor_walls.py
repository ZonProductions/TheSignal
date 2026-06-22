import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
print("World:", w.get_name() if w else None)

wall_zs = []
allmesh_zs = []
namecount = {}
n_actors = 0
for a in eas.get_all_level_actors():
    n_actors += 1
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if not sm:
        continue
    mn = sm.get_name()
    z = a.get_actor_location().z
    allmesh_zs.append(z)
    if "wall" in mn.lower() or "brick" in mn.lower():
        wall_zs.append(z)
        namecount[mn] = namecount.get(mn, 0) + 1

def stats(label, zs):
    if not zs:
        print("  %-18s: NONE" % label); return
    zs2 = sorted(zs)
    print("  %-18s: count=%d  Zmin=%.0f  Zmax=%.0f  Zmedian=%.0f" % (label, len(zs2), zs2[0], zs2[-1], zs2[len(zs2)//2]))

print("Total actors:", n_actors)
stats("all static meshes", allmesh_zs)
stats("wall/brick meshes", wall_zs)
print("wall/brick mesh names:", namecount)

ranges = {1:(-50,200),2:(400,700),3:(900,1200),4:(1400,1700),5:(1900,2300)}
for f,(lo,hi) in ranges.items():
    c = sum(1 for z in wall_zs if lo <= z <= hi)
    print("  floor %d  Z[%d..%d]: %d wall/brick actors" % (f, lo, hi, c))
