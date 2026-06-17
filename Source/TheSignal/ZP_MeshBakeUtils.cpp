// Copyright The Signal. All Rights Reserved.

#include "ZP_MeshBakeUtils.h"

#if WITH_EDITOR

#include "IMeshMergeUtilities.h"
#include "MeshMergeModule.h"
#include "MeshMerge/MeshMergingSettings.h"
#include "Engine/StaticMesh.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

UStaticMesh* UZP_MeshBakeUtils::BakeActorsToStaticMesh(const TArray<AActor*>& Actors, const FString& PackagePath, bool bSaveAsset)
{
	if (Actors.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[MeshBake] No actors provided."));
		return nullptr;
	}

	// Gather all valid skeletal/static mesh components, in their CURRENT pose.
	UWorld* World = nullptr;
	TArray<UPrimitiveComponent*> Components;
	for (AActor* Actor : Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (!World)
		{
			World = Actor->GetWorld();
		}

		TArray<UPrimitiveComponent*> Prims;
		Actor->GetComponents<UPrimitiveComponent>(Prims);
		for (UPrimitiveComponent* P : Prims)
		{
			if (USkeletalMeshComponent* Sk = Cast<USkeletalMeshComponent>(P))
			{
				if (Sk->GetSkeletalMeshAsset())
				{
					Components.Add(Sk);
				}
			}
			else if (UStaticMeshComponent* Sm = Cast<UStaticMeshComponent>(P))
			{
				if (Sm->GetStaticMesh())
				{
					Components.Add(Sm);
				}
			}
		}
	}

	if (!World || Components.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[MeshBake] No valid mesh components found across %d actor(s)."), Actors.Num());
		return nullptr;
	}

	// Convert to Static Mesh — bakes each component's current evaluated pose.
	const IMeshMergeUtilities& MeshUtil =
		FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();

	FMeshMergingSettings Settings;
	Settings.bMergePhysicsData = false; // reference/corpse mesh — no physics asset baking

	TArray<UObject*> CreatedAssets;
	FVector MergedLocation = FVector::ZeroVector;
	const float ScreenAreaSize = TNumericLimits<float>::Max();

	MeshUtil.MergeComponentsToStaticMesh(
		Components, World, Settings,
		/*InBaseMaterial*/ nullptr, /*InOuter*/ nullptr,
		PackagePath, CreatedAssets, MergedLocation, ScreenAreaSize, /*bSilent*/ false);

	UStaticMesh* Result = nullptr;
	for (UObject* Obj : CreatedAssets)
	{
		if (UStaticMesh* SM = Cast<UStaticMesh>(Obj))
		{
			Result = SM;
			break;
		}
	}

	if (!Result)
	{
		UE_LOG(LogTemp, Error, TEXT("[MeshBake] Merge produced no static mesh (input %d components)."), Components.Num());
		return nullptr;
	}

	// Register every created asset (mesh + material instances/textures) with the registry.
	FAssetRegistryModule& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (UObject* Obj : CreatedAssets)
	{
		AR.Get().AssetCreated(Obj);
		if (UPackage* P = Obj->GetOutermost())
		{
			P->MarkPackageDirty();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[MeshBake] Created %s from %d component(s); %d asset(s) total."),
		*Result->GetPathName(), Components.Num(), CreatedAssets.Num());

	if (bSaveAsset)
	{
		UPackage* Pkg = Result->GetOutermost();
		const FString FileName = FPackageName::LongPackageNameToFilename(
			Pkg->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		const bool bSaved = UPackage::SavePackage(Pkg, Result, *FileName, SaveArgs);
		UE_LOG(LogTemp, Log, TEXT("[MeshBake] Save %s -> %s"), *Pkg->GetName(), bSaved ? TEXT("OK") : TEXT("FAILED"));
	}

	return Result;
}

#endif
