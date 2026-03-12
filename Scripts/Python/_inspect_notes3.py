import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
path = bp.get_path_name()
wt = unreal.find_object(None, path + '.WidgetTree')
print(f"WidgetTree: {wt.get_name()} ({wt.get_class().get_name()})")

# Check WidgetTree API
print("\n--- WidgetTree methods/properties ---")
for attr in dir(wt):
    if not attr.startswith('_'):
        print(f"  {attr}")

# Try to get root widget
try:
    root = wt.get_editor_property('root_widget')
    print(f"\nRoot widget: {root.get_name() if root else 'None'} ({root.get_class().get_name() if root else 'N/A'})")
except Exception as e:
    print(f"\nroot_widget error: {e}")

# Try root_widget attribute
try:
    root = wt.root_widget
    print(f"wt.root_widget: {root}")
except Exception as e:
    print(f"wt.root_widget error: {e}")

# Try construct_widget
try:
    print(f"\nHas construct_widget: {'construct_widget' in dir(wt)}")
except:
    pass

# Try for_each_widget
try:
    print(f"Has for_each_widget: {'for_each_widget' in dir(wt)}")
except:
    pass
