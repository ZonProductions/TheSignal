import unreal

bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
cdo = unreal.get_default_object(bp.generated_class())

idle = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Idle/M_Neutral_Stand_Idle_Loop")
walk = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Loop_F")
run = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Run/M_Neutral_Run_Loop_F")

print(f"Idle: {idle}")
print(f"Walk: {walk}")
print(f"Run: {run}")

if idle:
    cdo.set_editor_property("IdleAnimation", idle)
if walk:
    cdo.set_editor_property("WalkAnimation", walk)
if run:
    cdo.set_editor_property("RunAnimation", run)

unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/BP_GraceCharacter")
print("Saved BP_GraceCharacter with locomotion anims")
