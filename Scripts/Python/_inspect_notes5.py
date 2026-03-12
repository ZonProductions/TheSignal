import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
path = bp.get_path_name()
wt = unreal.find_object(None, path + '.WidgetTree')
wt_path = wt.get_path_name()

# Find root
root = unreal.find_object(None, f"{wt_path}.BackgroundImage")
print(f"Root: {root.get_name()} ({root.get_class().get_name()})")

def dump_tree(widget, indent=0):
    prefix = "  " * indent
    name = widget.get_name()
    cls = widget.get_class().get_name()
    try:
        count = widget.get_children_count()
        print(f"{prefix}{name} ({cls}) [{count} children]")
        for i in range(count):
            child = widget.get_child_at(i)
            dump_tree(child, indent + 1)
    except:
        print(f"{prefix}{name} ({cls}) [leaf]")

dump_tree(root)
