import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if not m:
                continue
            name = m.get_name()
            if name not in ('CC_Dummy_AnimMesh', 'CCMH_Head_Male'):
                continue
            # Use the component to get bone names
            num = c.get_num_bones()
            unreal.log(f'=== {name}: {num} bones ===')
            jaw_found = False
            for i in range(num):
                bone_name = str(c.get_bone_name(i))
                if any(kw in bone_name.lower() for kw in ('jaw', 'mouth', 'lip', 'chin', 'tongue', 'teeth', 'face', 'head')):
                    parent_name = str(c.get_parent_bone(bone_name))
                    unreal.log(f'  [{i}] {bone_name} (parent: {parent_name})')
                    if 'jaw' in bone_name.lower():
                        jaw_found = True
            if not jaw_found:
                unreal.log(f'  NO JAW BONE - full list:')
                for i in range(num):
                    bone_name = str(c.get_bone_name(i))
                    unreal.log(f'  [{i}] {bone_name}')
        break
