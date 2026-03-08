// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_MapVolume
 *
 * Purpose: Level-placed actor that defines a map area's spatial bounds.
 *          When the player enters, sets the current area on MapComponent.
 *          Also serves as capture configuration for the map texture script.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Blueprint Extension Points:
 *   - AreaID, AreaDisplayName, MapTexture configurable per-instance.
 *   - CaptureHeight for automated texture generation.
 *
 * Dependencies:
 *   - UZP_MapComponent (on player character)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_MapVolume.generated.h"

class UBoxComponent;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_MapVolume : public AActor
{
	GENERATED_BODY()

public:
	AZP_MapVolume();

	/** Unique ID for this area. Must match MapPickup's AreaID. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FName AreaID;

	/** Display name shown on the map widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText AreaDisplayName;

	/** Captured map texture. Assigned after running capture script or manually. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TObjectPtr<UTexture2D> MapTexture;

	/** Height above the volume center to place capture camera. Set below ceiling height. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map|Capture")
	float CaptureHeight = 280.0f;

	/** Box defining the area bounds. Also used for overlap detection. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map")
	TObjectPtr<UBoxComponent> AreaBounds;

	/** Returns world-space min corner (XY) of this area. */
	FVector2D GetWorldBoundsMin() const;

	/** Returns world-space max corner (XY) of this area. */
	FVector2D GetWorldBoundsMax() const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
