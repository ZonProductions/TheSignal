// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_VentDoor
 *
 * Purpose: One-shot, key-gated vent panel. Player needs AZP_RequiredItemDA in
 *          inventory to open. When opened, the mesh tilts outward around a
 *          top edge hinge (AZP_HingeOffset is the hinge location in actor-local
 *          space; VentMesh is offset by -AZP_HingeOffset so its visual position
 *          stays put at construction). After full open: collision disabled
 *          and interaction volume deactivated — cannot be re-closed.
 *
 *          Use this for screwdriver-removed vents, plywood barricades, etc.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Blueprint Extension Points:
 *   - VentMesh: set static mesh in BP child.
 *   - AZP_RequiredItemDA / AZP_RequiredItemName: configure per-instance.
 *   - AZP_HingeOffset / AZP_OpenRotation: per-instance hinge geometry.
 *
 * Dependencies:
 *   - IZP_Interactable
 *   - Moonville inventory system (reflection-based)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_VentDoor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USceneComponent;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_VentDoor : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_VentDoor();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vent")
	TObjectPtr<USceneComponent> HingePivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vent")
	TObjectPtr<UStaticMeshComponent> VentMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vent")
	TObjectPtr<UBoxComponent> InteractionVolume;

	// --- Config ---

	/** The Moonville PDA_Item data asset required to open this vent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	TSoftObjectPtr<UObject> AZP_RequiredItemDA;

	/** Display name of the required item (shown in HUD prompts). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	FText AZP_RequiredItemName = FText::FromString(TEXT("Screwdriver"));

	/** Prompt shown when player has no required item. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	FText AZP_LockedPrompt = FText::FromString(TEXT("Needs something to unscrew this..."));

	/** Prompt shown when player has the required item. {0} = AZP_RequiredItemName. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	FText AZP_UnlockedPromptFormat = FText::FromString(TEXT("Use {0} to open it up"));

	/**
	 * Hinge location in actor-local space. The VentMesh is offset by -AZP_HingeOffset
	 * so its visible position stays at actor origin when HingePivot rotation = 0.
	 * For SM_AirDuct_Vent at scale (6.5, 2.5, 1.0) with mesh long-axis on actor +X:
	 *   AZP_HingeOffset = (76.5, 0, 0)  // top edge in actor-local
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	FVector AZP_HingeOffset = FVector(76.5f, 0.f, 0.f);

	/** Relative rotation applied to HingePivot when fully open (tilts mesh outward). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	FRotator AZP_OpenRotation = FRotator(-90.f, 0.f, 0.f);

	/** Seconds to play the open animation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	float AZP_OpenDuration = 0.7f;

	/** Half-extents of the interaction box (player must enter this volume to interact). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	FVector AZP_InteractionVolumeExtent = FVector(100.f, 100.f, 100.f);

	/** Relative offset of the interaction box from actor origin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	FVector AZP_InteractionVolumeOffset = FVector(0.f, 0.f, 0.f);

	/** If true, consume one of AZP_RequiredItemDA from inventory on use. Default false (reusable tool). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent")
	bool bAZP_ConsumeItemOnUse = false;

	// --- IZP_Interactable ---

	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

private:
	bool bIsOpen = false;
	bool bIsAnimating = false;
	float AnimT = 0.f;

	void ApplyHingeLayout();
	void StartOpen();
	void FinishOpen();

	bool CheckPlayerHasItem(ACharacter* Character);
	void ConsumeItem(ACharacter* Character);
	UActorComponent* GetMoonvilleInventoryComp(ACharacter* Character);
	bool HasItemInSlotArray(UActorComponent* InvComp, const FName& ArrayName, UObject* TargetDA);

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
