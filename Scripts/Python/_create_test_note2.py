"""Create a second test note by duplicating DA_Letter_Test."""
import unreal

source = '/Game/Core/Items/DA_Letter_Test'
target_name = 'DA_SecurityLog'
target_folder = '/Game/Core/Items'
target_path = target_folder + '/' + target_name

# Delete if exists
if unreal.EditorAssetLibrary.does_asset_exist(target_path):
    unreal.EditorAssetLibrary.delete_asset(target_path)
    print('Deleted existing')

# Duplicate via AssetTools
src = unreal.load_asset(source)
da = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
    target_name, target_folder, src)
if not da:
    print('ERROR: Duplication failed')
    raise SystemExit

print(f'Duplicated to {target_path}')

# Set name and description
da.set_editor_property('Name', 'Security Incident Report')
da.set_editor_property('Description',
    'INCIDENT #4471\nDate: February 28, 2026\n'
    'Filed by: M. Torres, Head of Security\n\n'
    'At 0247 hours, motion sensors on sublevel B3 triggered a cascading '
    'alert across zones 7 through 12. Security team found all doors in '
    'the east corridor standing open. Electronic locks showed no override '
    'codes. The system logs indicate the doors opened themselves.\n\n'
    'Maintenance says the wiring is fine. IT says the software is fine.\n\n'
    'I have requested a full lockdown of sublevel B3. Request denied by '
    'Dr. Vasquez, who is unavailable but monitoring the situation remotely.\n\n'
    'I do not believe Dr. Vasquez has been in this building for over a week.\n\n'
    '-- M. Torres')

# Tag with Item.Note
tags = da.get_editor_property('GameplayTags')
tags.import_text('(GameplayTags=("Item.Note"))')
da.set_editor_property('GameplayTags', tags)

# Save
unreal.EditorAssetLibrary.save_asset(target_path)
print(f'Created and saved {target_path}')
