"""Convert all blind actors to ISMs using add_component_by_class."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Collect all blind actors grouped by floor and mesh type
blind_data = {}
blind_actors = []

for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if 'blind' not in label.lower():
        continue

    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property('static_mesh')
    if not sm:
        continue

    floor = 'UNK'
    for prefix in ['F1_', 'F2_', 'F3_', 'F4_', 'F5_']:
        if label.startswith(prefix):
            floor = prefix[:2]
            break

    mesh_path = sm.get_path_name()
    key = (floor, mesh_path)

    loc = a.get_actor_location()
    rot = a.get_actor_rotation()
    scale = a.get_actor_scale3d()
    mat = smc.get_material(0) if smc.get_num_materials() > 0 else None

    if key not in blind_data:
        blind_data[key] = {'instances': [], 'mesh': sm, 'mat': mat}
    blind_data[key]['instances'].append((loc, rot, scale))
    blind_actors.append(a)

print(f"Found {len(blind_actors)} blind actors in {len(blind_data)} groups")

# Create ISM actors
spawned_isms = 0
for (floor, mesh_path), data in blind_data.items():
    instances = data['instances']
    mesh = data['mesh']
    mat = data['mat']
    mesh_name = mesh.get_name()

    # Spawn a StaticMeshActor as base, then swap its component
    # Better: spawn Actor, use Python to create ISMC properly
    actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
    if not actor:
        continue

    actor.set_actor_label(f"{floor}_ISM_{mesh_name}")

    # Get the existing static mesh component and just set its properties
    # We can't easily swap to ISMC in Python, so let's try using
    # the built-in "Convert Actors" approach instead
    # Actually - let's just use HierarchicalInstancedStaticMeshComponent via spawn
    actor.destroy_actor()

    # Alternative: Use EditorActorSubsystem to merge
    # Actually simplest: just batch the transforms and use a single HISMC actor

    # Create via Python new-style
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.Actor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    )
    if not actor:
        print(f"  FAILED for {floor}_{mesh_name}")
        continue

    actor.set_actor_label(f"{floor}_ISM_{mesh_name}")

    # Add ISMC via add_component_by_class
    ismc = actor.add_component_by_class(
        unreal.InstancedStaticMeshComponent,
        False,
        unreal.Transform(),
        False
    )
    if not ismc:
        print(f"  FAILED to add ISMC for {floor}_{mesh_name}")
        actor.destroy_actor()
        continue

    ismc.set_editor_property('static_mesh', mesh)
    if mat:
        ismc.set_material(0, mat)

    # Add instances
    for loc, rot, scale in instances:
        transform = unreal.Transform(
            location=[loc.x, loc.y, loc.z],
            rotation=[rot.roll, rot.pitch, rot.yaw],
            scale=[scale.x, scale.y, scale.z]
        )
        ismc.add_instance(transform, True)  # True = world space

    spawned_isms += 1
    print(f"  {floor}_{mesh_name}: {len(instances)} instances -> 1 ISM")

# Delete original blind actors
deleted = 0
for a in blind_actors:
    if a.is_valid():
        a.destroy_actor()
        deleted += 1

print(f"\nCreated {spawned_isms} ISM actors")
print(f"Deleted {deleted} individual blind actors")
print(f"Draw call reduction: {deleted} -> {spawned_isms}")
