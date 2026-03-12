"""Force-compile WBP_Notes and set NoteEntryWidgetClass default."""
import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
if not bp:
    print('ERROR: Failed to load WBP_Notes')
    raise SystemExit

# Force compile
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
print(f'Compiled WBP_Notes')

# Check parent
gc = bp.generated_class()
print(f'GeneratedClass: {gc.get_name()}')

# Check if NoteEntryWidgetClass property now exists
cdo = unreal.get_default_object(gc)
has_prop = hasattr(cdo, 'note_entry_widget_class')
print(f'Has note_entry_widget_class: {has_prop}')

if has_prop:
    # Load WBP_NoteEntry
    entry_bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_NoteEntry')
    if entry_bp:
        entry_class = entry_bp.generated_class()
        print(f'NoteEntry class: {entry_class.get_name()}')
        cdo.set_editor_property('note_entry_widget_class', entry_class)
        print(f'Set NoteEntryWidgetClass = {entry_class.get_name()}')
        # Save
        unreal.EditorAssetLibrary.save_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
        print('Saved WBP_Notes')
    else:
        print('WARNING: WBP_NoteEntry not found - run CreateNoteEntryWidget first')
else:
    print('ERROR: note_entry_widget_class NOT found after compile - parent may not be ZP_NotesWidget')
