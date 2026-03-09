import unreal

# Search for sound/music assets in the BigCompany pack
asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()

# Search for SoundWave and SoundCue assets in BigCompany folder
print("=== Sound assets in BigCompany pack ===")
paths_to_check = ['/Game/BigCompany', '/Game/Maps_BigCompany', '/Game/BigCompanyPack']
for base_path in paths_to_check:
    if not unreal.EditorAssetLibrary.does_directory_exist(base_path):
        continue
    assets = unreal.EditorAssetLibrary.list_assets(base_path, recursive=True)
    for asset_path in assets:
        if any(kw in str(asset_path).lower() for kw in ['sound', 'music', 'audio', 'cue', 'ambient']):
            print(f'  {asset_path}')

# Also check the level blueprint for audio nodes
print('\n=== Checking level blueprint ===')
world = unreal.EditorLevelLibrary.get_editor_world()
level = world.get_current_level() if world else None
print(f'World: {world}')
print(f'Level: {level}')
level_bp = world.get_level_script_actor() if world else None
print(f'Level Script Actor: {level_bp}')
if level_bp:
    class_name = level_bp.get_class().get_name()
    print(f'  Class: {class_name}')
    # Check for audio components on the level script actor
    comps = level_bp.get_components_by_class(unreal.AudioComponent)
    print(f'  AudioComponents: {len(comps)}')
    for c in comps:
        sound = c.get_editor_property('sound')
        print(f'    Sound: {sound.get_name() if sound else "None"}')
