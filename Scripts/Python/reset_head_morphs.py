import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if not m or m.get_name() != 'CCMH_Head_Male':
                continue
            c.set_morph_target('mod_jaw_height', 0.0)
            c.set_morph_target('mod_mouth_size', 0.0)
            c.set_morph_target('mod_chin_size', 0.0)
            unreal.log('Head morphs reset to 0')
        break
