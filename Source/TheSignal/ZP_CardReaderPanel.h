// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_CardReaderPanel
 *
 * Purpose: Wall-mounted card reader panel. Player overlaps trigger, presses E.
 *          Auto-checks Moonville inventory for the required key item.
 *          If found: consumes key, unlocks + opens linked door, light turns green.
 *          If missing: shows HUD prompt with required item name.
 *          No popup widget — immediate feedback.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Blueprint Extension Points:
 *   - PanelMesh: set static mesh in BP child.
 *   - RequiredItemDA / RequiredItemName: configure per-instance.
 *   - LinkedDoor: set reference to door actor in level editor.
 *
 * Dependencies:
 *   - IZP_Interactable interface
 *   - AZP_LockableDoor (linked door)
 *   - Moonville inventory system (reflection-based access)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_CardReaderPanel.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPointLightComponent;
class AZP_LockableDoor;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_CardReaderPanel : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_CardReaderPanel();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CardReader")
	TObjectPtr<UStaticMeshComponent> PanelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CardReader")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Red = locked, green = unlocked. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CardReader")
	TObjectPtr<UPointLightComponent> StatusLight;

	// --- Config ---

	/** The Moonville PDA_Item data asset required to unlock. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CardReader")
	TSoftObjectPtr<UObject> RequiredItemDA;

	/** Display name of the required item (shown in HUD feedback). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CardReader")
	FText RequiredItemName = FText::FromString(TEXT("Key Card"));

	/** The door this panel unlocks. Set per-instance in level editor. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "CardReader")
	TObjectPtr<AZP_LockableDoor> LinkedDoor;

	/** Whether to remove the key item from inventory after use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CardReader")
	bool bConsumeKeyOnUse = true;

	/** Prompt shown to the player when in range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CardReader")
	FText PromptText = FText::FromString(TEXT("Use Card Reader"));

	/** Message shown when item is missing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CardReader")
	FText MissingItemMessage = FText::FromString(TEXT("Required: {0}"));

	/** Message shown when access is granted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CardReader")
	FText AccessGrantedMessage = FText::FromString(TEXT("Access Granted"));

	// --- IZP_Interactable ---

	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	bool bUnlocked = false;

	/** Check if the character has RequiredItemDA in their Moonville inventory. */
	bool CheckPlayerHasItem(ACharacter* Character);

	/** Consume the key item, unlock + open the linked door. */
	void UseKey(ACharacter* Character);

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Search a Moonville slot array (InventorySlots or ShortcutSlots) for the target DA. */
	bool HasItemInSlotArray(UActorComponent* InvComp, const FName& ArrayName, UObject* TargetDA);

	/** Get the Moonville inventory component from the character. */
	UActorComponent* GetMoonvilleInventoryComp(ACharacter* Character);

	void SetStatusLightColor(FLinearColor Color);
};
