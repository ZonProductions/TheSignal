# Checkpoint: Character System — Session 5
**Date:** 2026-02-27 ~22:45
**System:** PlayerCharacter
**Branch:** proto/corridor-v1

---

## Current State

### C++ — COMPILED CLEAN (0 errors, 0 warnings)
- Build command: `"C:/Program Files/Epic Games/UE_5.4/Engine/Build/BatchFiles/Build.bat" TheSignalEditor Win64 Development -Project="C:/Users/Ommei/workspace/TheSignal/TheSignal.uproject" -WaitMutex`

**Files changed this session:**
- `Source/TheSignal/KinemationBridge.h` — added `AnimSetActiveSettings` declaration
- `Source/TheSignal/KinemationBridge.cpp` — added `#include "UObject/UnrealType.h"`, added `AnimSetActiveSettings` implementation (FProperty-based Blueprint variable setter)
- `Source/TheSignal/SignalTacticalCharacter.cpp` — removed broken FObjectFinder for DA_TacticalCharacterSettings, removed unused KinemationPaths::Settings and KinemationPaths::TacticalViewSettings, added CDO fallback for ActiveSettings in BeginPlay, added debug logging (Input_Move, Input_Look, SetupPlayerInputComponent)

### Blocker A: DA_TacticalCharacterSettings — FIXED
- Removed `ConstructorHelpers::FObjectFinder<UDataAsset>` from constructor (line was ~100)
- Root cause confirmed: DA_TacticalCharacterSettings is a **Blueprint PrimaryDataAsset class** (BPTYPE_Normal), not a raw UDataAsset instance — FObjectFinder can't find Blueprint classes
- CharacterSettings not currently needed — mesh/AnimBP loaded directly by path
- No more "Failed to find object" error in log

### Blocker B: ActiveSettings None — FIXED
- Added `FKinemationBridge::AnimSetActiveSettings()` — uses FProperty reflection to set Blueprint variable on dynamically-created BP component
- In BeginPlay: tries weapon's GetSettings first, falls back to loading DA_TacticalShooterViewSettings Blueprint class CDO
- Log confirms: `Set ActiveSettings on AC_TacticalShooterAnimation to Default__DA_TacticalShooterViewSettings_C`
- **No more "Accessed None trying to read property ActiveSettings" error spam**

### PIE — PARTIALLY WORKING (new issue: no movement)
- Character spawns, all 4 Kinemation components created OK
- ActiveSettings set successfully (CDO fallback)
- FPSCamera attached to FPCamera socket
- IMC_TacticalShooterPack applied
- **Jump works** (space bar causes jiggle — character lifts slightly)
- **WASD does NOT move** the character
- **Mouse does NOT look** around
- Warning: "SetAiming not found on AnimComp" — AC_TacticalShooterAnimation has no SetAiming function (it's a variable `IsAiming`, not a function)
- Debug logging added (Log level) to Input_Move, Input_Look, and SetupPlayerInputComponent — **NOT YET TESTED** (editor was just rebuilt, user hasn't reopened)

---

## MCP Investigation Results (this session)

### DA_TacticalCharacterSettings
- Blueprint class, parent: PrimaryDataAsset, path: `/Game/KINEMATION/TacticalShooterPack/Blueprints/Data/DA_TacticalCharacterSettings`
- Variables: UE4Mesh (F_SkeletalMeshData), UE5Mesh (F_SkeletalMeshData), Weapons (BP_TacticalShooterWeapon_C[])
- Used by BP_TacticalShooterCharacter in UpdateActiveMesh to swap mesh/AnimBP between UE4/UE5

### DA_TacticalShooterViewSettings
- Blueprint class, parent: PrimaryDataAsset, path: `/Game/KINEMATION/TacticalShooterPack/Blueprints/Data/DA_TacticalShooterViewSettings`
- Variables: IdlePose, Viewmodel, sway/locomotion settings (11 total)
- Each weapon has UE5ViewSettings and UE4ViewSettings referencing instances of this class

### AC_TacticalShooterAnimation
- Blueprint ActorComponent, 21 variables, 9 graphs
- ActiveSettings (DA_TacticalShooterViewSettings_C) — read every Tick by sway functions
- Has NO SetAiming function — IsAiming is a plain variable (must be set via FProperty, not ProcessEvent)
- Even on demo character BP_TacticalShooterCharacter, SCS template has ActiveSettings = None (set at runtime from weapon)

### BP_TacticalShooterCharacter (Kinemation demo)
- Has all 4 Kinemation components as SCS (not dynamic NewObject)
- UpdateActiveMesh: gets weapon → GetSettings → sets ActiveSettings on anim comp
- EventGraph (231 nodes) handles weapon spawning, input, UpdateActiveMesh

### MCP Tool Availability
- `set_component_property` — **CONFIRMED AVAILABLE** (was incorrectly marked unavailable in memory)
- `get_component_properties` — **CONFIRMED AVAILABLE**
- Memory updated to reflect this

---

## Failure Log

| Attempt | Result |
|---------|--------|
| FObjectFinder<UDataAsset> for Blueprint DataAsset | FAILED — Blueprint classes aren't UDataAsset instances (Blocker A root cause) |
| TFieldIterator + SetObjectPropertyValue_InContainer (session 4) | CRASHED — ContainerPtr assertion. Dead end. |
| FProperty::FindPropertyByName + ContainerPtrToValuePtr + SetObjectPropertyValue (this session) | **WORKS** — used in AnimSetActiveSettings, no crash |
| CDO fallback for ActiveSettings when no weapons | **WORKS** — eliminates None errors |

---

## Next Action

**Immediate: Diagnose no-movement issue**
1. Open editor, run PIE, press WASD + move mouse
2. Check Output Log for `Input_Move` and `Input_Look` lines (debug logging already added)
3. If logs appear → input fires but movement isn't applying (check CharacterMovementComponent)
4. If logs DON'T appear → input actions aren't triggering (check IMC bindings, action triggers)
5. Also check for `SetupPlayerInputComponent: EIC valid` line to confirm binding setup

**After movement is fixed:**
- Fix "SetAiming not found" — change `AnimSetAiming` to use FProperty (set IsAiming variable) instead of ProcessEvent (no SetAiming function exists)
- Add Gait variable setting on AC_TacticalShooterAnimation (for locomotion blend)
- Consider adding weapons to BP_Grace defaults or spawning them in BeginPlay
- GASP integration (TICKET-010)

---

## Open Questions
1. Why doesn't WASD/mouse work when jump does? All use same Enhanced Input system. Debug logs will answer.
2. Should we set IsAiming via FProperty like ActiveSettings, or add a KinemationBridge property setter helper?
3. Weapons array on BP_Grace is empty — need to decide: set via MCP Blueprint defaults, or spawn in C++ BeginPlay?

---

## Assumptions
- UE 5.4 at `C:\Program Files\Epic Games\UE_5.4`
- Branch: proto/corridor-v1
- BP_Grace at `/Game/Player/Blueprints/BP_Grace`, parent: SignalTacticalCharacter
- GM_TheSignal DefaultPawnClass = BP_Grace
- Enhanced Input properly configured (DefaultInput.ini lines 79-80)
- Auto-kill editor before build: `cmd.exe //c "taskkill /F /IM UnrealEditor.exe"`
