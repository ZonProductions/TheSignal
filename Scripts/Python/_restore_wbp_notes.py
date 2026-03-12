"""Restore WBP_Notes by duplicating from the original EGUI options menu."""
import unreal

source = '/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_EasyOptionsMenuMain'
target = '/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes'

# Step 1: Delete the current broken WBP_Notes
if unreal.EditorAssetLibrary.does_asset_exist(target):
    deleted = unreal.EditorAssetLibrary.delete_asset(target)
    print(f'Deleted old WBP_Notes: {deleted}')
else:
    print('WBP_Notes does not exist (already deleted)')

# Step 2: Duplicate the original EGUI options menu as WBP_Notes
duplicated = unreal.EditorAssetLibrary.duplicate_asset(source, target)
print(f'Duplicated WBP_EasyOptionsMenuMain -> WBP_Notes: {duplicated}')

if not duplicated:
    print('ERROR: Duplication failed!')
    raise SystemExit

# Step 3: Strip old EGUI graphs/variables/interfaces
result = unreal.ZP_EditorWidgetUtils.strip_notes_blueprint()
print(f'StripNotesBlueprint: {result}')

# Step 4: Strip widget tree children — keep ONLY the background, remove all settings UI
result_strip = unreal.ZP_EditorWidgetUtils.strip_notes_widget_children()
print(f'StripNotesWidgetChildren: {result_strip}')

# Step 5: Setup the notes widget tree (wraps preserved background in overlay, adds notes content)
result2 = unreal.ZP_EditorWidgetUtils.setup_notes_widget()
print(f'SetupNotesWidget: {result2}')

# Step 6: Set NoteEntryWidgetClass default (may fail — use _set_notes_defaults.py after compile)
result3 = unreal.ZP_EditorWidgetUtils.set_notes_widget_defaults()
print(f'SetNotesWidgetDefaults: {result3}')

print('DONE — WBP_Notes restored from original EGUI source')
