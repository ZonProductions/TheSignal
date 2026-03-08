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
            if mname not in ('CC_Dummy_AnimMesh', 'CCMH_Head_Male'):
                continue

            unreal.log(f'=== {c.get_name()} -> {mname} ===')
            materials = m.get_editor_property('materials')
            for i, mat_slot in enumerate(materials):
                mat = mat_slot.get_editor_property('material_interface')
                mat_name = mat.get_name() if mat else 'None'
                slot_name = mat_slot.get_editor_property('material_slot_name')
                unreal.log(f'  [{i}] slot={slot_name} mat={mat_name}')

            # Also check override materials on the component
            num_mats = c.get_num_materials()
            unreal.log(f'  Component material overrides ({num_mats}):')
            for i in range(num_mats):
                override_mat = c.get_material(i)
                override_name = override_mat.get_name() if override_mat else 'None'
                unreal.log(f'    [{i}] {override_name}')
        break
