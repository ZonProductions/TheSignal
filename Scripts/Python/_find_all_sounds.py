import unreal

# Find all sound assets by scanning content directories
print("=== Sound assets in project (excluding engine/plugins) ===")
dirs_to_scan = ['/Game/TheSignal', '/Game/BigCompany', '/Game/Maps_BigCompany']

for d in dirs_to_scan:
    if not unreal.EditorAssetLibrary.does_directory_exist(d):
        continue
    assets = unreal.EditorAssetLibrary.list_assets(d, recursive=True)
    for asset_path in assets:
        asset = unreal.load_asset(asset_path)
        if asset and (isinstance(asset, unreal.SoundWave) or isinstance(asset, unreal.SoundCue) or isinstance(asset, unreal.SoundBase)):
            print(f'  [{asset.get_class().get_name()}] {asset_path}')

# Also scan root /Game for stray sound assets
print("\n=== Sound assets at /Game root ===")
if unreal.EditorAssetLibrary.does_directory_exist('/Game'):
    root_assets = unreal.EditorAssetLibrary.list_assets('/Game', recursive=False)
    for asset_path in root_assets:
        asset = unreal.load_asset(asset_path)
        if asset and (isinstance(asset, unreal.SoundWave) or isinstance(asset, unreal.SoundCue)):
            print(f'  [{asset.get_class().get_name()}] {asset_path}')

# Level Blueprint
print("\n=== Level Script Actor ===")
subsys = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = subsys.get_editor_world()
if world:
    lsa = world.get_level_script_actor()
    if lsa:
        bp_path = lsa.get_class().get_path_name()
        print(f'  Class: {lsa.get_class().get_name()}')
        print(f'  Path: {bp_path}')
