import unreal

bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
cdo = unreal.get_default_object(bp.generated_class())

crouch_idle = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Idle/M_Neutral_Crouch_Idle_Loop")
crouch_walk = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Crouch_WIP/M_Neutral_Crouch_Loop_F")

print(f"CrouchIdle: {crouch_idle}")
print(f"CrouchWalk: {crouch_walk}")

if crouch_idle:
    cdo.set_editor_property("CrouchIdleAnimation", crouch_idle)
if crouch_walk:
    cdo.set_editor_property("CrouchWalkAnimation", crouch_walk)

unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/BP_GraceCharacter")
print("Saved BP_GraceCharacter with crouch anims")
