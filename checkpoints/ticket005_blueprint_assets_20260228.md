# Checkpoint: TICKET-005 Blueprint Assets Created
**Date:** 2026-02-28
**System:** Core / PlayerCharacter / Input
**Branch:** main (HEAD)

---

## Current State

### All Blueprint assets created and configured via MCP + Python automation
- DA_GraceMovement_Default, 5 Input Actions, IMC_Grace, BP_GraceCharacter, PC_Grace, GM_TheSignal
- CDO defaults set and verified via Python
- DefaultEngine.ini updated to point to GM_TheSignal Blueprint
- **Awaiting PIE test** to verify movement/input works

---

## Bug Fix: .uproject Missing Modules Section

The engine couldn't find ZP_ classes at runtime (`Failed to find object 'Class /Script/TheSignal.ZP_GameInstance'`) despite successful build. Root cause: `TheSignal.uproject` was missing the `Modules` section — the DLL was built but never loaded.

**Fix applied:**
```json
"Modules": [
    {
        "Name": "TheSignal",
        "Type": "Runtime",
        "LoadingPhase": "Default"
    }
],
```

After fix, PIE confirmed all ZP_ classes loading correctly:
```
LogLoad: Game class is 'ZP_GameMode'
LogTemp: [TheSignal] ZP_GameMode constructed — DefaultPawn: ZP_GraceCharacter, Controller: ZP_PlayerController
LogTemp: [TheSignal] ZP_GraceCharacter BeginPlay — Config: NONE (using defaults), Stamina: 100
```

## Bug Fix: Stale Hot-Reload Artifacts

Deleted `UnrealEditor-TheSignal.patch_0.*` files from `Binaries/Win64/` — leftover from previous Live Coding session, potentially interfering with DLL loading.

---

## Assets Created

### Data Assets
| Asset | Path | Type |
|-------|------|------|
| DA_GraceMovement_Default | `/Game/Core/Data/DA_GraceMovement_Default` | UZP_GraceMovementConfig instance |

### Input Actions
| Asset | Path | Value Type |
|-------|------|------------|
| IA_Move | `/Game/Core/Input/Actions/IA_Move` | Axis2D |
| IA_Look | `/Game/Core/Input/Actions/IA_Look` | Axis2D |
| IA_Sprint | `/Game/Core/Input/Actions/IA_Sprint` | Boolean |
| IA_Jump | `/Game/Core/Input/Actions/IA_Jump` | Boolean |
| IA_Interact | `/Game/Core/Input/Actions/IA_Interact` | Boolean |

### Input Mapping Context
| Asset | Path | Mappings |
|-------|------|----------|
| IMC_Grace | `/Game/Core/Input/IMC_Grace` | 8 mappings (see below) |

**IMC_Grace Mappings:**
| Key | Action | Modifiers |
|-----|--------|-----------|
| W | IA_Move | SwizzleAxis(YXZ) |
| S | IA_Move | SwizzleAxis(YXZ), Negate |
| A | IA_Move | Negate |
| D | IA_Move | (none) |
| Mouse2D | IA_Look | (none) |
| LeftShift | IA_Sprint | (none) |
| SpaceBar | IA_Jump | (none) |
| E | IA_Interact | (none) |

### Blueprints
| Asset | Path | Parent | CDO Defaults Set |
|-------|------|--------|-----------------|
| BP_GraceCharacter | `/Game/Core/Player/BP_GraceCharacter` | ZP_GraceCharacter | MovementConfig, MoveAction, LookAction, SprintAction, JumpAction, InteractAction |
| PC_Grace | `/Game/Core/Player/PC_Grace` | ZP_PlayerController | DefaultMappingContext = IMC_Grace |
| GM_TheSignal | `/Game/Core/Framework/GM_TheSignal` | ZP_GameMode | DefaultPawnClass = BP_GraceCharacter, PlayerControllerClass = PC_Grace |

---

## Config Changes

- `TheSignal.uproject`: Added `Modules` section with TheSignal Runtime module
- `DefaultEngine.ini`: Changed `GlobalDefaultGameMode` from `/Script/TheSignal.ZP_GameMode` to `/Game/Core/Framework/GM_TheSignal.GM_TheSignal_C`

---

## Scripts Created

| Script | Purpose |
|--------|---------|
| `Scripts/Python/create_ticket005_assets.py` | Creates DA, IAs, IMC via UE Python API |
| `Scripts/Python/setup_imc_grace.py` | Configures IMC_Grace key mappings with modifiers |
| `Scripts/Python/verify_imc.py` | Verifies IMC key bindings |
| `Scripts/Python/configure_blueprints.py` | Sets CDO defaults on BP_GraceCharacter, PC_Grace, GM_TheSignal |

---

## MCP Tooling Notes

- `set_blueprint_default` CANNOT resolve non-Blueprint object references (DataAssets, InputActions, IMCs) — use Python CDO manipulation instead
- Python CDO approach: `bp.generated_class()` → `unreal.get_default_object(gen_class)` → `set_editor_property()`
- UE Python Array has no `.clear()` — use `while len(arr) > 0: arr.pop()`
- Swizzle enum: `unreal.InputAxisSwizzle.YXZ` (not `unreal.EInputAxisSwizzle`)
- FKey creation: `k = unreal.Key(); k.set_editor_property('key_name', 'W')`

---

## Next Steps

1. **PIE test** — restart PIE and verify:
   - `Game class is 'GM_TheSignal_C'` in logs
   - Input bound shows all OK (not MISSING)
   - MovementConfig shows DA_GraceMovement_Default
   - WASD movement, mouse look, head bob, Shift sprint, Space jump all work
2. If Axis2D input doesn't fire (known dead end risk from session 5b), investigate IMC modifier setup
3. After PIE verified: Kinemation integration (TICKET-006)

## Known Risks
- **IA_Move/IA_Look Axis2D callbacks may not fire** — documented dead end from session 5b. If this recurs, may need to add explicit modifiers or restructure IMC.

## Assumptions
- UE 5.4 at `C:\Program Files\Epic Games\UE_5.4`
- Branch: main (HEAD)
- BlueprintMCP running on localhost:9847
- Editor is open with PIE ready to test
