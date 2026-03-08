// Copyright The Signal. All Rights Reserved.

#include "ZP_GameInstance.h"
#include "ZP_EventBroadcaster.h"

void UZP_GameInstance::Init()
{
	Super::Init();

	// Verify EventBroadcaster subsystem is alive
	UZP_EventBroadcaster* Broadcaster = GetSubsystem<UZP_EventBroadcaster>();
	if (Broadcaster)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] GameInstance initialized. EventBroadcaster: OK"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] GameInstance initialized. EventBroadcaster: MISSING"));
	}
}

void UZP_GameInstance::Shutdown()
{
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] GameInstance shutting down."));
	Super::Shutdown();
}
