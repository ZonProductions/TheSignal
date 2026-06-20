import unreal

def show(label, path, prop="post_process_anim_blueprint"):
    a = unreal.load_asset(path)
    print("=====", label, "=====")
    if not a:
        print("  NOT FOUND:", path)
        return a
    print("  asset:", a.get_name(), "class:", a.get_class().get_name())
    return a

mesh = show("MeleeViewMesh asset", "/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono.SKM_Operator_Mono")
if mesh:
    skel = mesh.get_editor_property("skeleton")
    print("  mesh skeleton:", skel.get_name() if skel else None)
    try:
        pp = mesh.get_editor_property("post_process_anim_blueprint")
        print("  post_process_anim_blueprint:", pp)
    except Exception as e:
        print("  pp err:", e)

abp = show("Hands ABP", "/Game/Marcus/ABP_MarcusMeleeHands")
if abp:
    try:
        ts = abp.get_editor_property("target_skeleton")
        print("  hands ABP target_skeleton:", ts.get_name() if ts else None)
    except Exception as e:
        print("  ts err:", e)
    try:
        pc = abp.get_editor_property("parent_class")
        print("  hands ABP parent_class:", pc)
    except Exception as e:
        print("  pc err:", e)

# Grace BP defaults for the hand offsets
bp = unreal.load_asset("/Game/Blueprints/BP_GraceCharacter")
print("===== Grace BP defaults =====")
if bp:
    cdo = unreal.get_default_object(bp.generated_class()) if hasattr(bp, "generated_class") else None
    if cdo:
        for p in ["MeleeHandLOffset","MeleeHandROffset","BlockHandLOffset","BlockHandROffset","MeleeViewOffset","BlockViewOffset","MeleeHandMaterial"]:
            try:
                print("  ", p, "=", cdo.get_editor_property(p))
            except Exception as e:
                print("  ", p, "ERR", e)
else:
    print("  BP_GraceCharacter not found at /Game/Blueprints/")
