// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_ObjectiveHudBridge
 *
 * Purpose: Glue between UZP_ObjectiveSubsystem and the EasyGameUI quest HUD module. Binds
 *          OnTrackerRefresh, creates/holds the quest widget in C++ (avoids BP CreateWidget),
 *          and drives three tiny BlueprintImplementableEvents on the BP child (BPC_ObjectiveHudGlue
 *          on PC_Grace): BeginObjective -> AddObjectiveRow (once per sub) -> EndObjective. The BP
 *          builds the S_QuestObjectiveDefinition list and calls DisplayNewQuest on the passed widget.
 *
 * Owner Subsystem: Objectives (HUD glue).
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_ObjectiveHudBridge.generated.h"

class UZP_ObjectiveSubsystem;
class UUserWidget;

UCLASS(Blueprintable, ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_ObjectiveHudBridge : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_ObjectiveHudBridge();

	/** The EasyGameUI quest module widget class — set to WBP_EHB_QuestStatusDisplayer in BPC defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TSubclassOf<UUserWidget> QuestWidgetClass;

	/** Force a refresh (also called on bind). BP-callable for testing. */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void RefreshNow() { HandleTrackerRefresh(); }

	/** Use an externally-owned quest widget (e.g. a WBP_HUD child) instead of the auto-created one.
	 *  Removes the fallback standalone widget if one was already created. Call from WBP_HUD Construct. */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void RegisterQuestWidget(UUserWidget* Widget);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** BP: clear the working objective list (start a fresh quest push). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Objectives")
	void BeginObjective();

	/** BP: append one row (Make S_QuestObjectiveDefinition -> add to list). Called once per sub. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Objectives")
	void AddObjectiveRow(FName SubId, const FText& Description, bool bComplete);

	/** BP: push the assembled list to the widget (DisplayNewQuest on Widget). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Objectives")
	void EndObjective(UUserWidget* Widget, const FText& QuestTitle);

	/** BP: clear/hide the quest widget (no active main objective). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Objectives")
	void HideObjective(UUserWidget* Widget);

	UFUNCTION()
	void HandleTrackerRefresh();

private:
	UPROPERTY()
	TObjectPtr<UZP_ObjectiveSubsystem> Subsystem;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> QuestWidget;

	bool bAutoCreated = false; // true if QuestWidget was auto-created (vs registered from the HUD)
};
