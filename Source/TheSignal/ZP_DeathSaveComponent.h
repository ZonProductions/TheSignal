// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_DeathSaveComponent
 *
 * Purpose: Per-enemy save/state + revival glue. Enemies are hand-placed actors that do NOT Destroy() on
 *          death — they stay as a corpse (tracked by UZP_HealthComponent::bIsDead). This component:
 *            1. SAVE: writes the enemy's dead flag (ZP_Dead) into the EasyGameUI per-slot JSON.
 *            2. LOAD: if the enemy was dead, restores it to its corpse state IN PLACE (via IZP_Revivable),
 *               instead of destroying it — so it can still be brought back later.
 *            3. REVIVE: binds the global UZP_ObjectiveSubsystem and, when this enemy's configured
 *               ReviveOnObjective (an objective / sub-objective id, or a flag) fires, resets a dead enemy
 *               to a fully alive patrolling state (via IZP_Revivable::ReviveEnemy).
 *
 *          This REPLACES the old Destroy()+RegisterDestruction approach, which was permanent (EGUI has no
 *          un-register and never recreates a destroyed actor) and therefore could never support revival.
 *
 * Owner Subsystem: SaveSystem (EGUI integration) + campaign ObjectiveSubsystem.
 *
 * Setup per enemy BP:
 *   - Add this component.
 *   - Add a BP_EasySaveGameComponent: ActorSaveType=Persistent, HasVariablesToSave=true,
 *     RegisterDestruction=FALSE (we persist STATE, we never destroy the actor).
 *   - The enemy must have a UZP_HealthComponent (all ZP enemies do).
 *   - For revival, the enemy must implement IZP_Revivable (Shambler + Scytheer do). Without it, death
 *     still persists (generic re-kill on load) but the enemy cannot be revived.
 *   - Per placed instance, set ReviveOnObjective to the Objectives.json Id that should bring it back
 *     (leave None for an enemy that simply stays dead once killed).
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JsonObjectWrapper.h"
#include "ZP_DeathSaveComponent.generated.h"

class UZP_HealthComponent;
class UZP_ObjectiveSubsystem;

UCLASS(ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_DeathSaveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_DeathSaveComponent();

	/** Objective / sub-objective id (or progression flag) from Objectives.json whose completion REVIVES
	 *  this enemy if it is dead. Use the stable Id (e.g. "OFF_CLUES"), NOT the display title — titles change
	 *  per stage. Matches an OnObjectiveCompleted, OnSubObjectiveCompleted, or OnFlagSet broadcast. None =
	 *  this enemy never auto-revives (stays dead once killed). Settable per placed instance in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Persistence")
	FName ReviveOnObjective = NAME_None;

protected:
	virtual void BeginPlay() override;

private:
	/** Find the sibling BP_EasySaveGameComponent and bind OnEguiSaveLoadVariables to its dispatcher. */
	void BindSaveComponent();

	/** Bind the global ObjectiveSubsystem's completion/flag dispatchers so we can react to the revive beat. */
	void BindObjectiveEvents();

	/** EGUI save/load hook. OperationType: 0 = Save (write ZP_Dead), 1 = Load (restore corpse if was dead). */
	UFUNCTION()
	void OnEguiSaveLoadVariables(uint8 OperationType, FJsonObjectWrapper JsonObject);

	// Objective-subsystem dispatcher handlers — all funnel into HandleReviveTrigger.
	UFUNCTION() void OnObjectiveCompletedEvt(FName ObjectiveId);
	UFUNCTION() void OnSubObjectiveCompletedEvt(FName SubObjectiveId);
	UFUNCTION() void OnFlagSetEvt(FName Flag);

	/** If Id matches ReviveOnObjective and the enemy is currently dead, revive it. */
	void HandleReviveTrigger(FName Id);

	/** Restore the dead/corpse state in place (IZP_Revivable, or a generic re-kill fallback). */
	void RestoreDeadState();

	/** True if ReviveOnObjective is already satisfied in the ObjectiveSubsystem (objective/sub complete or
	 *  flag set) — used on load so an enemy whose revive beat already passed comes back ALIVE, not as a corpse. */
	bool IsReviveConditionMet() const;

	UZP_HealthComponent* GetHealth() const;
	UZP_ObjectiveSubsystem* GetObjectiveSubsystem() const;
	/** Owner actor or one of its components implementing IZP_Revivable, else nullptr. */
	UObject* FindRevivable() const;
};
