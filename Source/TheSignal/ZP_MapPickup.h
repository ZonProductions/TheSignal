// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_MapPickup
 *
 * Purpose: World-placed pickup that unlocks a map for a specific area.
 *          When interacted, calls MapComponent->DiscoverMap(AreaID).
 *          Destroys self after pickup. Diegetic: Grace picks up a
 *          facility floor plan, clipboard, etc.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Blueprint Extension Points:
 *   - PickupMesh: set static mesh in BP child (clipboard, rolled map, etc.).
 *   - AreaID: which map area this pickup unlocks.
 *   - PromptText: customizable interaction text.
 *
 * Dependencies:
 *   - IZP_Interactable interface
 *   - UZP_MapComponent (on player character)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_MapPickup.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPointLightComponent;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_MapPickup : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_MapPickup();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UBoxComponent> InteractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UPointLightComponent> PickupGlow;

	// --- Config ---

	/** Which map area this pickup unlocks. Auto-generated if empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FName AreaID;

	/** Map texture to display. Set this to your exported PNG texture. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TObjectPtr<UTexture2D> MapTexture;

	/** Display name shown on the map widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText AreaDisplayName = FText::FromString(TEXT("Floor Map"));

	/** Auto-spawn a MapVolume covering this floor. Disable if placing volumes manually. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	bool bAutoCreateVolume = true;

	/** World bounds used to render the map texture. Set by Dev Tools export.
	 *  If set (non-zero), volume uses these exact bounds instead of scanning geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FVector2D MapBoundsMin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FVector2D MapBoundsMax = FVector2D::ZeroVector;

	/** Text shown on HUD when player is in range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FText PromptText = FText::FromString(TEXT("Pick Up Map"));

	// --- IZP_Interactable ---

	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	bool bPickedUp = false;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
