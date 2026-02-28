# Checkpoint: Kinemation Camera Integration (TICKET-006 Phase 1)
**Date:** 2026-02-28
**System:** PlayerCharacter / Kinemation Camera
**Branch:** main (HEAD)
**Status:** IN PROGRESS — iterating on mesh positioning, awaiting PIE test

---

## Current State

### BUILD: COMPILED CLEAN (0 errors, 0 warnings)
- 4 actions, 3.22 seconds

### What Was Done
Integrated Kinemation's AC_FirstPersonCamera for camera shake, FOV interpolation, and freelook. Added visible first-person mesh (SKM_Operator_Mono) as child of FirstPersonCamera. C++ auto-detects AC_FirstPersonCamera at BeginPlay and wires it via FKinemationBridge. Public API: TriggerCameraShake() and SetTargetFOV() on AZP_GraceCharacter.

### Architecture
```
Capsule (collision + movement)
+-- FirstPersonCamera (on capsule, bUsePawnControlRotation=true)
|   |   ^ head bob + peek write position/roll here (unchanged)
|   |
|   +-- PlayerMesh (SkeletalMeshComponent — SKM_Operator_Mono)
|       RelativeLocation: (0, 0, -152)
|       RelativeRotation: (Pitch=0, Yaw=-90, Roll=0)
|       OnlyOwnerSee: true, CastShadow: false
|
+-- AC_FirstPersonCamera (SCS component — camera shake, FOV, freelook)
        bAutoActivate: true, TargetFOV: 90.0
        Camera + Mesh wired at runtime via InitKinemationCamera()
```

---

## Iteration History

1. **Initial:** SKM_Operator_Mono at (0,0,0) relative to camera — boots filling screen (mesh root at feet, rendered at eye level)
2. **Attempted FPP mesh:** Switched to SK_Shirt_01a_FPP (arms-only) — arms invisible because no animation driving them (bind pose not visible from camera)
3. **Back to full body + offset:** SKM_Operator_Mono at Z=-152 — body visible but facing 90° wrong (camera looking at left shoulder)
4. **Rotation fix attempt 1:** unreal.Rotator(0, -90, 0) — Python constructor mapped to Pitch=-90 instead of Yaw=-90 (mesh face-down, invisible)
5. **Rotation fix attempt 2:** Explicit r.yaw=-90 — confirmed Pitch=0, Yaw=-90, Roll=0 — **awaiting PIE test**

---

## Files Modified (2 C++ files)

| File | What Changed |
|------|-------------|
| `ZP_GraceCharacter.h` | Added `USkeletalMeshComponent* PlayerMesh` UPROPERTY. Added `TriggerCameraShake()`, `SetTargetFOV()` public API. Added `InitKinemationCamera()` private helper. Forward decl USkeletalMeshComponent. |
| `ZP_GraceCharacter.cpp` | Added includes: KinemationBridge.h, SkeletalMeshComponent.h. Constructor: creates PlayerMesh as child of FirstPersonCamera (OnlyOwnerSee, no shadow). BeginPlay: calls InitKinemationCamera(). InitKinemationCamera: auto-detects AC_FirstPersonCamera by class name, wires UpdateTargetCamera + UpdatePlayerMesh via FKinemationBridge. TriggerCameraShake/SetTargetFOV delegate to FKinemationBridge. |

## Blueprint Changes (via MCP)

| Asset | What Changed |
|-------|-------------|
| `BP_GraceCharacter` | Added AC_FirstPersonCamera_C SCS component. Set bAutoActivate=true. Set TargetFOV=90.0. PlayerMesh: SkeletalMeshAsset=SKM_Operator_Mono, RelativeLocation=(0,0,-152), RelativeRotation=(P=0,Y=-90,R=0). |

## Files Created

| File | Purpose |
|------|---------|
| `Scripts/Python/setup_kinemation_camera.py` | Original script (failed on SCS API — used MCP tools instead). Kept for reference. |

---

## AC_FirstPersonCamera Properties (set via MCP)

| Property | Value | Why |
|----------|-------|-----|
| bAutoActivate | true | Component must activate to apply effects |
| TargetFOV | 90.0 | Match our config default (was 70, would fight our camera) |
| Camera | None (set at runtime) | Wired to FirstPersonCamera via KinemationBridge |
| Mesh | None (set at runtime) | Wired to PlayerMesh via KinemationBridge |

## PlayerMesh CDO State

| Property | Value |
|----------|-------|
| SkeletalMeshAsset | SKM_Operator_Mono |
| RelativeLocation | (0, 0, -152) |
| RelativeRotation | (Pitch=0, Yaw=-90, Roll=0) |
| bVisible | true |
| bOnlyOwnerSee | true |
| bHiddenInGame | false |
| CastShadow | false |

---

## Known Issues / Next Steps

- **Awaiting PIE test** of Yaw=-90 rotation fix
- Yaw direction may need to be +90 instead of -90 — will adjust if body faces backward
- Z offset of -152 is calculated (capsule half-height 88 + camera Z offset 64) — may need tuning
- Full body mesh is placeholder — FPP arms-only mesh (SK_Shirt_01a_FPP) is correct long-term but needs AC_TacticalShooterAnimation to drive arm pose
- No animation on mesh yet — will be in bind/T-pose
- Head bob + peek untouched and should still work
- Camera shake + FOV API implemented but not yet tested

## Dead Ends (this feature)

- FPP arms-only mesh without animation — invisible from camera (bind pose not in view)
- unreal.Rotator positional constructor — order is (Roll, Pitch, Yaw), not (Pitch, Yaw, Roll)

## User Notes (from another session, re: Kinemation)

- Set Auto Activate to false on OTHER cameras, not the main one
- UE 5.4 requires explicit Update Rotation function call for locomotion
- RetargetPoseFromMesh Source Mesh must point to hidden "Mesh" (GASP one), not visible "Player" mesh
