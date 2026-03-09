import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

for a in all_actors:
    if a.get_actor_label() == 'F5_kiosk13':
        smc = a.static_mesh_component
        mesh = smc.static_mesh
        print(f'Label: {a.get_actor_label()}')
        print(f'Mesh: {mesh.get_name()} ({mesh.get_path_name()})')
        print(f'Location: {a.get_actor_location()}')
        print(f'Rotation: {a.get_actor_rotation()}')
        num_mats = smc.get_num_materials()
        print(f'Material slots: {num_mats}')
        for i in range(num_mats):
            mat = smc.get_material(i)
            if mat:
                print(f'  Slot {i}: {mat.get_name()} ({mat.get_path_name()})')

                # Check if it's a material instance — get parent and textures
                if isinstance(mat, unreal.MaterialInstanceConstant):
                    parent = mat.get_editor_property('parent')
                    print(f'    Parent: {parent.get_name() if parent else "None"}')
                    # List texture parameters
                    tex_params = mat.get_editor_property('texture_parameter_values')
                    for tp in tex_params:
                        pname = tp.get_editor_property('parameter_info').get_editor_property('name')
                        pval = tp.get_editor_property('parameter_value')
                        print(f'    TexParam "{pname}": {pval.get_name() if pval else "None"}')
                    # List scalar parameters
                    scalar_params = mat.get_editor_property('scalar_parameter_values')
                    for sp in scalar_params:
                        pname = sp.get_editor_property('parameter_info').get_editor_property('name')
                        pval = sp.get_editor_property('parameter_value')
                        print(f'    ScalarParam "{pname}": {pval}')
            else:
                print(f'  Slot {i}: None')
        break
