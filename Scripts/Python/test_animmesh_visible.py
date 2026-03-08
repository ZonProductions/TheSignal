import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        head_mat = None
        anim_comp = None
        head_comp = None

        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if not m:
                continue
            if m.get_name() == 'CCMH_Head_Male':
                head_comp = c
                head_mat = c.get_material(0)  # Face skin material
            if m.get_name() == 'CC_Dummy_AnimMesh':
                anim_comp = c

        if anim_comp and head_mat and head_comp:
            # Reset head morphs first
            head_comp.set_morph_target('mod_jaw_height', 0.0)
            head_comp.set_morph_target('mod_mouth_size', 0.0)

            # Hide the Head mesh
            head_comp.set_visibility(False)
            unreal.log('Head mesh HIDDEN')

            # Apply face material to AnimMesh
            anim_comp.set_material(0, head_mat)
            unreal.log(f'AnimMesh material set to: {head_mat.get_name()}')

            # Set jawOpen to max
            anim_comp.set_morph_target('jawOpen', 1.0)
            unreal.log('jawOpen=1.0 on AnimMesh with face material')
            unreal.log('Check viewport — is there a visible face with open jaw?')
        break
