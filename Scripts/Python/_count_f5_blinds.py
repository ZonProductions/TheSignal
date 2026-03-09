"""Count and summarize F5 blind actors by unique XY position (each window grouping)."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

blinds = []
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if label.startswith('F5_') and 'blind' in label.lower():
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        scale = a.get_actor_scale3d()
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        mesh_path = ''
        if smc:
            sm = smc.get_editor_property('static_mesh')
            mesh_path = sm.get_path_name() if sm else ''
        blinds.append({
            'label': label,
            'x': loc.x, 'y': loc.y, 'z': loc.z,
            'roll': rot.roll, 'pitch': rot.pitch, 'yaw': rot.yaw,
            'sx': scale.x, 'sy': scale.y, 'sz': scale.z,
            'mesh': mesh_path
        })

print(f"Total F5 blind actors: {len(blinds)}")

# Group by XY position (rounded to nearest 1 UU)
groups = {}
for b in blinds:
    key = (round(b['x'], 0), round(b['y'], 0))
    if key not in groups:
        groups[key] = []
    groups[key].append(b)

print(f"Unique XY positions (window groups): {len(groups)}")

# Show each group
for (x, y), items in sorted(groups.items()):
    z_min = min(i['z'] for i in items)
    z_max = max(i['z'] for i in items)
    yaw = items[0]['yaw']
    meshes = set(i['mesh'].split('.')[-1] for i in items)
    print(f"  ({x:.0f}, {y:.0f}) yaw={yaw:.0f} — {len(items)} parts, Z: {z_min:.0f}-{z_max:.0f}, meshes: {meshes}")

# Unique meshes used
all_meshes = set(b['mesh'] for b in blinds)
print(f"\nUnique meshes:")
for m in sorted(all_meshes):
    print(f"  {m}")

# Check materials on first blind
if blinds:
    for a in eas.get_all_level_actors():
        if a.get_actor_label() == blinds[0]['label']:
            smc = a.get_component_by_class(unreal.StaticMeshComponent)
            if smc:
                print(f"\nMaterials on {blinds[0]['label']}:")
                for i in range(smc.get_num_materials()):
                    mat = smc.get_material(i)
                    print(f"  Slot {i}: {mat.get_name() if mat else 'None'}")
            break
