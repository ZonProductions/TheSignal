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
                c.set_visibility(True)
                c.set_morph_target('mod_jaw_height', 0.0)
                c.set_morph_target('mod_mouth_size', 0.0)
                c.set_morph_target('mod_chin_size', 0.0)
                unreal.log('Head mesh RESTORED')
            if m.get_name() == 'CC_Dummy_AnimMesh':
                c.set_morph_target('jawOpen', 0.0)
                # Restore original material
                orig = unreal.load_asset('/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Dummy/MI_Simple_Masked')
                if orig:
                    c.set_material(0, orig)
                    unreal.log('AnimMesh material restored')
        break
