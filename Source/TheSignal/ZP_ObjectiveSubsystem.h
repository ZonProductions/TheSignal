// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_ObjectiveSubsystem
 *
 * Purpose: The campaign progression backbone (game state). Loads objective definitions from
 *          Content/Data/Objectives.json, tracks active/completed objectives + sub-objectives +
 *          flags (all FName, matching the project's TriggeredNarrativeBeats idiom), enforces
 *          sequential main gating, and broadcasts changes for the HUD tracker + transit gating.
 *          GameInstance lifetime — survives level travel. Persisted via UZP_SaveGame (M4).
 *
 * Owner Subsystem: this.
 *
 * Front-ends (simple "update game state"): SetFlag / CompleteSubObjective / CompleteObjective /
 *          StartObjective — callable from a BP node, a trigger actor, note pickup, or dialogue.
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZP_ObjectiveTypes.h"
#include "ZP_ObjectiveSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZP_OnObjectiveEvent, FName, ObjectiveId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZP_OnFlagEvent, FName, Flag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZP_OnTrackerRefresh);

class UZP_SaveGame;

UCLASS()
class THESIGNAL_API UZP_ObjectiveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** (Re)load objective definitions from Content/Data/Objectives.json. */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	bool LoadDefinitions();

	// --- State updates (front-ends) ---
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void StartObjective(FName ObjectiveId);

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void CompleteObjective(FName ObjectiveId);

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void CompleteSubObjective(FName SubObjectiveId);

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void SetFlag(FName Flag);

	/** Item the player acquired/lost — re-runs auto-advance so HasItem requirements resolve.
	 *  Call this from the item pickup / inventory-changed path. ItemId is informational. */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void NotifyInventoryChanged(FName ItemId);

	/** Force a re-evaluation of all auto-advance rules (e.g. after an inventory change). */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void ReevaluateObjectives();

	/** Wipe ALL objective progress (active/completed/subs/reveals/stages/flags), delete the persisted
	 *  slot, and re-start bStartOnLoad objectives. For testing a clean run. */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void ResetAllProgress();

	// --- Queries ---
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool IsObjectiveComplete(FName ObjectiveId) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool IsObjectiveActive(FName ObjectiveId) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool IsSubObjectiveComplete(FName SubObjectiveId) const;

	/** True once a (hidden) sub-objective's RevealRequirements are met and it's tracked in the HUD. */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool IsSubObjectiveRevealed(FName SubObjectiveId) const;

	/** Current stage index a sub-objective is displaying (0 = first). Drives the re-titling step. */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	int32 GetSubObjectiveStage(FName SubObjectiveId) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool HasFlag(FName Flag) const;

	/** First currently-active objective (for the HUD tracker). Returns false if none active. */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetActiveObjective(FZP_ObjectiveDef& OutDef) const;

	/** First active MAIN objective (skips side objectives) — what the HUD tracker shows. */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetActiveMainObjective(FZP_ObjectiveDef& OutDef) const;

	/** The HUD tracker view of the active main: the objective itself + one row per currently-VISIBLE,
	 *  not-yet-complete sub, each already resolved to its current-stage title. Hidden (unrevealed) and
	 *  completed subs are omitted so a finished step fades out and the rest remain. */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetActiveMainObjectiveView(FZP_ObjectiveDef& OutDef, TArray<FZP_ObjectiveRow>& OutRows) const;

	/** Call when a tab menu (map/inventory/notes) closes — re-fires OnTrackerRefresh so the HUD pops for 8s. */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void NotifyMenuClosed();

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetObjectiveDef(FName ObjectiveId, FZP_ObjectiveDef& OutDef) const;

	// --- Save/load hooks (M4 wires these into UZP_SaveGame) ---
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void GetSaveState(TArray<FName>& OutActive, TArray<FName>& OutCompleted, TArray<FName>& OutCompletedSubs, TArray<FName>& OutFlags) const;

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void RestoreSaveState(const TArray<FName>& InActive, const TArray<FName>& InCompleted, const TArray<FName>& InCompletedSubs, const TArray<FName>& InFlags);

	/** Copy current objective state INTO a save object — call just before SaveGameToSlot in the main save flow. */
	UFUNCTION(BlueprintCallable, Category = "Objectives|Save")
	void WriteToSave(UZP_SaveGame* Save) const;

	/** Restore objective state FROM a save object — call just after LoadGameFromSlot in the main load flow. */
	UFUNCTION(BlueprintCallable, Category = "Objectives|Save")
	void ReadFromSave(const UZP_SaveGame* Save);

	/** Self-contained persistence: write objective state to a dedicated slot (auto-called on change when bAutoPersist). */
	UFUNCTION(BlueprintCallable, Category = "Objectives|Save")
	void SaveObjectiveState();

	/** Self-contained persistence: restore objective state from the dedicated slot. */
	UFUNCTION(BlueprintCallable, Category = "Objectives|Save")
	void LoadObjectiveState();

	/** Dedicated slot used by SaveObjectiveState/LoadObjectiveState (separate from the player's manual save slots). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objectives|Save")
	FString ObjectiveStateSlot = TEXT("TheSignal_Objectives");

	/** When true, objective state auto-persists to ObjectiveStateSlot on every change and restores on game
	 *  start. FALSE = save-file-tied: a fresh PIE / new game starts clean (bStartOnLoad only); save/restore is
	 *  driven from the EasyGameUI save hook (AZP_GraceCharacter::OnEguiSaveLoadVariables → Save/LoadObjectiveState). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objectives|Save")
	bool bAutoPersist = false;

	// --- Events ---
	UPROPERTY(BlueprintAssignable, Category = "Objectives") FZP_OnObjectiveEvent OnObjectiveStarted;
	UPROPERTY(BlueprintAssignable, Category = "Objectives") FZP_OnObjectiveEvent OnObjectiveCompleted;
	UPROPERTY(BlueprintAssignable, Category = "Objectives") FZP_OnObjectiveEvent OnSubObjectiveCompleted;
	UPROPERTY(BlueprintAssignable, Category = "Objectives") FZP_OnFlagEvent OnFlagSet;

	/** Fires on ANY objective/sub/flag change AND on NotifyMenuClosed — the HUD tracker binds to this. */
	UPROPERTY(BlueprintAssignable, Category = "Objectives") FZP_OnTrackerRefresh OnTrackerRefresh;

private:
	UPROPERTY()
	TArray<FZP_ObjectiveDef> Definitions; // keyed in logic by FZP_ObjectiveDef::Id

	TSet<FName> ActiveObjectives;
	TSet<FName> CompletedObjectives;
	TSet<FName> CompletedSubObjectives;
	TSet<FName> RevealedSubObjectives;       // hidden subs whose RevealRequirements have been met
	TMap<FName, int32> SubObjectiveStage;    // per-sub completed-stage count (monotonic; survives item consumption)
	TSet<FName> Flags;
	TArray<FName> StartOnLoadIds; // objectives auto-started at game start (JSON "startOnLoad")

	bool bAdvancing = false;
	bool bRestoring = false; // suppress auto-persist while restoring from a save

	const FZP_ObjectiveDef* FindDef(FName Id) const;
	/** Locate a sub-objective by id across all definitions; optionally returns its owning main. */
	const FZP_SubObjectiveDef* FindSubDef(FName SubId, const FZP_ObjectiveDef** OutOwner = nullptr) const;
	bool EvaluateRequirement(const FZP_ObjectiveRequirement& Req) const;
	bool AreRequirementsMet(const TArray<FZP_ObjectiveRequirement>& Reqs) const; // all met (empty = false)
	void TryAdvance(); // reveal subs, advance stages, complete subs/mains, auto-start unlocked mains

	/** True if the player's Moonville inventory currently holds the item at ItemPath (a PDA_Item asset path). */
	bool PlayerHasItem(const FString& ItemPath) const;
};
