// Copyright The Signal. All Rights Reserved.

#include "ZP_MapComponent.h"
#include "ZP_MapVolume.h"
#include "EngineUtils.h"

UZP_MapComponent::UZP_MapComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_MapComponent::BeginPlay()
{
	Super::BeginPlay();

	// Scan for all MapVolume actors in the level
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<AZP_MapVolume> It(World); It; ++It)
	{
		AZP_MapVolume* Volume = *It;
		if (Volume && !Volume->AreaID.IsNone())
		{
			AreaVolumes.Add(Volume->AreaID, Volume);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] MapComponent: Registered area '%s' (%s)"),
				*Volume->AreaID.ToString(), *Volume->AreaDisplayName.ToString());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] MapComponent: %d map areas registered"), AreaVolumes.Num());
}

void UZP_MapComponent::DiscoverMap(FName AreaID)
{
	if (AreaID.IsNone()) return;
	if (DiscoveredMaps.Contains(AreaID)) return;

	DiscoveredMaps.Add(AreaID);
	OnMapDiscovered.Broadcast(AreaID);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] MapComponent: Discovered map '%s'"), *AreaID.ToString());
}

bool UZP_MapComponent::IsMapDiscovered(FName AreaID) const
{
	return DiscoveredMaps.Contains(AreaID);
}

AZP_MapVolume* UZP_MapComponent::GetAreaVolume(FName AreaID) const
{
	const TObjectPtr<AZP_MapVolume>* Found = AreaVolumes.Find(AreaID);
	return Found ? Found->Get() : nullptr;
}

AZP_MapVolume* UZP_MapComponent::GetCurrentVolume() const
{
	return GetAreaVolume(CurrentAreaID);
}

void UZP_MapComponent::SetCurrentArea(FName AreaID)
{
	if (CurrentAreaID == AreaID) return;
	CurrentAreaID = AreaID;
	OnAreaChanged.Broadcast(CurrentAreaID);
}

void UZP_MapComponent::ClearCurrentArea(FName AreaID)
{
	if (CurrentAreaID == AreaID)
	{
		CurrentAreaID = NAME_None;
		OnAreaChanged.Broadcast(CurrentAreaID);
	}
}
