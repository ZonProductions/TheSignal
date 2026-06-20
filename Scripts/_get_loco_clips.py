import unreal
bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
cdo = unreal.get_default_object(bp.generated_class())
for p in ["Idle Animation","Walk Animation","Run Animation","Crouch Idle Animation","Crouch Walk Animation",
          "IdleAnimation","WalkAnimation","RunAnimation","CrouchIdleAnimation","CrouchWalkAnimation"]:
    try:
        v = cdo.get_editor_property(p)
        if v: print(p, "=>", v.get_path_name())
    except Exception: pass
