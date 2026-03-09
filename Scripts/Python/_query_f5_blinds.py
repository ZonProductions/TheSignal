"""Find all blind/shade actors on F5 near WindowWall locations."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

blinds = []
window_walls = []

for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if label.startswith('F5_') and ('blind' in label.lower() or 'shade' in label.lower() or 'curtain' in label.lower()):
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        scale = a.get_actor_scale3d()
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        mesh_name = ''
        if smc:
            sm = smc.get_editor_property('static_mesh')
            mesh_name = sm.get_path_name() if sm else 'None'
        blinds.append((label, loc, rot, scale, mesh_name))
        print(f"BLIND: {label}")
        print(f"  Loc: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
        print(f"  Rot: (R={rot.roll:.1f}, P={rot.pitch:.1f}, Y={rot.yaw:.1f})")
        print(f"  Scale: ({scale.x:.3f}, {scale.y:.3f}, {scale.z:.3f})")
        print(f"  Mesh: {mesh_name}")

if not blinds:
    # Try broader search - maybe they aren't prefixed with F5_
    print("No F5_ blinds found. Searching ALL blind actors...")
    for a in eas.get_all_level_actors():
        label = a.get_actor_label()
        if 'blind' in label.lower() or 'WindowBlind' in label:
            loc = a.get_actor_location()
            print(f"  {label} at Z={loc.z:.1f}")

print(f"\nTotal F5 blinds found: {len(blinds)}")
