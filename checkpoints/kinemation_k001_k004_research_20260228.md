# Deep Research: Kinemation Issues K001–K004
**Date:** 2026-02-28
**Scope:** Research-only — no code changes made
**Source checkpoint:** `kinemation_full_integration_20260228c.md`

---

## Table of Contents
1. [K001: Cannot Fire Weapon](#k001)
2. [K002: Body Geometry Bleeds Into Camera](#k002)
3. [K003: Lower Body Not Visible](#k003)
4. [K004: No Shadow Cast From Player Mesh](#k004)
5. [Recommended Fix Order](#fix-order)
6. [Implementation Plans](#implementation-plans)

---

<a name="k001"></a>
## K001: Cannot Fire Weapon

### Severity: Expected / Low (prototype has no combat — but useful for Kinemation validation)

### Root Cause
The fire input pipeline has **zero implementation**. The C++ UPROPERTY `FireAction` exists (ZP_GraceCharacter.h:123) but nothing wires it up:

| Component | Status |
|-----------|--------|
| `IA_Fire` asset in `/Game/Core/Input/Actions/` | **MISSING** (one exists in Kinemation pack but that's external) |
| `IA_Fire` mapping in IMC_Grace | **MISSING** |
| `FireAction` CDO binding on BP_GraceCharacter | **NULL** |
| `Input_FireStarted` / `Input_FireCompleted` handlers | **NOT DECLARED** |
| Binding in `SetupPlayerInputComponent()` | **NOT PRESENT** |
| `KinemationBridge::WeaponOnFirePressed/Released` | **Already implemented** (KinemationBridge.cpp:189-197) |

### What Already Works
- `WeaponOnFirePressed()` and `WeaponOnFireReleased()` are already implemented in KinemationBridge — they call `OnFirePressed` and `OnFireReleased` on the weapon actor
- The weapon (`BP_TR15`) has full fire logic internally: ammo tracking, fire modes, recoil integration, muzzle flash, sound, camera shake
- The weapon expects its `TacticalComponent`, `RecoilComponent`, and `CameraAnimator` to be set (done automatically via weapon's BeginPlay when it finds owner's components)

### Kinemation Fire Chain (from documentation research)
```
Input (LMB) → IA_Fire → IMC_Grace mapping
  → SetupPlayerInputComponent binding
    → Input_FireStarted() → KinemationBridge::WeaponOnFirePressed(ActiveWeapon)
      → Weapon checks HasActiveAction, fire rate timing
        → Sets IsFiring=true on TacticalComponent
        → Calls internal Fire() → recoil, montages, effects, ammo decrement
    → Input_FireCompleted() → KinemationBridge::WeaponOnFireReleased(ActiveWeapon)
      → Sets IsFiring=false, stops recoil, clears fire timer
```

### Fix Plan (3 parts: C++, Python script, build)

**Part A — C++ changes (ZP_GraceCharacter.h + .cpp):**

1. Add handler declarations to the private section of the header (after `Input_AimCompleted`):
```cpp
void Input_FireStarted(const FInputActionValue& Value);
void Input_FireCompleted(const FInputActionValue& Value);
```

2. Add bindings in `SetupPlayerInputComponent()` (after the AimAction block, line ~362):
```cpp
if (FireAction)
{
    EIC->BindAction(FireAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_FireStarted);
    EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AZP_GraceCharacter::Input_FireCompleted);
}
```

3. Add the handler implementations:
```cpp
void AZP_GraceCharacter::Input_FireStarted(const FInputActionValue& Value)
{
    if (ActiveWeapon)
    {
        FKinemationBridge::WeaponOnFirePressed(ActiveWeapon);
    }
}

void AZP_GraceCharacter::Input_FireCompleted(const FInputActionValue& Value)
{
    if (ActiveWeapon)
    {
        FKinemationBridge::WeaponOnFireReleased(ActiveWeapon);
    }
}
```

4. Update the log line in `SetupPlayerInputComponent` to include `Fire:%s`.

**Part B — Python script (`Scripts/Python/setup_fire_input.py`):**
Follow the pattern from `setup_aim_input.py`:
1. Create `IA_Fire` at `/Game/Core/Input/Actions/IA_Fire` (Boolean)
2. Add `LeftMouseButton → IA_Fire` mapping to IMC_Grace
3. Set `FireAction` on BP_GraceCharacter CDO

**Part C — Build + test:**
Kill editor → Build.bat → reopen editor → run Python script → PIE test LMB fire

### Optional: Also wire ReloadAction
Same pattern. Create `IA_Reload` (R key), bind to `Input_ReloadStarted` which calls `FKinemationBridge::WeaponOnReload(ActiveWeapon)`. Low priority but trivial once fire is done.

### Concern: AimAction conflict with Peek
Currently `AimAction` (RMB) drives `bWantsPeek` (peek system). If we later want RMB for ADS (aim down sights via `FKinemationBridge::AnimSetAiming`), we need to either:
- Separate peek and ADS into different inputs
- Or make RMB do both (ADS + peek context-sensitive)

For now, fire is independent of this — LMB doesn't conflict.

---

<a name="k002"></a>
## K002: Body Geometry Bleeds Into Camera

### Severity: HIGH — breaks immersion

### Root Cause
Using full-body mesh `SKM_Operator_Mono` with Z=-152 offset from camera. Torso/chest/shoulder geometry is too close to camera near plane. Looking down shows body filling the screen.

### Current Setup
```
FirstPersonCamera (Z=64 from capsule center)
  └── PlayerMesh (SKM_Operator_Mono, full body)
      RelativeLocation: (0, 0, -152)
      RelativeRotation: (P=0, Y=-90, R=0)
      bOnlyOwnerSee: true
      CastShadow: false
```

### Available FPP Meshes (from Kinemation pack)
Located at `/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/`:

| Mesh | Size | Description |
|------|------|-------------|
| **SK_Shirt_01a_FPP** | 349 KB | **Arms-only FPP torso — purpose-built for first person** |
| SK_Arm_01a | 477 KB | Separate arm mesh |
| SK_Hand_01a | 612 KB | Separate hand mesh |
| SK_Glove_01a | — | Glove overlay |

Also available: `SK_Mannequin_Arms` (demo mannequin arms)

### Dead End (confirmed)
- Previously tried `SK_Shirt_01a_FPP` **without AnimBP** → invisible (bind pose out of camera view)
- NOW we have `ABP_TacticalShooter_UE5` active → **should work**

### Fix Options (ranked)

#### OPTION A: Switch to SK_Shirt_01a_FPP (RECOMMENDED)
**Why:** Purpose-built FPP mesh. No torso/legs geometry to clip. Designed to work with the same skeleton and AnimBP.

**Changes needed:**
1. **C++ (ZP_GraceCharacter.cpp constructor, line 30-34):** No code change needed — mesh is set via Blueprint CDO, not hardcoded
2. **Python/MCP:** Change PlayerMesh SkeletalMeshAsset from `SKM_Operator_Mono` to `SK_Shirt_01a_FPP`
3. **Adjust Z offset:** FPP mesh is smaller — Z=-152 was calculated for full body (capsule half-height 88 + camera Z 64). FPP mesh likely needs less offset. **Start at Z=0 and tune from there** since the FPP mesh is designed to be at camera level.
4. **Test:** Verify arms are visible, weapon attachment works at "VB ik_hand_gun" socket

**Risks:**
- Socket `VB ik_hand_gun` must exist on the FPP mesh skeleton (likely yes — same skeleton)
- FPP mesh may need different rotation offset (currently Y=-90)
- Animations designed for full body may look wrong on partial mesh (unlikely — Kinemation's AnimBP handles upper body independently)

**Validation steps:**
- Verify socket exists: Python `unreal.load_asset('/Game/KINEMATION/.../SK_Shirt_01a_FPP').skeleton.get_bone_names()` or check socket list
- PIE test: arms visible, weapon attached, no clipping

#### OPTION B: Increase near clip plane
**Why not:** Changes global rendering. Affects interaction with close objects. Band-aid.

#### OPTION C: Adjust Z offset further
**Why not:** Any Z offset still shows torso when looking down. The geometry exists — you can only push it so far before arms are too low.

#### OPTION D: Hide bones below chest
**Why not:** Hacky. `HideBoneByName` affects shadow casting. Requires identifying exact bones. Fragile.

### Recommendation: **Option A.** The FPP mesh exists specifically for this purpose. The previous failure (invisible mesh) was because there was no AnimBP — now there is one.

---

<a name="k003"></a>
## K003: Lower Body Not Visible

### Severity: LOW — expected behavior for FPS

### Root Cause
With the full-body mesh at Z=-152, legs are well below the camera frustum. This is normal for first-person games — legs aren't visible unless you specifically implement a look-down system.

### Industry Context
- **Most FPS games don't show legs** (Call of Duty, Battlefield, Half-Life, etc.)
- Games that show legs (Halo, Mirror's Edge, Dying Light) use one of:
  1. Separate leg mesh with independent visibility/animation
  2. Full body with head hidden via `HideBoneByName`
  3. UE5.5+ First Person Rendering primitive types

### Options

#### OPTION A: Do nothing (RECOMMENDED for prototype)
The prototype has no combat and no reason to see legs. This is not an immersion issue for a horror game where you're focused on what's ahead of you.

#### OPTION B: Separate leg mesh (future)
If look-down body visibility is desired later:
1. Add a separate `USkeletalMeshComponent` for legs
2. Use the full-body mesh (or a legs-only mesh)
3. Set it as `bOwnerNoSee = false`, position it below camera
4. Animate via `SetLeaderPoseComponent` following the PlayerMesh
5. Only show when looking down (check camera pitch angle)

#### OPTION C: Single full-body with head hidden
1. Use full-body mesh parented to capsule (not camera)
2. Hide head bone via `HideBoneByName("head", true)`
3. Camera follows head bone position
4. Body is always visible including legs
5. **Problem:** This fights with Kinemation's camera-parented mesh system

### Recommendation: **Option A — do nothing.** Revisit after vertical slice if narrative/immersion demands it. The FPP mesh switch from K002 makes this moot anyway.

---

<a name="k004"></a>
## K004: No Shadow Cast From Player Mesh

### Severity: MEDIUM — reduces grounding/presence in the environment

### Root Cause
```cpp
// ZP_GraceCharacter.cpp constructor, lines 33-34
PlayerMesh->bCastDynamicShadow = false;
PlayerMesh->CastShadow = false;
```
Plus `bOnlyOwnerSee = true` (line 32).

### UE5 Shadow Rendering + bOnlyOwnerSee (Research Results)

**Key finding:** In single-player, `bOnlyOwnerSee = true` meshes DO cast shadows visible to the owning player. Shadows are calculated at the ViewFamily level, not per-player. So simply enabling `CastShadow = true` on PlayerMesh would cast shadows — but they'd be the shadow of the FPP arms mesh, not a full body. **Floating arm shadows look bad.**

### Fix Options (ranked)

#### OPTION A: Shadow-Only Full-Body Mesh (RECOMMENDED)
Add a dedicated skeletal mesh component that's invisible but casts a full-body shadow.

**Properties for shadow-only mesh:**
| Property | Value | Purpose |
|----------|-------|---------|
| `bHiddenInGame` | `true` | Invisible to all rendering |
| `CastShadow` | `true` | Enables shadow |
| `bCastHiddenShadow` | `true` | Cast shadow even when hidden |
| `bCastDynamicShadow` | `true` | Dynamic shadow (not baked) |
| `VisibilityBasedAnimTickOption` | `AlwaysTickPoseAndRefreshBones` | Keep animating when hidden |

**Animation sync:** Use `SetLeaderPoseComponent()` pointing to the default inherited `Mesh` component (ACharacter's built-in mesh). This avoids running the AnimBP twice.

**Implementation (C++ in ZP_GraceCharacter):**

1. Add UPROPERTY in header:
```cpp
UPROPERTY(VisibleAnywhere, Category = "Shadow")
TObjectPtr<USkeletalMeshComponent> ShadowMesh;
```

2. Create in constructor:
```cpp
ShadowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShadowMesh"));
ShadowMesh->SetupAttachment(GetMesh());  // Attach to inherited Mesh for transform
ShadowMesh->SetHiddenInGame(true);
ShadowMesh->CastShadow = true;
ShadowMesh->bCastDynamicShadow = true;
ShadowMesh->bCastHiddenShadow = true;
ShadowMesh->SetVisibilityBasedAnimTickOption(EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones);
```

3. In BeginPlay, set leader pose:
```cpp
if (ShadowMesh && GetMesh())
{
    ShadowMesh->SetLeaderPoseComponent(GetMesh());
}
```

4. Set SkeletalMesh asset via Python CDO to `SKM_Operator_Mono` (full body) — or set in constructor if we want a C++ default.

**Wait — architectural question:** The default `Mesh` component (from ACharacter) is currently unused. It has no skeletal mesh set and no AnimBP. For the shadow mesh approach to work with `SetLeaderPoseComponent`, the leader needs to be animated. Two sub-options:

**A1: ShadowMesh with its own simple AnimBP**
- Set SkeletalMesh to SKM_Operator_Mono
- Set AnimClass to a simple idle/locomotion ABP
- Attach to capsule root (not camera)
- Independent of PlayerMesh entirely
- Pro: Self-contained, no dependency on GASP or other systems
- Con: Running a second AnimBP; shadow won't match upper-body weapon animations

**A2: ShadowMesh following PlayerMesh (SetLeaderPoseComponent)**
- Set SkeletalMesh to SKM_Operator_Mono
- `SetLeaderPoseComponent(PlayerMesh)` — copies pose from visible arms mesh
- Pro: Shadow matches arm animations exactly
- Con: SKM_Operator_Mono (full body) following SK_Shirt_01a_FPP (arms only) — bone mismatch likely causes issues. Legs won't be animated.

**A3: Use inherited Mesh component AS the shadow mesh (SIMPLEST)**
- Don't create a new component
- Configure the already-existing inherited `GetMesh()` component:
  - Set SkeletalMesh to SKM_Operator_Mono
  - Set `bHiddenInGame = true`
  - Set `CastShadow = true`, `bCastHiddenShadow = true`
  - Set `VisibilityBasedAnimTickOption = AlwaysTickPoseAndRefreshBones`
- When GASP is eventually integrated, this mesh will already be the motion-matching mesh and will be animated
- For now, just set a basic AnimBP or leave in T-pose (shadow of T-pose is still better than no shadow for grounding)

**RECOMMENDED: Option A3 for now.** The inherited Mesh component is already there doing nothing. Configure it for shadow-only rendering. When GASP integration happens (TICKET-006 Phase 3), this mesh gets a real AnimBP anyway. A T-pose shadow is a reasonable placeholder — it still grounds the character in the world.

#### OPTION B: Enable shadow on PlayerMesh directly
Simply set `CastShadow = true` on the existing PlayerMesh.
**Problem:** After K002 fix, PlayerMesh will be SK_Shirt_01a_FPP (arms only). Arms-only shadow looks weird — floating arms shadow on the ground.

#### OPTION C: UE5.5+ First Person Rendering system
Uses `FirstPersonPrimitiveType = WorldSpaceRepresentation` for automatic shadow mesh behavior.
**Problem:** Requires Nanite or Hardware Ray-Traced Shadows. Performance impact. Engine version dependency.

### Recommendation: **Option A3 — configure inherited Mesh component as shadow mesh.** Zero new components. Works now. Gets upgraded automatically when GASP arrives.

### Implementation for Option A3:

**C++ constructor changes (ZP_GraceCharacter.cpp, after PlayerMesh setup):**
```cpp
// Configure inherited Mesh as shadow-only body for world grounding
// When GASP is integrated, this becomes the motion-matching mesh too
USkeletalMeshComponent* BodyMesh = GetMesh();
BodyMesh->SetHiddenInGame(true);
BodyMesh->CastShadow = true;
BodyMesh->bCastDynamicShadow = true;
BodyMesh->bCastHiddenShadow = true;
BodyMesh->SetVisibilityBasedAnimTickOption(EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones);
BodyMesh->SetOnlyOwnerSee(false);  // Shadow must be visible to all (including self)
```

**Python CDO:** Set SkeletalMesh on inherited Mesh to `SKM_Operator_Mono`.

**Known gotchas:**
- `bCastHiddenShadow` has inconsistent behavior with Virtual Shadow Maps in some UE5 versions. Test with both VSM on and off.
- The shadow will be in T-pose until we give it an AnimBP. This is acceptable for prototype.
- If T-pose shadow is distracting, we could set a simple idle animation montage at BeginPlay.

---

<a name="fix-order"></a>
## Recommended Fix Order

| Order | Issue | Effort | Why This Order |
|-------|-------|--------|----------------|
| 1 | **K002** | Medium | Fixes the most visible immersion-breaking bug. Mesh swap + offset tune. |
| 2 | **K004** | Low | Configure existing component. Quick C++ change + Python CDO. |
| 3 | **K001** | Medium | C++ handlers + Python script + build. Validates full Kinemation weapon chain. |
| 4 | **K003** | None | Do nothing. Revisit post-vertical-slice. |

### Dependency Note
K002 (FPP mesh swap) should be done BEFORE K004 (shadow). If we enable shadows on the full-body mesh first, then swap to FPP, we'd need to re-test shadow behavior. Better to swap mesh first, then configure shadow.

---

<a name="implementation-plans"></a>
## Implementation Plans

### Plan: K002 + K004 Together (Recommended First Pass)

**C++ changes (ZP_GraceCharacter.cpp constructor):**
1. Keep PlayerMesh creation as-is (mesh asset set via CDO)
2. Add shadow configuration for inherited Mesh component:
```cpp
USkeletalMeshComponent* BodyMesh = GetMesh();
BodyMesh->SetHiddenInGame(true);
BodyMesh->CastShadow = true;
BodyMesh->bCastDynamicShadow = true;
BodyMesh->bCastHiddenShadow = true;
BodyMesh->SetVisibilityBasedAnimTickOption(
    EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones);
```

**Python script (new: `Scripts/Python/fix_k002_k004.py`):**
1. Set PlayerMesh SkeletalMeshAsset → `SK_Shirt_01a_FPP`
2. Set PlayerMesh RelativeLocation → `(0, 0, 0)` (reset, tune from there)
3. Set inherited Mesh SkeletalMeshAsset → `SKM_Operator_Mono`
4. Save asset

**Build + test:**
- Kill editor → Build.bat → reopen → run Python script → PIE
- Verify: arms visible, no torso clipping, shadow on ground (even if T-pose)

### Plan: K001 (Fire Input)

**C++ changes:**
1. ZP_GraceCharacter.h: add `Input_FireStarted`, `Input_FireCompleted` declarations
2. ZP_GraceCharacter.cpp: add binding + handler implementations
3. Update log line

**Python script (new: `Scripts/Python/setup_fire_input.py`):**
1. Create IA_Fire at `/Game/Core/Input/Actions/IA_Fire`
2. Add LeftMouseButton mapping to IMC_Grace
3. Set FireAction on BP_GraceCharacter CDO

**Build + test:**
- Kill editor → Build.bat → reopen → run Python script → PIE
- Verify: LMB fires weapon, recoil plays, camera shake, muzzle flash, ammo decrements

---

## Files That Will Be Modified

| File | Changes | Issues |
|------|---------|--------|
| `ZP_GraceCharacter.h` | +2 fire handler declarations | K001 |
| `ZP_GraceCharacter.cpp` | +fire bindings in SetupPlayerInputComponent, +fire handlers, +shadow mesh config in constructor | K001, K004 |
| `Scripts/Python/fix_k002_k004.py` (NEW) | FPP mesh swap + shadow mesh setup | K002, K004 |
| `Scripts/Python/setup_fire_input.py` (NEW) | Fire input asset creation + mapping | K001 |
| `BP_GraceCharacter` (via Python/MCP) | PlayerMesh mesh swap, inherited Mesh mesh set, FireAction CDO | K001, K002, K004 |

## Open Questions for User

1. **K001:** Should we also wire ReloadAction (R key) while we're at it? Trivial to add.
2. **K002:** After FPP mesh swap, the PlayerMesh Z offset will need tuning. Start at Z=0 — is visual iteration OK or do you want a calculated starting point?
3. **K004:** T-pose shadow is acceptable for prototype? Or should we find a simple idle animation to set on the shadow mesh?
4. **General:** K002 and K004 can be done in one build cycle. K001 requires a second build cycle. Want them all in one pass (more changes, harder to debug) or two passes?
