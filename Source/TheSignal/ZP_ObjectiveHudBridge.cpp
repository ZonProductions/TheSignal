// Copyright The Signal. All Rights Reserved.

#include "ZP_ObjectiveHudBridge.h"
#include "ZP_ObjectiveSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

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

	HandleTrackerRefresh(); // initial state
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
	if (!QuestWidget && QuestWidgetClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
		{
			QuestWidget = CreateWidget<UUserWidget>(PC, QuestWidgetClass);
			if (QuestWidget) { QuestWidget->AddToViewport(50); bAutoCreated = true; }
		}
	}

	FZP_ObjectiveDef Def;
	if (!Subsystem->GetActiveMainObjective(Def))
	{
		HideObjective(QuestWidget);
		return;
	}

	// Drive the BP glue: Begin -> one row per sub -> End. (Loop in C++ keeps each BP graph trivial.)
	BeginObjective();
	for (const FZP_SubObjectiveDef& Sub : Def.SubObjectives)
	{
		AddObjectiveRow(Sub.Id, Sub.Title, Subsystem->IsSubObjectiveComplete(Sub.Id));
	}
	EndObjective(QuestWidget, Def.Title);
}
