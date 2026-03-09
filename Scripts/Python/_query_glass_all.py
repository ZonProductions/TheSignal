"""Find all glass-related actors and WindowWall actors in the level."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Search for glass materials or window-related meshes
glass_mat_actors = []
window_actors = []

for a in all_actors:
    cls = a.get_class().get_name()
    if cls != 'StaticMeshActor':
        continue
    label = a.get_actor_label()
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property("static_mesh")
    mesh_name = sm.get_name() if sm else ""

    # Check materials for glass
    has_glass_mat = False
    mat_names = []
    for i in range(smc.get_num_materials()):
        mat = smc.get_material(i)
        if mat:
            mn = mat.get_name()
            mat_names.append(mn)
            if 'glass' in mn.lower() or 'Glass' in mn:
                has_glass_mat = True

    if has_glass_mat:
        glass_mat_actors.append((a, mesh_name, mat_names))

    if ('Window' in label or 'window' in label.lower() or 'Window' in mesh_name) and not has_glass_mat:
        window_actors.append((a, mesh_name, mat_names))

# Also check the SM_ElevatorWall mesh bounds
elev_wall = None
for a in all_actors:
    if a.get_actor_label() == 'F5_SM_ElevatorWall2':
        elev_wall = a
        break

if elev_wall:
    bounds = elev_wall.get_actor_bounds(False)
    print(f"=== F5_SM_ElevatorWall2 bounds (raw) === {bounds}")
    origin, extent = bounds[0], bounds[1]
    print(f"  Origin: ({origin.x:.1f}, {origin.y:.1f}, {origin.z:.1f})")
    print(f"  Extent: ({extent.x:.1f}, {extent.y:.1f}, {extent.z:.1f})")

print(f"\n=== Actors with glass materials ({len(glass_mat_actors)}) ===")
# Group by floor (Z)
for a, mesh, mats in sorted(glass_mat_actors, key=lambda x: x[0].get_actor_location().z):
    loc = a.get_actor_location()
    label = a.get_actor_label()
    floor = int(loc.z // 500) + 1
    print(f"  F{floor} {label:45s} | {mesh:25s} | pos=({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}) | {', '.join(mats)}")

print(f"\n=== Window actors ({len(window_actors)}) — first 20 ===")
for a, mesh, mats in sorted(window_actors, key=lambda x: x[0].get_actor_location().z)[:20]:
    loc = a.get_actor_location()
    label = a.get_actor_label()
    floor = int(loc.z // 500) + 1
    print(f"  F{floor} {label:45s} | {mesh:25s} | pos=({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}) | {', '.join(mats)}")
