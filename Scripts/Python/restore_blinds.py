"""Replicate all F5 blind actors to F1-F4."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Collect all F5 blind actors
f5_blinds = []
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if label.startswith('F5_') and 'blind' in label.lower():
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        scale = a.get_actor_scale3d()
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if not smc:
            continue
        sm = smc.get_editor_property('static_mesh')
        if not sm:
            continue
        # Collect materials
        mats = []
        for i in range(smc.get_num_materials()):
            mat = smc.get_material(i)
            mats.append(mat)
        f5_blinds.append({
            'label': label,
            'x': loc.x, 'y': loc.y, 'z': loc.z,
            'roll': rot.roll, 'pitch': rot.pitch, 'yaw': rot.yaw,
            'sx': scale.x, 'sy': scale.y, 'sz': scale.z,
            'mesh': sm,
            'mats': mats
        })

print(f"Found {len(f5_blinds)} F5 blind actors to replicate")

# Load meshes
blade_mesh = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Environment/SM_BlindCurtainBlade")
body_mesh = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Environment/SM_BlindCurtainBody")

# Floor Z offsets from F5 (F5=2000, F4=1500, F3=1000, F2=500, F1=0)
floors = {
    'F1': -2000,
    'F2': -1500,
    'F3': -1000,
    'F4': -500
}

spawned = 0
for floor_name, z_offset in floors.items():
    for b in f5_blinds:
        # Create new label: replace F5_ with Fx_
        new_label = b['label'].replace('F5_', f'{floor_name}_')

        # Spawn actor
        new_loc = unreal.Vector(b['x'], b['y'], b['z'] + z_offset)
        new_rot = unreal.Rotator(roll=b['roll'], pitch=b['pitch'], yaw=b['yaw'])

        actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, new_loc, new_rot)
        if not actor:
            continue

        actor.set_actor_label(new_label)
        actor.set_actor_scale3d(unreal.Vector(b['sx'], b['sy'], b['sz']))

        smc = actor.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            smc.set_editor_property('static_mesh', b['mesh'])
            for i, mat in enumerate(b['mats']):
                if mat:
                    smc.set_material(i, mat)

        spawned += 1

    print(f"  {floor_name}: spawned {len(f5_blinds)} blinds")

print(f"\nTotal spawned: {spawned} blind actors across 4 floors")
