import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            mname = m.get_name() if m else 'None'
            vis = c.is_visible()
            hig = c.get_editor_property('hidden_in_game')
            leader = c.get_editor_property('leader_pose_component')
            leader_name = leader.get_name() if leader else 'None'
            unreal.log(f'{c.get_name()} -> {mname} | visible={vis} hidden_in_game={hig} leader={leader_name}')
