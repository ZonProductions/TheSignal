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

	// --- Queries ---
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool IsObjectiveComplete(FName ObjectiveId) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool IsObjectiveActive(FName ObjectiveId) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool IsSubObjectiveComplete(FName SubObjectiveId) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool HasFlag(FName Flag) const;

	/** First currently-active objective (for the HUD tracker). Returns false if none active. */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetActiveObjective(FZP_ObjectiveDef& OutDef) const;

	/** First active MAIN objective (skips side objectives) — what the HUD tracker shows. */
	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetActiveMainObjective(FZP_ObjectiveDef& OutDef) const;

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
	TSet<FName> Flags;
	TArray<FName> StartOnLoadIds; // objectives auto-started at game start (JSON "startOnLoad")

	bool bAdvancing = false;

	const FZP_ObjectiveDef* FindDef(FName Id) const;
	bool EvaluateRequirement(const FZP_ObjectiveRequirement& Req) const;
	void TryAdvance(); // auto-complete subs/mains whose reqs are met; auto-start unlocked mains
};
