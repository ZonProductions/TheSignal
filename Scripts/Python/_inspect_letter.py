import unreal

# Search for DA_Letter_Test
ar = unreal.AssetRegistryHelpers.get_asset_registry()

# Try direct load with various paths
paths = [
    '/Game/InventorySystemPro/Blueprints/Items/DA_Letter_Test',
    '/Game/Core/Items/DA_Letter_Test',
    '/Game/TheSignal/Items/DA_Letter_Test',
]

asset = None
for p in paths:
    asset = unreal.load_asset(p)
    if asset:
        print(f'Found at: {p}')
        break

if not asset:
    # Search by name pattern
    print('Direct paths failed. Searching all assets...')
    all_assets = ar.get_all_assets()
    for a in all_assets:
        name = str(a.asset_name)
        if 'letter' in name.lower() or 'Letter' in name:
            print(f'  {a.package_path}/{a.asset_name} ({a.asset_class_path})')

if asset:
    print(f'\nAsset class: {asset.get_class().get_name()}')
    for prop in ['name', 'description', 'type', 'thumbnail_image', 'preview_image', 'static_mesh', 'gameplay_tags']:
        try:
            val = asset.get_editor_property(prop)
            print(f'  {prop}: {val}')
        except Exception as e:
            print(f'  {prop}: ERROR - {e}')
