# Checkpoint: Character System — Session 5b
**Date:** 2026-02-27 ~23:00
**System:** PlayerCharacter
**Branch:** proto/corridor-v1

---

## Current State

### C++ — COMPILED CLEAN (0 errors, 0 warnings)
- Build command: `"C:/Program Files/Epic Games/UE_5.4/Engine/Build/BatchFiles/Build.bat" TheSignalEditor Win64 Development -Project="C:/Users/Ommei/workspace/TheSignal/TheSignal.uproject" -WaitMutex`

### Blocker A: DA_TacticalCharacterSettings — FIXED
- Removed broken `ConstructorHelpers::FObjectFinder<UDataAsset>` from constructor
- Root cause: DA_TacticalCharacterSettings is a Blueprint PrimaryDataAsset class, not a raw UDataAsset instance

### Blocker B: ActiveSettings None — FIXED
- Added `FKinemationBridge::AnimSetActiveSettings()` using FProperty reflection
- BeginPlay loads DA_TacticalShooterViewSettings Blueprint class CDO as fallback
- Log confirms: `Set ActiveSettings on AC_TacticalShooterAnimation to Default__DA_TacticalShooterViewSettings_C`
- **No more "Accessed None" error spam**

### NEW BLOCKER: No WASD/Mouse Movement
- Character spawns, components created, ActiveSettings set — all good
- Jump works (space bar), Aim works (SetAiming warning fires from right-click)
- **WASD and Mouse do NOT produce movement or look**
- All input actions loaded: `Move=OK Look=OK Jump=OK Aim=OK`
- `Input_Move` and `Input_Look` C++ callbacks NEVER fire (no log output)
- Bound to all 3 trigger events (Started+Triggered+Completed) — still doesn't fire
- Enhanced Input properly configured (DefaultInput.ini: EnhancedPlayerInput + EnhancedInputComponent)
- IMC_TacticalShooterPack applied successfully

### Active Debug Instrumentation
On-screen debug text shows (4 lines):
1. Green: MoveInput values + Speed + Gait
2. Yellow: Action pointer status (all OK)
3. Cyan: Velocity + Location
4. **Red: RAW Enhanced Input polling** (GetActionValue directly from UEnhancedPlayerInput) — **NOT YET TESTED**

The red line bypasses the callback system entirely and polls the action value directly. This will tell us:
- If RAW values change when pressing WASD → Enhanced Input receives input but BindAction callbacks are broken
- If RAW values stay at 0 → IMC doesn't map WASD to IA_Move, or something else is wrong upstream

---

## Key Investigation Results

### MCP Investigation (DataAssets)
- DA_TacticalCharacterSettings: Blueprint class (PrimaryDataAsset), has UE4Mesh/UE5Mesh/Weapons
- DA_TacticalShooterViewSettings: Blueprint class (PrimaryDataAsset), has sway/locomotion settings
- AC_TacticalShooterAnimation: has NO `SetAiming` function — `IsAiming` is a plain variable
- Even demo character starts with ActiveSettings=None on SCS (set at runtime from weapon)
- `set_component_property` / `get_component_properties` MCP tools CONFIRMED AVAILABLE

### Input Investigation
- All IA_*.uasset files exist at `/Game/KINEMATION/TacticalShooterPack/Inputs/`
- IMC_TacticalShooterPack.uasset exists
- Jump (digital/bool action) works with ETriggerEvent::Started
- Aim (digital/bool action) works with ETriggerEvent::Started
- Move (Axis2D action) does NOT work with Started/Triggered/Completed
- Look (Axis2D action) does NOT work with Started/Triggered/Completed
- Key difference: working actions are digital (bool), non-working are Axis2D

---

## Files Changed This Session

### KinemationBridge.h
- Added `AnimSetActiveSettings(UObject* AnimComp, UObject* Settings)` declaration

### KinemationBridge.cpp
- Added `#include "UObject/UnrealType.h"`
- Added `AnimSetActiveSettings` implementation (FProperty-based BP variable setter)

### SignalTacticalCharacter.cpp
- Added `#include "EnhancedPlayerInput.h"`
- Removed broken FObjectFinder for DA_TacticalCharacterSettings
- Removed unused KinemationPaths::Settings and KinemationPaths::TacticalViewSettings
- Added CDO fallback for ActiveSettings in BeginPlay
- Changed Move/Look bindings to Started+Triggered+Completed
- Added on-screen debug text (4 lines: MoveInput, Actions, Vel/Loc, RAW polling)
- Added Input_Move and Input_Look log statements (Log level)
- Added SetupPlayerInputComponent log statement

---

## Next Action

1. **Open editor, run PIE, click viewport, press WASD** — check RED debug line for RAW input values
2. Based on result:
   - **If RAW values change:** BindAction callback system is broken — try using Tick-based input polling instead of callbacks
   - **If RAW values stay 0:** IMC doesn't map WASD to IA_Move — need to inspect IMC contents or try a different IMC
3. After movement works:
   - Fix "SetAiming not found" → use FProperty to set IsAiming variable directly (not ProcessEvent)
   - Set Gait variable on AC_TacticalShooterAnimation
   - Add weapons to BP_Grace
   - Remove debug instrumentation

---

## Known Issues (non-blocking)
- "SetAiming not found on AnimComp" — AC_TacticalShooterAnimation has no SetAiming function; IsAiming is a plain variable. Need to change AnimSetAiming to use FProperty instead of ProcessEvent.
- Weapons array on BP_Grace is empty — no weapon equipped, CDO fallback used for ActiveSettings
- Debug on-screen text active — remove after movement is fixed

---

## Assumptions
- UE 5.4 at `C:\Program Files\Epic Games\UE_5.4`
- Branch: proto/corridor-v1
- BP_Grace at `/Game/Player/Blueprints/BP_Grace`, parent: SignalTacticalCharacter
- GM_TheSignal DefaultPawnClass = BP_Grace
- Auto-kill editor before build: `cmd.exe //c "taskkill /F /IM UnrealEditor.exe"`
