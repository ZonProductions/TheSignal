import unreal

for variant in ["RPG", "Horror"]:
    if variant == "RPG":
        path = "/Game/InventorySystemPro/ExampleContent/RPG/UI/Widgets/WBP_FirstTimePickupNotification_RPG"
    else:
        path = "/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror"

    bp = unreal.load_asset(path)
    if not bp:
        print(f"\n=== {variant}: COULD NOT LOAD ===")
        continue

    gen = bp.generated_class()
    cdo = unreal.get_default_object(gen)

    print(f"\n=== {variant} ===")
    print(f"Class: {gen.get_name()}")

    # Check all properties on the CDO
    for name in ["ItemImage", "ItemNameText2", "ItemDescriptionRichText",
                  "ItemNameText", "ItemDescriptionText",
                  "ItemImageRef", "ItemNameRef", "ItemDescriptionRef",
                  "PlayerInventory", "PlayerController",
                  "ItemTexture", "ItemName", "ItemDescription"]:
        try:
            v = cdo.get_editor_property(name)
            print(f"  {name}: {v} ({type(v).__name__})")
        except:
            pass  # silently skip non-existent

    # Check for any widget-type variables
    print(f"  --- All widget-like props ---")
    for p in sorted(set(dir(cdo))):
        if p.startswith("_") or p.startswith("get_") or p.startswith("set_") or p.startswith("on_"):
            continue
        try:
            v = cdo.get_editor_property(p)
            tname = type(v).__name__
            if tname in ["Image", "CommonTextBlock", "CommonRichTextBlock", "TextBlock", "Button", "CommonButtonBase", "NoneType"]:
                if v is None and tname == "NoneType":
                    continue  # skip None values that might just be uninitialized
                print(f"  {p}: {tname}")
        except:
            pass
