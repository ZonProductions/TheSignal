import unreal

# The C++ SetPickupDescriptionFontSize already handles CommonRichTextBlock (description)
# but fails on CommonTextBlock (title) because UTextBlock uses 'font' not 'DefaultTextStyleOverride'.
# CommonTextBlock also has a 'style' property (TSubclassOf<UCommonTextStyle>) that may override font.
# This script directly modifies the widget tree template.

result = unreal.ZP_EditorWidgetUtils.set_pickup_description_font_size(18)
print("C++ font fix (description):", result)

# Now fix the title (CommonTextBlock) via direct widget tree access
wbp_path = '/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror.WBP_FirstTimePickupNotification_Horror'
wbp = unreal.load_object(name=wbp_path, outer=None)

if not wbp:
    print("FAILED to load WBP")
else:
    print("WBP loaded:", wbp.get_name())
    # Widget tree widgets are sub-objects — try finding ItemNameText2
    # In UE5, widget tree template widgets exist as WidgetTree->AllWidgets
    # Access via reflection
    tree = wbp.get_editor_property('widget_tree')
    if tree:
        print("WidgetTree:", tree)
        # ForEachWidget
        all_widgets = tree.get_editor_property('all_widgets') if hasattr(tree, 'get_editor_property') else None
        print("AllWidgets:", all_widgets)
    else:
        print("No widget_tree property")
