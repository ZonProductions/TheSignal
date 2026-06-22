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

USTRUCT(BlueprintType)
struct FZP_SubObjectiveDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	FName Id = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TArray<FZP_ObjectiveRequirement> Requirements;
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
