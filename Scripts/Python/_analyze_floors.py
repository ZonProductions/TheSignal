import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

import re
prefix_z = {}     # label prefix (F1_/F2_/...) -> [z list]
noprefix = 0
zhist = {}
floor_meshes = {} # mesh name -> count, for likely floor/ceiling meshes
total = 0
for a in eas.get_all_level_actors():
    total += 1
    lbl = a.get_actor_label()
    z = a.get_actor_location().z
    m = re.match(r'^(F\d+)[_]', lbl)
    if m:
        prefix_z.setdefault(m.group(1), []).append(z)
    else:
        noprefix += 1
    bucket = int(z // 100) * 100
    zhist[bucket] = zhist.get(bucket, 0) + 1

print("total actors:", total, " no-floor-prefix:", noprefix)
print("\n=== floor-prefix label counts + Z range ===")
for p in sorted(prefix_z):
    zs = sorted(prefix_z[p])
    print("  %-5s count=%-4d Zmin=%.0f Zmax=%.0f Zmed=%.0f" % (p, len(zs), zs[0], zs[-1], zs[len(zs)//2]))

print("\n=== Z histogram (100UU buckets) ===")
for b in sorted(zhist):
    print("  z[%5d..%5d]: %d" % (b, b+100, zhist[b]))
