import unreal
import os

src_path = 'C:/Users/Ommei/workspace/TheSignal/Building1.wav'
dest_path = '/Game/TheSignal/Audio/Music'
asset_name = 'SFX_Building1_Ambience'

# Check file exists
print(f'File exists: {os.path.exists(src_path)}')
print(f'File size: {os.path.getsize(src_path)}')

# Ensure destination directory exists
if not unreal.EditorAssetLibrary.does_directory_exist(dest_path):
    unreal.EditorAssetLibrary.make_directory(dest_path)
    print(f'Created directory: {dest_path}')

# Try import with more explicit settings
task = unreal.AssetImportTask()
task.set_editor_property('filename', src_path)
task.set_editor_property('destination_path', dest_path)
task.set_editor_property('destination_name', asset_name)
task.set_editor_property('replace_existing', True)
task.set_editor_property('replace_existing_settings', True)
task.set_editor_property('automated', True)
task.set_editor_property('save', True)

result = unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
print(f'Import result: {result}')

# Check if task has result info
imported_objects = task.get_editor_property('imported_object_paths')
print(f'Imported paths: {imported_objects}')

result_obj = task.get_editor_property('result')
print(f'Task result: {result_obj}')

# Try loading
asset = unreal.EditorAssetLibrary.load_asset(f'{dest_path}/{asset_name}')
print(f'Asset loaded: {asset}')
