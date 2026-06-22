// Copyright The Signal. All Rights Reserved.

#include "ZP_TransitSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UZP_TransitSubsystem::TravelTo(TSoftObjectPtr<UWorld> TargetLevel, FName ArrivalPointTag)
{
	if (TargetLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitSubsystem::TravelTo — TargetLevel is null, aborting"));
		return;
	}

	PendingArrivalTag = ArrivalPointTag;

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] TransitSubsystem: Traveling to %s (arrival tag '%s')"),
		*TargetLevel.ToString(), *ArrivalPointTag.ToString());

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TargetLevel);
}

FName UZP_TransitSubsystem::ConsumePendingArrivalTag()
{
	const FName Tag = PendingArrivalTag;
	PendingArrivalTag = NAME_None;
	return Tag;
}
