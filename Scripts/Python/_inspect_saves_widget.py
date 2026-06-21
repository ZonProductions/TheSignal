import unreal

w = unreal.load_asset("/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SavesManagerUI")
if not w:
    print("widget asset not loaded")
else:
    gen = w.generated_class() if hasattr(w, "generated_class") else None
    cls = gen if gen else w
    print("class:", cls.get_name())
    # Interfaces
    try:
        bp = w
        ifaces = bp.get_editor_property("implemented_interfaces")
        print("interfaces:", [str(i) for i in ifaces])
    except Exception as e:
        print("iface read err:", e)
    # Functions / events on the generated class
    names = []
    try:
        for f in unreal.get_type_from_class(cls).__dir__() if False else []:
            pass
    except Exception:
        pass
    # Use reflection: list UFunctions
    try:
        funcs = []
        it = cls
        # default object
        cdo = unreal.get_default_object(cls) if hasattr(unreal, "get_default_object") else None
        print("CDO:", cdo)
    except Exception as e:
        print("cdo err:", e)
    # Search for focus/nav/activate functions by name via find_function on CDO
    cdo = unreal.get_default_object(cls)
    for fname in ["InitWidget","SetFocusToFirst","ActivateWidget","OnActivated","SetupNavigation",
                  "FocusFirstSlot","SetKeyboardFocus","NavigateToFirst","BPI_InputNavigation"]:
        fn = cdo.find_function(unreal.Name(fname)) if hasattr(cdo,"find_function") else None
        print("  has %-22s : %s" % (fname, "YES" if fn else "no"))
