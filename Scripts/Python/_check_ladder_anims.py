import unreal

# Find the BP_GraceCharacter to check which ladder animations are assigned
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
if not bp:
    for path in ['/Game/Blueprints/BP_GraceCharacter', '/Game/Characters/BP_GraceCharacter']:
        bp = unreal.load_asset(path)
        if bp:
            break

if bp:
    cdo = unreal.get_default_object(bp.generated_class())
    for prop_name in ['LadderClimbUpAnimation', 'LadderClimbDownAnimation', 'LadderIdleAnimation']:
        try:
            val = cdo.get_editor_property(prop_name)
            if val:
                print(f'{prop_name}: {val.get_path_name()} duration={val.get_play_length():.3f}s')
            else:
                print(f'{prop_name}: None')
        except Exception as e:
            print(f'{prop_name}: ERROR - {e}')
else:
    print('BP_GraceCharacter not found. Searching...')
    # Search for it
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    results = ar.get_assets_by_class('Blueprint')
    for asset in results:
        name = str(asset.asset_name)
        if 'Grace' in name and 'Character' in name:
            print(f'Found: {asset.package_name}')
