# Checkpoint: Context-Sensitive Cover Peek System
**Date:** 2026-02-28
**System:** PlayerCharacter / Peek
**Branch:** main (HEAD)
**Status:** COMPLETE — tested and working in PIE

---

## Current State

### BUILD: COMPILED CLEAN (0 errors, 0 warnings)
- 4/4 actions, 3.27 seconds
- Build command: `"C:/Program Files/Epic Games/UE_5.4/Engine/Build/BatchFiles/Build.bat" TheSignalEditor Win64 Development "C:/Users/Ommei/workspace/TheSignal/TheSignal.uproject" -waitmutex`
- Kill editor+LiveCoding before build: `powershell -Command "Stop-Process -Name 'UnrealEditor' -Force -ErrorAction SilentlyContinue; Stop-Process -Name 'LiveCodingConsole' -Force -ErrorAction SilentlyContinue; Stop-Process -Name 'UnrealTraceServer' -Force -ErrorAction SilentlyContinue"`

### What Was Done
Implemented full context-sensitive cover peek system. Hold RMB near a wall → camera leans toward the opening side. 5-ray fan sphere traces per side detect walls. Direction locks on RMB press so looking around doesn't break it. Camera smoothly offsets laterally + forward with roll lean. Head bob stored as offsets and composed with peek in a single final camera position (no fighting). Sprint/airborne cancels peek. Debug logs stripped. PIE tested — confirmed working.

---

## Files Modified (4 C++ files)

| File | What Changed |
|------|-------------|
| `ZP_GraceMovementConfig.h` | Added 12 peek tuning UPROPERTYs under `Movement\|Peek` category |
| `ZP_EventBroadcaster.h` | Added `FOnPlayerPeekChanged` delegate (int32 Direction, float Alpha) + `OnPlayerPeekChanged` UPROPERTY + `BroadcastPeekChanged()` |
| `ZP_EventBroadcaster.cpp` | Added `BroadcastPeekChanged()` implementation |
| `ZP_GraceCharacter.h` | Added `EZP_PeekDirection` enum (None/Left/Right), public state (CurrentPeekDirection, PeekAlpha, bWantsPeek), private state (PreviousPeekDirection, LockedPeekDirection, bPeekLocked, CurrentPeekRoll, HeadBobOffsetY/Z), private functions (TracePeekSide, DetectPeekDirection, UpdatePeek, Input_AimStarted, Input_AimCompleted) |
| `ZP_GraceCharacter.cpp` | Bound AimAction (Started/Completed), added UpdatePeek() to Tick, refactored UpdateHeadBob to store offsets (HeadBobOffsetY/Z) instead of applying directly, implemented TracePeekSide (5-ray fan with vertical surface validation), DetectPeekDirection (wall comparison), UpdatePeek (direction locking + interp + composed camera position + roll + broadcast) |

## Files Created (1 Python script)

| File | Purpose |
|------|---------|
| `Scripts/Python/setup_aim_input.py` | Creates IA_Aim (Boolean), adds RMB→IA_Aim to IMC_Grace, sets AimAction on BP_GraceCharacter CDO — **already executed successfully** |

---

## Peek Config Values (ZP_GraceMovementConfig.h)

| Property | Default | Description |
|----------|---------|-------------|
| PeekLateralOffset | 25.0 | Camera Y offset toward opening (cm) |
| PeekForwardOffset | 8.0 | Camera X offset past corner (cm) |
| PeekRollAngle | 3.0 | Roll lean (degrees) |
| PeekInterpSpeed | 8.0 | Enter speed |
| PeekReturnInterpSpeed | 6.0 | Exit speed |
| PeekWallDetectionRange | 180.0 | Sphere trace range (cm) |
| PeekTraceRadius | 12.0 | Sphere trace radius (cm) |
| PeekWallHitThreshold | 2 | Min traces that must hit per side |
| PeekTraceFanHalfAngle | 75.0 | Fan spread half-angle (degrees) |
| HeadBobPeekDamping | 0.15 | Horizontal bob damping during peek |
| HeadBobPeekVerticalDamping | 0.5 | Vertical bob damping during peek |
| PeekMaxWallAngleFromVertical | 20.0 | Max surface angle to count as wall |

## Peek System Architecture

### Detection: 5-Ray Fan Per Side
- From camera position, cast 5 sphere traces per side (left/right)
- Rays at: perpendicular (0°), ±37.5°, ±75° from side direction
- 180cm range, 12cm radius sphere sweeps
- Surface validated as roughly vertical (impact normal angle from vertical > 70°)
- Side = "wall" if ≥2 of 5 traces hit
- Wall left + opening right → peek right (vice versa)
- Both walls (corridor) or no walls (open) → no peek

### Direction Locking
- Direction detected **once** on RMB press, then locked for the duration of the hold
- Looking around while peeking does NOT re-detect or drop the peek
- Lock released on RMB release, sprint, or leaving ground

### Camera Effect
- Lateral Y offset toward opening (25cm)
- Forward X offset past corner (8cm)
- Roll via RelativeRotation.Roll (safe — bUsePawnControlRotation only drives Pitch/Yaw)
- All interpolated via FInterpTo (enter: 8.0, exit: 6.0)
- PreviousPeekDirection tracked for smooth exit interpolation back to correct side

### Head Bob Integration (Bob Fix)
- UpdateHeadBob stores offsets to HeadBobOffsetY/Z (does NOT apply to camera directly)
- UpdatePeek composes final camera position in one place: `FinalY = BobY + PeekY`, `FinalZ = BaseZ + BobZ`
- No frame-to-frame fighting between systems
- Head bob amplitude damped by PeekAlpha (lerp between 1.0 and damping factor)

### Cancellation
- Sprint cancels peek (bIsSprinting check)
- Airborne cancels peek (IsMovingOnGround check)
- Release RMB → smooth return via ReturnInterpSpeed

### Event Broadcasting
- `BroadcastPeekChanged(Direction, Alpha)` fires on direction change
- Direction: -1=Left, 0=None, 1=Right

---

## Iteration History
1. Initial: 3-ray fan, 80cm range, 30° half-angle → only 1 of 3 rays hit (too narrow)
2. Widened fan to 60°, range to 120cm, radius to 10cm → better but still tight
3. Attempted dynamic corner-distance system → too subtle, reverted
4. Fixed head bob oscillation: stored offsets instead of direct application, composed in UpdatePeek
5. Added direction locking: detect once on RMB press, hold until release
6. Final: 5-ray fan, 75° half-angle, 180cm range, 12cm radius → confirmed working in PIE

## Dead Ends (from this feature)
- Dynamic corner-distance peek (forward-offset perpendicular sampling) — behavior too subtle and ineffective, reverted to static binary detection

---

## Input Assets Created
- `IA_Aim` (Boolean) at `/Game/Core/Input/Actions/IA_Aim`
- RMB → IA_Aim mapping added to `IMC_Grace`
- `AimAction` set on `BP_GraceCharacter` CDO

## Known Considerations
- AimAction UPROPERTY was already declared in header (Input|Tactical category, originally a Kinemation stub) — reused for peek
- Peek and future ADS are mutually exclusive by design (wall present = peek, no wall = future ADS)
- Camera X (forward) is set directly by peek (no other system writes X)
- PeekAlpha snaps to 0 when < 0.001 to avoid floating-point drift
- Old config values (PeekWallHitThreshold, PeekTraceFanHalfAngle) still in header — still used by fan system
