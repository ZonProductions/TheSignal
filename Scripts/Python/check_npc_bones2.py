import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if not m or m.get_name() != 'CC_Dummy_AnimMesh':
                continue

            # Use the component to get bone names
            num_bones = c.get_num_bones()
            unreal.log(f'CC_Dummy_AnimMesh total bones: {num_bones}')
            for i in range(num_bones):
                name = c.get_bone_name(i)
                name_str = str(name)
                if any(x in name_str.lower() for x in ['jaw', 'mouth', 'lip', 'chin', 'tongue', 'teeth', 'face', 'head']):
                    parent_idx = c.get_parent_bone(name)
                    unreal.log(f'  [{i}] {name_str} (parent: {parent_idx})')
        break
