import unreal

# Restore Head mesh visibility
actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name():
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            m = c.get_skeletal_mesh_asset()
            if m and m.get_name() == 'CCMH_Head_Male':
                c.set_visibility(True)
                unreal.log('Head mesh RESTORED')
            if m and m.get_name() == 'CC_Dummy_AnimMesh':
                c.set_morph_target('jawOpen', 0.0)
                unreal.log('jawOpen reset to 0')
        break

# Search ALL skeletal meshes in the CC folder for ones with jawOpen morph
unreal.log('=== Searching CC folder for meshes with jawOpen ===')
asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
assets = asset_reg.get_assets_by_path('/Game/CharacterCustomizer', recursive=True)
for asset in assets:
    if asset.asset_class_path.asset_name != 'SkeletalMesh':
        continue
    mesh = unreal.load_asset(str(asset.package_name))
    if not mesh:
        continue
    morphs = mesh.get_editor_property('morph_targets')
    morph_names = [m.get_name() for m in morphs]
    if 'jawOpen' in morph_names:
        unreal.log(f'  HAS jawOpen: {asset.package_name} ({len(morphs)} morphs)')
    elif any('CCMH_Head' in str(asset.package_name) for _ in [1]):
        unreal.log(f'  Head mesh (no jawOpen): {asset.package_name} ({len(morphs)} morphs)')
