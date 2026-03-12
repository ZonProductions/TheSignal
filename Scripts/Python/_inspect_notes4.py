import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
path = bp.get_path_name()
wt = unreal.find_object(None, path + '.WidgetTree')

# The root widget is stored in the WidgetTree. Since root_widget property isn't
# exposed, let's try to find it by looking for known widget subobjects.
# In UE5, widgets in the tree are outed to the WidgetTree.

# List all objects whose outer is this WidgetTree
print("--- Objects outed to WidgetTree ---")
# We can't enumerate subobjects directly, but we can try known names
# from the EasyGameUI options menu structure
test_names = [
    'RootWidget', 'Root', 'CanvasPanel', 'Canvas',
    'BackgroundImage', 'HorizontalBox', 'Overlay',
    'VerticalBox', 'CommonBackground', 'LeftPanel', 'RightPanel',
    'ContentPanel', 'MainPanel', 'SettingsPanel',
    '[Root]', 'CanvasPanel_0', 'Overlay_0', 'HorizontalBox_0', 'VerticalBox_0',
    'BackgroundBlur', 'Image', 'SizeBox', 'Border',
    'ScaleBox', 'NamedSlot', 'InvalidationBox', 'RetainerBox',
]

wt_path = wt.get_path_name()
found = []
for name in test_names:
    obj = unreal.find_object(None, f"{wt_path}.{name}")
    if obj:
        found.append((name, obj))
        print(f"  FOUND: {name} -> {obj.get_class().get_name()}")

# Also check if PanelWidget's add_child is exposed
print("\n--- ScrollBox methods ---")
sb_class = unreal.ScrollBox
for attr in dir(sb_class):
    if 'child' in attr.lower() or 'add' in attr.lower() or 'slot' in attr.lower():
        print(f"  {attr}")

print("\n--- PanelWidget methods ---")
pw = unreal.PanelWidget
for attr in dir(pw):
    if 'child' in attr.lower() or 'add' in attr.lower() or 'remove' in attr.lower():
        print(f"  {attr}")

# Check OverlaySlot
print("\n--- Check new_object works ---")
try:
    test_tb = unreal.new_object(type=unreal.TextBlock, outer=wt, name='__test_textblock')
    print(f"new_object TextBlock: {test_tb.get_name()} ({test_tb.get_class().get_name()})")
    # Clean up - rename to avoid pollution
except Exception as e:
    print(f"new_object error: {e}")
