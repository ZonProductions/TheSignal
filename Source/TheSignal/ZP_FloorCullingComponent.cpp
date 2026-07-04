// Copyright The Signal. All Rights Reserved.

#include "ZP_FloorCullingComponent.h"
#include "ZP_RuntimeISMBatcher.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Engine/Light.h"
#include "Components/LightComponent.h"

// Actor types that should NEVER be culled (global/essential)
static bool ShouldSkipActor(AActor* Actor, const TArray<FString>& SkipClasses)
{
	FString ClassName = Actor->GetClass()->GetName();
	// Strip _C suffix from Blueprint classes
	ClassName.RemoveFromEnd(TEXT("_C"));

	return SkipClasses.Contains(ClassName);
}

UZP_FloorCullingComponent::UZP_FloorCullingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_FloorCullingComponent::BeginPlay()
{
	Super::BeginPlay();
	// No auto-init — GraceCharacter calls Initialize() after ISMBatcher is wired
}

void UZP_FloorCullingComponent::Initialize()
{
	CollectActors();
	CheckPlayerFloor();

	GetWorld()->GetTimerManager().SetTimer(
		FloorCheckTimerHandle,
		this,
		&UZP_FloorCullingComponent::CheckPlayerFloor,
		AZP_CheckInterval,
		true
	);
}

void UZP_FloorCullingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FloorCheckTimerHandle);
	}

	// Restore all actors to visible on shutdown
	for (TArray<TWeakObjectPtr<AActor>>& Floor : FloorActors)
	{
		for (TWeakObjectPtr<AActor>& WeakActor : Floor)
		{
			if (AActor* Actor = WeakActor.Get())
			{
				Actor->SetActorHiddenInGame(false);

				// Re-enable lights
				if (ALight* Light = Cast<ALight>(Actor))
				{
					Light->GetLightComponent()->SetVisibility(true);
				}
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UZP_FloorCullingComponent::CollectActors()
{
	FloorActors.SetNum(AZP_NumFloors);

	AActor* Owner = GetOwner();
	int32 TotalCollected = 0;
	int32 LightsCollected = 0;
	int32 SkippedBatched = 0;

	// Get batched actors set from ISM batcher (if available)
	const TSet<TWeakObjectPtr<AActor>>* BatchedActors = nullptr;
	if (ISMBatcher)
	{
		BatchedActors = &ISMBatcher->GetBatchedActors();
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;

		// Skip self, owner, player-owned
		if (Actor == Owner || Actor->GetOwner() == Owner)
		{
			continue;
		}

		// Skip Pawns, Controllers, GameMode, etc.
		if (Actor->IsA(APawn::StaticClass()) || Actor->IsA(AController::StaticClass()))
		{
			continue;
		}

		// Skip global/essential actor types
		if (ShouldSkipActor(Actor, AZP_SkipActorClassNames))
		{
			continue;
		}

		// Skip actors already batched by ISM batcher — they're managed via SetFloorVisible
		if (BatchedActors && BatchedActors->Contains(Actor))
		{
			SkippedBatched++;
			continue;
		}

		// Skip actors in always-visible zones (stairwells, etc.) — they stay visible at all times
		{
			const FVector Loc = Actor->GetActorLocation();
			bool bInZone = false;
			for (const FBox& Zone : AZP_AlwaysVisibleZones)
			{
				if (Zone.IsInsideOrOn(Loc))
				{
					bInZone = true;
					break;
				}
			}
			if (bInZone)
			{
				continue;
			}
		}

		const float Z = Actor->GetActorLocation().Z;
		const int32 Floor = GetFloorForZ(Z);
		FloorActors[Floor].Add(Actor);
		TotalCollected++;

		if (Actor->IsA(ALight::StaticClass()))
		{
			LightsCollected++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("FloorCulling: Collected %d actors (%d lights), skipped %d batched, across %d floors:"),
		TotalCollected, LightsCollected, SkippedBatched, AZP_NumFloors);
	for (int32 i = 0; i < AZP_NumFloors; ++i)
	{
		UE_LOG(LogTemp, Log, TEXT("  Floor %d: %d actors"), i + 1, FloorActors[i].Num());
	}
}

void UZP_FloorCullingComponent::CheckPlayerFloor()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const int32 Floor = GetFloorForZ(Owner->GetActorLocation().Z);

	if (Floor != LastAppliedFloor)
	{
		CurrentFloor = Floor;
		ApplyFloorVisibility(Floor);
		LastAppliedFloor = Floor;

		UE_LOG(LogTemp, Log, TEXT("FloorCulling: Player on floor %d, showing floors %d-%d"),
			Floor + 1,
			FMath::Max(1, Floor + 1 - AZP_AdjacentFloorsToShow),
			FMath::Min(AZP_NumFloors, Floor + 1 + AZP_AdjacentFloorsToShow));
	}
}

void UZP_FloorCullingComponent::ApplyFloorVisibility(int32 TargetFloor)
{
	for (int32 i = 0; i < AZP_NumFloors; ++i)
	{
		const bool bShouldBeVisible = FMath::Abs(i - TargetFloor) <= AZP_AdjacentFloorsToShow;

		// Toggle ISM visibility for batched actors on this floor
		if (ISMBatcher)
		{
			ISMBatcher->SetFloorVisible(i, bShouldBeVisible);
		}

		// Toggle non-batched actors
		for (TWeakObjectPtr<AActor>& WeakActor : FloorActors[i])
		{
			if (AActor* Actor = WeakActor.Get())
			{
				Actor->SetActorHiddenInGame(!bShouldBeVisible);

				// Lights need SetVisibility on the component — HiddenInGame alone
				// doesn't stop them from contributing to the lighting pass
				if (ALight* Light = Cast<ALight>(Actor))
				{
					Light->GetLightComponent()->SetVisibility(bShouldBeVisible);
				}
			}
		}
	}
}

int32 UZP_FloorCullingComponent::GetFloorForZ(float Z) const
{
	const int32 Floor = FMath::FloorToInt((Z - AZP_FloorBaseZ) / AZP_FloorHeight);
	return FMath::Clamp(Floor, 0, AZP_NumFloors - 1);
}
