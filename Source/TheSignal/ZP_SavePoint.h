// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_SavePoint
 *
 * Purpose: Computer terminal save point in the world. Player approaches,
 *          sees interaction prompt, presses E to open save screen.
 *          Uses Bp_Desk02 mesh (or any desk variant) as the visual.
 *
 * Owner Subsystem: Gameplay / Save System
 *
 * Blueprint Extension Points:
 *   - DeskMesh: set the static mesh in BP child (BP_SavePoint).
 *   - PromptText: customizable interaction prompt.
 *   - OnSavePointActivated: BlueprintImplementableEvent for opening save UI.
 *
 * Dependencies:
 *   - IZP_Interactable interface
 *   - UZP_HUDWidget (for interaction prompt display)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_SavePoint.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPointLightComponent;
class UUserWidget;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_SavePoint : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_SavePoint();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SavePoint")
	TObjectPtr<UStaticMeshComponent> DeskMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SavePoint")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Soft glow from the monitor screen. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SavePoint")
	TObjectPtr<UPointLightComponent> MonitorGlow;

	// --- Config ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SavePoint")
	FText PromptText = FText::FromString(TEXT("Save Progress"));

	/** Widget class to spawn when player interacts. Set to WBP_ESGU_SavesManagerUI in BP defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SavePoint")
	TSubclassOf<UUserWidget> SaveMenuWidgetClass;

	// --- IZP_Interactable ---

	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

	/** Override in Blueprint to open the save UI. */
	UFUNCTION(BlueprintImplementableEvent, Category = "SavePoint")
	void OnSavePointActivated(ACharacter* Interactor);

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
