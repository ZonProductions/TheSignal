// Copyright The Signal. All Rights Reserved.

#include "ZP_EventBroadcaster.h"

DEFINE_LOG_CATEGORY_STATIC(LogEventBroadcaster, Log, All);

void UZP_EventBroadcaster::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogEventBroadcaster, Log, TEXT("[TheSignal] EventBroadcaster initialized."));
}

void UZP_EventBroadcaster::Deinitialize()
{
	UE_LOG(LogEventBroadcaster, Log, TEXT("[TheSignal] EventBroadcaster shutting down."));
	Super::Deinitialize();
}

void UZP_EventBroadcaster::BroadcastFacilityPowerChanged(bool bPowered)
{
	UE_LOG(LogEventBroadcaster, Log, TEXT("FacilityPowerChanged: %s"), bPowered ? TEXT("ON") : TEXT("OFF"));
	OnFacilityPowerChanged.Broadcast(bPowered);
}

void UZP_EventBroadcaster::BroadcastNarrativeBeat(FName BeatID)
{
	UE_LOG(LogEventBroadcaster, Log, TEXT("NarrativeBeat: %s"), *BeatID.ToString());
	OnNarrativeBeatTriggered.Broadcast(BeatID);
}

void UZP_EventBroadcaster::BroadcastSprintChanged(bool bIsSprinting)
{
	OnPlayerSprintChanged.Broadcast(bIsSprinting);
}

void UZP_EventBroadcaster::BroadcastInteractionTargetChanged(AActor* NewTarget)
{
	OnInteractionTargetChanged.Broadcast(NewTarget);
}

void UZP_EventBroadcaster::BroadcastStaminaChanged(float NormalizedStamina)
{
	OnStaminaChanged.Broadcast(NormalizedStamina);
}

void UZP_EventBroadcaster::BroadcastPeekChanged(int32 Direction, float Alpha)
{
	UE_LOG(LogEventBroadcaster, Verbose, TEXT("PeekChanged: Dir=%d Alpha=%.2f"), Direction, Alpha);
	OnPlayerPeekChanged.Broadcast(Direction, Alpha);
}

void UZP_EventBroadcaster::BroadcastDialogueStarted(FName DialogueID)
{
	UE_LOG(LogEventBroadcaster, Log, TEXT("DialogueStarted: %s"), *DialogueID.ToString());
	OnDialogueStarted.Broadcast(DialogueID);
}

void UZP_EventBroadcaster::BroadcastDialogueFinished(FName DialogueID)
{
	UE_LOG(LogEventBroadcaster, Log, TEXT("DialogueFinished: %s"), *DialogueID.ToString());
	OnDialogueFinished.Broadcast(DialogueID);
}
