import unreal
from collections import defaultdict

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_sub.get_all_level_actors()

ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"
TARGET_Z = 3380.434814

tiles = []
for a in actors:
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property('static_mesh')
    if sm and sm.get_path_name() == ROOF_MESH:
        tiles.append((a, smc))

print("Total SM_Floor_3 StaticMeshActors:", len(tiles))

# Z distribution
zb = defaultdict(int)
for a, smc in tiles:
    zb[round(a.get_actor_location().z, 2)] += 1
print("\n=== Z distribution of SM_Floor_3 ===")
for z in sorted(zb.keys()):
    print("Z=%.2f  count=%d" % (z, zb[z]))

# Rotation / scale variance among tiles at target Z
TOL = 5.0
at_z = [(a, smc) for (a, smc) in tiles if abs(a.get_actor_location().z - TARGET_Z) < TOL]
print("\n=== SM_Floor_3 within %.1f of target Z: %d ===" % (TOL, len(at_z)))

rotset = defaultdict(int)
scaleset = defaultdict(int)
matvariants = defaultdict(int)
attached = 0
parented = 0
for a, smc in at_z:
    r = a.get_actor_rotation()
    s = a.get_actor_scale3d()
    rotset[(round(r.roll,1), round(r.pitch,1), round(r.yaw,1))] += 1
    scaleset[(round(s.x,3), round(s.y,3), round(s.z,3))] += 1
    # material overrides on the component
    mats = smc.get_editor_property('override_materials')
    key = tuple((m.get_path_name() if m else 'None') for m in mats)
    matvariants[key] += 1
    if a.get_attach_parent_actor():
        parented += 1

print("\n=== Rotation variants ===")
for k, n in sorted(rotset.items(), key=lambda x:-x[1]):
    print("%5d  roll=%.1f pitch=%.1f yaw=%.1f" % (n, k[0], k[1], k[2]))

print("\n=== Scale variants ===")
for k, n in sorted(scaleset.items(), key=lambda x:-x[1]):
    print("%5d  scale=%s" % (n, k))

print("\n=== Material override variants (empty tuple = no override, uses mesh default) ===")
for k, n in sorted(matvariants.items(), key=lambda x:-x[1]):
    print("%5d  %s" % (n, k))

print("\nParented/attached tiles:", parented)

# What material does the mesh itself use by default?
mesh_asset = unreal.load_asset(ROOF_MESH)
if mesh_asset:
    smats = mesh_asset.get_editor_property('static_materials')
    print("\n=== SM_Floor_3 default materials (%d slots) ===" % len(smats))
    for i, sm in enumerate(smats):
        mi = sm.get_editor_property('material_interface')
        print("  slot %d: %s = %s" % (i, sm.get_editor_property('material_slot_name'), mi.get_path_name() if mi else 'None'))
