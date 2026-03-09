// Copyright The Signal. All Rights Reserved.

#include "ZP_MapComponent.h"
#include "ZP_MapVolume.h"
#include "Components/BoxComponent.h"
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

	// Initial area check — overlap events don't fire if the player starts inside a volume
	AActor* Owner = GetOwner();
	if (Owner && CurrentAreaID.IsNone())
	{
		const FVector Loc = Owner->GetActorLocation();
		for (auto& Pair : AreaVolumes)
		{
			AZP_MapVolume* Vol = Pair.Value;
			if (!Vol || !Vol->AreaBounds.Get()) continue;

			const FVector Center = Vol->GetActorLocation();
			const FVector Extent = Vol->AreaBounds.Get()->GetScaledBoxExtent();
			if (Loc.X >= Center.X - Extent.X && Loc.X <= Center.X + Extent.X &&
				Loc.Y >= Center.Y - Extent.Y && Loc.Y <= Center.Y + Extent.Y &&
				Loc.Z >= Center.Z - Extent.Z && Loc.Z <= Center.Z + Extent.Z)
			{
				SetCurrentArea(Pair.Key);
				UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MapComponent: Player starts inside area '%s'"),
					*Pair.Key.ToString());
				break;
			}
		}
	}
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
