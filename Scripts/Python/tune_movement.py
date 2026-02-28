"""
Movement tuning values for DA_GraceMovement_Default.
Edit the numbers below, then run via MCP or UE Python console to apply.

Usage: exec(open('Scripts/Python/tune_movement.py').read())
"""
import unreal

# ============================================================
#  EDIT THESE VALUES
# ============================================================

TUNING = {
    # --- Walk / Sprint ---
    'WalkSpeed': 200.0,
    'SprintSpeed': 350.0,
    'BrakingDeceleration': 1400.0,
    'MaxAcceleration': 1200.0,
    'GroundFriction': 6.0,

    # --- Stamina ---
    'MaxStamina': 100.0,
    'StaminaDrainRate': 20.0,
    'StaminaRegenRate': 15.0,
    'StaminaRegenDelay': 1.5,

    # --- Head Bob ---
    'HeadBobFrequency': 1.6,
    'HeadBobVerticalAmplitude': 0.8,       # cm - vertical nod per step
    'HeadBobHorizontalAmplitude': 0.4,     # cm - side-to-side sway
    'SprintBobFrequencyMultiplier': 1.4,
    'SprintBobAmplitudeMultiplier': 1.5,
    'HeadBobReturnSpeed': 6.0,

    # --- Camera ---
    'CameraHeightOffset': 64.0,            # unused (socket drives position)
    'DefaultFOV': 90.0,

    # --- Peek ---
    'PeekLateralOffset': 25.0,             # cm - how far to lean
    'PeekForwardOffset': 8.0,              # cm - head past corner
    'PeekRollAngle': 3.0,                  # degrees - lean tilt
    'PeekInterpSpeed': 8.0,
    'PeekReturnInterpSpeed': 6.0,
    'PeekWallDetectionRange': 180.0,
    'PeekTraceRadius': 12.0,
    'PeekWallHitThreshold': 2,
    'PeekTraceFanHalfAngle': 75.0,
    'HeadBobPeekDamping': 0.15,
    'HeadBobPeekVerticalDamping': 0.5,
    'PeekMaxWallAngleFromVertical': 20.0,

    # --- Interaction ---
    'InteractionTraceRange': 250.0,

    # --- Jump ---
    'JumpZVelocity': 300.0,
    'AirControl': 0.15,

    # --- Camera Sway ---
    'IdleSwayFrequency': 0.4,
    'IdleSwayAmplitude': 0.3,
}

# ============================================================
#  APPLY (don't edit below)
# ============================================================

DA_PATH = '/Game/Core/Data/DA_GraceMovement_Default'
da = unreal.load_asset(DA_PATH)
if not da:
    raise RuntimeError(f'Failed to load {DA_PATH}')

for prop, value in TUNING.items():
    try:
        da.set_editor_property(prop, value)
    except Exception as e:
        unreal.log_warning(f'Skipped {prop}: {e}')

unreal.EditorAssetLibrary.save_asset(DA_PATH)
unreal.log(f'Applied {len(TUNING)} tuning values to {DA_PATH}')
