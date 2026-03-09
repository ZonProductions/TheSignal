"""Performance diagnostic — identify likely FPS bottlenecks."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

all_actors = eas.get_all_level_actors()
print(f"=== TOTAL ACTORS IN LEVEL: {len(all_actors)} ===\n")

# Count by type
type_counts = {}
static_mesh_count = 0
light_count = 0
shadow_casting_lights = 0
total_triangles_estimate = 0
actors_per_floor = {0: 0, 1: 0, 2: 0, 3: 0, 4: 0}  # floors 0-4 (F1-F5)

for a in all_actors:
    cls = a.get_class().get_name()
    type_counts[cls] = type_counts.get(cls, 0) + 1

    z = a.get_actor_location().z
    floor_idx = max(0, min(4, int(z / 500.0)))
    actors_per_floor[floor_idx] += 1

    # Check static meshes
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        static_mesh_count += 1

    # Check lights
    lc = a.get_component_by_class(unreal.LightComponent)
    if lc:
        light_count += 1
        if lc.get_editor_property('cast_shadows'):
            shadow_casting_lights += 1

print("--- ACTORS PER FLOOR ---")
for f in range(5):
    print(f"  Floor {f+1} (Z {f*500}-{(f+1)*500}): {actors_per_floor[f]} actors")

print(f"\n--- KEY COUNTS ---")
print(f"  Static mesh actors: {static_mesh_count}")
print(f"  Lights: {light_count}")
print(f"  Shadow-casting lights: {shadow_casting_lights}")

print(f"\n--- TOP 20 ACTOR TYPES ---")
sorted_types = sorted(type_counts.items(), key=lambda x: -x[1])
for cls, count in sorted_types[:20]:
    print(f"  {cls}: {count}")

# Check for duplicate meshes (ISM candidates)
mesh_usage = {}
for a in all_actors:
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        sm = smc.get_editor_property('static_mesh')
        if sm:
            name = sm.get_name()
            mesh_usage[name] = mesh_usage.get(name, 0) + 1

print(f"\n--- TOP 20 REPEATED MESHES (ISM candidates) ---")
sorted_meshes = sorted(mesh_usage.items(), key=lambda x: -x[1])
for name, count in sorted_meshes[:20]:
    print(f"  {name}: {count} instances")

# Check for overlapping actors at same position
print(f"\n--- UNIQUE MESH TYPES: {len(mesh_usage)} ---")
print(f"--- TOTAL MESH INSTANCES: {sum(mesh_usage.values())} ---")
