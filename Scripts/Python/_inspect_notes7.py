import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
path = bp.get_path_name()
wt = unreal.find_object(None, path + '.WidgetTree')
wt_path = wt.get_path_name()

# Try accessing root_widget with different property name casings
for prop_name in ['RootWidget', 'root_widget', 'rootWidget', 'Root']:
    try:
        val = wt.get_editor_property(prop_name)
        print(f"GET '{prop_name}': {val}")
    except Exception as e:
        print(f"GET '{prop_name}': FAILED - {e}")

# Try setting root widget
print("\n--- Try creating and setting root ---")
try:
    overlay = unreal.new_object(type=unreal.Overlay, outer=wt, name='RootOverlay')
    print(f"Created Overlay: {overlay.get_name()}")

    for prop_name in ['RootWidget', 'root_widget']:
        try:
            wt.set_editor_property(prop_name, overlay)
            print(f"SET '{prop_name}': SUCCESS")
        except Exception as e:
            print(f"SET '{prop_name}': FAILED - {e}")
except Exception as e:
    print(f"Create/set error: {e}")

# Try call_method on WidgetTree
print("\n--- Try call_method ---")
try:
    result = wt.call_method('FindWidget', args={'Name': 'BackgroundImage'})
    print(f"call_method FindWidget: {result}")
except Exception as e:
    print(f"call_method FindWidget: FAILED - {e}")

# Check if WidgetBlueprintFactory exists
print("\n--- WidgetBlueprintFactory ---")
try:
    factory = unreal.WidgetBlueprintFactory()
    print(f"Factory created: {factory.get_name()}")
    for attr in dir(factory):
        if 'parent' in attr.lower() or 'class' in attr.lower():
            print(f"  {attr}")
except Exception as e:
    print(f"Factory error: {e}")
