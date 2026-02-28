# Checkpoint: Peek/HeadBob Fix + Input Refactor

**Date:** 2026-02-28
**Status:** IN PROGRESS — built clean, awaiting PIE test after editor reopen.

---

## Context

Continues from `camera_socket_fix_20260228.md`. Camera is attached to PlayerMesh at FPCamera socket. Peek and head bob were broken after that change: peek offsets were in socket space (rotated with spine), head bob was disabled when Kinemation active.

---

## Changes Made This Session

### 1. Peek Coordinate Space Fix (TICKET-008)
**Root cause:** `UpdatePeek` applied lateral/forward/roll offsets as camera `SetRelativeLocation`. With camera on FPCamera socket, those offsets were in socket space (rotated with spine pitch), causing "up-right" drift instead of lateral lean.

**Fix (iteration 1):** Transform offsets from capsule space to socket space:
```cpp
FQuat DeltaRot = SocketWorldRot.Inverse() * CapsuleWorldRot;
FinalCameraLoc = DeltaRot.RotateVector(CapsuleSpaceOffset);
```

**Fix (iteration 2 — current):** Moved peek offset to PlayerMesh instead of camera. Gun, camera, and arms all lean together. Head bob stays camera-only.
- Peek → `CachedMeshComponent->SetRelativeLocation(CachedMeshBaseLocation + PeekOffset)`
- Bob → `CameraComponent->SetRelativeLocation(BobSocketSpace)` (still socket-space transformed)

### 2. Head Bob Re-enabled (TICKET-009)
- Removed `bUseBuiltInHeadBob = false` block from `ZP_GraceCharacter::BeginPlay:92-97`
- Head bob was already `true` on CDO — confirmed via MCP Python
- Bumped C++ default amplitudes: 1.2→3.0cm vertical, 0.6→1.5cm horizontal
- **DataAsset instance NOT yet updated** — existing DA may still have old 1.2/0.6 values

### 3. Input Refactor: Q=Peek, RMB=ADS
- Added `PeekAction` (UInputAction*) to `ZP_GraceCharacter`
- Added `Input_PeekStarted`/`Input_PeekCompleted` handlers → sets `bWantsPeek`
- `AimAction` handlers now set `bWantsAim` (was `bWantsPeek`)
- Added `bWantsAim` property to `ZP_GraceGameplayComponent`
- `UpdatePeek` logic:
  - Q (`bWantsPeek`): always peek, direction locked on press
  - RMB (`bWantsAim`): peek only if wall detected, direction locked on first detection
  - Neither: unlock, return to neutral

### 4. IA_Peek Asset Created
- `IA_Peek` (Boolean) at `/Game/Core/Input/Actions/IA_Peek`
- Q key mapping added to `IMC_Grace` (13 total mappings)
- `PeekAction` CDO property set on `BP_GraceCharacter`
- Created via `Scripts/Python/setup_peek_input.py`

---

## Files Modified

| File | Change |
|------|--------|
| `ZP_GraceCharacter.h` | Added `PeekAction` input, `Input_PeekStarted`/`Completed` declarations |
| `ZP_GraceCharacter.cpp` | Removed head bob disable block. Added PeekAction binding. Peek handlers set `bWantsPeek`, Aim handlers set `bWantsAim`. |
| `ZP_GraceGameplayComponent.h` | Added `bWantsAim`, `CachedMeshComponent`, `CachedMeshBaseLocation` |
| `ZP_GraceGameplayComponent.cpp` | BeginPlay caches mesh ref + base location. UpdatePeek: peek→mesh, bob→camera with socket-space transform. Bumped bob fallback defaults. |
| `ZP_GraceMovementConfig.h` | HeadBobVerticalAmplitude 1.2→3.0, HorizontalAmplitude 0.6→1.5 |

## Scripts Created

| Script | Purpose |
|--------|---------|
| `setup_peek_input.py` | Creates IA_Peek, binds Q in IMC_Grace, sets PeekAction CDO |
| `check_headbob.py` | Queries bUseBuiltInHeadBob, sets PeekAction CDO |

## Assets Modified (via MCP Python)

| Asset | Change |
|-------|--------|
| `/Game/Core/Input/Actions/IA_Peek` | Created (Boolean) |
| `/Game/Core/Input/IMC_Grace` | Q→IA_Peek mapping added |
| `/Game/Core/Player/BP_GraceCharacter` | PeekAction CDO set to IA_Peek |

---

## User Test Feedback (mid-session)

After first build (coordinate fix only, peek still camera-only):
- "character bobs a lot, but camera doesn't bob at all" → head bob amplitudes too small
- "camera angle of peek is nice" → coordinate space fix works
- "only triggered by right click and not Q" → PeekAction CDO wasn't set (old binaries)
- "only camera no gun" → led to iteration 2 (peek on mesh)

---

## Known Issues / TODO

1. **DataAsset instance head bob values** — C++ defaults updated but existing DA_GraceMovement_Default may still have 1.2/0.6. Need to update via MCP Python after editor reopens.
2. **ADS behavior** — `bWantsAim` is tracked but no actual ADS zoom/weapon behavior implemented yet. Only wall-peek triggers from ADS.
3. **PIE test needed** — second build (peek-on-mesh + head bob bump) not yet tested.
4. **Roll is camera-only** — gun doesn't tilt during peek. Positional lean is there but visual roll is camera-only. May be fine aesthetically.

---

## Component Hierarchy (current)

```
Capsule (root)
  ├── PlayerMesh (RelLoc=0,0,-90 + peek offset, SKM_Operator_Mono, ABP_TacticalShooter_UE5)
  │   └── FirstPersonCamera (at FPCamera socket, bob offset only)
  ├── ZP_GraceGameplayComponent (stamina, peek→mesh, bob→camera, interaction)
  ├── ZP_KinemationComponent (Kinemation wiring, weapon lifecycle)
  └── Kinemation SCS components (AC_FirstPersonCamera, AC_TacticalShooterAnimation, etc.)
```

---

## Build Info
- Branch: main
- Last build: clean (8 actions, 31.06s)
- Editor needs reopen for new binaries
