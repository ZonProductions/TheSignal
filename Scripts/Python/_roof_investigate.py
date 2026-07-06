import unreal
from collections import defaultdict

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_sub.get_all_level_actors()

TARGET_Z = 3380.434814

# First, histogram Z values near the target to understand the spread
zbuckets = defaultdict(int)
for a in actors:
    loc = a.get_actor_location()
    if abs(loc.z - TARGET_Z) < 200.0:
        zbuckets[round(loc.z, 1)] += 1

print("=== Z histogram within +/-200 of target ===")
for z in sorted(zbuckets.keys()):
    print("Z=%.3f  count=%d" % (z, zbuckets[z]))

# Now pick actors with matching Z (loose tol) and group by mesh + class
TOL = 50.0
roof = []
for a in actors:
    loc = a.get_actor_location()
    if abs(loc.z - TARGET_Z) < TOL:
        roof.append(a)

print("\n=== Total actors within %.0f of Z=%.3f: %d ===" % (TOL, TARGET_Z, len(roof)))

meshcount = defaultdict(int)
classcount = defaultdict(int)
sample_names = []
for a in roof:
    classcount[a.get_class().get_name()] += 1
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        sm = smc.get_editor_property('static_mesh')
        meshcount[sm.get_path_name() if sm else 'None'] += 1
    else:
        meshcount['<no StaticMeshComponent>'] += 1
    if len(sample_names) < 8:
        sample_names.append(a.get_actor_label())

print("\n=== Class breakdown ===")
for c, n in sorted(classcount.items(), key=lambda x: -x[1]):
    print("%5d  %s" % (n, c))

print("\n=== Static mesh breakdown ===")
for m, n in sorted(meshcount.items(), key=lambda x: -x[1]):
    print("%5d  %s" % (n, m))

print("\n=== Sample actor labels ===")
for s in sample_names:
    print("  ", s)
