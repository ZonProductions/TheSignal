"""Find ALL actors near the elevator wall to identify the glass."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# F5_SM_ElevatorWall2 is at X=1275.6, Y=-1316.1, Z=1999.2
# Search a box around it on F5
REF_X, REF_Y, REF_Z = 1275.6, -1316.1, 1999.2

print("=== All actors within 300 UU XY of F5_SM_ElevatorWall2 (F5 Z range) ===")
nearby = []
for a in all_actors:
    loc = a.get_actor_location()
    dx = abs(loc.x - REF_X)
    dy = abs(loc.y - REF_Y)
    dz = abs(loc.z - REF_Z)
    if dx < 300 and dy < 300 and dz < 300:
        nearby.append(a)

nearby.sort(key=lambda a: a.get_actor_label())
for a in nearby:
    loc = a.get_actor_location()
    rot = a.get_actor_rotation()
    label = a.get_actor_label()
    cls = a.get_class().get_name()
    mesh_name = ""
    if cls == 'StaticMeshActor':
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            sm = smc.get_editor_property("static_mesh")
            if sm:
                mesh_name = sm.get_name()
            # Check for glass-like materials
            mats = []
            for i in range(smc.get_num_materials()):
                mat = smc.get_material(i)
                if mat:
                    mats.append(mat.get_name())
            if mats:
                mesh_name += " | mats: " + ", ".join(mats)
    print(f"  {label:50s} | {cls:25s} | pos=({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}) | {mesh_name}")
