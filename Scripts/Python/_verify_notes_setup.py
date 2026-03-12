import unreal

print("=== WBP_Notes Verification ===")
bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
if bp:
    gen = bp.generated_class()
    cdo = gen.get_default_object()
    print(f"GeneratedClass: {gen.get_name()}")

    # Check NoteEntryWidgetClass
    try:
        val = cdo.get_editor_property('NoteEntryWidgetClass')
        print(f"NoteEntryWidgetClass: {val}")
    except Exception as e:
        print(f"NoteEntryWidgetClass: ERROR - {e}")

    # Check widget tree
    wt = unreal.find_object(None, bp.get_path_name() + '.WidgetTree')
    if wt:
        wt_path = wt.get_path_name()
        required = ['NoteListScrollBox', 'NoteTitle', 'NoteContent', 'NotesEmptyText', 'NotesRoot']
        for name in required:
            obj = unreal.find_object(None, f"{wt_path}.{name}")
            status = f"{obj.get_class().get_name()}" if obj else "NOT FOUND"
            marker = "OK" if obj else "MISSING"
            print(f"  [{marker}] {name}: {status}")

print("\n=== WBP_NoteEntry Verification ===")
bp2 = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_NoteEntry')
if bp2:
    gen2 = bp2.generated_class()
    print(f"GeneratedClass: {gen2.get_name()}")

    wt2 = unreal.find_object(None, bp2.get_path_name() + '.WidgetTree')
    if wt2:
        wt2_path = wt2.get_path_name()
        for name in ['EntryButton', 'EntryTitle']:
            obj = unreal.find_object(None, f"{wt2_path}.{name}")
            status = f"{obj.get_class().get_name()}" if obj else "NOT FOUND"
            marker = "OK" if obj else "MISSING"
            print(f"  [{marker}] {name}: {status}")
else:
    print("WBP_NoteEntry NOT FOUND")
