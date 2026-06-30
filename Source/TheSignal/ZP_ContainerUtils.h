// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_ContainerUtils
 *
 * Purpose: Small Blueprint-callable helpers shared by searchable/lootable container
 *          actors (BP_LootLocker, future BP_ObjectiveContainer). Keeps geometry math
 *          in C++ so container BPs stay thin.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Blueprint Extension Points: static library — call the UFUNCTIONs directly.
 *
 * Dependencies: StaticMeshComponent, BoxComponent.
 */

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZP_ContainerUtils.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class THESIGNAL_API UZP_ContainerUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Resize and recenter a BoxComponent to enclose a StaticMeshComponent's mesh bounds.
	 * The Box must be attached under the mesh (so it inherits the mesh's world scale).
	 * Call at BeginPlay so the collision follows whatever mesh a placed instance uses.
	 * @param Mesh    Source mesh component (provides the bounds).
	 * @param Box     Box collision to resize/recenter.
	 * @param Padding Extra half-extent (UU) added on every axis (e.g. to keep the camera off the mesh).
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Container")
	static void FitBoxToMeshBounds(UStaticMeshComponent* Mesh, UBoxComponent* Box, float Padding = 0.f);
};
