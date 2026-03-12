"""
Strip all old EGUI options menu graphs and variables from WBP_Notes.
These are dead code from the original options menu copy that still
runs on Construct, creating a "NOTES" header with green underline.
"""
import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
if not bp:
    print("ERROR: Could not load WBP_Notes")
else:
    gen = bp.generated_class()
    print(f"WBP_Notes loaded, GeneratedClass: {gen.get_name()}")

    # Use KismetEditorUtilities to clear the event graph
    # We can't delete EventGraph pages, but we can remove all nodes
    # The simplest approach: remove all user-constructed nodes from the uber graph
    # by accessing the blueprint's graphs

    # Get all graphs
    try:
        graphs = bp.get_editor_property('function_graphs')
        print(f"Function graphs: {len(graphs) if graphs else 0}")
    except Exception as e:
        print(f"function_graphs error: {e}")

    # Try uber_graph_pages (EventGraph)
    try:
        uber_pages = bp.get_editor_property('uber_graph_pages')
        print(f"Uber graph pages: {len(uber_pages) if uber_pages else 0}")
        if uber_pages:
            for page in uber_pages:
                nodes = page.get_editor_property('nodes')
                print(f"  Page '{page.get_name()}': {len(nodes) if nodes else 0} nodes")
                # Clear all nodes
                if nodes:
                    for node in list(nodes):
                        page.remove_node(node)
                    print(f"    -> Cleared all nodes")
    except Exception as e:
        print(f"uber_graph_pages error: {e}")

    # Remove function graphs
    try:
        func_graphs = bp.get_editor_property('function_graphs')
        if func_graphs:
            count = len(func_graphs)
            func_graphs.clear()
            print(f"Cleared {count} function graphs")
    except Exception as e:
        print(f"Clear function_graphs error: {e}")

    # Save
    try:
        unreal.EditorAssetLibrary.save_asset(bp.get_path_name().rsplit('.', 1)[0])
        print("Saved WBP_Notes")
    except Exception as e:
        print(f"Save error: {e}")
