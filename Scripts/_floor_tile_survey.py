import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_editor_world()

# 1) Which MESHES use MI_Floor, and how many actors each
import collections
mesh_users = collections.Counter()
floor_tile_mesh = None
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.StaticMeshActor):
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m and m.get_name() == 'MI_Floor':
                mesh = c.static_mesh
                if mesh:
                    mesh_users[mesh.get_path_name()] += 1
                    if a.get_actor_label().startswith('FloorTile_'):
                        floor_tile_mesh = mesh.get_path_name()
                break

unreal.log('--- meshes using MI_Floor:')
for path, n in mesh_users.most_common(10):
    unreal.log(f'  {path.split(chr(47))[-1]}: {n} actors')
unreal.log(f'FloorTile mesh: {floor_tile_mesh}')

# 2) Ceiling tile material
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.StaticMeshActor):
    if 'SillingTile' in a.get_actor_label():
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            mesh = c.static_mesh
            unreal.log(f'ceiling actor {a.get_actor_label()}: mesh={mesh.get_name() if mesh else None}')
            for i in range(c.get_num_materials()):
                m = c.get_material(i)
                unreal.log(f'  slot {i}: {m.get_path_name() if m else None}')
        break
