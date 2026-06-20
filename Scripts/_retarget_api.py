import unreal, inspect
op = unreal.IKRetargetBatchOperation
for m in ["duplicate_and_retarget","run_batch_retarget"]:
    f = getattr(op, m, None)
    print("---", m)
    try: print(inspect.signature(f))
    except Exception as e: print("sig err", e)
    print((f.__doc__ or "")[:600])
# IK rigs
for p in ["/Game/CharacterCustomizer/Characters/CCMH/Animation/MH_IK_Rig",
          "/Game/Characters/Mannequins/Rigs/IK_Mannequin"]:
    a = unreal.load_asset(p)
    print("RIG", p.split("/")[-1], "->", a.get_class().get_name() if a else "MISSING")
    if a:
        try:
            sk = a.get_preview_mesh() if hasattr(a,"get_preview_mesh") else None
            print("  preview mesh:", sk.get_name() if sk else None)
        except Exception as e: print("  ", e)
