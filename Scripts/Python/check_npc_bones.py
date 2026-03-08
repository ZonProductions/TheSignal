import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if not m:
                continue
            mname = m.get_name()
            if mname != 'CC_Dummy_AnimMesh':
                continue

            skel = m.get_editor_property('skeleton')
            if not skel:
                unreal.log(f'{mname}: no skeleton')
                continue

            unreal.log(f'{mname} skeleton: {skel.get_name()}')

            # Get bone names - check for jaw-related bones
            ref_skel = m.get_editor_property('ref_skeleton')
            if not ref_skel:
                unreal.log('  No ref_skeleton')
                continue

            num_bones = ref_skel.get_num()
            unreal.log(f'  Total bones: {num_bones}')
            for i in range(num_bones):
                name = ref_skel.get_bone_name(i)
                name_str = str(name)
                # Print jaw/face/mouth related bones
                if any(x in name_str.lower() for x in ['jaw', 'mouth', 'lip', 'chin', 'tongue', 'teeth', 'face', 'head']):
                    parent_idx = ref_skel.get_parent_index(i)
                    parent_name = ref_skel.get_bone_name(parent_idx) if parent_idx >= 0 else 'ROOT'
                    unreal.log(f'  [{i}] {name_str} (parent: {parent_name})')
