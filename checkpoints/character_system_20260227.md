# Checkpoint: Character System — Full C++ Rewrite
**Date:** 2026-02-27
**System:** PlayerCharacter (ASignalTacticalCharacter + BP_Grace)
**Branch:** proto/corridor-v1

---

## Current State

### C++ Code — COMPILED AND VERIFIED
All C++ builds clean (0 errors, 0 warnings) with UE 5.4.

| File | Status | Purpose |
|------|--------|---------|
| `Source/TheSignal/SignalTacticalCharacter.h` | DONE | Self-contained character class (~230 lines) |
| `Source/TheSignal/SignalTacticalCharacter.cpp` | DONE | Full implementation (~530 lines) |
| `Source/TheSignal/KinemationBridge.h` | DONE | Static utility for calling Kinemation BP functions (22 wrappers) |
| `Source/TheSignal/KinemationBridge.cpp` | DONE | ProcessEvent implementations (~200 lines) |
| `Source/TheSignal/SignalCharacterSetup.h/.cpp` | DELETED | Was a temporary BlueprintFunctionLibrary, replaced |

### What the C++ constructor creates:
- `PlayerMesh` (USkeletalMeshComponent "Player") — SKM_Manny mesh + ABP_TacticalShooter_UE5 AnimBP
- `FPSCamera` (UCameraComponent "FPSCamera") — attached to PlayerMesh at FPCamera socket
- All 16 input actions loaded via ConstructorHelpers from `/Game/KINEMATION/TacticalShooterPack/Inputs/IA_*`
- IMC loaded: `IMC_TacticalShooterPack`
- Settings loaded: `DA_TacticalCharacterSettings`

### What BeginPlay creates at runtime:
- 4 Kinemation Blueprint components via `LoadClass` + `NewObject` + `RegisterComponent`:
  - TacticalCameraComp (AC_FirstPersonCamera)
  - TacticalAnimComp (AC_TacticalShooterAnimation)
  - RecoilAnimComp (AC_RecoilAnimation)
  - IKMotionComp (AC_IKMotionPlayer)
- Applies IMC to controller's EnhancedInput subsystem
- Calls `FKinemationBridge::UpdateTargetCamera` and `UpdatePlayerMesh`
- Equips first weapon if Weapons array is populated

### BP_Grace Blueprint — PARTIALLY CONFIGURED
- **Parent:** ASignalTacticalCharacter (reparented, verified)
- **Interface:** BPI_TacticalShooterCharacter added (verified)
- **Interface stubs wired (3/4 DONE):**
  - GetPlayerMesh → calls C++ `GetPlayerMeshComponent()` → connected to return pin
  - GetMainWeapon → calls C++ `GetActiveWeapon()` → connected to return pin
  - GetPrimaryWeapon → calls C++ `GetActiveWeapon()` → connected to return pin
  - GetTacticalShooterComponent → **NEEDS FIX** (see Next Action)

---

## Verified Steps
1. C++ compiles clean (UHT + compilation + link) — VERIFIED
2. BP_Grace reparented from TheSignalCharacter to SignalTacticalCharacter — VERIFIED
3. BPI_TacticalShooterCharacter interface added to BP_Grace — VERIFIED
4. 3/4 interface stubs wired correctly (GetPlayerMesh, GetMainWeapon, GetPrimaryWeapon) — VERIFIED via connect_pins success
5. C++ function renamed: `GetTacticalShooterComponent` → `GetTacticalAnimComponent` to avoid name collision with interface — VERIFIED (compiled clean after rename)

---

## Failure Log

### Name collision: GetTacticalShooterComponent
- **Error:** `Overriden function is not compatible with the parent function. Check flags: Exec, Final, Static.`
- **Cause:** C++ had `UFUNCTION(BlueprintCallable) UActorComponent* GetTacticalShooterComponent()` which clashed with the Blueprint Interface function `GetTacticalShooterComponent()` that returns `AC_TacticalShooterAnimation_C*`
- **Fix:** Renamed C++ function to `GetTacticalAnimComponent()`. Rebuilt. Now need to update the Blueprint CallFunction node to use the new name.

### Type incompatibility: ActorComponent vs AC_TacticalShooterAnimation_C
- **Error:** `Cannot connect ReturnValue (object) to TacticalShooterComponent (object) — types are incompatible`
- **Cause:** C++ `GetTacticalAnimComponent` returns `UActorComponent*` but interface return pin expects `AC_TacticalShooterAnimation_C*`
- **Fix:** Added DynamicCast node in the Blueprint stub. Cast node is already placed and wired (entry exec → cast exec → return exec, callfunction return → cast object input, cast "As" output → return pin). The CallFunction node (ID: `86EEAA474F332E349E17C1AE44030304`) still references the OLD function name `GetTacticalShooterComponent` which no longer exists after rename. Must delete it and add a new one calling `GetTacticalAnimComponent`.

---

## Next Action

**Open editor, then execute these MCP steps in order:**

1. Delete the stale CallFunction node in GetTacticalShooterComponent graph:
   - `delete_node(blueprint="BP_Grace", nodeId="86EEAA474F332E349E17C1AE44030304")`

2. Add new CallFunction node for the renamed C++ function:
   - `add_node(blueprint="BP_Grace", graph="GetTacticalShooterComponent", nodeType="CallFunction", className="SignalTacticalCharacter", functionName="GetTacticalAnimComponent")`

3. Connect new CallFunction ReturnValue → DynamicCast Object input:
   - `connect_pins(... sourceNodeId=NEW_NODE_ID, sourcePinName="ReturnValue", targetNodeId="95947BDA4879ABB786BDFC8048A9C0C5", targetPinName="Object")`

4. Validate BP_Grace: `validate_blueprint("BP_Grace")`

5. Set GM_TheSignal DefaultPawnClass to BP_Grace:
   - `set_blueprint_default(blueprint="GM_TheSignal", property="DefaultPawnClass", value="BP_Grace")`

6. Deprecate BP_GraceTactical:
   - `rename_asset(assetPath="/Game/Player/Blueprints/BP_GraceTactical", newPath="/Game/_DEPRECATED/DEP_BP_GraceTactical")`

7. Validate again and PIE test.

---

## Open Questions
1. **Will dynamically-created Kinemation Blueprint components work the same as SCS-created ones?** Not yet verified at runtime. The `NewObject` + `RegisterComponent` pattern is standard but Kinemation components may expect SCS initialization order.
2. **GASP locomotion:** GASP is NOT installed (TICKET-010 OPEN). Current setup uses ABP_TacticalShooter_UE5 which handles upper body/weapons. Lower body animations will be missing until GASP is integrated. The user stated "LOCOMOTION MUST EXIST" — this is unresolved.
3. **Weapon spawning:** Weapons array is EditAnywhere but empty by default. No weapons will be visible until configured in BP_Grace defaults or level.
4. **FPCamera socket:** Assumed to exist on SKM_Manny skeleton. Not yet verified at runtime.

---

## Assumptions
- **UE Version:** 5.4 (installed at `C:\Program Files\Epic Games\UE_5.4`)
- **Build tool:** `UE_5.4\Engine\Build\BatchFiles\Build.bat`
- **Kinemation asset paths** (all verified via filesystem glob):
  - Mesh: `/Game/KINEMATION/TacticalShooterPack/Demo/Manny/UE5/SKM_Manny`
  - AnimBP: `/Game/KINEMATION/TacticalShooterPack/Animations/UE5/ABP_TacticalShooter_UE5`
  - Settings: `/Game/KINEMATION/TacticalShooterPack/Blueprints/Data/DA_TacticalCharacterSettings`
  - IMC: `/Game/KINEMATION/TacticalShooterPack/Inputs/IMC_TacticalShooterPack`
  - AC_FirstPersonCamera: `/Game/KINEMATION/Common/FirstPersonCamera/AC_FirstPersonCamera`
  - AC_TacticalShooterAnimation: `/Game/KINEMATION/TacticalShooterPack/Blueprints/AC_TacticalShooterAnimation`
  - AC_RecoilAnimation: `/Game/KINEMATION/Common/Recoil/AC_RecoilAnimation`
  - AC_IKMotionPlayer: `/Game/KINEMATION/Common/IKMotion/AC_IKMotionPlayer`
- **Component names used in C++:** "Player", "FPSCamera", "TacticalCamera", "AC_TacticalShooterAnimation", "AC_RecoilAnimation", "AC_IKMotionPlayer"
- **BP_Grace exists at:** `/Game/Player/Blueprints/BP_Grace`
- **GM_TheSignal exists at:** `/Game/Core/GM_TheSignal`
- **BP_GraceTactical** (old, broken) still exists at `/Game/Player/Blueprints/BP_GraceTactical` — to be deprecated

---

## Key Node IDs in BP_Grace (for resuming MCP work)

### GetTacticalShooterComponent graph:
- Entry: `AE2E26FD4DB9DEE573E744B27445C3CB`
- Return: `B8220E60468E38A78245C29AB37B3C0F`
- DynamicCast (AC_TacticalShooterAnimation): `95947BDA4879ABB786BDFC8048A9C0C5`
- OLD CallFunction (STALE — references deleted function): `86EEAA474F332E349E17C1AE44030304` — DELETE THIS

### GetPlayerMesh graph:
- Entry: `36E52629470AEFDA911742AAC9BC183F`
- Return: `5A7455BF46F6B1E3AF2EFE82DB303AEB`
- CallFunction (GetPlayerMeshComponent): `F6CE41E048B254D72D107ABFBBDC085B` — DONE

### GetMainWeapon graph:
- Entry: `4D90849E4D7C042E8EC0E1A8B0118EA2`
- Return: `50E7392142BADCF87F35ECBB8CFFC942`
- CallFunction (GetActiveWeapon): `92874FCF4E330F7CD97477A084E88E3A` — DONE

### GetPrimaryWeapon graph:
- Entry: `72F7D5F84F6122940DCAA1A08D806E63`
- Return: `498F8EEE4CEB1547F72DDF83BDDC76C6`
- CallFunction (GetActiveWeapon): `F4B913BB43255BA50E93918334810C90` — DONE
