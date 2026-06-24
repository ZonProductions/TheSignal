// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_TransitPanel
 *
 * Purpose: Placeable elevator/tram console. Player overlaps trigger, presses E, and a
 *          floor-selection widget (WBP_TransitMenu) opens listing available destinations.
 *          Selecting one travels to that map via UZP_TransitSubsystem. INDEPENDENT of the
 *          card-reader keypad (door clearance) — they only share the inventory key-check.
 *
 * Owner Subsystem: Transit
 *
 * Blueprint Extension Points:
 *   - PanelMesh: set the console mesh in the BP child.
 *   - Destinations: per-placement list of floors/areas.
 *   - TransitMenuWidgetClass: WBP_TransitMenu (must extend UZP_TransitMenuWidget).
 *
 * Dependencies: IZP_Interactable, AZP_GraceCharacter (interaction + UI input), UZP_TransitSubsystem.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_TransitTypes.h"
#include "ZP_TransitPanel.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UUserWidget;
class AZP_Elevator;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_TransitPanel : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_TransitPanel();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transit")
	TObjectPtr<UStaticMeshComponent> PanelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transit")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Destinations offered by this panel (per-placement; promote to DataAsset in M5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit")
	TArray<FZP_TransitDestination> Destinations;

	/** The elevator car this console rides/controls. Required only if any destination is of type
	 *  InMapElevator. The panel auto-attaches to this car at BeginPlay so the console rides with it. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Transit")
	TObjectPtr<AZP_Elevator> LinkedElevator;

	/** Floor-selection widget. Must extend UZP_TransitMenuWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transit")
	TSubclassOf<UUserWidget> TransitMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transit")
	FText PromptText = FText::FromString(TEXT("Use Elevator"));

	// --- IZP_Interactable ---
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

	/** Called by the widget when a destination row is chosen. Validates + travels. */
	UFUNCTION(BlueprintCallable, Category = "Transit")
	void TravelToDestination(int32 DestinationIndex);

	/** Build the display rows for the widget. M1: all available; M3 applies gating. */
	UFUNCTION(BlueprintCallable, Category = "Transit")
	void BuildMenuEntries(TArray<FZP_TransitMenuEntry>& OutEntries) const;

	/** Whether a destination is currently selectable. M1: always true; M3 adds objective + key gating. */
	bool IsDestinationAvailable(const FZP_TransitDestination& Dest, ACharacter* ForCharacter) const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	TWeakObjectPtr<ACharacter> CurrentUser;
};
