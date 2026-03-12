import unreal

# Check the base class to understand what variables exist
base = unreal.load_asset("/Game/InventorySystemPro/ExampleContent/Base/UI/Widgets/WBP_FirstTimePickupNotificationBase")
if base:
    gen = base.generated_class()
    cdo = unreal.get_default_object(gen)
    print("=== BASE CLASS CDO properties ===")
    for name in ["ItemImage", "ItemNameText2", "ItemDescriptionRichText",
                  "ItemImageRef", "ItemNameRef", "ItemDescriptionRef",
                  "PlayerInventory", "PlayerController", "ItemTexture",
                  "ItemNameText", "ItemDescriptionText",
                  "WBP_InventorySimpleButton"]:
        try:
            v = cdo.get_editor_property(name)
            print(f"  {name}: {type(v).__name__} = {v}")
        except:
            pass

# Now try to instantiate the Horror widget and check its children
# Can't do this on CDO — need to check widget tree differently
# Let's check if there's a way to access FProperty list on generated class
horror_bp = unreal.load_asset("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror")
gen = horror_bp.generated_class()

# Use unreal.FProperty iteration if available
print("\n=== Horror generated class FProperty names ===")
# Try reflection to find all FObjectProperty that are widget types
cdo = unreal.get_default_object(gen)
# Get ALL attributes, including inherited, check for widget-like objects
all_attrs = dir(cdo)
widget_types = set()
for attr in sorted(all_attrs):
    if attr.startswith("_"):
        continue
    try:
        val = cdo.get_editor_property(attr)
        if val is None:
            # Could be a widget ref — just log the name
            widget_types.add(attr)
    except:
        pass

print("Properties that are None (potential widget refs):")
for w in sorted(widget_types):
    print(f"  {w}")
