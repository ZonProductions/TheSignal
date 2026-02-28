# Checkpoint: GASP Phase 1 — Camera Fix (Session 3)

**Date:** 2026-02-28
**Status:** DIAGNOSING — spine rotation investigation, reference comparison pending

---

## Context

Continues from `gasp_phase1_camera_fix2_20260228.md`. Previous session found UpdatePeek/BaseCameraZ override as root cause of camera Z=64 issue and fixed it. This session investigated why the gun disappears when looking up (camera pitches but skeleton doesn't follow).

---

## Investigation: AnimBP Aim Pipeline (Fully Traced)

### AnimBP Variables — CONFIRMED WORKING AT RUNTIME
Via Python PIE debug script (`debug_aim_offset.py`):
- **ABP_TacticalShooter_UE5 Pitch = -18.3** (correctly computed from ControlRotation vs ActorRotation)
- **Character = BP_GraceCharacter_C_0** (TryGetPawnOwner works correctly)
- **ShooterComponent = AC_TacticalShooterAnimation_C** (found via GetComponentByClass)
- **InheritedMesh (CharacterMesh0): SKM=None, AnimInst=NULL** (empty, as expected)

### AnimBP Architecture
```
ABP_TacticalShooter_UE5 (parent, on PlayerMesh)
  ├── EventGraph: BlueprintUpdateAnimation → GetControlRotation/GetActorRotation → NormalizedDeltaRotator → SET Pitch
  ├── EventGraph: BlueprintInitializeAnimation → TryGetPawnOwner → SET Character → DelayNextTick → GetComponentByClass → SET ShooterComponent
  ├── AnimGraph:
  │   ├── Grounded/InAir state machines (locomotion)
  │   ├── UE5_ABP_IK_Main (Linked AnimGraph) ← SPINE ROTATION HERE
  │   ├── UE5_ABP_IK_Ads (Linked AnimGraph)
  │   ├── ABP_UE5_Sway (Linked AnimGraph)
  │   ├── UE5_ABP_IK_Pose (Linked AnimGraph)
  │   ├── Transform Modify Bone: index_02_r, index_03_r (trigger finger)
  │   └── Copy Bone, Layered Blend Per Bone, etc.
```

### UE5_ABP_IK_Main (Key Linked AnimBP)
**Variables:** IkWeight, Pitch, Character, TacticalShooterComponent, HideHeadMesh, RecoilComponent, MovementWeight, LeanAngle, MaxLeanAngle, Velocity, IdleIntensity, IkMotionPlayer

**EventGraph:** Independently computes Pitch via TryGetPawnOwner → GetControlRotation/GetActorRotation → NormalizedDeltaRotator. Also discovers TacticalShooterComponent, RecoilComponent, IkMotionPlayer via GetComponentByClass.

**AnimGraph — "Step 5: Rotate spine bones":**
- Transform (Modify) Bone on spine_01, spine_02, spine_03 (with Pitch divided among them)
- Additional spine modifications on spine_01-05 for leaning (Step 3)
- Two Bone IK on hand_r, hand_l (Step 6)
- Transform Modify Bone on VB ik_hand_gun (curve-based, Step 4)

### AC_TacticalShooterAnimation
**Does NOT read mesh or AnimInstance.** Only computes sway values (MoveSway, AimSway, WeaponSway) and stores them as component variables. The AnimBP reads these via ShooterComponent reference.

---

## Camera Height Investigation

### Runtime Positions (at Pitch=-18.3°, from `debug_socket_rotation.py`)
```
Camera WorldLoc:      Z=184.9  (capsule center + CameraHeightOffset 64)
FPCamera Socket:      Z=176.2  (on skeleton neck_02)
ik_hand_gun (weapon): Z=176.3
spine_02:             Z=131.5
Delta camera-gun:     8.6 units (camera ABOVE gun)
```

### Reference vs Our Setup
| | Reference (BP_TacticalShooterCharacter) | Ours (BP_GraceCharacter) |
|---|---|---|
| Camera position | (0,0,0) relative to capsule = capsule center | Z=64 relative = CameraHeightOffset from DataAsset |
| Camera vs FPCamera socket | Camera BELOW socket (~80 units) | Camera ABOVE socket (~9 units) |
| Gun visibility looking up | Unknown (needs testing) | Disappears at ~60° up pitch |

### Test: Camera at Z=0 (matching reference)
- Disabled ZP_GraceGameplayComponent tick, set camera Z=0 via Python
- **Result: Same issue** — gun still disappears when looking up
- BUT: camera at waist level causes looking down to see inside body mesh (K002 body clipping)
- Looking forward/up: gun IS visible and properly positioned
- **Conclusion: CameraHeightOffset is NOT the root cause** — spine rotation is insufficient regardless of camera height

---

## Captures Analysis

### 20260228_141549 (Camera Z=0 test)
- Frames 0000-0004: Looking down → camera inside body mesh (K002 body clipping from waist-level camera)
- Frame 0005: Looking forward → gun visible, properly positioned ✓
- Frame 0006: Looking slightly up → gun still visible ✓

### 20260228_141921 (Camera Z=64, "better for looking up")
- Frame 0000: Forward (~15°) → gun perfect ✓
- Frame 0001: Moderate up (~35°) → gun at bottom, partially visible ✓
- Frame 0002: Steep up (~60°) → gun GONE, dark body mesh visible at top ✗
- Frame 0003: Extreme up (~85°) → just sky, gun completely gone ✗

---

## Current Hypothesis

The spine rotation in UE5_ABP_IK_Main IS being applied (gun stays visible at moderate angles) but has insufficient range — the gun disappears too early (~60° up). This could be:
1. **Normal Kinemation behavior** — reference character has same limitation
2. **Spine rotation settings** — ActiveSettings from weapon DataAsset may clamp rotation range
3. **Linked AnimBP not fully initialized** — Pitch or Character might be null in the linked instance

### Pending Verification
**GM_TheSignal DefaultPawnClass switched to BP_TacticalShooterCharacter** for direct comparison. User needs to start PIE and look up at steep angles to see if the reference has the same issue.

---

## Files Modified This Session

| File | Change |
|------|--------|
| `GM_TheSignal` (via MCP) | DefaultPawnClass → BP_TacticalShooterCharacter (FOR TESTING — must revert) |

## Debug Scripts Created
| Script | Purpose |
|--------|---------|
| `Scripts/Python/debug_aim_offset.py` | Queries AnimBP Pitch, Character, ShooterComponent at runtime |
| `Scripts/Python/debug_socket_rotation.py` | Queries FPCamera/gun/spine socket positions at runtime |
| `Scripts/Python/fix_camera_height.py` | Forces camera Z=0 + disables gameplay tick for testing |
| `Scripts/Python/debug_linked_abp.py` | Queries linked AnimBP instances (not yet run — PIE ended) |

---

## State That Must Be Reverted
- **GM_TheSignal DefaultPawnClass** was changed to BP_TacticalShooterCharacter for comparison testing. MUST revert to BP_GraceCharacter after test.

## Next Steps
1. **PIE with BP_TacticalShooterCharacter** — compare gun visibility at steep upward angles
2. If reference has same behavior → accept as Kinemation limitation, adjust pitch clamp
3. If reference keeps gun at steep angles → investigate what's different:
   - Run `debug_linked_abp.py` on BOTH characters to compare linked AnimBP variable values
   - Check if reference has different ViewPitchMin/Max on PlayerController
   - Check if ActiveSettings (weapon config) affects spine rotation range
4. **Revert GM_TheSignal** to BP_GraceCharacter after comparison
5. Update checkpoint with comparison results

## Build Info
- Branch: main
- No C++ changes this session — all investigation via MCP + Python
- PIE testing with runtime Python scripts (no rebuild needed)
- Editor running with latest build from previous session
