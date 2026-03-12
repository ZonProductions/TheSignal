import unreal

# Check what panel MapImage lives in (this is where NotesWidget gets added)
bp = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror')
if not bp:
    print("ERROR: Could not load WBP_InventoryMenu_Horror")
else:
    wt = unreal.find_object(None, bp.get_path_name() + '.WidgetTree')
    if wt:
        wt_path = wt.get_path_name()
        map_img = unreal.find_object(None, f"{wt_path}.MapImage")
        if map_img:
            print(f"MapImage: {map_img.get_name()} ({map_img.get_class().get_name()})")
            # Get parent
            try:
                parent = map_img.get_parent()
                if parent:
                    print(f"MapImage parent: {parent.get_name()} ({parent.get_class().get_name()})")
                    print(f"Parent child count: {parent.get_children_count()}")
                    for i in range(parent.get_children_count()):
                        c = parent.get_child_at(i)
                        print(f"  child[{i}]: {c.get_name()} ({c.get_class().get_name()})")
                else:
                    print("MapImage has no parent!")
            except Exception as e:
                print(f"Error getting parent: {e}")
        else:
            print("MapImage not found in widget tree")

# Check WBP_Notes graphs (old EGUI code that might be rendering "NOTES" header)
print("\n=== WBP_Notes Graphs ===")
notes_bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
if notes_bp:
    gen = notes_bp.generated_class()
    print(f"GeneratedClass: {gen.get_name()}")
    # Check if there's a Construct event that might run
    # Look for functions
    print(f"BP class: {notes_bp.get_class().get_name()}")
