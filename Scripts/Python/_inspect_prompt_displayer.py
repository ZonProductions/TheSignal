import unreal

bp = unreal.load_asset("/Game/EasyGameUI/EasyInputPrompts/Core/WBP_EasyInputPromptDisplayer")
cls = bp.generated_class()
cdo = unreal.get_default_object(cls)
print("=== WBP_EasyInputPromptDisplayer instance-editable properties ===")
for prop in dir(cdo):
    if prop.startswith("__"): continue
    if any(k in prop.lower() for k in ["input","action","key","prompt","display","brand","device","fallback","any"]):
        try:
            v = cdo.get_editor_property(prop)
            print("   %s = %s" % (prop, v.get_name() if isinstance(v, unreal.Object) and v else v))
        except Exception:
            pass

print("\n=== BFL_EasyInputPromptsUtilities functions ===")
bfl = unreal.load_asset("/Game/EasyGameUI/EasyInputPrompts/Core/BFL_EasyInputPromptsUtilities")
if bfl:
    gc = bfl.generated_class()
    fns = unreal.PythonBPLibrary if False else None
    # list functions via the class's function names
    try:
        for f in gc.get_function_names() if hasattr(gc,"get_function_names") else []:
            print("   fn:", f)
    except Exception as e:
        print("  fn list err:", e)
