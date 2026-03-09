"""Query elevator wall and glass actors to understand flush issue."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

target = None
elevator_walls = []
glass_actors = []

for a in all_actors:
    label = a.get_actor_label()
    if 'ElevatorWall' in label or 'elevatorwall' in label.lower():
        elevator_walls.append(a)
        if label == 'F5_SM_ElevatorWall2':
            target = a
    if 'glass' in label.lower() or 'Glass' in label:
        cls = a.get_class().get_name()
        if cls == 'StaticMeshActor':
            glass_actors.append(a)

print(f"=== Target: F5_SM_ElevatorWall2 ===")
if target:
    loc = target.get_actor_location()
    rot = target.get_actor_rotation()
    scale = target.get_actor_scale3d()
    print(f"  Location: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
    print(f"  Rotation: (R={rot.roll:.1f}, P={rot.pitch:.1f}, Y={rot.yaw:.1f})")
    print(f"  Scale: ({scale.x:.3f}, {scale.y:.3f}, {scale.z:.3f})")
    smc = target.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        sm = smc.get_editor_property("static_mesh")
        if sm:
            print(f"  Mesh: {sm.get_name()}")
        num_mats = smc.get_num_materials()
        for i in range(num_mats):
            mat = smc.get_material(i)
            if mat:
                print(f"  Material[{i}]: {mat.get_name()}")

print(f"\n=== All ElevatorWall actors ({len(elevator_walls)}) ===")
for a in sorted(elevator_walls, key=lambda x: (x.get_actor_location().z, x.get_actor_label())):
    loc = a.get_actor_location()
    rot = a.get_actor_rotation()
    label = a.get_actor_label()
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    mesh_name = ""
    if smc:
        sm = smc.get_editor_property("static_mesh")
        if sm:
            mesh_name = sm.get_name()
    print(f"  {label:40s} | pos=({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}) | rot=({rot.roll:.1f}, {rot.pitch:.1f}, {rot.yaw:.1f}) | {mesh_name}")

# Find glass near the F5 elevator wall
if target:
    t_loc = target.get_actor_location()
    print(f"\n=== Glass actors near F5_SM_ElevatorWall2 (within 300 UU XY) ===")
    for a in glass_actors:
        loc = a.get_actor_location()
        dx = abs(loc.x - t_loc.x)
        dy = abs(loc.y - t_loc.y)
        dz = abs(loc.z - t_loc.z)
        if dx < 300 and dy < 300 and dz < 100:
            rot = a.get_actor_rotation()
            scale = a.get_actor_scale3d()
            label = a.get_actor_label()
            smc = a.get_component_by_class(unreal.StaticMeshComponent)
            mesh_name = ""
            if smc:
                sm = smc.get_editor_property("static_mesh")
                if sm:
                    mesh_name = sm.get_name()
            print(f"  {label:40s} | pos=({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}) | rot=({rot.roll:.1f}, {rot.pitch:.1f}, {rot.yaw:.1f}) | scale=({scale.x:.3f}, {scale.y:.3f}, {scale.z:.3f}) | {mesh_name}")
