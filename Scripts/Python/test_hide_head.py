import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if not m:
                continue
            if m.get_name() == 'CCMH_Head_Male':
                c.set_visibility(False)
                unreal.log('Head mesh HIDDEN')
            if m.get_name() == 'CC_Dummy_AnimMesh':
                c.set_morph_target('jawOpen', 1.0)
                unreal.log('jawOpen set to 1.0 on AnimMesh')
        break
