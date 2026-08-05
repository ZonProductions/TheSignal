// Copyright The Signal. All Rights Reserved.

#include "ZP_ObjectiveHudBridge.h"
#include "ZP_ObjectiveSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UZP_ObjectiveHudBridge::UZP_ObjectiveHudBridge()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_ObjectiveHudBridge::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			Subsystem = GI->GetSubsystem<UZP_ObjectiveSubsystem>();
			if (Subsystem)
			{
				Subsystem->OnTrackerRefresh.AddDynamic(this, &UZP_ObjectiveHudBridge::HandleTrackerRefresh);
			}
		}
	}

	// Defer the initial refresh one tick so every BeginPlay bootstrap settles first — a per-level
	// reactor's StartObjective (e.g. RESEARCH1 in ResearchFacility) may run AFTER this component's
	// BeginPlay, and an immediate refresh would push the PREVIOUS level's bStartOnLoad main
	// ("Search for clues") into the tracker's 8s show window before the takeover.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UZP_ObjectiveHudBridge::HandleTrackerRefresh);
	}
}

void UZP_ObjectiveHudBridge::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Subsystem)
	{
		Subsystem->OnTrackerRefresh.RemoveDynamic(this, &UZP_ObjectiveHudBridge::HandleTrackerRefresh);
	}
	Super::EndPlay(Reason);
}

void UZP_ObjectiveHudBridge::RegisterQuestWidget(UUserWidget* Widget)
{
	// Swap to the HUD-owned widget; drop the fallback standalone one if we made it.
	if (bAutoCreated && QuestWidget && QuestWidget != Widget)
	{
		QuestWidget->RemoveFromParent();
	}
	QuestWidget = Widget;
	bAutoCreated = false;
	HandleTrackerRefresh();
}

void UZP_ObjectiveHudBridge::HandleTrackerRefresh()
{
	if (!Subsystem) return;

	// Create the quest widget once (C++ side — avoids BP CreateWidget).
	if (!QuestWidget && AZP_QuestWidgetClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
		{
			QuestWidget = CreateWidget<UUserWidget>(PC, AZP_QuestWidgetClass);
			if (QuestWidget) { QuestWidget->AddToViewport(50); bAutoCreated = true; }
		}
	}

	FZP_ObjectiveDef Def;
	TArray<FZP_ObjectiveRow> Rows;
	if (!Subsystem->GetActiveMainObjectiveView(Def, Rows) || Rows.Num() == 0)
	{
		// No active main, OR the active main has no revealed step yet (e.g. RESEARCH1 at level
		// start: every sub is reveal-gated). Clear the tracker instead of pushing an empty quest
		// list — an empty DisplayNewQuest never visibly replaces the previous quest, which left a
		// prior main's steps stuck on screen. Steps re-push here the moment one reveals.
		if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(FadeTimer);
		HideObjective(QuestWidget);
		return;
	}

	// Drive the BP glue: Begin -> one row per VISIBLE sub (current-stage title) -> End.
	// (Reveal/stage/complete filtering happens in the subsystem so each BP graph stays trivial.)
	BeginObjective();
	for (const FZP_ObjectiveRow& Row : Rows)
	{
		AddObjectiveRow(Row.SubId, Row.Title, Row.bComplete);
	}
	EndObjective(QuestWidget, Def.Title);

	// Show window: this call IS a show event (level load / objective update / menu close all route here).
	// (Re)start the countdown so the tracker fades out AZP_ShowDuration seconds after the most recent event.
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(FadeTimer, this, &UZP_ObjectiveHudBridge::FadeOutTracker, AZP_ShowDuration, false);
	}
}

void UZP_ObjectiveHudBridge::FadeOutTracker()
{
	// EasyGameUI ClearCurrentQuest plays the module's fade-out; a later show event re-displays it.
	HideObjective(QuestWidget);
}
