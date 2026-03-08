// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * Map system data structures.
 * FZP_MapAreaData describes a single mappable area (floor, building section).
 */

#include "CoreMinimal.h"
#include "ZP_MapTypes.generated.h"

/**
 * Runtime snapshot of a map area's data.
 * Populated from AZP_MapVolume actors at BeginPlay.
 */
USTRUCT(BlueprintType)
struct FZP_MapAreaData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FName AreaID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText AreaDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TObjectPtr<UTexture2D> MapTexture = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map")
	FVector2D WorldBoundsMin = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map")
	FVector2D WorldBoundsMax = FVector2D::ZeroVector;
};
