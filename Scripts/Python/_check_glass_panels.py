"""Check scale/bounds of glass panel candidates near elevator wall on F5."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Candidates from the screenshot: glass panels left of exit door (X < 939)
# on the elevator wall (Y ~ -1316 to -1330), F5 (Z ~ 2000-2300)
targets = ['F5_line_box_glass3', 'F5_line_box_glass4', 'F5_SM_Cube59']

# Also find ALL actors in X=300-939, Y=-1340 to -1300, Z=2050-2300 on F5
# These are the wall-mounted items at mid-to-upper height
print("=== Glass panel candidates ===\n")

for a in all_actors:
    label = a.get_actor_label()
    loc = a.get_actor_location()

    if label in targets or (300 <= loc.x <= 939 and -1340 <= loc.y <= -1300 and 2050 <= loc.z <= 2300):
        rot = a.get_actor_rotation()
        scale = a.get_actor_scale3d()
        bounds = a.get_actor_bounds(False)
        origin, extent = bounds[0], bounds[1]

        cls = a.get_class().get_name()
        mesh_name = ""
        mat_list = ""
        if cls == 'StaticMeshActor':
            smc = a.get_component_by_class(unreal.StaticMeshComponent)
            if smc:
                sm = smc.get_editor_property("static_mesh")
                if sm:
                    mesh_name = sm.get_name()
                mats = []
                for i in range(smc.get_num_materials()):
                    mat = smc.get_material(i)
                    if mat:
                        mats.append(mat.get_name())
                mat_list = ", ".join(mats)

        print(f"  {label}")
        print(f"    Mesh: {mesh_name} | Mats: {mat_list}")
        print(f"    Pos: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
        print(f"    Rot: (R={rot.roll:.1f}, P={rot.pitch:.1f}, Y={rot.yaw:.1f})")
        print(f"    Scale: ({scale.x:.3f}, {scale.y:.3f}, {scale.z:.3f})")
        print(f"    Bounds: origin=({origin.x:.1f},{origin.y:.1f},{origin.z:.1f}) extent=({extent.x:.1f},{extent.y:.1f},{extent.z:.1f})")
        print()

# Check if these exist on F1
print("=== F1 equivalents (same mesh, same XY +-10, Z 0-500) ===\n")
for a in all_actors:
    label = a.get_actor_label()
    loc = a.get_actor_location()
    if not (-50 <= loc.z <= 500):
        continue
    if not (300 <= loc.x <= 939 and -1340 <= loc.y <= -1300):
        continue
    cls = a.get_class().get_name()
    if cls != 'StaticMeshActor':
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property("static_mesh")
    mesh_name = sm.get_name() if sm else ""
    scale = a.get_actor_scale3d()
    mats = []
    for i in range(smc.get_num_materials()):
        mat = smc.get_material(i)
        if mat:
            mats.append(mat.get_name())
    print(f"  {label:40s} | {mesh_name:30s} | ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) | scale=({scale.x:.2f},{scale.y:.2f},{scale.z:.2f}) | {', '.join(mats)}")
