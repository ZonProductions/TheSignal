# Checkpoint: GASP Phase 1 — Camera Fix Investigation

**Date:** 2026-02-28
**Status:** DEBUGGING — all 3 fix attempts show weapon at far-left, runtime debug started but incomplete

---

## Context

Phase 1 component extraction (previous checkpoint) compiled clean and PIE showed arms + weapon visible after the InitializeKinemation timing fix. However, **camera-weapon divergence** was discovered: the camera didn't follow the weapon when looking up/down. This checkpoint documents the investigation and three fix attempts.

---

## Root Cause Analysis

### Problem
Camera stays at a fixed position while arms/weapon animate independently. Looking up moves the weapon above the camera view.

### Investigation Findings

1. **Reference BP_TacticalShooterCharacter** has `bUsePawnControlRotation = false` on its Camera SCS component. Our C++ had it `true`, fighting AC_FirstPersonCamera.

2. **Reference uses inherited Mesh (CharacterMesh0)** for arms, not a custom subobject:
   - CharacterMesh0: RelLoc=(0, 0, -90), RelRot=(0, yaw=-90, 0), SKM_Operator_Mono + ABP_TacticalShooter_UE5
   - AC_FirstPersonCamera auto-discovers via `GetMesh()` → finds the correct mesh + AnimBP

3. **Our BP_GraceCharacter** uses a separate `PlayerMesh` (C++ default subobject):
   - CharacterMesh0: empty (no mesh, no AnimBP)
   - PlayerMesh: RelLoc=(0, 0, -152), RelRot=(0, yaw=-90, 0), SKM_Operator_Mono + ABP_TacticalShooter_UE5
   - `GetMesh()` returns empty CharacterMesh0 — **AC_FirstPersonCamera discovers the WRONG mesh**

4. **AC_FirstPersonCamera caches `OwnerAnimInstance`** in its BeginPlay (during Super::BeginPlay) from `GetMesh()`. Our `UpdatePlayerMesh()` call in InitializeKinemation (after Super) updates the Mesh reference but NOT the cached AnimInstance. Result: AC_FirstPersonCamera can't drive the skeleton.

5. **FPCamera socket confirmed** on SKM_Operator_Mono (174 sockets total, FPCamera is first). Bind-pose position: (5.6, 8.4, 5.0) in component space.

6. **Kinemation GASP tutorial** states: "Camera attached to Player at FPCamera socket" — camera should be physically attached to the skeleton socket, not code-driven by AC_FirstPersonCamera for position.

---

## Fix Attempts (chronological)

### Attempt 1: Detach mesh from camera + disable bUsePawnControlRotation
- **Change:** PlayerMesh→SetupAttachment(Capsule), bUsePawnControlRotation=false
- **Result:** Camera rotated correctly but arms disappeared (stayed in capsule space, didn't rotate with look direction)
- **Why it failed:** Nothing applied controller pitch to the mesh; AnimBP couldn't drive bones because AC_FirstPersonCamera had null OwnerAnimInstance

### Attempt 2: Camera at FPCamera socket
- **Change:** Camera→SetupAttachment(PlayerMesh, "FPCamera")
- **Result:** Height correct but weapon at far-left edge of screen
- **Why it failed:** AC_FirstPersonCamera still had null OwnerAnimInstance, so it couldn't drive aim rotation into the skeleton. Arms were in partial animation (sway worked, aim offset didn't).

### Attempt 3: Camera at socket + RefreshOwnerAnimInstance
- **Change:** Added `FKinemationBridge::RefreshOwnerAnimInstance()` that force-sets OwnerAnimInstance on AC_FirstPersonCamera via reflection after UpdatePlayerMesh. Camera still at FPCamera socket.
- **Build:** Clean (5 actions, 3.98s)
- **Result:** Same as Attempt 2 — weapon at far-left edge, not centered
- **Why it failed:** RefreshOwnerAnimInstance alone doesn't fix the arm positioning. Either OwnerAnimInstance was already correct (UpdatePlayerMesh may refresh it internally), or the problem is elsewhere. Runtime debugging was started but interrupted.

---

## Current Code State

### ZP_GraceCharacter.cpp constructor (camera/mesh hierarchy)
```
Capsule (root)
  └── PlayerMesh (Z=64, yaw=-90 from BP override, SKM_Operator_Mono + ABP_TacticalShooter_UE5)
      └── FirstPersonCamera (at FPCamera socket, bUsePawnControlRotation=false)
```

### KinemationBridge.h/.cpp
- NEW: `RefreshOwnerAnimInstance(UObject* CameraComp, USkeletalMeshComponent* Mesh)` — sets OwnerAnimInstance via reflection from Mesh->GetAnimInstance()

### ZP_KinemationComponent.cpp InitKinemationCamera()
- After UpdatePlayerMesh: calls `RefreshOwnerAnimInstance(TacticalCameraComp, PlayerMeshComponent)`

---

## Files Modified This Session

| File | Change |
|------|--------|
| `ZP_GraceCharacter.cpp` | Constructor: PlayerMesh on capsule, camera at FPCamera socket on PlayerMesh, bUsePawnControlRotation=false |
| `KinemationBridge.h` | Added RefreshOwnerAnimInstance declaration |
| `KinemationBridge.cpp` | Added RefreshOwnerAnimInstance implementation (reflection-based property set) |
| `ZP_KinemationComponent.cpp` | InitKinemationCamera calls RefreshOwnerAnimInstance after UpdatePlayerMesh |

---

## PIE Test Checklist (pending)
- [ ] Arms + TR15 centered in view (not at edge)
- [ ] Camera follows weapon when looking up/down
- [ ] Output log shows `RefreshOwnerAnimInstance → ABP_TacticalShooter_UE5_C` (not NULL)
- [ ] Fire (LMB) works
- [ ] Reload (R) works
- [ ] Sprint (Shift) works
- [ ] Peek (RMB near walls) — may be affected by camera architecture change
- [ ] Jump, crouch, interact functional

## Known Risks
- **Peek system** modifies camera relative location/roll. With camera at skeleton socket, AC_FirstPersonCamera may overwrite peek offsets each tick. Peek may need rework.
- **BP PlayerMesh override** at Z=-152 from previous session — C++ sets Z=64 but BP deserializes Z=-152 on top. May need Python to clear the BP override.
- **If RefreshOwnerAnimInstance returns NULL:** AnimBP may not be initialized yet at InitializeKinemation time. Would need a deferred call (timer).

## Runtime Debug (incomplete — interrupted)
- Wrote `debug_camera_runtime2.py` to query live PIE state
- Hit `get_pawn` API issue on ZP_PlayerController (need `k2_get_pawn` or `acknowledged_pawn`)
- **Must complete next session:** Query runtime FPCamera socket world position, camera world position, AC_FirstPersonCamera properties (Camera, Mesh, OwnerAnimInstance, OwnerCharacter), and control rotation — all during PIE

## Next Steps
1. **Complete runtime debugging** — fix the Python script and query live state during PIE
2. Key questions to answer:
   - Is OwnerAnimInstance null or set at runtime?
   - What is the FPCamera socket world position vs camera world position during animation?
   - Is AC_FirstPersonCamera actually ticking and driving the camera?
3. If OwnerAnimInstance is null → AnimBP not initialized at wiring time, need deferred call
4. If socket position is correct but camera is wrong → AC_FirstPersonCamera overriding socket attachment
5. Consider using inherited Mesh (CharacterMesh0) instead of custom PlayerMesh — matches reference exactly

## Build Info
- Branch: main
- Last build: clean (5 actions, 3.98s)
- Editor must be reopened for PIE test
