"""Set NoteEntryWidgetClass on WBP_Notes CDO via set_editor_property."""
import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
entry_bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_NoteEntry')

if not bp:
    print('ERROR: WBP_Notes not found')
    raise SystemExit
if not entry_bp:
    print('ERROR: WBP_NoteEntry not found')
    raise SystemExit

gc = bp.generated_class()
entry_class = entry_bp.generated_class()
cdo = unreal.get_default_object(gc)

print(f'WBP_Notes GeneratedClass: {gc.get_name()}')
print(f'WBP_NoteEntry GeneratedClass: {entry_class.get_name()}')

# Try set_editor_property - EditDefaultsOnly props may not show in dir() but can be set
try:
    cdo.set_editor_property('note_entry_widget_class', entry_class)
    print('SUCCESS: Set note_entry_widget_class via set_editor_property')
except Exception as e:
    print(f'set_editor_property failed: {e}')
    # Fallback: use C++ utility
    try:
        result = unreal.ZP_EditorWidgetUtils.set_notes_widget_defaults()
        print(f'SetNotesWidgetDefaults C++ result: {result}')
    except Exception as e2:
        print(f'C++ fallback also failed: {e2}')

# Save
unreal.EditorAssetLibrary.save_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
print('Saved WBP_Notes')
