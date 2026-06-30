# Objective Container — Plan / Resume Doc

**Status (2026-06-29, post-wire): C++ DONE & BUILT. BP graph WIRED & compiles clean. Content/config NOT made.**

The 2026-06-29 ~11:02 session that wrote the deposit library was interrupted by a machine
hard-freeze mid-compile (see checkpoint `2026-06-29d`). Recovery was complete (the freeze cost nothing
on the build side). Since then (checkpoint `2026-06-29f`): added `DefaultToSelf` meta + a one-call
`SetupDeposit` convenience to the library (rebuilt clean), and **wired `BP_ObjectiveContainer`'s
EventGraph** — BeginPlay → Parent → `SetupDeposit`; `OnCloseInventory` → `SubmitDeposit` → Branch.
What remains is per-instance content/config (dev design choices) + QA.

---

## ⚠️ ARCHITECTURE PIVOT — read this first

There are TWO C++ approaches in the tree. **Only the second is live.**

- **(A) `AZP_ObjectiveContainer` C++ ACTOR** (`ZP_ObjectiveContainer.{h,cpp}`) — a self-contained actor
  with native `Root/ContainerMesh/InteractionVolume/StatusLight`, `IZP_Interactable`, have-ALL →
  consume-ALL → unlock. **SUPERSEDED for the container.** Reparenting the Moonville container BP to
  this actor hit the documented SCS-drop dead end (a Moonville BP's `DefaultSceneRoot` subtree does
  NOT instantiate under a C++ native root — same trap as `BP_Elevator → AZP_Elevator`, 2026-06-24).
  **NOT DELETED and still compiles** — its `FZP_RequiredItem` struct is reused by (B). Treat the actor
  itself as dead code unless a future non-Moonville container wants it; the struct is live.

- **(B) `UZP_ObjectiveDepositLibrary` + Moonville BP** — **THE LIVE PATH.** The container stays a
  Moonville BP (child of `BP_ItemContainer_Horror`), and the requirement/flag logic lives in a
  Blueprint function library (`ZP_ObjectiveDepositLibrary.{h,cpp}`). Model = **incremental DEPOSIT**:
  the player drops the required items INTO the container's grid, then submits.

---

## What it is (live = deposit model)
A reusable Moonville deposit container: the designer configures `RequiredItems` (Details), the player
drops those items into the container grid and submits; on a valid deposit it (optionally) consumes the
items and sets an objective flag that unlocks progression. Deposit-side sibling of `BP_CardReaderPanel`.

## Already done — DO NOT redo
- **C++ built & compiling into the live DLL** (verified: build OK, up-to-date 13:13):
  - `Source/TheSignal/ZP_ObjectiveDepositLibrary.{h,cpp}` — 5 Blueprint functions, all fully implemented:
    - `FindContainerInventory(Container)` → the Moonville inventory comp that owns `ItemSlots`.
    - `RequiredCellCount(RequiredItems)` → sum of counts (min 1).
    - `SetupDepositGrid(Inv, RequiredItems)` → writes `InventorySizeExpansion` so the grid is exactly
      N×1 cells (N = required cell count). Call at BeginPlay.
    - `ValidateDeposit(Inv, RequiredItems)` → true once the grid holds every item in ≥ its Count.
    - `SubmitDeposit(Container, RequiredItems, bConsume, ObjectiveFlag)` → validate → optionally
      `RemoveItemByDataAsset` each → `UZP_ObjectiveSubsystem::SetFlag(ObjectiveFlag)` (persists) → true.
  - `FZP_RequiredItem { TSoftObjectPtr<UObject> Item; int32 Count; }` lives in `ZP_ObjectiveContainer.h`.
  - `UZP_ContainerUtils::FitBoxToMeshBounds` (shared with loot locker) — available if needed.
- **`BP_ObjectiveContainer` asset exists** at `/Game/Core/Actors/` (~28 KB). Per a disk binary scan its
  parent is the Moonville `BP_ItemContainer_Horror_C` and it has a `RequiredItems` var — **but its graph
  does NOT yet call the deposit library** (asset mtime 10:57 predates the 11:01 source; editor-unverified).

## Remaining steps (the actual gap)
1. [DONE 2026-06-29f] **`BP_ObjectiveContainer` EventGraph wired** (compiles clean, `validate_blueprint`
   Valid). BP variables already present: `RequiredItems` (ZP_RequiredItem[]), `ObjectiveFlag` (name),
   `bConsumeOnSubmit` (bool).
   - BeginPlay (was a *disabled* placeholder override — now enabled) → **Parent: BeginPlay**
     (preserves Moonville `BP_ItemContainer_Horror` setup) → `SetupDeposit(RequiredItems)` (sizes the
     deposit grid to N×1).
   - `OnCloseInventory` (override) → `SubmitDeposit(RequiredItems, bConsumeOnSubmit, ObjectiveFlag)` →
     Branch on the bool. **Branch TRUE/FALSE outputs are intentionally left open** for the dev to add
     success FX (SFX / open door / lights) and a "still missing" cue.
   - `Container` pins are left unconnected on purpose — `DefaultToSelf` auto-fills them (MCP can't make
     Self nodes).
   - The inherited `ActorBeginOverlap` / `Tick` placeholder overrides are left DISABLED so the Moonville
     parent's overlap/interaction keeps running — do NOT enable them without adding a Parent call.
2. **⚠ Configure the container to START EMPTY (critical for deposit use).** `BP_ItemContainer_Horror` is
   a *loot* container — by default it can randomize/spawn contained items, which would pre-fill the
   deposit grid. For a deposit container its contained-items / randomize settings must be empty/off so
   the player fills it. Set this on the BP/instance (Moonville container details).
3. **Per instance / themed child:** set the container mesh and `RequiredItems` (e.g. `DA_Fuse` ×3),
   `ObjectiveFlag` (e.g. `FuseBoxPowered`), `bConsumeOnSubmit`.
3. **Create example content if using fuses:** `DA_Fuse` (Moonville PDA_Item) + a fuse `BP_ItemPickup` —
   neither exists yet.
4. **Add the matching objective** to **`SourceData/Objectives.json`** (the LIVE config — NOT
   `Content/Data/Objectives.json`, which only holds `DT_Objectives.uasset`). Add a (sub)objective whose
   requirement is `FlagSet = <ObjectiveFlagOnUnlock>`. Current file has only `OFFICE1`
   (`OFF_CLUES`/`OFF_CARD`); no container entry exists yet.
5. Place container + pickups in the level. QA: deposit < required → no unlock; deposit all + submit →
   items consumed, flag set, progression advances; save/quit/reload after unlock → still unlocked (flag).

## Persistence note (verify in QA)
- **Unlock persists for free:** `SetFlag` writes to `UZP_ObjectiveSubsystem`, which is saved. A completed
  container stays completed across save/load.
- **PARTIAL (deposited-but-not-submitted) state:** the deposit library has NO per-container save hook, so
  whether half-finished deposits survive save/load depends entirely on Moonville `BP_ItemContainer_Horror`
  container-content persistence — UNVERIFIED. If partial progress must survive, that needs an EGUI
  `OnEguiSaveLoadVariables` hook on the container BP. QA this before relying on it.

## Files / refs
- LIVE: `Source/TheSignal/ZP_ObjectiveDepositLibrary.{h,cpp}` (built; untracked in git — commit it).
- Shared struct: `Source/TheSignal/ZP_ObjectiveContainer.{h,cpp}` (actor superseded; struct reused).
- Helper: `Source/TheSignal/ZP_ContainerUtils.{h,cpp}`.
- Objective API: `Source/TheSignal/ZP_ObjectiveSubsystem.h` (`SetFlag`/`HasFlag`/`CompleteSubObjective`).
- Live objective config: `SourceData/Objectives.json`.
- Checkpoints: `2026-06-29c` (BP created), `2026-06-29d` (freeze diagnosis),
  `2026-06-29e` (this reconcile).
