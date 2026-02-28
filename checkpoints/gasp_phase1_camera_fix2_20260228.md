# Checkpoint: GASP Phase 1 — Camera Fix (Session 2)

**Date:** 2026-02-28
**Status:** FIX APPLIED — root cause found and fixed, awaiting PIE verification

---

## Context

Continues from `gasp_phase1_camera_fix_20260228.md`. Previous session identified camera-weapon divergence (weapon at far-left) after 3 fix attempts. This session performed systematic debugging to find the actual root cause.

---

## Root Cause Chain (fully traced)

### Problem
Camera stays at Z=64 above capsule center despite C++ constructor and PostInitializeComponents both setting Z=0. Arms/weapon invisible or at wrong position.

### Investigation: AC_FirstPersonCamera Analysis
- Examined ALL graphs in AC_FirstPersonCamera via MCP (`get_blueprint`, `describe_graph`)
- **AC_FirstPersonCamera only calls `K2_SetWorldRotation` on the camera — NEVER sets position**
- No `SetWorldLocation`, `SetRelativeLocation`, or any position-related call in any of its 10 graphs
- Parent class: base `ActorComponent` — no C++ tick logic
- Reference BP_TacticalShooterCharacter: Camera at root (0,0,0), Event Tick is EMPTY

### Investigation: Component Tick Bisection
Disabled all component ticks via Python during PIE, forced camera Z=0, then re-enabled one by one:
- ALL ticks disabled → **Z stays at 0** ✓
- CharacterMovementComponent only → Z stays at 0 ✓
- CameraComponent only → Z stays at 0 ✓
- All 4 Kinemation components → Z stays at 0 ✓
- **ZP_GraceGameplayComponent only → Z reverts to 64!** ← CULPRIT

### Root Cause
`ZP_GraceGameplayComponent::UpdatePeek()` (line 401) sets camera RelativeLocation every tick:
```cpp
FinalCameraLoc.Z = BaseCameraZ + HeadBobOffsetZ;
CameraComponent->SetRelativeLocation(FinalCameraLoc);
```

`BaseCameraZ` is captured in BeginPlay (line 56):
```cpp
BaseCameraZ = CameraComponent->GetRelativeLocation().Z;  // captures 64!
```

At BeginPlay time, the camera still has the stale BP serialized override Z=64 (from old constructor code). `BaseCameraZ` captures 64 and re-applies it every tick via UpdatePeek, overriding any runtime zeroing attempts.

### Why previous fix attempts failed
- All 3 socket-based fixes (Attempts 2-4) had camera at FPCamera socket AND BaseCameraZ=64 being applied every tick
- PostInitializeComponents zeroing worked (confirmed Z=0 at that point) but was overridden by UpdatePeek starting from BeginPlay
- Python runtime zeroing worked momentarily but reverted within 1 frame due to UpdatePeek tick

---

## Fix Applied

### ZP_GraceGameplayComponent.cpp (line 54-57)
```cpp
// BEFORE:
BaseCameraZ = CameraComponent->GetRelativeLocation().Z;  // captures stale 64

// AFTER:
BaseCameraZ = 0.0f;  // Camera at capsule center (matching reference)
```

### ZP_GraceCharacter.cpp constructor
Camera attached to capsule root (not FPCamera socket), matching reference:
```
Capsule (root)
  ├── FirstPersonCamera (RelLoc=0,0,0, bUsePawnControlRotation=false)
  └── PlayerMesh (RelLoc=0,0,-90, Yaw=-90, SKM_Operator_Mono + ABP_TacticalShooter_UE5)
```

### ZP_GraceCharacter.cpp PostInitializeComponents
Added camera RelLoc/RelRot zeroing (belt-and-suspenders for stale BP override).

### BP_GraceCharacter (via MCP Python)
- PlayerMesh RelLoc set to (0,0,-90) — was stale at Z=-152
- PlayerMesh Yaw=-90 confirmed correct
- Camera CDO was already Z=0 (C++ rebuild took effect)

---

## Key Findings (for future reference)

1. **AC_FirstPersonCamera ONLY drives rotation** — confirmed by exhaustive graph search. It reads FPCamera socket curves from AnimInstance and combines control rotation + recoil + freelook via quaternion math, then calls K2_SetWorldRotation. Position is never touched.

2. **Reference BP_TacticalShooterCharacter setup**: Camera at root (0,0,0), CharacterMesh0 at (0,0,-90) Yaw=-90, Event Tick EMPTY. AC_FirstPersonCamera handles all camera rotation.

3. **BP SCS component template overrides persist independently of C++ CDO** — changing constructor value and even setting via CDO Python doesn't clear the serialized BP override. The override is applied during actor construction but can be overridden in PostInitializeComponents.

4. **BaseEyeHeight = 64 is a red herring** — tested at runtime, not the source of the Z=64 camera offset.

---

## Files Modified This Session

| File | Change |
|------|--------|
| `ZP_GraceCharacter.h` | Updated PlayerMesh comment |
| `ZP_GraceCharacter.cpp` | Camera at capsule root (not socket), PostInitializeComponents zeroing, removed BeginPlay zeroing |
| `ZP_GraceGameplayComponent.cpp` | `BaseCameraZ = 0.0f` instead of reading stale BP value |
| `BP_GraceCharacter` (via MCP) | PlayerMesh RelLoc fixed to (0,0,-90) |

## Debug Scripts Created
| Script | Purpose |
|--------|---------|
| `Scripts/Python/debug_positions.py` | Queries all component positions, sockets, bones during PIE |
| `Scripts/Python/force_camera_zero.py` | Forces camera Z=0 at runtime to test revert behavior |
| `Scripts/Python/isolate_tick.py` | Disables AC_FirstPersonCamera tick to isolate |
| `Scripts/Python/isolate_all_ticks.py` | Disables ALL component ticks |
| `Scripts/Python/isolate_kinemation.py` | Re-enables only Kinemation ticks |
| `Scripts/Python/isolate_gameplay.py` | Re-enables only ZP_GraceGameplayComponent — found culprit |
| `Scripts/Python/isolate_bisect.py` | Generic single-component re-enable test |
| `Scripts/Python/test_base_eye_height.py` | Tests BaseEyeHeight as potential source |
| `Scripts/Python/clear_overrides2.py` | Clears stale BP component overrides via CDO |

---

## PIE Test Checklist (PENDING — build done, needs editor restart + PIE)

- [ ] Camera at capsule center (Z=0 relative, ~Z=112 world)
- [ ] Arms + TR15 visible and roughly centered
- [ ] Camera rotation follows mouse (AC_FirstPersonCamera driving)
- [ ] Looking up/down — weapon stays in view
- [ ] Fire (LMB), Reload (R), Sprint (Shift) functional
- [ ] Head bob works when walking (if bUseBuiltInHeadBob enabled)
- [ ] Peek (RMB near walls) functional

## Expected Height Comparison (from debug data)
```
Camera (fixed):   Z ≈ 112.8  (capsule center, Z=0 relative)
FPCamera socket:  Z ≈ 167.8  (neck_02 bone, ~55 above camera)
ik_hand_gun:      Z ≈ 153.2  (weapon, ~40 above camera)
PlayerMesh:       Z ≈  22.8  (capsule - 90)
```

Camera at capsule center, weapon ~40 units above — weapon should be visible in upper portion of screen. If weapon is too high, may need to adjust PlayerMesh Z or camera Z offset.

## Known Risks
- Camera at capsule center may be too low (waist height) — weapon could appear too high in view
- If weapon isn't centered, the reference architecture (camera at root) may require the AnimBP to pose arms at capsule-center height, which it may not do for our custom PlayerMesh vs inherited CharacterMesh0
- Peek system now uses BaseCameraZ=0, which is correct for the new camera setup
- Stale BP override on camera (Z=64) still exists in serialized BP — won't cause issues because PostInitializeComponents zeroes it and BaseCameraZ no longer reads it

## Next Steps
1. **PIE test** — verify camera position and weapon visibility
2. If weapon too high/low → adjust PlayerMesh Z offset
3. If weapon off-center → investigate AnimBP aim offset behavior on custom PlayerMesh vs CharacterMesh0
4. Consider switching to inherited CharacterMesh0 (like reference) if PlayerMesh continues causing issues
5. Update main checkpoint and session_prompt.txt after verification

## Build Info
- Branch: main
- Last build: clean (4 actions, 2.84s)
- Editor must be reopened for PIE test
