# Checkpoint: K002/K004 Reverted, Last-Known-Good Restored

**Date:** 2026-02-28
**Session:** K002/K004 revert session
**Status:** STABLE — arms animated, TR15 in hands, fire working, movement sway active

---

## What Was Done

### Reverted K004 (shadow mesh) from constructor
- Removed 12-line block configuring inherited Mesh as shadow-only body
- Inherited Mesh back to default untouched state
- Shadow mesh CDO cleared (SkeletalMeshAsset = None)

### Reverted K002 (bone hiding) from BeginPlay
- Removed HideBoneByName block (head, neck_01, neck_02, pelvis)
- This was hiding the entire body recursively — confirmed dead end

### Reverted K004 (leader pose) from BeginPlay
- Removed SetLeaderPoseComponent(PlayerMesh) on inherited Mesh
- Cross-attachment-parent leader pose = broken orientation — confirmed dead end

### Fixed PlayerMesh rotation bug
- `unreal.Rotator(0.0, -90.0, 0.0)` → positional args are `(roll, pitch, yaw)`, NOT `(pitch, yaw, roll)`
- Was setting pitch=-90 (mesh pointing down) instead of yaw=-90
- Fixed with `unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0)`

### Kept K001 (fire input)
- IA_Fire + LMB mapping in IMC_Grace
- Input_FireStarted/Completed → FKinemationBridge::WeaponOnFirePressed/Released
- Confirmed working in PIE

---

## Current PlayerMesh CDO State (BP_GraceCharacter)

| Property | Value |
|----------|-------|
| SkeletalMeshAsset | SKM_Operator_Mono (UE5/) |
| RelativeLocation | (0, 0, -152) |
| RelativeRotation | (P=0, Y=-90, R=0) |
| AnimClass | ABP_TacticalShooter_UE5 |
| Visible | True |
| HiddenInGame | False |
| OnlyOwnerSee | True |
| CastShadow | False |

## Inherited Mesh CDO State

| Property | Value |
|----------|-------|
| SkeletalMeshAsset | None (cleared) |
| HiddenInGame | False |
| AnimClass | None |

---

## PIE Test Results

- Arms visible with TR15 rifle ✓
- LMB fires weapon (recoil, sound) ✓
- Movement sway active ✓
- Draw animation plays ✓
- No crash ✓
- Kinemation log: Camera=OK Anim=OK Weapon=BP_TR15_C_0 ✓

## Known Issues (Accepted Prototype Limitations)

- **K002 (body clipping):** Full torso/legs visible when looking down — DEFERRED to GASP integration
- **K004 (no shadow):** No player shadow — DEFERRED to GASP integration
- **Reload animation:** Not working — needs investigation (new issue)

---

## Dead Ends Added to CLAUDE.md

| Approach | Why It Failed |
|----------|---------------|
| `HideBoneByName("pelvis")` | Hides entire body recursively — no per-bone override |
| `SK_Shirt_01a_FPP` + `ABP_TacticalShooter_UE5` | Invisible — confirmed x2 |
| `SetLeaderPoseComponent` across camera/capsule parents | Broken bone orientation |
| Shadow-only inherited Mesh without GASP AnimBP | T-pose or broken |

## Python API Lesson Learned

- `unreal.Rotator(a, b, c)` positional = `(roll, pitch, yaw)` — NOT like C++ `FRotator(pitch, yaw, roll)`
- Always use keyword args: `unreal.Rotator(roll=0, pitch=0, yaw=-90)`

---

## Files Modified This Session

| File | Action |
|------|--------|
| `Source/TheSignal/ZP_GraceCharacter.cpp` | Removed 3 blocks (shadow config, HideBoneByName, SetLeaderPoseComponent) |
| `Scripts/Python/revert_k002_k004.py` | Created — reverts PlayerMesh CDO + clears inherited Mesh |
| `Scripts/Python/fix_rotation.py` | Created — fixes rotation bug |
| `Scripts/Python/inspect_playermesh.py` | Created — diagnostic inspection |
| `CLAUDE.md` | Added 4 dead ends |
| `session_prompt.txt` | K002/K004 status → DEFERRED |

---

## Next Priorities

1. **Reload animation** — not working, needs investigation
2. **K002 body clipping** — deferred to GASP (TICKET-006 Phase 3)
3. **K004 shadow** — deferred to GASP (TICKET-006 Phase 3)
