import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if not m or m.get_name() != 'CCMH_Head_Male':
                continue

            # Test jaw-related morphs at extreme values
            # Set each one to 1.0 and log what it does
            c.set_morph_target('mod_jaw_height', 1.0)
            c.set_morph_target('mod_mouth_size', 1.0)
            c.set_morph_target('mod_chin_size', 1.0)
            unreal.log('Set mod_jaw_height=1.0, mod_mouth_size=1.0, mod_chin_size=1.0 on Head mesh')
            unreal.log('Check the NPC in the viewport — does the jaw/mouth area look different?')
        break
