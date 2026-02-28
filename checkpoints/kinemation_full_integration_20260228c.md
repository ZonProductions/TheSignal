# Checkpoint: Full Kinemation Integration (TICKET-006 Phase 2) — PIE Test Results
**Date:** 2026-02-28
**System:** PlayerCharacter / Kinemation Full Stack
**Branch:** main (HEAD)
**Status:** FUNCTIONAL — arms + weapon visible, 4 open issues

---

## PIE Test Result: SUCCESS (with issues)

Kinemation full stack is working. Arms animated, TR15 rifle in hands, draw animation played, movement sway active. Four issues remain.

### Captures: `captures/20260228_024757/`

---

## Open Issues

### ISSUE-K001: Cannot fire weapon
- **Severity:** Expected / Low (prototype has no combat)
- **Cause:** No fire input wired. IA_Fire not mapped in IMC_Grace, FireAction UPROPERTY not set on BP_GraceCharacter.
- **Fix:** Add IA_Fire to IMC_Grace (LMB), set FireAction on CDO, bind in SetupPlayerInputComponent, call FKinemationBridge::WeaponOnFirePressed/Released.

### ISSUE-K002: Body geometry bleeds into camera
- **Severity:** HIGH — breaks immersion
- **Cause:** Using full-body mesh (SKM_Operator_Mono) with Z=-152 offset. Torso/chest/shoulder geometry is too close to camera and clips through the near clip plane. Visible in capture 0010 — looking down shows body filling the screen.
- **Fix options:**
  - (A) Switch to FPP arms-only mesh (SK_Shirt_01a_FPP or similar) — correct long-term solution but needs animation driving it (previously failed without AnimBP, should work NOW with ABP_TacticalShooter_UE5)
  - (B) Increase near clip plane distance — hides clipping but reduces close-object visibility
  - (C) Adjust Z offset to push body further down — band-aid, still clips when looking down
  - (D) Hide body mesh below chest via material or bone scaling — hacky

### ISSUE-K003: Lower body not visible
- **Severity:** LOW — expected with current setup
- **Cause:** Full-body mesh at Z=-152 puts legs below camera frustum. First-person games typically don't show legs unless using a separate leg mesh or look-down system.
- **Fix:** Not needed for prototype. If desired later, add separate leg mesh with separate visibility rules.

### ISSUE-K004: No shadow cast from player mesh
- **Severity:** MEDIUM — reduces grounding/presence
- **Cause:** PlayerMesh has `CastShadow = false` and `bCastDynamicShadow = false` (set in C++ constructor, line 33-34 of ZP_GraceCharacter.cpp). Also `bOnlyOwnerSee = true` which may affect shadow visibility for the local player.
- **Fix:** Enable `CastShadow = true` on PlayerMesh. May need a separate shadow-only mesh if bOnlyOwnerSee prevents shadow rendering for the owning client (UE5 quirk — OnlyOwnerSee meshes don't cast shadows visible to the owner). Alternative: use the hidden default "Mesh" component (inherited from ACharacter) as shadow caster with `bOwnerNoSee = true, bHiddenInGame = false, CastShadow = true`.

---

## What Was Fixed This Session

### Root Cause of Crash: WeaponDraw missing parameter
- **Bug:** `FKinemationBridge::WeaponDraw()` called `CallNoParams()` but weapon's `Draw()` function requires a `PlayAnimation` (bool) parameter. ProcessEvent read from nullptr → ACCESS_VIOLATION.
- **Fix:** Changed WeaponDraw to pass `{ PlayAnimation = true }` struct to ProcessEvent.
- **File:** `KinemationBridge.cpp` line 178-185

### Deferred WeaponDraw (kept)
- WeaponDraw deferred to next tick via SetTimerForNextTick — gives weapon internals + AnimInstance one frame to initialize after SpawnActor.

### BPI Interface Functions Implemented (MCP)
- GetPlayerMesh → PlayerMesh
- GetTacticalShooterComponent → AC_TacticalShooterAnimation (SCS)
- GetPrimaryWeapon → ActiveWeapon
- GetMainWeapon → ActiveWeapon

---

## Current Architecture

```
BP_GraceCharacter (child of AZP_GraceCharacter)
  Implements: BPI_TacticalShooterCharacter (4 functions wired)

SCS Components (4):
  AC_FirstPersonCamera, AC_TacticalShooterAnimation,
  AC_RecoilAnimation, AC_IKMotionPlayer

C++ Components:
  FirstPersonCamera (bUsePawnControlRotation=true)
    +-- PlayerMesh (SKM_Operator_Mono, ABP_TacticalShooter_UE5)
        Z=-152, Yaw=-90, OnlyOwnerSee, CastShadow=false

CDO: WeaponClass=BP_TR15, bUseBuiltInHeadBob=false

BeginPlay:
  → InitKinemationCamera() → InitKinemationAnimation()
  → SpawnAndEquipWeapon() [spawn BP_TR15, attach at "VB ik_hand_gun",
     feed settings to AC_TacticalShooterAnimation,
     deferred WeaponDraw next tick]
  → disable head bob if Kinemation active
```

## Files Modified (Full Session)

| File | Changes |
|------|---------|
| `ZP_GraceCharacter.h` | +WeaponClass, +ActiveWeapon, +InitKinemationAnimation(), +SpawnAndEquipWeapon() |
| `ZP_GraceCharacter.cpp` | +InitKinemationAnimation, +SpawnAndEquipWeapon (deferred draw), BeginPlay updated |
| `KinemationBridge.cpp` | Fixed WeaponDraw: passes `{PlayAnimation=true}` instead of nullptr |
| `BP_GraceCharacter` (MCP) | +3 SCS components, +BPI interface (4 functions implemented), AnimBP set, WeaponClass=BP_TR15, bUseBuiltInHeadBob=false |
| `Scripts/Python/setup_kinemation_full.py` | New: sets AnimClass + WeaponClass via CDO |

## Key Lessons

1. **Force-load Blueprint classes before MCP add_component** — UE5 lazy-loads; use `unreal.load_asset()` first
2. **Always check Blueprint function signatures** — `CallNoParams` is only safe for truly parameterless functions. Inspect via `get_blueprint_graph` before calling.
3. **Defer weapon init to next tick** — weapon internals need one frame after SpawnActor
4. **Kill CrashReportClient.exe** — holds DLL locks after editor crash

## Dead Ends (This Feature)

| Approach | Why It Failed |
|----------|---------------|
| FPP arms-only mesh without animation | Invisible from camera (bind pose not in view) — may work NOW with ABP |
| unreal.Rotator positional constructor | Order is (Roll, Pitch, Yaw) not (Pitch, Yaw, Roll) |
| CallNoParams for Draw() | Draw requires PlayAnimation bool param — nullptr crash |
| Same-frame WeaponDraw after SpawnActor | Weapon internals not ready — deferred to next tick |
