"""Find remaining draw call bloat — repeated meshes that are purely cosmetic."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

mesh_usage = {}
total = 0

for a in eas.get_all_level_actors():
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property('static_mesh')
    if not sm:
        continue

    name = sm.get_name()
    if name not in mesh_usage:
        mesh_usage[name] = 0
    mesh_usage[name] += 1
    total += 1

print(f"Total static mesh actors: {total}")
print(f"\nTop 30 repeated meshes:")
sorted_meshes = sorted(mesh_usage.items(), key=lambda x: -x[1])
for name, count in sorted_meshes[:30]:
    print(f"  {count:5d}x  {name}")

print(f"\nTotal unique mesh types: {len(mesh_usage)}")
print(f"Meshes with 50+ instances (ISM candidates): {sum(1 for _, c in mesh_usage.items() if c >= 50)}")
