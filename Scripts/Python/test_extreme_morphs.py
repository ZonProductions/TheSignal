import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if not m or m.get_name() != 'CCMH_Head_Male':
                continue

            # Test with extreme values (5x normal range)
            c.set_morph_target('mod_jaw_height', 5.0)
            c.set_morph_target('mod_mouth_size', 5.0)
            unreal.log('Set mod_jaw_height=5.0, mod_mouth_size=5.0')
            unreal.log('Check viewport - does the jaw actually open now?')
        break
