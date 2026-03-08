// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_KeyPickup
 *
 * Purpose: World-placed pickup that adds a Moonville PDA_Item data asset
 *          to the player's inventory on interact. Destroys self after pickup.
 *          Used for key items (ID cards, keycards, etc.) and any single-use
 *          collectible that needs to exist in the world.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Blueprint Extension Points:
 *   - PickupMesh: set static mesh in BP child.
 *   - ItemDataAsset / ItemAmount: configure per-instance.
 *   - PromptText: customizable interaction text.
 *
 * Dependencies:
 *   - IZP_Interactable interface
 *   - Moonville inventory system (AddItemSimple via reflection)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_KeyPickup.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPointLightComponent;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_KeyPickup : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_KeyPickup();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Glow to draw player attention. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UPointLightComponent> PickupGlow;

	// --- Config ---

	/** The Moonville PDA_Item data asset to grant on pickup. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	TSoftObjectPtr<UObject> ItemDataAsset;

	/** Number of items to add. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	int32 ItemAmount = 1;

	/** Text shown on HUD when player is in range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FText PromptText = FText::FromString(TEXT("Pick Up"));

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
