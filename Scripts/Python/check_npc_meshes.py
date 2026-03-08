import unreal

# Find all BP_NPC instances in the level
actors = unreal.EditorLevelLibrary.get_all_level_actors()
for actor in actors:
    cls_name = actor.get_class().get_name()
    if 'NPC' in cls_name and 'BP_' in cls_name:
        unreal.log(f'=== Found NPC: {actor.get_name()} (class: {cls_name}) ===')

        # List ALL SkeletalMeshComponents
        components = actor.get_components_by_class(unreal.SkeletalMeshComponent)
        unreal.log(f'  SkeletalMeshComponent count: {len(components)}')

        for comp in components:
            mesh = comp.get_skeletal_mesh_asset()
            mesh_name = mesh.get_name() if mesh else 'None'
            unreal.log(f'  - {comp.get_name()} -> mesh: {mesh_name}')

            if mesh:
                morphs = mesh.get_editor_property('morph_targets')
                morph_names = [m.get_name() for m in morphs]
                has_jaw = 'jawOpen' in morph_names
                unreal.log(f'    morph_targets: {len(morphs)}, has jawOpen: {has_jaw}')

        # Also list all component types
        all_comps = actor.get_components_by_class(unreal.ActorComponent)
        unreal.log(f'  Total components: {len(all_comps)}')
        for c in all_comps:
            unreal.log(f'    {c.get_name()} ({c.get_class().get_name()})')
