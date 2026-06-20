import unreal

def info(path):
    a = unreal.load_asset(path)
    if not a:
        print("MISSING:", path); return
    add = None
    base = None
    try:
        add = a.get_editor_property("additive_anim_type")
    except Exception as e:
        add = "err:%s" % e
    try:
        base = a.get_editor_property("ref_pose_type")
    except Exception as e:
        base = "err:%s" % e
    skel = None
    try:
        skel = a.get_editor_property("skeleton")
    except Exception:
        pass
    print("%-44s additive=%s ref_pose_type=%s skel=%s" % (
        a.get_name(), add, base, skel.get_name() if skel else None))

for p in [
    "/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle.A_MeleePipe_Idle",
    "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockLoop.FPP_Longs_BlockLoop",
    "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockStart.FPP_Longs_BlockStart",
    "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockStop.FPP_Longs_BlockStop",
]:
    info(p)
