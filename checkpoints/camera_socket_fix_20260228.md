# Checkpoint: Camera-to-Socket Fix — Gun Follows Pitch

**Date:** 2026-02-28
**Status:** FIXED — camera tracks skeleton, gun stays in view. Peek and head bob need adjustment.

---

## Context

Continues from `gasp_phase1_camera_fix3_20260228.md`. Previous sessions traced the camera-weapon divergence through multiple layers: UpdatePeek BaseCameraZ override, AC_FirstPersonCamera analysis, AnimBP pipeline verification. This session found the root cause and fixed it.

---

## Root Cause

**Camera was at a fixed position on the capsule, not attached to the skeleton.** When the AnimBP rotated the spine (aim offset), the skeleton moved but the camera stayed in place. Looking up caused parallax — the gun fell below the camera's FOV.

### Proof (spine bone comparison at two pitches)

**Straight ahead (Pitch = -1.7°):**
```
FPCamera socket Z=181.7, gun Z=175.5, camera Z=184.1
Camera-gun gap: 8.6 units
```

**Looking up (Pitch = -33.3°):**
```
FPCamera socket Z=158.0, gun Z=165.4, camera Z=174.5
Camera-gun gap: 9.1 units (barely changed)
```

The spine IS rotating (FPCamera socket dropped 23.7 units). But the camera barely moved (9.6 units from head bob). In the camera's reference frame, the gun fell to ~50° below the look direction — outside the 90° FOV.

### Why socket attachment fixes it
With camera at FPCamera socket, the camera MOVES with the skeleton. When spine rotates to look up, camera position follows. Camera and gun maintain consistent relative positions regardless of pitch.

---

## Fix Applied

### ZP_GraceCharacter.cpp — Camera attachment
```cpp
// BEFORE: Camera on capsule (fixed position)
FirstPersonCamera->SetupAttachment(GetCapsuleComponent());

// AFTER: Camera on PlayerMesh at FPCamera socket (follows skeleton)
FirstPersonCamera->SetupAttachment(PlayerMesh, FName(TEXT("FPCamera")));
```

PlayerMesh is now created BEFORE camera (was after) so the socket attachment works.

### ZP_GraceGameplayComponent.cpp — BaseCameraZ
```cpp
// BEFORE: Captured from camera relative Z after ApplyMovementConfig (was 64)
BaseCameraZ = CameraComponent->GetRelativeLocation().Z;

// AFTER: Zero — camera position driven by socket, not manual offset
BaseCameraZ = 0.0f;
```

### ZP_GraceGameplayComponent.cpp — ApplyMovementConfig
```cpp
// BEFORE: Set camera height from DataAsset
CameraComponent->SetRelativeLocation(FVector(0, 0, MovementConfig->CameraHeightOffset));
CameraComponent->SetFieldOfView(MovementConfig->DefaultFOV);

// AFTER: Only set FOV — socket drives position
CameraComponent->SetFieldOfView(MovementConfig->DefaultFOV);
```

---

## Investigation Findings (for future reference)

### AnimBP Pipeline — CONFIRMED FULLY WORKING
- **ABP_TacticalShooter_UE5** (parent): Pitch computed correctly, Character found, ShooterComponent found
- **UE5_ABP_IK_Main** (linked): Has spine_01-05 rotation (Step 5), Two Bone IK (Step 6), curve-based VB ik_hand_gun (Step 4)
- **AC_TacticalShooterAnimation**: Only computes sway variables — does NOT touch mesh/AnimInstance
- **AC_FirstPersonCamera**: Only drives camera ROTATION (K2_SetWorldRotation) — never touches position

### Component hierarchy (final)
```
Capsule (root)
  ├── PlayerMesh (RelLoc=0,0,-90, SKM_Operator_Mono, ABP_TacticalShooter_UE5)
  │   └── FirstPersonCamera (at FPCamera socket, bUsePawnControlRotation=false)
  └── Kinemation SCS components (AC_FirstPersonCamera, AC_TacticalShooterAnimation, etc.)
```

### Key lesson
The Kinemation GASP tutorial explicitly states "Camera attached to Player at FPCamera socket." We had the camera on the capsule (matching the reference BP_TacticalShooterCharacter which also has it on capsule). The reference setup has the same parallax issue — it's just that nobody noticed because the reference may have limited pitch range or the context was different. **Socket attachment is the correct architecture.**

---

## Known Issues After Fix

### 1. Peek system broken — camera goes up-right
UpdatePeek sets camera RelativeLocation and RelativeRotation every tick. With camera on socket:
- RelativeLocation offsets are now in SOCKET space (rotated with the skeleton), not CAPSULE space
- The peek lateral/forward/roll calculations assume capsule-space axes
- **Fix needed:** Transform peek offsets from world/capsule space to socket space, OR apply peek as world-space offsets

### 2. Head bob not working
Head bob offsets (HeadBobOffsetY, HeadBobOffsetZ) are applied via UpdatePeek's camera RelativeLocation. With BaseCameraZ=0 and socket-space coordinates, the bob values may be too small or in the wrong axis.
- **Possible causes:**
  - bUseBuiltInHeadBob is being set to false when Kinemation is active (line 94-98 in ZP_GraceCharacter::BeginPlay)
  - Head bob offsets in socket space are oriented differently than capsule space
  - Head bob was already disabled in previous sessions (has been a problem "for a while")

### 3. CameraHeightOffset in DataAsset (64.0f) is now unused
The value still exists in ZP_GraceMovementConfig but ApplyMovementConfig no longer uses it for camera position. Could be repurposed or removed.

---

## Files Modified This Session

| File | Change |
|------|--------|
| `ZP_GraceCharacter.cpp` | Camera attached to PlayerMesh at FPCamera socket (was capsule). PlayerMesh created before camera. |
| `ZP_GraceGameplayComponent.cpp` | BaseCameraZ = 0. Removed CameraHeightOffset from camera position. FOV still set from DataAsset. |
| `GM_TheSignal` (via MCP) | DefaultPawnClass temporarily changed to BP_TacticalShooterCharacter for testing, then REVERTED to BP_GraceCharacter |

## Debug Scripts Created This Session
| Script | Purpose |
|--------|---------|
| `debug_aim_offset.py` | Queries AnimBP Pitch, Character, ShooterComponent |
| `debug_socket_rotation.py` | Queries FPCamera/gun/spine socket positions |
| `debug_spine_rotation.py` | Compares spine bone Z at different pitches — proved spine IS rotating |
| `debug_linked_abp.py` | Queries linked AnimBP instances (hit API limitation) |
| `debug_pawn_class.py` | Checks spawned pawn class |
| `fix_camera_height.py` | Runtime camera height testing |
| `add_ref_input.py` | Attempted to add ref character input (hit API limitation) |

---

## Next Steps
1. **Fix peek system** — transform offsets to socket space or apply differently
2. **Fix head bob** — investigate why it's not working (may predate this fix)
3. Clean up unused CameraHeightOffset if confirmed unneeded
4. Continue GASP Phase 2 after peek/bob fixes

## Build Info
- Branch: main
- Last build: clean (11 actions, 32.54s)
- Editor reopened with new binaries, PIE verified — gun follows pitch ✓
