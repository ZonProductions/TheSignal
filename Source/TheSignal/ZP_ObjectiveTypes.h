// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * ZP_ObjectiveTypes
 *
 * Purpose: Data types for the campaign Objective backbone. FZP_ObjectiveDef is a DataTable row
 *          (FTableRowBase) — authored in DT_Objectives (imported from SourceData/Objectives.json,
 *          reimport on edit). The shipped game reads the cooked DataTable, not a loose file.
 *          Main objectives contain sub-objectives; mains are sequentially gated ('Requires').
 *
 * Owner Subsystem: UZP_ObjectiveSubsystem
 */

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ZP_ObjectiveTypes.generated.h"

UENUM(BlueprintType)
enum class EZP_ObjReqType : uint8
{
	FlagSet              UMETA(DisplayName = "Flag Set"),
	ObjectiveComplete    UMETA(DisplayName = "Objective Complete"),
	SubObjectiveComplete UMETA(DisplayName = "Sub-Objective Complete"),
	HasItem              UMETA(DisplayName = "Has Item"),          // value = item DataAsset path (M3 eval)
	HeardDialogue        UMETA(DisplayName = "Heard Dialogue"),    // value = dialogue id (M3 eval)
	CollectedNote        UMETA(DisplayName = "Collected Note"),    // value = note id (M3 eval)
	ReachedTrigger       UMETA(DisplayName = "Reached Trigger")    // value = trigger id (M3 eval)
};

/** One requirement on a sub-objective. Meaning of Value depends on Type. */
USTRUCT(BlueprintType)
struct FZP_ObjectiveRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	EZP_ObjReqType Type = EZP_ObjReqType::FlagSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	FString Value;
};

/** One stage of a multi-stage sub-objective. The sub displays the CURRENT stage's Title; when a
 *  stage's Requirements are all met it advances to the next stage (or completes the sub if it's the
 *  last). This is how a single tracked step re-titles itself as the player progresses, e.g.
 *  "Find security card" -> "Access east wing", or "Search for clues" -> "Go to the empty floor". */
USTRUCT(BlueprintType)
struct FZP_ObjectiveStage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TArray<FZP_ObjectiveRequirement> Requirements;

	/** Progression flag set the moment the objective ENTERS this stage (None = no flag). Lets a step
	 *  unlock other systems — e.g. a transit floor's RequiredFlag — the instant it becomes the current
	 *  step. Flags auto-persist, so the unlock survives save/load. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	FName EnterFlag = NAME_None;
};

USTRUCT(BlueprintType)
struct FZP_SubObjectiveDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	FName Id = NAME_None;

	/** Stage-0 title (also the title of a single-stage sub). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	FText Title;

	/** Stage-0 requirements. When met (and the sub has no extra Stages) the sub completes;
	 *  if extra Stages exist, meeting these advances to Stages[0]. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TArray<FZP_ObjectiveRequirement> Requirements;

	/** Empty = the sub is visible/tracked as soon as the parent objective is active. Non-empty = the
	 *  sub stays HIDDEN until ALL of these are met (e.g. a flag set after the player tries a locked
	 *  card reader), then it pops into the tracker. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TArray<FZP_ObjectiveRequirement> RevealRequirements;

	/** Extra stages AFTER stage 0. Effective stage list = [{Title, Requirements}] + Stages. The sub
	 *  completes when the LAST stage's requirements are met; its tracker title is the current stage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TArray<FZP_ObjectiveStage> Stages;

	/** Full ordered stage list: stage 0 = {Title, Requirements}, then the authored Stages. */
	TArray<FZP_ObjectiveStage> GetEffectiveStages() const
	{
		TArray<FZP_ObjectiveStage> Out;
		FZP_ObjectiveStage Stage0;
		Stage0.Title = Title;
		Stage0.Requirements = Requirements;
		Out.Add(Stage0);
		Out.Append(Stages);
		return Out;
	}
};

/** A resolved tracker row for the HUD: one currently-visible sub with its current-stage title.
 *  Built in C++ (UZP_ObjectiveSubsystem::GetActiveMainObjectiveView) so the HUD glue stays trivial. */
USTRUCT(BlueprintType)
struct FZP_ObjectiveRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	FName SubId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	bool bComplete = false;
};

/** A main objective — one DataTable row, keyed by its row name (= the objective id). */
USTRUCT(BlueprintType)
struct FZP_ObjectiveDef : public FTableRowBase
{
	GENERATED_BODY()

	/** Canonical, stable objective id — authored, and the key used everywhere. Decoupled from the
	 *  table/row that holds it, so objectives can be defined per level and shared across levels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	FName Id = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	FText Title;

	/** Prior main-objective ids that must be complete before this one can start (sequential gate). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TArray<FName> Requires;

	/** Side (optional) objective vs main. The HUD tracker shows the main only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	bool bSideObjective = false;

	/** Auto-start this objective at game start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	bool bStartOnLoad = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TArray<FZP_SubObjectiveDef> SubObjectives;
};
