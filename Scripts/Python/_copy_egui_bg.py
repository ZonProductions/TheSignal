"""Copy the CommonBackground from WBP_EasyOptionsMenuMain into WBP_Notes."""
import unreal

# Load source (original options menu) and target (our notes widget)
source_bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_EasyOptionsMenuMain')
target_bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')

if not source_bp or not target_bp:
    print(f'source: {source_bp}, target: {target_bp}')
    raise SystemExit

# Get the source widget tree root to find the background
source_tree = source_bp.get_editor_property('widget_tree')
target_tree = target_bp.get_editor_property('widget_tree')

print(f'Source tree: {source_tree}')
print(f'Target tree: {target_tree}')

# Find CommonBackground in source
if source_tree:
    root = source_tree.root_widget
    if root:
        print(f'Source root: {root.get_name()} ({root.get_class().get_name()})')
        # Check children
        count = root.get_child_count() if hasattr(root, 'get_child_count') else 0
        for i in range(count):
            child = root.get_child_at(i)
            if child:
                print(f'  [{i}] {child.get_name()} ({child.get_class().get_name()})')

if target_tree:
    root = target_tree.root_widget
    if root:
        print(f'Target root: {root.get_name()} ({root.get_class().get_name()})')
        count = root.get_child_count() if hasattr(root, 'get_child_count') else 0
        for i in range(count):
            child = root.get_child_at(i)
            if child:
                print(f'  [{i}] {child.get_name()} ({child.get_class().get_name()})')
