# Objective Reactor — Plan (data-driven "objective complete → world reactions")

**Status: PLANNED (not built).** Architecture chosen from the 2026-06-30 research brief (see checkpoint
`2026-06-30_objective_reactor_research_and_instance_editable_fix`). This is the REACTION half — the
trigger half already exists.

## Goal
When an objective/sub-objective completes (or a flag is set), run an ordered set of in-world reactions —
unlock/open doors, change lights red→white, play audio — with a clean upgrade path to cutscenes.

## Trigger (ALREADY EXISTS — verified)
`UZP_ObjectiveSubsystem` broadcasts these `BlueprintAssignable` delegates (ZP_ObjectiveSubsystem.h:146-149):
- `OnObjectiveCompleted(FName ObjectiveId)`
- `OnSubObjectiveCompleted(FName SubObjectiveId)`
- `OnFlagSet(FName Flag)`  ← the deposit container calls `SetFlag`, so this fires for container completion
- `OnObjectiveStarted(FName)`, `OnTrackerRefresh()`
The enemy-revival feature already listens here, proving the publish side works.

## To build
1. **`AZP_ObjectiveReactor`** (C++ actor, `Blueprintable`) → `BP_ObjectiveReactor` at `/Game/Core/Actors/`:
   - Config (`EditInstanceOnly`): `ListenMode` {Objective | SubObjective | Flag}, `ListenId` (FName),
     `Reactions` (a `DA_ObjectiveReactions` ref, or inline steps).
   - `BeginPlay`: get the subsystem, `AddDynamic` to the matching delegate (store the handle).
     `EndPlay`: **unbind** (binding outlives the actor otherwise).
   - On fire, if `Id == ListenId`: run the reaction steps in order.
   - **Idempotency (critical):** gate on a persisted per-`ListenId` "already fired" flag — on save-load,
     apply the END STATE instantly (door open, lights white), do NOT replay the fade. Mirror
     `UZP_DeathSaveComponent` / the objective-flag persistence.
2. **`BPI_ObjectiveReactable`** (BP interface) implemented by reacting actors (`BP_Elevator` /
   `BP_LockableDoor`, `BP_HorrorLight`, an audio actor): `OpenDoor()`, `SetMood(LinearColor, float)`,
   `PlayStinger(SoundBase)`, `PlaySequence(LevelSequence)`. Call via the **Message** node (non-implementers
   ignored).
3. **`DA_ObjectiveReactions`** (DataAsset): `ListenId → ordered steps {Target (soft actor ref) or
   TargetTag, Action enum, params, duration, target color}`. Keeps all tuning out of code/graphs.
4. **Reaction tiers** (pick per beat):
   - Instant → direct interface call.
   - Eased, self-contained (red→white over N s) → `UTimeline` + curve inside the reacting actor.
   - Choreographed / multi-actor / "will be a cutscene" → a `SQ_` Level Sequence the reactor `Play`s;
     promote to a cutscene later by adding a Camera Cut Track (same asset; Director Event Track routes
     back through the same `BPI_ObjectiveReactable`).

## Gotchas (from research — plan for these now)
- **Idempotent reactions / no replay on load** (the #1 risk for a save-heavy horror game).
- Sequencer "Keep State" is session-only, NOT serialized — gameplay code owns the end-state; leave the
  Level Sequence actor's global Restore State OFF.
- Unbind the subsystem dispatcher on `EndPlay`.
- `ListenId` must match exactly (same failure class as the MapVolume AreaID rule).
- Sequencer C++ path needs `LevelSequence`, `MovieScene`, `MovieSceneTracks` in Build.cs.

## Refs
- `Source/TheSignal/ZP_ObjectiveSubsystem.h` (delegates).
- Research brief + reading list: checkpoint `2026-06-30_objective_reactor_research_and_instance_editable_fix`.
- Companion: `Docs/ObjectiveContainer_Plan.md` (the trigger source for fuse-box-style beats).
