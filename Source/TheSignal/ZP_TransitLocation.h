// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_TransitLocation
 *
 * Purpose: A placeable marker for an in-map elevator STOP (a floor). Place one at each floor at the
 *          height the elevator's pivot rests when parked there. An AZP_TransitPanel destination links
 *          to one of these; the panel computes the travel Z from the marker's world Z (relative to the
 *          elevator's BeginPlay origin) and HIDES that destination from the menu while the car is
 *          already at it (you can't travel to the floor you're on).
 *
 * Owner Subsystem: Transit
 *
 * Blueprint Extension Points:
 *   - AZP_LocationId / DisplayName: identify the stop (DisplayName optional; the destination row keeps its
 *     own label).
 *   - Place at the elevator's RESTING PIVOT height for the floor (not the floor surface) so the
 *     "currently here" comparison matches the elevator pivot.
 *
 * Dependencies: None — pure transform marker. Referenced by FZP_TransitDestination.ElevatorLocation.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_TransitLocation.generated.h"

class UBillboardComponent;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_TransitLocation : public AActor
{
	GENERATED_BODY()

public:
	AZP_TransitLocation();

	/** Stable id for this stop (e.g. Floor1). Optional but handy for logs/debugging. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transit")
	FName AZP_LocationId = NAME_None;

	/** Optional human label for the floor (the transit destination row keeps its own DisplayName). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transit")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transit")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	/** Editor-only sprite so the marker is visible/selectable in the viewport. */
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;
#endif
};
