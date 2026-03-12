import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
path = bp.get_path_name()
wt = unreal.find_object(None, path + '.WidgetTree')
wt_path = wt.get_path_name()

# Try many common widget naming patterns to find all widgets in the tree
widget_types = [
    'CanvasPanel', 'Overlay', 'HorizontalBox', 'VerticalBox',
    'SizeBox', 'ScaleBox', 'Border', 'ScrollBox', 'Image',
    'TextBlock', 'Button', 'Spacer', 'GridPanel', 'WrapBox',
    'UniformGridPanel', 'WidgetSwitcher', 'InvalidationBox',
    'BackgroundBlur', 'RetainerBox', 'SafeZone', 'NamedSlot',
    'RichTextBlock', 'EditableText', 'MultiLineEditableText',
]

# Try Widget_N, Type_N patterns
found_widgets = []
for wtype in widget_types:
    for n in range(10):
        suffix = f"{wtype}_{n}" if n > 0 else wtype
        obj = unreal.find_object(None, f"{wt_path}.{suffix}")
        if obj:
            found_widgets.append((suffix, obj))

# Also try known names from EasyGameUI
named = [
    'BackgroundImage', 'CommonBackground', 'ContentOverlay', 'ContentPanel',
    'LeftPanel', 'RightPanel', 'MainHBox', 'HeaderText', 'TitleText',
    'SettingsScrollBox', 'SettingsList', 'NoteList', 'NoteContent',
    'Overlay', 'Overlay_0', 'Overlay_1', 'Content', 'InnerContent',
    'BackgroundImage_0', 'CommonBackground_0', 'HBox', 'VBox',
]
for name in named:
    obj = unreal.find_object(None, f"{wt_path}.{name}")
    if obj:
        found_widgets.append((name, obj))

# Deduplicate by path
seen = set()
for name, obj in found_widgets:
    p = obj.get_path_name()
    if p not in seen:
        seen.add(p)
        cls = obj.get_class().get_name()
        # Try to find parent
        try:
            slot = obj.slot
            parent = slot.parent if slot else None
            parent_name = parent.get_name() if parent else 'no_slot'
        except:
            parent_name = '?'
        print(f"  {name} ({cls}) parent={parent_name}")

if not found_widgets:
    print("No additional widgets found in tree!")

# Let's also check: is BackgroundImage a PanelWidget?
root = unreal.find_object(None, f"{wt_path}.BackgroundImage")
print(f"\nBackgroundImage class hierarchy:")
cls = root.get_class()
while cls:
    print(f"  {cls.get_name()}")
    try:
        cls = unreal.SystemLibrary.get_class_parent(cls) if hasattr(unreal.SystemLibrary, 'get_class_parent') else None
    except:
        cls = None

# Check if it has a content slot / named slot
print(f"\nBackgroundImage dir (slot/content related):")
for attr in dir(root):
    if 'slot' in attr.lower() or 'content' in attr.lower() or 'child' in attr.lower() or 'named' in attr.lower():
        print(f"  {attr}")
