import unreal

# Remove all book meshes from F5 bookshelves, keeping the shelf meshes.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

BOOK_KEYWORDS = ['book', 'Book']
deleted = 0
kept_shelves = 0

for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    label = a.get_actor_label()
    # Only F5 actors
    if not label.startswith('F5'):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    mesh_name = smc.static_mesh.get_name()
    if any(kw in mesh_name for kw in BOOK_KEYWORDS):
        print(f'  Deleted: {label} ({mesh_name})')
        a.destroy_actor()
        deleted += 1
    elif 'shelf' in mesh_name.lower() or 'Shelf' in mesh_name:
        kept_shelves += 1

print(f'\nDeleted {deleted} book actors from F5')
print(f'Kept {kept_shelves} shelf actors')

if deleted > 0:
    subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsys_level.save_all_dirty_levels()
    print('Level saved.')
else:
    print('No books found — checking mesh names...')
    # Debug: list all F5 mesh names to find the right keyword
    mesh_names = set()
    for a in all_actors:
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        label = a.get_actor_label()
        if not label.startswith('F5'):
            continue
        smc = a.static_mesh_component
        if smc and smc.static_mesh:
            mesh_names.add(smc.static_mesh.get_name())
    for name in sorted(mesh_names):
        if any(kw in name.lower() for kw in ['book', 'paper', 'magazine', 'binder', 'folder']):
            print(f'  Possible book mesh: {name}')
