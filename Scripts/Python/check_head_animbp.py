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
            if name != 'CCMH_Head_Male':
                continue

            # Check AnimBP
            anim_inst = c.get_anim_instance()
            if anim_inst:
                unreal.log(f'Head AnimInstance class: {anim_inst.get_class().get_name()}')
                unreal.log(f'Head AnimInstance parent: {anim_inst.get_class().get_super_class().get_name()}')
                # Check morph target related properties
                props = anim_inst.get_class().get_editor_property_names() if hasattr(anim_inst.get_class(), 'get_editor_property_names') else []
            else:
                unreal.log('Head: NO AnimInstance')

            # Check animation mode
            anim_mode = c.get_editor_property('animation_mode')
            unreal.log(f'Head animation_mode: {anim_mode}')

            # Check post process anim BP
            try:
                pp = c.get_editor_property('post_process_anim_blueprint')
                unreal.log(f'Head PostProcessAnimBP: {pp}')
            except:
                unreal.log('Head PostProcessAnimBP: N/A')

            # Check anim class
            try:
                ac = c.get_editor_property('anim_class')
                unreal.log(f'Head anim_class: {ac}')
            except:
                pass

            # Check current morph targets
            morph_names = ['jawOpen', 'mod_jaw_height', 'mod_mouth_size']
            for mn in morph_names:
                val = c.get_morph_target(mn)
                unreal.log(f'  Morph "{mn}": {val}')

            # Try setting jawOpen and reading back
            c.set_morph_target('jawOpen', 1.0)
            val = c.get_morph_target('jawOpen')
            unreal.log(f'  After set jawOpen=1.0, read back: {val}')

        break
