import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if not m or m.get_name() != 'CCMH_Head_Male':
                continue
            # Check what bone transform methods are available
            methods = [x for x in dir(c) if 'bone' in x.lower() or 'transform' in x.lower()]
            for method in methods:
                unreal.log(f'  {method}')
        break
