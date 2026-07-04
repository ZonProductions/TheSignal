// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_MapPickup
 *
 * Purpose: World-placed pickup that unlocks a map for a specific area.
 *          When interacted, calls MapComponent->DiscoverMap(AZP_AreaID).
 *          Destroys self after pickup. Diegetic: Grace picks up a
 *          facility floor plan, clipboard, etc.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Blueprint Extension Points:
 *   - PickupMesh: set static mesh in BP child (clipboard, rolled map, etc.).
 *   - AZP_AreaID: which map area this pickup unlocks.
 *   - AZP_PromptText: customizable interaction text.
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
	FName AZP_AreaID;

	/** Map texture to display. Set this to your exported PNG texture. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TObjectPtr<UTexture2D> AZP_MapTexture;

	/** Display name shown on the map widget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText AZP_AreaDisplayName = FText::FromString(TEXT("Floor Map"));

	/** Auto-spawn a MapVolume covering this floor. Disable if placing volumes manually. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	bool bAZP_AutoCreateVolume = true;

	/** World bounds used to render the map texture. Set by Dev Tools export.
	 *  If set (non-zero), volume uses these exact bounds instead of scanning geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FVector2D AZP_MapBoundsMin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FVector2D AZP_MapBoundsMax = FVector2D::ZeroVector;

	/** Hardcoded Z-height bands that decide which Building1 floor number this pickup belongs to when picking the map_bounds_F<N>.txt export file - level-specific and will be wrong on any other map. */
	UPROPERTY(EditAnywhere, Category = "Map|AutoVolume")
	float AZP_FloorZThresholds[4] = { 1400.f, 900.f, 400.f, 0.f };

	/** Rough whole-level XY bounds used for the auto-spawned MapVolume when no Dev Tools bounds export file is found - level-specific magic numbers. */
	UPROPERTY(EditAnywhere, Category = "Map|AutoVolume")
	float AZP_FallbackMapBounds[4] = { -5000.f, -2000.f, 3000.f, 6000.f };

	/** Z half-extent (UU) of the auto-spawned MapVolume's AreaBounds box - must cover the floor's playable height band for map display to trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|AutoVolume")
	float AZP_AutoVolumeHalfHeight = 300.f;

	/** Text shown on HUD when player is in range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FText AZP_PromptText = FText::FromString(TEXT("Pick Up Map"));

	/** Box extent of the pickup's interaction/overlap volume - how close the player must be for the prompt; this project has a history of interaction volumes needing shrink/offset tuning (open-through-walls fix). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Interaction")
	FVector AZP_InteractionVolumeExtent = FVector(100.f, 100.f, 80.f);

	/** Point-light intensity of the pickup glow highlight (visual feel: how brightly the map pickup advertises itself in a dark horror level). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Glow")
	float AZP_GlowIntensity = 300.f;

	/** Attenuation radius of the pickup glow light - how far the glow spills into the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Glow")
	float AZP_GlowAttenuationRadius = 150.f;

	/** Color of the pickup glow light (currently cool blue, the project's color-code for map pickups). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Glow")
	FLinearColor AZP_GlowColor = FLinearColor(0.4f, 0.7f, 0.9f);

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
