"""Survey what actor types and meshes exist in the current level."""
import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()

# Count by class
by_class = {}
for a in actors:
    cn = a.get_class().get_name()
    by_class[cn] = by_class.get(cn, 0) + 1

print('=== ACTOR TYPES ===')
for cn, count in sorted(by_class.items(), key=lambda x: -x[1])[:30]:
    print(f'  {count:4d}  {cn}')

# Find unique static mesh assets used
print('\n=== UNIQUE STATIC MESHES ===')
mesh_names = set()
for a in actors:
    if a.get_class().get_name() == 'StaticMeshActor':
        comp = a.get_editor_property('static_mesh_component')
        if comp:
            mesh = comp.get_editor_property('static_mesh')
            if mesh:
                mesh_names.add(mesh.get_path_name())

# Group by folder
by_folder = {}
for path in sorted(mesh_names):
    parts = path.rsplit('/', 1)
    folder = parts[0] if len(parts) > 1 else '/'
    if folder not in by_folder:
        by_folder[folder] = []
    by_folder[folder].append(parts[-1] if len(parts) > 1 else path)

for folder in sorted(by_folder.keys()):
    meshes = by_folder[folder]
    print(f'\n  {folder}/ ({len(meshes)} meshes)')
    for m in sorted(meshes)[:10]:
        print(f'    {m}')
    if len(meshes) > 10:
        print(f'    ... +{len(meshes)-10} more')

print(f'\nTotal unique meshes: {len(mesh_names)}')
print(f'Total actors: {len(actors)}')
