# Checkpoint: C++ Config Consolidation
**Date:** 2026-02-28
**Status:** DONE — clean build, all values in one header

## What Changed

### Problem
Movement tuning values were scattered across 4 locations:
- `ZP_GraceMovementConfig.h` UPROPERTY defaults
- `DA_GraceMovement_Default` DataAsset (binary)
- Ternary fallbacks in `ZP_GraceGameplayComponent.cpp` (16 instances)
- Hardcoded constructor values in `ZP_GraceCharacter.cpp`

Changing a value required knowing which file to edit. Ternary fallbacks meant the DataAsset could be missing and behavior would silently differ from config defaults.

### Solution
**Self-constructing config.** If no DataAsset is assigned, `PostInitializeComponents` (character) and `BeginPlay` (gameplay component) create a transient `UZP_GraceMovementConfig` via `NewObject<>()`. All values come from UPROPERTY defaults in the .h file.

### Files Modified

1. **`ZP_GraceMovementConfig.h`**
   - Added: `CapsuleRadius` (34.0f), `CapsuleHalfHeight` (88.0f), `CrouchedHalfHeight` (58.0f), `PlayerMeshOffsetZ` (-90.0f)
   - Synced: `HeadBobVerticalAmplitude` 1.5f → 0.8f, `HeadBobHorizontalAmplitude` 0.8f → 0.4f
   - This is now the SINGLE SOURCE OF TRUTH for all movement tuning

2. **`ZP_GraceGameplayComponent.cpp`**
   - `BeginPlay`: creates default config via `NewObject<>()` if null
   - Removed all 16 ternary `MovementConfig ? X : fallback` patterns → direct access
   - Removed early-return guard in `ApplyMovementConfig()`

3. **`ZP_GraceCharacter.cpp`**
   - `PostInitializeComponents`: creates default config if null, applies capsule size, mesh offset, crouch height, all movement values from config
   - Constructor retains hardcoded values as safe CDO defaults (constructor can't access config)

### What Was NOT Changed
- `ZP_GraceMovementConfig` remains a `UDataAsset` subclass — can still create DataAsset instances later for level-specific tuning
- `EditDefaultsOnly` UPROPERTY on character for MovementConfig — BP override still works
- Constructor hardcoded values kept as CDO safety net

### How to Tune Values Now
1. Open `ZP_GraceMovementConfig.h`
2. Change the UPROPERTY default
3. Kill editor → rebuild → reopen
4. Values take effect with no DataAsset needed

### Verification
- Clean build (0 errors, 0 warnings)
- PIE test needed: walk, sprint, peek (Q/RMB), head bob
