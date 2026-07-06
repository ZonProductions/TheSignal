import unreal
from collections import defaultdict

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"
actors = actor_sub.get_all_level_actors()
print("Total level actors:", len(actors))

# 1) any InstancedStaticMeshComponent using SM_Floor_3
print("\n=== Instanced components using SM_Floor_3 ===")
found_ism = 0
for a in actors:
    for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
        sm = c.get_editor_property('static_mesh')
        if sm and sm.get_path_name() == ROOF_MESH:
            rs = c.get_editor_property('relative_scale3d')
            ovr = c.get_editor_property('override_materials')
            print("  actor='%s' label='%s' comp='%s' class=%s inst=%d relScale=(%.2f,%.2f,%.2f) overrides=%s" % (
                a.get_name(), a.get_actor_label(), c.get_name(), c.get_class().get_name(),
                c.get_instance_count(), rs.x, rs.y, rs.z,
                [m.get_path_name() if m else None for m in ovr]))
            found_ism += 1
print("  (instanced comps found:", found_ism, ")")

# 2) individual StaticMeshActors with SM_Floor_3, bucketed by Z
print("\n=== Individual SM_Floor_3 StaticMeshActors by Z ===")
zb = defaultdict(int)
for a in actors:
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property('static_mesh')
    if sm and sm.get_path_name() == ROOF_MESH:
        zb[round(a.get_actor_location().z, 1)] += 1
for z in sorted(zb.keys()):
    print("  Z=%.1f count=%d" % (z, zb[z]))
print("  total individual SM_Floor_3:", sum(zb.values()))

# 3) any actor whose label mentions Roof/Consolidated
print("\n=== Actors labeled *Roof*/*Consolidat* ===")
for a in actors:
    lbl = a.get_actor_label()
    if 'roof' in lbl.lower() or 'consolidat' in lbl.lower():
        print("  label='%s' class=%s" % (lbl, a.get_class().get_name()))
