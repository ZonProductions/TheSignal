# Checkpoint: Character System — Session 4
**Date:** 2026-02-28 ~04:10
**System:** PlayerCharacter
**Branch:** proto/corridor-v1

---

## Current State

### C++ — COMPILED CLEAN (0 errors, 0 warnings)
- UE 5.4 at `C:\Program Files\Epic Games\UE_5.4`
- Build command: `"C:/Program Files/Epic Games/UE_5.4/Engine/Build/BatchFiles/Build.bat" TheSignalEditor Win64 Development -Project="C:/Users/Ommei/workspace/TheSignal/TheSignal.uproject" -WaitMutex`

**Files:**
- `Source/TheSignal/SignalTacticalCharacter.h` — unchanged from session 3
- `Source/TheSignal/SignalTacticalCharacter.cpp` — added `#include "UObject/UnrealType.h"`, added `TacticalViewSettings` path to namespace, has TODO for ActiveSettings property setting
- `Source/TheSignal/KinemationBridge.h/.cpp` — unchanged from session 3

### BP_Grace — CONFIGURED AND VALIDATES CLEAN
- Path: `/Game/Player/Blueprints/BP_Grace`
- Parent: `SignalTacticalCharacter`
- Interface: `BPI_TacticalShooterCharacter` — all 4 stubs wired
- GM_TheSignal DefaultPawnClass = BP_Grace

### PIE — NOT WORKING (2 blockers)

**BLOCKER 1: DA_TacticalCharacterSettings fails to load in constructor**
```
LogUObjectGlobals: Warning: Failed to find object 'DataAsset /Game/KINEMATION/TacticalShooterPack/Blueprints/Data/DA_TacticalCharacterSettings.DA_TacticalCharacterSettings'
LogUObjectGlobals: Error: CDO Constructor (SignalTacticalCharacter): Failed to find /Game/KINEMATION/TacticalShooterPack/Blueprints/Data/DA_TacticalCharacterSettings
```
- ConstructorHelpers::FObjectFinder<UDataAsset> can't find it — likely a Blueprint DataAsset (not a raw UDataAsset), so LoadClass pattern or correct type needed
- CharacterSettings ends up null

**BLOCKER 2: ActiveSettings None on AC_TacticalShooterAnimation**
```
PIE: Error: Blueprint Runtime Error: "Accessed None trying to read property ActiveSettings"
```
- Floods log with errors from ComputeMoveSway, ComputeMoveTargetSway, ComputeTargetAimSway, ComputeReadyPose
- Component created dynamically via NewObject — SCS per-instance defaults not inherited
- DA_TacticalShooterViewSettings needs to be loaded and set on the component
- **CRASHED when using TFieldIterator + SetObjectPropertyValue_InContainer** (assertion ContainerPtr at UnrealType.h:625)
- Property reflection approach abandoned — need safer method (ProcessEvent or MCP investigation)

### What DOES work in PIE:
- Character spawns (BP_Grace_C_0 visible in log)
- All 4 Kinemation components created OK (Camera, Anim, Recoil, IK)
- FPSCamera attached to FPCamera socket
- IMC_TacticalShooterPack applied
- No crash (just warnings)

### What does NOT work in PIE:
- No movement (input not responding)
- No body/weapon visible
- ActiveSettings flood errors break Kinemation animation system

---

## Root Cause Analysis

### DA_TacticalCharacterSettings
The path `/Game/KINEMATION/TacticalShooterPack/Blueprints/Data/DA_TacticalCharacterSettings` is likely a **Blueprint DataAsset** (a Blueprint whose parent is UDataAsset or a C++ DataAsset subclass). `ConstructorHelpers::FObjectFinder<UDataAsset>` looks for raw UDataAsset instances, not Blueprint-wrapped ones.

**Investigation needed:**
1. Check if DA_TacticalCharacterSettings is a Blueprint or raw DataAsset via MCP or filesystem
2. If Blueprint: need to load the Blueprint class and get CDO, or load a specific instance
3. If raw DataAsset with wrong type: need correct UClass for FObjectFinder

### ActiveSettings on AC_TacticalShooterAnimation
The AC_TacticalShooterAnimation Blueprint component class has a variable `ActiveSettings` of type `DA_TacticalShooterViewSettings_C`. When the component exists as SCS on the Kinemation demo character, the SCS template sets this to a specific DataAsset instance. When we create via `NewObject`, we only get CDO defaults (which have ActiveSettings = None).

**Investigation needed:**
1. Use MCP to examine the Kinemation demo character's SCS — what DataAsset instance is ActiveSettings set to?
2. Or: examine AC_TacticalShooterAnimation's CDO defaults to see if ActiveSettings is set there
3. Find the actual DataAsset instance path to load
4. Find a safe way to set it (ProcessEvent on setter function, or MCP set_component_property on BP_Grace SCS)

### No Movement
Could be caused by:
1. CharacterSettings being null (constructor load failed)
2. ActiveSettings being null breaking the animation system
3. Input mapping context not actually binding (though log says "Applied IMC_TacticalShooterPack")
4. Some other initialization dependency

---

## Attempted Fixes This Session

### Attempt 1: TFieldIterator + SetObjectPropertyValue_InContainer
```cpp
for (TFieldIterator<FObjectPropertyBase> It(TacticalAnimComp->GetClass()); It; ++It)
{
    if (It->GetName().StartsWith(TEXT("ActiveSettings")))
    {
        It->SetObjectPropertyValue_InContainer(TacticalAnimComp, ViewSettings);
    }
}
```
**RESULT: CRASH** — Assertion failed: ContainerPtr at UnrealType.h:625

### Attempt 2: TFieldIterator<FProperty> + CastField + SetObjectPropertyValue
```cpp
FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(*It);
uint8* ValuePtr = It->ContainerPtrToValuePtr<uint8>(TacticalAnimComp);
ObjProp->SetObjectPropertyValue(ValuePtr, ViewSettings);
```
**RESULT: NOT TESTED** — removed before build because same pattern likely crashes

### Current state: Property setting code removed, TODO comment in place

---

## EXACT NEXT STEPS

### Step 1: Investigate DataAsset types via MCP
While editor is running:
```
get_blueprint_summary("AC_TacticalShooterAnimation")  — check ActiveSettings variable details
get_blueprint_summary("DA_TacticalShooterViewSettings")  — is this a Blueprint class or DataAsset instance?
get_blueprint_summary("DA_TacticalCharacterSettings")  — same question
```

### Step 2: Find the actual DataAsset instances
```
list_blueprints(filter="DA_Tactical")  — find all related DataAsset assets
```
Also check filesystem for .uasset files in Data folder.

### Step 3: Fix DA_TacticalCharacterSettings loading
Based on investigation, either:
- Fix the ConstructorHelpers path/type
- Or move to runtime loading in BeginPlay instead of constructor

### Step 4: Fix ActiveSettings on TacticalAnimComp
Options (ranked by safety):
1. **MCP approach**: Add AC_TacticalShooterAnimation as SCS component on BP_Grace instead of dynamic creation, set ActiveSettings via set_component_property
2. **ProcessEvent approach**: Find/create a setter function on the component, call via KinemationBridge
3. **Property reflection approach**: Fix the crash (maybe the loaded object was wrong type?)
4. **Hybrid**: Create all 4 Kinemation components as SCS on BP_Grace via MCP, abandon dynamic creation

### Step 5: Rebuild + PIE test

---

## Key File Paths
- SignalTacticalCharacter.cpp: `C:\Users\Ommei\workspace\TheSignal\Source\TheSignal\SignalTacticalCharacter.cpp`
- KinemationBridge: `C:\Users\Ommei\workspace\TheSignal\Source\TheSignal\KinemationBridge.h/.cpp`
- BP_Grace: `/Game/Player/Blueprints/BP_Grace`
- GM_TheSignal: `/Game/Core/GM_TheSignal`
- DA_TacticalCharacterSettings: `/Game/KINEMATION/TacticalShooterPack/Blueprints/Data/DA_TacticalCharacterSettings`
- DA_TacticalShooterViewSettings: `/Game/KINEMATION/TacticalShooterPack/Blueprints/Data/DA_TacticalShooterViewSettings`
- AC_TacticalShooterAnimation: `/Game/KINEMATION/TacticalShooterPack/Blueprints/AC_TacticalShooterAnimation`

## Open Tickets
- TICKET-007: First-person player movement — IN PROGRESS (blocked by above)
- TICKET-009: Fix Kinemation character locomotion — IN PROGRESS
- TICKET-010: Download and integrate GASP — OPEN (blocks full locomotion)
- TICKET-008: Ren communication proof-of-concept — PLANNED
