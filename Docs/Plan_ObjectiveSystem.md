# Objective System — As-Built + Remaining (2026-06-21)

**Status:** Core working in-editor. Backbone + DataTable authoring + EasyGameUI HUD tracker all done and
confirmed showing. Remaining: complete-state ticks, menu-close re-show, external requirement eval, save
persistence, convenience front-ends.

**What it is:** the reusable **campaign-progression backbone**. Hierarchical objectives (main → sub) with
sequential gating, authored in JSON → baked to a cooked DataTable, tracked in a GameInstance subsystem that
survives level travel, and surfaced in a top-left HUD tracker (EasyGameUI). Other systems (transit panel,
doors, dialogue) gate on objective/flag state.

> Supersedes the "progression" half of `Plan_TransitSystem.md` (which still describes the abandoned
> GameplayTag / `UZP_ProgressionSubsystem` design — see "Decisions that changed" below).

---

## Architecture (3 layers)

### A. Authoring → cooked data
- **Author** `SourceData/Objectives.json` — project root, **outside `Content/` so it never ships**. Per row:
  `Id` (canonical id = the key), `Title` (player-facing, dev-authored), `bSideObjective`, `bStartOnLoad`,
  `Requires` (prior main ids), `SubObjectives` (each: `Id`, `Title`, `Requirements`).
- **Bake** `Scripts/Python/bake_objectives.py` → `/Game/Data/DT_Objectives` (DataTable, RowStruct
  `FZP_ObjectiveDef`). It reads the JSON, **injects each row's `Name` = its `Id`** (so you author one id,
  never a redundant Name), and fills the table. Verified: nested SubObjectives + Requirements import.
- **Runtime reads the cooked `DT_Objectives` `.uasset`** — never the loose JSON. JSON is a dev source like a
  `.fbx`. Packaging: the DataTable cooks normally; the JSON is never included. **No C++ rebuild for content
  edits** — edit JSON → bake → Play.

### B. Runtime backbone — `UZP_ObjectiveSubsystem` (GameInstanceSubsystem)
- Lives on the GameInstance → **survives level travel** (objectives are global, persist across maps).
- `Initialize` → `LoadDefinitions()` (reads DT_Objectives, keys by `Id`, warns on duplicate id), then
  auto-starts every row flagged `bStartOnLoad`.
- **State** (all `TSet<FName>`, matching the project's `TriggeredNarrativeBeats` idiom): `ActiveObjectives`,
  `CompletedObjectives`, `CompletedSubObjectives`, `Flags`.
- **Sequential gating + auto-advance** (`TryAdvance`, re-entrancy-guarded): a sub auto-completes when all its
  *self-evaluable* requirements are met; a main completes when all its subs are done; a gated main
  auto-starts when its `Requires` are all complete. Ungated mains start explicitly (or via `bStartOnLoad`).
- **BP API** (all BlueprintCallable — these ARE the "front-end" nodes): `StartObjective`, `CompleteObjective`,
  `CompleteSubObjective`, `SetFlag`; `IsObjectiveComplete/Active`, `IsSubObjectiveComplete`, `HasFlag`,
  `GetActiveObjective`, `GetActiveMainObjective`, `GetObjectiveDef`, `LoadDefinitions`, `NotifyMenuClosed`.
- **Dispatchers:** `OnObjectiveStarted`, `OnObjectiveCompleted`, `OnSubObjectiveCompleted`, `OnFlagSet`, and
  `OnTrackerRefresh` (fires on ANY state change — the HUD binds this).
- **Save hooks (built, not yet wired):** `GetSaveState`/`RestoreSaveState` (Active/Completed/CompletedSubs/Flags).

### C. HUD tracker — bridge + EasyGameUI, embedded in WBP_HUD
- **`UZP_ObjectiveHudBridge`** (UActorComponent; base class of `BPC_ObjectiveHudGlue`, which sits on
  **PC_Grace**). Binds `OnTrackerRefresh`; on change, gets the active MAIN objective + subs and drives three
  tiny BlueprintImplementableEvents — `BeginObjective` → `AddObjectiveRow` (once per sub) → `EndObjective` —
  plus `HideObjective` when none active. The per-sub loop is in C++ so the BP graphs stay trivial.
- **`BPC_ObjectiveHudGlue`** (`/Game/Core/Player/`) implements them: BeginObjective → Array Clear; AddObjectiveRow
  → Make `S_QuestObjectiveDefinition` → Array Add; EndObjective → `DisplayNewQuest(Widget, Title, list)`;
  HideObjective → `ClearCurrentQuest`. (EasyGameUI interface `BPI_EasyHudModulesInterface`.)
- **The widget = EasyGameUI `WBP_EHB_QuestStatusDisplayer`, placed as a child of `WBP_HUD`** (so placement is
  controlled in WBP_HUD's canvas, not code). `WBP_HUD` Event Construct calls
  `Get Component (ZP_ObjectiveHudBridge) → RegisterQuestWidget(<the child>)`. The bridge then drives that
  instance; if nothing registers it falls back to a standalone auto-created widget (and removes that fallback
  when a HUD widget registers). EasyGameUI's own `Setup Quest Clear` timer gives the ~8s fade-out free.

---

## Data model (`ZP_ObjectiveTypes.h`)
- `EZP_ObjReqType`: `FlagSet`, `ObjectiveComplete`, `SubObjectiveComplete` (self-evaluable now) +
  `HasItem`, `HeardDialogue`, `CollectedNote`, `ReachedTrigger` (external — eval pending, see Remaining).
- `FZP_ObjectiveRequirement { EZP_ObjReqType Type; FString Value }` — Value meaning depends on Type.
- `FZP_SubObjectiveDef { FName Id; FText Title; TArray<FZP_ObjectiveRequirement> Requirements }`.
- `FZP_ObjectiveDef : public FTableRowBase { FName Id; FText Title; TArray<FName> Requires; bool bSideObjective;
  bool bStartOnLoad; TArray<FZP_SubObjectiveDef> SubObjectives }`.

---

## Decisions that changed from the original plan
- **FName ids, NOT GameplayTags** — matches the project's existing `TriggeredNarrativeBeats`/`PlayedDialogueIDs`
  idiom; no new tag taxonomy.
- **`UZP_ObjectiveSubsystem`, NOT `UZP_ProgressionSubsystem`** — the "progression" layer became the objective
  system the dev asked for (hierarchical, sequential, JSON, HUD-tracked).
- **DataTable-baked-from-JSON, NOT a loose runtime JSON** — a loose `.json` read at runtime was rejected as
  fragile/tamperable/packaging-risky. JSON is now a dev source baked into a validated cooked DataTable.
- **Explicit `Id`, NOT row-name-as-id** — decouples identity from the table/row so objectives can be split
  per level and shared across levels, and dup ids are detectable. (Bake derives row Name from Id.)
- **Reuse EasyGameUI quest displayer**, embedded in `WBP_HUD` for placement (not the EasyGameUI HUD-context
  manager, not a custom widget).
- **Player-facing text is dev-authored** (objective titles live in the JSON); never baked in code
  (see memory `feedback_expose_player_facing_text`).

---

## Files
**C++:** `Source/TheSignal/ZP_ObjectiveTypes.h`, `ZP_ObjectiveSubsystem.h/.cpp`, `ZP_ObjectiveHudBridge.h/.cpp`.
`TheSignal.Build.cs` gained `Json`, `JsonUtilities`.
**Assets:** `/Game/Data/DT_Objectives` (cooked data), `/Game/Core/Player/BPC_ObjectiveHudGlue` (HUD glue),
`WBP_HUD` (Construct hookup + embedded `WBP_EHB_QuestStatusDisplayer_0`), `PC_Grace` (`ObjectiveHudGlue` comp).
**Source/scripts:** `SourceData/Objectives.json`, `Scripts/Python/bake_objectives.py`.

## Authoring workflow (current)
1. Edit `SourceData/Objectives.json` (author `Id`, `Title`, `bSideObjective`, `bStartOnLoad`, `Requires`,
   `SubObjectives[].{Id,Title,Requirements[].{Type,Value}}`).
2. Run `bake_objectives.py` (via MCP python endpoint) → updates `DT_Objectives`.
3. Play. Trigger objectives in a level from any BP: `Get Game Instance Subsystem → ZP Objective Subsystem →
   Start/Complete Objective / Complete Sub Objective / Set Flag`.

---

## Remaining work
1. **Complete-state ticks** — `BPC_ObjectiveHudGlue.AddObjectiveRow` currently leaves `ObjectiveDefaultState`
   at default ("active"); wire `bComplete` → `Select` the `E_QuestObjectiveState` so done steps tick.
2. **Menu-close re-show** — call `ZP Objective Subsystem → NotifyMenuClosed` from the map / inventory / notes
   close paths (re-pulses `OnTrackerRefresh` → tracker shows 8s).
3. **External requirement eval** — `HasItem` / `HeardDialogue` / `CollectedNote` / `ReachedTrigger` currently
   return false in `EvaluateRequirement`; wire to InventorySystem (reuse `AZP_CardReaderPanel::CheckPlayerHasItem`),
   `UZP_DialogueManager` (`PlayedDialogueIDs`), `UZP_NoteSubsystem`, and trigger actors. (Until then those
   subs complete only via an explicit `CompleteSubObjective` front-end.)
4. **Save persistence (M4)** — add `TSet<FName> CompletedObjectives, ActiveObjectives, CompletedSubObjectives,
   ProgressFlags` to `UZP_SaveGame`; serialize like `TriggeredNarrativeBeats`; call the subsystem's
   `GetSaveState`/`RestoreSaveState` on save/load.
5. **Convenience front-ends** — drop-in `AZP_ObjectiveTrigger` actor (overlap/interact → Start/Complete/SetFlag);
   auto-hooks so a note pickup / dialogue line completes a sub without wiring.
6. **One-click "Bake Objectives"** Editor Utility button so JSON→DT isn't a manual script run.
7. **Transit gating (M3)** — `AZP_TransitPanel` reads objective completion to gate destinations + spawn the
   leading objective when a prereq is missing.

---

## Gotchas (learned this session)
- **DataTable JSON import requires every row-struct property to be a column** — don't keep redundant/extra
  properties; author `Id` only (bake injects `Name`). A missing column logs a per-row warning.
- A **WBP child must have `bIsVariable` = true AND the BP recompiled** before it's referenceable as a variable
  in the graph.
- A generic `UUserWidget` pin can't connect to an **interface** pin — use the concrete type (`GetComponentByClass`
  with the class set returns the typed component; or a `DynamicCast`).
- **`VariableSet` on an array can't be left empty** (compile error) → use `Array Clear`.
- MCP: `nwiro create_widget_blueprint` crashes (SavePackage ensure — never use); `blueprint-mcp add_component`
  needs the `_C` suffix for BP component classes; `connect_pins` is individual-call only (batch param shape is broken).
- Subsystem `Initialize` runs at GI creation = each PIE start, so JSON/DataTable edits apply on next Play.
