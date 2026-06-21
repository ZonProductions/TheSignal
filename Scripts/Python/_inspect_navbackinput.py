import unreal

bp = unreal.load_asset("/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SavesManagerUI")
cls = bp.generated_class()
cdo = unreal.get_default_object(cls)

w = None
for nm in ["NavBackInput", "NavBackInput_", "WBP_NavBackInput"]:
    try:
        w = cdo.get_editor_property(nm)
        if w:
            print("Found widget property:", nm)
            break
    except Exception:
        pass

if not w:
    # list all object properties that look like input/nav widgets
    print("NavBackInput not a direct property; scanning CDO props...")
    for prop in dir(cdo):
        if "nav" in prop.lower() or "back" in prop.lower() or "input" in prop.lower():
            try:
                v = cdo.get_editor_property(prop)
                print("  ", prop, "=", type(v).__name__, v.get_name() if isinstance(v, unreal.Object) and v else v)
            except Exception:
                pass
else:
    print("NavBackInput class:", type(w).__name__)
    for prop in dir(w):
        if prop.startswith("__"): continue
        if any(k in prop.lower() for k in ["action","input","key","text","icon","brush","display","type","any","gamepad"]):
            try:
                v = w.get_editor_property(prop)
                if v not in (None, ""):
                    print("   %s = %s" % (prop, v.get_name() if isinstance(v, unreal.Object) and v else v))
            except Exception:
                pass
