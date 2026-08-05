# Objectives — How to Create / Update (per map)

## The pipeline (never skip step 2)
1. **Author** `SourceData/Objectives.json` (project root — never cooked/shipped).
2. **Bake** — run `Scripts/Python/bake_objectives.py` (via the MCP Python endpoint, editor open).
   The game reads the baked `/Game/Data/DT_Objectives` table, **not** the JSON. Edited JSON without
   a re-bake = nothing changes in game (a warning now fires if a started Id has no table row).
3. **Play.** To reset a test run, delete `Saved\SaveGames\TheSignal_Objectives.sav`.

## JSON shape (one MAIN per map, subs are the tracker rows)
```json
{ "Id": "RESEARCH1", "Title": "Find a way out",
  "bSideObjective": false, "bStartOnLoad": false, "Requires": [],
  "SubObjectives": [
    { "Id": "RF_POWER", "Title": "Get the power back",
      "RevealRequirements": [ { "Type": "FlagSet", "Value": "ELEVATOR_BLOCKED_SEEN" } ],
      "Requirements":       [ { "Type": "FlagSet", "Value": "FUSE_BOX" } ],
      "Stages": [] } ] }
```
- **Requirement types:** `FlagSet`, `HasItem` (full DA asset path), `ObjectiveComplete`,
  `SubObjectiveComplete`. (`HeardDialogue`/`CollectedNote`/`ReachedTrigger` are not implemented yet.)
- **RevealRequirements** = when the step *appears*. Empty `[]` = visible immediately.
  **Requirements** = when it *completes*.
- **Stages** (optional) re-title one step as it progresses ("Search for clues" → "Go to the empty
  floor"); a stage's `EnterFlag` is raised the moment that stage becomes current.
- A main completes when **at least one** step has revealed and all revealed steps are done — an
  all-hidden main waits (fixed 2026-07-18; before that it completed instantly).
- Titles are player-facing — author them yourself; Claude only places placeholders.

## Same map — updating existing objectives
Edit the entry in the JSON → bake → play. Ids are permanent references (saves, `Requires`,
reactor/trigger fields, `ReviveOnObjective`) — change titles/requirements freely, **never rename an
Id** that's already in use.

## New map — the per-map wiring (all in the level, no code)
1. **Add the main** to Objectives.json with `"bStartOnLoad": false` (only OFFICE1 — the global
   game-start objective — uses `true`). Bake.
2. **Place an `AZP_ObjectiveReactor`**, set **`AZP_StartObjectiveOnBeginPlay`** = your new main's Id.
   That map now starts (and after a save-load, re-asserts) its own main; the most recently started
   main owns the HUD tracker. Optional on the same actor: `AZP_ListenId` = a flag/objective Id that
   retints emergency lights + plays `AZP_CompleteSound` when it completes; `AZP_StartupSound` at map start.
3. **Make flags fire in-world** — any of:
   - `ZP_ObjectiveTrigger` volume (walk-in sets its flag, e.g. `ELEVATOR_BLOCKED_SEEN`),
   - a deposit container (`BP_ObjectiveContainer`): completing it sets its `ObjectiveFlag`; just
     opening it without the items auto-sets `<ObjectiveFlag>_TOUCHED` — free "player has seen it" reveal,
   - any BP: `Get Game Instance Subsystem → ZP Objective Subsystem → Set Flag / Complete Sub Objective`.
4. **Gate the exit** — `AZP_TransitReturn.AZP_RequiredObjective` / `AZP_InteractDoor.AZP_ObjectiveOverride`
   accept a flag **or** main-objective Id.

**Checklist for "my objective doesn't show":** baked after editing? reactor placed with the right Id?
reveal flag actually fired (check `[TheSignal]` log lines: Objective STARTED / Flag SET)? stale
`TheSignal_Objectives.sav`?
