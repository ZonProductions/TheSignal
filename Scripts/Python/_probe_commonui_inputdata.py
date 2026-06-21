import unreal

def handle_str(h):
    try:
        dt = h.get_editor_property("data_table"); rn = h.get_editor_property("row_name")
        return "table=%s row=%s" % (dt.get_name() if dt else None, rn)
    except Exception as e:
        return "ERR:%s" % e

# 1) Moonville InputData (the one CommonInputSettings points at) — direct prop reads.
bp = unreal.load_asset("/Game/InventorySystemPro/Blueprints/Input/Common/BP_InventoryUIInputData")
cdo = unreal.get_default_object(bp.generated_class())
print("=== BP_InventoryUIInputData (CommonInputSettings.InputData) ===")
for p in ["default_click_action","default_back_action","enhanced_input_click_action","enhanced_input_back_action"]:
    try:
        v = cdo.get_editor_property(p)
        if isinstance(v, unreal.Object):
            print("  %-30s = %s" % (p, v.get_name() if v else "None"))
        else:
            print("  %-30s = %s" % (p, handle_str(v)))
    except Exception as e:
        print("  %-30s : (no prop) %s" % (p, e))

# 2) Find EGUI's own CommonUIInputData (it should ship one with click/back configured).
print("\n=== InputData-like assets project-wide ===")
hits = unreal.EditorAssetLibrary.list_assets("/Game", recursive=True, include_folder=False)
for a in hits:
    nm = a.split("/")[-1]
    if "InputData" in nm and "BACKUP" not in a:
        print("  ", a)

# 3) Are the problem widgets CommonActivatableWidgets? (need a stack/activation for focus+input)
print("\n=== widget base classes ===")
for p in ["/Game/InventorySystemPro/Blueprints/UI/SubWidgets/WBP_FirstTimePickupNotificationBase",
          "/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SavesManagerUI"]:
    w = unreal.load_asset(p)
    if not w: print("  MISSING", p); continue
    cls = w.generated_class()
    # walk up to find native base
    base = cls
    chain = []
    while base:
        chain.append(base.get_name())
        base = base.get_super_class()
        if len(chain) > 8: break
    print("  %s\n     %s" % (p.split("/")[-1], " <- ".join(chain)))
