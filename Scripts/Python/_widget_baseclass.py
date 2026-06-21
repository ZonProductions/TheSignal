import unreal

for p in ["/Game/InventorySystemPro/Blueprints/UI/SubWidgets/WBP_FirstTimePickupNotificationBase",
          "/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror",
          "/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SavesManagerUI"]:
    bp = unreal.load_asset(p)
    if not bp:
        print("MISSING", p); continue
    chain = []
    pc = bp.get_editor_property("parent_class")
    while pc:
        chain.append(pc.get_name())
        try:
            pc = pc.get_super_class()
        except Exception:
            # native UClass: use get_outer? fall back to stop
            try:
                pc = unreal.MathLibrary  # dummy
                break
            except Exception:
                break
    print(p.split("/")[-1], "parent:", bp.get_editor_property("parent_class").get_name())
