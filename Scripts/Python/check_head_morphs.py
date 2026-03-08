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
            morphs = m.get_editor_property('morph_targets')
            if len(morphs) > 0:
                unreal.log(f'--- {c.get_name()} -> {mname} ({len(morphs)} morphs) ---')
                for mt in sorted(morphs, key=lambda x: str(x.get_name())):
                    unreal.log(f'  {mt.get_name()}')
        break
