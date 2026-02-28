# Checkpoint: ZP_ C++ Foundation — Clean Rebuild
**Date:** 2026-02-28
**System:** Core / PlayerCharacter
**Branch:** main (HEAD)

---

## Current State

### BUILD: COMPILED CLEAN (0 errors, 0 warnings)
- 18/18 actions, 3.64 seconds
- Build command: `"C:/Program Files/Epic Games/UE_5.4/Engine/Build/BatchFiles/Build.bat" TheSignalEditor Win64 Development "C:/Users/Ommei/workspace/TheSignal/TheSignal.uproject" -waitmutex`
- Kill editor before build: `powershell -Command "Stop-Process -Name 'UnrealEditor' -Force -ErrorAction SilentlyContinue; Stop-Process -Name 'LiveCodingConsole' -Force -ErrorAction SilentlyContinue"`

### What Was Done
Nuked the old hack-layered character system (SignalTacticalCharacter + TheSignalCharacter) and built clean ZP_ C++ foundation from scratch per The_Signal.md architecture spec.

---

## Files Created (14 new source files)

| File | Class | Purpose |
|------|-------|---------|
| `ZP_EventBroadcaster.h/cpp` | `UZP_EventBroadcaster` | GameInstanceSubsystem — typed event bus (FacilityPower, NarrativeBeat, Sprint, Stamina, InteractionTarget) |
| `ZP_GraceMovementConfig.h/cpp` | `UZP_GraceMovementConfig` | UDataAsset — all movement tuning (walk/sprint speed, stamina drain/regen/delay, head bob freq/amp, camera sway, jump, interaction range, camera height, FOV) |
| `ZP_GameInstance.h/cpp` | `UZP_GameInstance` | Persistent state, verifies EventBroadcaster on Init |
| `ZP_GameMode.h/cpp` | `AZP_GameMode` | Sets defaults: ZP_GraceCharacter pawn, ZP_PlayerController, ZP_GameState |
| `ZP_GameState.h/cpp` | `AZP_GameState` | Game-wide state: CurrentAct (int32), FacilityBreachLevel (float), bContainmentBreached (bool) |
| `ZP_PlayerController.h/cpp` | `AZP_PlayerController` | IMC application + runtime Add/RemoveMappingContext helpers |
| `ZP_GraceCharacter.h/cpp` | `AZP_GraceCharacter` | Full character — see details below |

### ZP_GraceCharacter Details
- UCameraComponent attached to capsule root (NOT skeleton), bUsePawnControlRotation=true
- Movement config from UZP_GraceMovementConfig DataAsset (with hardcoded fallback defaults)
- Enhanced Input bindings: Move, Look, Sprint, Jump, Interact (+ AimAction, FireAction, ReloadAction stubs for Kinemation)
- Head bob system ported from TheSignalCharacter (DataAsset-driven values)
- Stamina system (drain while sprinting, regen delay, forced stop at 0)
- Interaction trace (line trace from camera each Tick, broadcasts on target change)
- `bUseBuiltInHeadBob` flag — disable when Kinemation controls camera
- UPROPERTY slots for Kinemation: TacticalCameraComp, TacticalAnimComp, RecoilAnimComp, IKMotionComp (BlueprintReadWrite, set by BP child)
- All events broadcast through UZP_EventBroadcaster (sprint, stamina, interaction target)

## Files Deleted (10 source files)
- `SignalTacticalCharacter.h/cpp` — hack-layered mess with dynamic Kinemation component creation
- `TheSignalCharacter.h/cpp` — code ported into ZP_GraceCharacter
- `TheSignalGameMode.h/cpp` — replaced by ZP_GameMode
- `TheSignalGameInstance.h/cpp` — replaced by ZP_GameInstance
- `TheSignalPlayerController.h/cpp` — replaced by ZP_PlayerController

## Files Kept (unchanged)
- `KinemationBridge.h/cpp` — working BP component reflection utility
- `TheSignal.h/cpp` — module boilerplate
- `TheSignal.Build.cs` — deps unchanged (Core, CoreUObject, Engine, InputCore, EnhancedInput)

## Config Changes
- `DefaultEngine.ini`: Added `GlobalDefaultGameMode=/Script/TheSignal.ZP_GameMode` and `GameInstanceClass=/Script/TheSignal.ZP_GameInstance` under `[/Script/EngineSettings.GameMapsSettings]`

---

## Final Source File List
```
Source/TheSignal/
├── KinemationBridge.h/cpp          (kept)
├── TheSignal.h/cpp                 (kept — module)
├── TheSignal.Build.cs              (kept)
├── ZP_EventBroadcaster.h/cpp       (NEW)
├── ZP_GameInstance.h/cpp            (NEW)
├── ZP_GameMode.h/cpp                (NEW)
├── ZP_GameState.h/cpp               (NEW)
├── ZP_GraceCharacter.h/cpp          (NEW)
├── ZP_GraceMovementConfig.h/cpp     (NEW)
└── ZP_PlayerController.h/cpp        (NEW)
```

---

## Next Steps (Phase 2: Character + Kinemation Integration)

1. Open editor, verify ZP_GameMode is active (check log for `[TheSignal] ZP_GameMode constructed`)
2. Create `DA_GraceMovement_Default` DataAsset (instance of ZP_GraceMovementConfig) in editor
3. Create Input Actions: IA_Move (Axis2D), IA_Look (Axis2D), IA_Sprint (Bool), IA_Jump (Bool), IA_Interact (Bool)
4. Create IMC_Grace (Input Mapping Context) with WASD→IA_Move, Mouse→IA_Look, Shift→IA_Sprint, Space→IA_Jump, E→IA_Interact
5. Create `BP_GraceCharacter` Blueprint extending ZP_GraceCharacter — assign DataAsset + Input Actions
6. Create `PC_Grace` Blueprint extending ZP_PlayerController — assign IMC_Grace
7. Create `GM_TheSignal` Blueprint extending ZP_GameMode — set DefaultPawnClass=BP_GraceCharacter, PlayerControllerClass=PC_Grace
8. PIE test: verify WASD movement, mouse look, head bob, sprint+stamina, jump
9. When Kinemation plugin installed: add SCS components to BP_GraceCharacter, implement BPI_TacticalShooterCharacter, set Kinemation refs, disable bUseBuiltInHeadBob

## Known Issues
- **BP_Grace from previous sessions references deleted SignalTacticalCharacter** — this BP is now broken. Create fresh BP_GraceCharacter from ZP_GraceCharacter instead.
- **Input Action Axis2D issue** from session 5b may recur — if IA_Move/IA_Look don't fire callbacks, investigate IMC modifier setup or create custom IMC with explicit axis mappings.
- **No Build.bat wrapper exists** — use full UE5 Build.bat path directly.

## Assumptions
- UE 5.4 at `C:\Program Files\Epic Games\UE_5.4`
- Branch: main (HEAD) — plan says proto/corridor-v1 but current work is on main
- BlueprintMCP plugin installed at `Plugins/BlueprintMCP/`
- `python` (not `python3`) on this Windows system
