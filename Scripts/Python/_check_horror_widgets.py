import unreal

bp = unreal.load_asset("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror")
gen = bp.generated_class()
cdo = unreal.get_default_object(gen)

for name in ["ItemImage", "ItemNameText2", "ItemDescriptionRichText", "ItemDescriptionText", "ItemNameText", "WBP_InventorySimpleButton_Horror", "WBP_InventorySimpleButton"]:
    try:
        w = cdo.get_editor_property(name)
        print(f"{name}: {w} ({type(w).__name__})")
    except:
        print(f"{name}: NOT FOUND")

# Also list all widget-like properties
print("\n=== All properties ===")
for p in dir(cdo):
    if p.startswith("_"):
        continue
    if any(x in p.lower() for x in ["item", "text", "image", "button", "description", "name", "scroll", "notification"]):
        print(f"  {p}")
