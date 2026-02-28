# Checkpoint: GASP Phase 1 — Component Extraction

**Date:** 2026-02-28
**Status:** COMPILED — awaiting PIE verification after timing fix

---

## What Was Done

### Extracted gameplay logic into two reusable ActorComponents

**Goal:** Extract all gameplay and Kinemation logic from `AZP_GraceCharacter` into portable ActorComponents so they can be reused on a GASP-based character Blueprint in Phase 2.

### UZP_GraceGameplayComponent (NEW)
- Stamina system (StartSprint, StopSprint, UpdateStamina, regen delay)
- Peek/cover system (5-ray fan wall detection, direction locking, camera lean + roll)
- Head bob (velocity-based, sprint multiplier, peek damping)
- Interaction trace (camera-forward line trace, EventBroadcaster integration)
- Movement config application (applies DataAsset values to CMC + camera)
- Auto-discovers CameraComponent by name "FirstPersonCamera"
- Ticks itself via TickComponent

### UZP_KinemationComponent (NEW)
- Kinemation auto-detection (AC_FirstPersonCamera, AC_TacticalShooterAnimation, AC_RecoilAnimation, AC_IKMotionPlayer)
- Camera API (TriggerCameraShake, SetTargetFOV)
- Weapon lifecycle (SpawnAndEquipWeapon with deferred WeaponDraw)
- Fire/Reload forwarding via FKinemationBridge
- Auto-discovers CameraComponent and PlayerMeshComponent by name
- **InitializeKinemation()** — explicit call, NOT in BeginPlay (timing-critical)

### AZP_GraceCharacter refactored to thin shell
- Constructor: creates camera, mesh, GameplayComp, KinemationComp
- PostInitializeComponents: propagates config (MovementConfig, WeaponClass, camera/mesh refs) to components
- BeginPlay: calls `KinemationComp->InitializeKinemation()` after Super, disables headbob if Kinemation active
- SetupPlayerInputComponent: binds all 9 input actions, delegates to components
- PrimaryActorTick.bCanEverTick = false (components tick themselves)

### Files Created/Modified
| File | Action |
|------|--------|
| `ZP_GraceGameplayComponent.h` | NEW — gameplay component header |
| `ZP_GraceGameplayComponent.cpp` | NEW — gameplay component implementation |
| `ZP_KinemationComponent.h` | NEW — Kinemation component header |
| `ZP_KinemationComponent.cpp` | NEW — Kinemation component implementation |
| `ZP_GraceCharacter.h` | REFACTORED — thin shell, components own the logic |
| `ZP_GraceCharacter.cpp` | REFACTORED — delegates to components |

---

## Bug Found and Fixed

### Component BeginPlay Ordering Issue
- **Symptom:** Arms invisible, weapon can't fire (peek and sprint still worked)
- **Root cause:** KinemationComponent's BeginPlay ran DURING `Super::BeginPlay()`, before SCS Blueprint components (AC_FirstPersonCamera, etc.) had their BeginPlay. Those components' initialization overwrote our wiring.
- **Fix:** Moved Kinemation wiring out of component BeginPlay into explicit `InitializeKinemation()` called from character's BeginPlay AFTER Super. This matches original timing.
- **Build status:** Compiled clean (0 errors, 0 warnings). Awaiting PIE test.

---

## Architecture Notes

### Config propagation pattern
- Character keeps MovementConfig and WeaponClass as UPROPERTY (for BP defaults compatibility)
- PostInitializeComponents propagates them to components before BeginPlay
- Components also accept direct property setting (for Phase 2 GASP BP via SCS defaults)

### Component discovery pattern
- Components auto-discover camera/mesh by name ("FirstPersonCamera", "PlayerMesh")
- Works for both AZP_GraceCharacter (C++ subobjects) and future GASP BP (SCS components)

### EZP_PeekDirection enum
- Moved from ZP_GraceCharacter.h to ZP_GraceGameplayComponent.h
- UE reflection registers enums globally — no BP breakage

---

## PIE Test Checklist (pending)
- [ ] Arms + TR15 visible and animated
- [ ] Fire (LMB) works
- [ ] Reload (R) works
- [ ] Sprint (Shift) works with stamina drain/regen
- [ ] Peek (RMB near walls) works with camera lean
- [ ] Head bob disabled (Kinemation active)
- [ ] Jump, crouch, interact all functional

## Next Steps
1. PIE verify the timing fix
2. If stable → proceed to Phase 2 (GASP character Blueprint)
3. Phase 2 starts with duplicating BP_TacticalShooterCharacter, reparenting to CBP_SandboxCharacter

## Build Info
- Branch: main
- Last build: clean (7 actions, 4.12s)
- Editor must be reopened for PIE test
