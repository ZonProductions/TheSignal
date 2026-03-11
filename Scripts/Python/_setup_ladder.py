"""
Setup Ladder System — Assigns ladder animations to BP_GraceCharacter CDO
and creates BP_Ladder child of AZP_Ladder.
Run via MCP Python endpoint.
"""
import unreal

# ============================================================
# 1. Assign ladder animations on BP_GraceCharacter CDO
# ============================================================

grace_bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
if not grace_bp:
    raise RuntimeError("BP_GraceCharacter not found!")

cdo = unreal.get_default_object(grace_bp.generated_class())

# Animation paths from Advanced Ladder Climbing System pack
climb_up_path   = "/Game/LadderClimbingSystem/Animations/Ladder/Loop/ClimbUp/A_ClimbUp_Loop_UE5"
climb_down_path = "/Game/LadderClimbingSystem/Animations/Ladder/Loop/ClimbDown/A_ClimbDownLoop_UE5"
idle_path       = "/Game/LadderClimbingSystem/Animations/Ladder/Idle/A_LadderIdleL_UE5"

climb_up_anim   = unreal.load_asset(climb_up_path)
climb_down_anim = unreal.load_asset(climb_down_path)
idle_anim       = unreal.load_asset(idle_path)

if not climb_up_anim:
    raise RuntimeError(f"Climb up anim not found: {climb_up_path}")
if not climb_down_anim:
    raise RuntimeError(f"Climb down anim not found: {climb_down_path}")
if not idle_anim:
    raise RuntimeError(f"Idle anim not found: {idle_path}")

cdo.set_editor_property("LadderClimbUpAnimation", climb_up_anim)
cdo.set_editor_property("LadderClimbDownAnimation", climb_down_anim)
cdo.set_editor_property("LadderIdleAnimation", idle_anim)

# Save BP_GraceCharacter
grace_pkg = grace_bp.get_outer()
unreal.EditorAssetLibrary.save_asset(grace_pkg.get_path_name())

print(f"[Ladder] Assigned climb up: {climb_up_anim.get_name()}")
print(f"[Ladder] Assigned climb down: {climb_down_anim.get_name()}")
print(f"[Ladder] Assigned idle: {idle_anim.get_name()}")
print("[Ladder] BP_GraceCharacter saved.")
print("[Ladder] DONE — animation assignment complete.")
