import unreal

bp = unreal.load_asset("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror")
gen = bp.generated_class()
cdo = unreal.get_default_object(gen)

# Try to enumerate all properties via UStruct iteration
print("=== All properties on Horror CDO ===")
for p in sorted(set(dir(cdo))):
    if p.startswith("_"):
        continue
    try:
        v = cdo.get_editor_property(p)
        tname = type(v).__name__
        # Only show interesting types (widgets, objects, not functions)
        if tname not in ["method", "builtin_function_or_method", "function"]:
            if tname in ["NoneType"]:
                # Check if it might be a widget ref (None on CDO)
                print(f"  {p}: None (could be widget ref)")
            elif "Widget" in tname or "Block" in tname or "Image" in tname or "Button" in tname or "Box" in tname:
                print(f"  {p}: {tname} = {v}")
    except:
        pass

# Also try to create a widget instance to see populated widget tree
print("\n=== Trying to add_node VariableGet for common widget names ===")
for name in ["ItemImage", "ItemNameText2", "ItemDescriptionRichText",
             "ItemNameTextBlock", "ItemDescriptionTextBlock",
             "WBP_InventorySimpleButton", "WBP_InventorySimpleButton_Horror",
             "ButtonInteract", "CloseButton", "ConfirmButton"]:
    try:
        v = cdo.get_editor_property(name)
        print(f"  {name}: EXISTS ({type(v).__name__})")
    except:
        print(f"  {name}: NOT FOUND")
