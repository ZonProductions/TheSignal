import unreal

# Check what happened
notes_bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
entry_bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_NoteEntry')

print(f"WBP_Notes: {notes_bp}")
print(f"WBP_NoteEntry: {entry_bp}")

if notes_bp:
    gen = notes_bp.generated_class()
    print(f"WBP_Notes GenClass: {gen.get_name() if gen else 'None'}")

if entry_bp:
    gen = entry_bp.generated_class()
    print(f"WBP_NoteEntry GenClass: {gen.get_name() if gen else 'None'}")
