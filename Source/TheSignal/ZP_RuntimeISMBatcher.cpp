// Copyright The Signal. All Rights Reserved.

#include "ZP_RuntimeISMBatcher.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

// Batch key: groups static mesh actors by (floor, mesh, material set)
struct FISMBatchKey
{
	int32 FloorIndex;
	UStaticMesh* Mesh;
	uint32 MaterialHash;

	bool operator==(const FISMBatchKey& Other) const
	{
		return FloorIndex == Other.FloorIndex && Mesh == Other.Mesh && MaterialHash == Other.MaterialHash;
	}
};

uint32 GetTypeHash(const FISMBatchKey& Key)
{
	return HashCombine(HashCombine(::GetTypeHash(Key.FloorIndex), ::GetTypeHash(Key.Mesh)), Key.MaterialHash);
}

UZP_RuntimeISMBatcher::UZP_RuntimeISMBatcher()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_RuntimeISMBatcher::BeginPlay()
{
	Super::BeginPlay();
	// No auto-batch — GraceCharacter calls BatchStaticMeshes() after syncing floor params
}

void UZP_RuntimeISMBatcher::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Restore hidden actors
	for (const TWeakObjectPtr<AActor>& WeakActor : BatchedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			Actor->SetActorHiddenInGame(false);
		}
	}

	// Destroy all ISMCs across all floors
	for (TArray<TObjectPtr<UInstancedStaticMeshComponent>>& FloorArray : FloorISMCs)
	{
		for (UInstancedStaticMeshComponent* ISMC : FloorArray)
		{
			if (ISMC)
			{
				ISMC->DestroyComponent();
			}
		}
	}
	FloorISMCs.Empty();
	BatchedActors.Empty();

	Super::EndPlay(EndPlayReason);
}

void UZP_RuntimeISMBatcher::BatchStaticMeshes()
{
	struct FMeshGroup
	{
		int32 FloorIndex = 0;
		UStaticMesh* Mesh = nullptr;
		TArray<UMaterialInterface*> Materials;
		TArray<FTransform> Transforms;
		TArray<AActor*> SourceActors;
	};

	FloorISMCs.SetNum(NumFloors);

	TMap<FISMBatchKey, FMeshGroup> Groups;

	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AStaticMeshActor* SMA = *It;
		if (!SMA || SMA == GetOwner())
		{
			continue;
		}

		// Skip actors in always-visible zones (stairwells, etc.) — they must not be batched
		{
			const FVector Loc = SMA->GetActorLocation();
			bool bInZone = false;
			for (const FBox& Zone : AlwaysVisibleZones)
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

		UStaticMeshComponent* SMC = SMA->GetStaticMeshComponent();
		if (!SMC || !SMC->GetStaticMesh())
		{
			continue;
		}

		UStaticMesh* Mesh = SMC->GetStaticMesh();
		const int32 Floor = GetFloorForZ(SMA->GetActorLocation().Z);

		// Build material hash from all slot pointers
		uint32 MatHash = 0;
		for (int32 i = 0; i < SMC->GetNumMaterials(); ++i)
		{
			MatHash = HashCombine(MatHash, ::GetTypeHash(SMC->GetMaterial(i)));
		}

		FISMBatchKey Key;
		Key.FloorIndex = Floor;
		Key.Mesh = Mesh;
		Key.MaterialHash = MatHash;

		FMeshGroup& Group = Groups.FindOrAdd(Key);
		if (!Group.Mesh)
		{
			Group.FloorIndex = Floor;
			Group.Mesh = Mesh;
			// Capture materials from this instance (all in group share same material set)
			for (int32 i = 0; i < SMC->GetNumMaterials(); ++i)
			{
				Group.Materials.Add(SMC->GetMaterial(i));
			}
		}

		Group.Transforms.Add(SMA->GetActorTransform());
		Group.SourceActors.Add(SMA);
	}

	int32 TotalBatched = 0;
	int32 TotalISMs = 0;
	int32 Skipped = 0;

	AActor* Owner = GetOwner();

	for (auto& Pair : Groups)
	{
		FMeshGroup& Group = Pair.Value;

		if (Group.Transforms.Num() < MinInstanceCount)
		{
			Skipped += Group.Transforms.Num();
			continue;
		}

		// Create ISMC on the owner actor
		UInstancedStaticMeshComponent* ISMC = NewObject<UInstancedStaticMeshComponent>(Owner);
		ISMC->SetStaticMesh(Group.Mesh);

		// Apply materials from group (all instances share the same material set)
		for (int32 i = 0; i < Group.Materials.Num(); ++i)
		{
			if (Group.Materials[i])
			{
				ISMC->SetMaterial(i, Group.Materials[i]);
			}
		}

		ISMC->SetWorldTransform(FTransform::Identity);
		ISMC->RegisterComponent();

		// Add all instances
		ISMC->PreAllocateInstancesMemory(Group.Transforms.Num());
		for (const FTransform& T : Group.Transforms)
		{
			ISMC->AddInstance(T, true);
		}

		FloorISMCs[Group.FloorIndex].Add(ISMC);
		TotalISMs++;
		TotalBatched += Group.Transforms.Num();

		// Hide source actors visually but KEEP collision
		for (AActor* SrcActor : Group.SourceActors)
		{
			SrcActor->SetActorHiddenInGame(true);
			BatchedActors.Add(SrcActor);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ISMBatcher: Batched %d actors into %d ISMs across %d floors (%d actors below threshold, kept individual)"),
		TotalBatched, TotalISMs, NumFloors, Skipped);
	for (int32 i = 0; i < NumFloors; ++i)
	{
		UE_LOG(LogTemp, Log, TEXT("  Floor %d: %d ISMs"), i + 1, FloorISMCs[i].Num());
	}
}

void UZP_RuntimeISMBatcher::SetFloorVisible(int32 FloorIndex, bool bVisible)
{
	if (FloorIndex >= 0 && FloorIndex < FloorISMCs.Num())
	{
		for (UInstancedStaticMeshComponent* ISMC : FloorISMCs[FloorIndex])
		{
			if (ISMC)
			{
				ISMC->SetVisibility(bVisible, true);
			}
		}
	}
}

int32 UZP_RuntimeISMBatcher::GetFloorForZ(float Z) const
{
	const int32 Floor = FMath::FloorToInt((Z - FloorBaseZ) / FloorHeight);
	return FMath::Clamp(Floor, 0, NumFloors - 1);
}
