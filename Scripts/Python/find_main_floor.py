"""Find all flat meshes near Z=0 that could be the main interior floor."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

candidates = []
mesh_usage = {}

for a in all_actors:
    label = a.get_actor_label()
    if label.startswith(('F2_', 'F3_', 'F4_', 'F5_')):
        continue
    loc = a.get_actor_location()
    if abs(loc.z) > 20:
        continue

    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue

    origin, extent = comps[0].get_local_bounds()
    mesh = comps[0].get_editor_property('static_mesh')
    mname = mesh.get_name() if mesh else '?'

    # Track mesh usage counts
    mesh_usage[mname] = mesh_usage.get(mname, 0) + 1

    # Any flat-ish mesh (Z extent < 30) with decent size
    if extent.z < 30 and (extent.x > 100 or extent.y > 100):
        candidates.append((extent.x * extent.y * 4, label, mname,
                          loc.x, loc.y, extent.x * 2, extent.y * 2))

# Show unique mesh types used for floors
print("=== Floor mesh types (by frequency) ===")
for mname, count in sorted(mesh_usage.items(), key=lambda x: -x[1])[:20]:
    print(f"  {mname}: {count} instances")

print(f"\n=== Largest flat meshes at Z=0 (top 20) ===")
candidates.sort(reverse=True)
for area, label, mname, x, y, sx, sy in candidates[:20]:
    print(f"  {label}: {mname} size={sx:.0f}x{sy:.0f} at ({x:.0f},{y:.0f})")
