import unreal

path = '/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror'
wbp = unreal.load_asset(path)
if wbp is None:
    print("FAILED to load WBP")
else:
    print("Loaded:", type(wbp).__name__, wbp.get_name())

    # Use EditorWidgetUtils to check what widgets exist - call the C++ function
    # which already finds ItemNameText2 and ItemDescriptionRichText
    # Let's check the class of ItemNameText2
    result = unreal.ZP_EditorWidgetUtils.set_pickup_description_font_size(12)
    print("SetFont result:", result)
