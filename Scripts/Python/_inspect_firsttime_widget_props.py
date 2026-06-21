import unreal

paths = [
    "/Game/InventorySystemPro/Blueprints/UI/SubWidgets/WBP_FirstTimePickupNotificationBase",
    "/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror",
]
for p in paths:
    bp = unreal.load_asset(p)
    if not bp:
        print("MISSING", p); continue
    cls = bp.generated_class()
    cdo = unreal.get_default_object(cls)
    print("\n=== %s ===" % p)
    # Iterate properties looking for InputAction / Key / soft refs
    for prop in dir(cdo):
        if prop.startswith("__"):
            continue
        try:
            val = cdo.get_editor_property(prop)
        except Exception:
            continue
        tn = type(val).__name__
        if "InputAction" in tn or "Key" in tn or (isinstance(val, unreal.Object) and val and "InputAction" in type(val).__name__):
            print("   %s : %s = %s" % (prop, tn, val.get_name() if isinstance(val, unreal.Object) and val else val))
