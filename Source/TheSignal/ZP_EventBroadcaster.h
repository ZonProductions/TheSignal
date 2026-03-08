// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_EventBroadcaster
 *
 * Purpose: GameInstanceSubsystem — typed event bus for all cross-system
 *          communication. Systems bind to delegates here instead of
 *          coupling to each other directly.
 *
 * Owner Subsystem: Core
 *
 * Blueprint Extension Points:
 *   - All delegates are BlueprintAssignable for BP binding.
 *   - Broadcast functions are BlueprintCallable for BP triggering.
 *
 * Dependencies:
 *   - None (intentionally dependency-free — all systems depend on THIS)
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZP_EventBroadcaster.generated.h"

// --- Delegate Signatures ---

/** Facility power state changed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFacilityPowerChanged, bool, bPowered);

/** A narrative beat was triggered. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNarrativeBeatTriggered, FName, BeatID);

/** Player sprint state changed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerSprintChanged, bool, bIsSprinting);

/** Player interaction target changed (null = nothing targeted). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionTargetChanged, AActor*, NewTarget);

/** Player stamina changed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, NormalizedStamina);

/** Player peek state changed. Direction: -1=Left, 0=None, 1=Right. Alpha: 0-1 blend. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerPeekChanged, int32, Direction, float, Alpha);

/** A dialogue sequence started. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueEvent, FName, DialogueID);


UCLASS()
class THESIGNAL_API UZP_EventBroadcaster : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Delegates ---

	UPROPERTY(BlueprintAssignable, Category = "Events|Facility")
	FOnFacilityPowerChanged OnFacilityPowerChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Narrative")
	FOnNarrativeBeatTriggered OnNarrativeBeatTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Events|Player")
	FOnPlayerSprintChanged OnPlayerSprintChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Player")
	FOnInteractionTargetChanged OnInteractionTargetChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Player")
	FOnStaminaChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Player")
	FOnPlayerPeekChanged OnPlayerPeekChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Dialogue")
	FOnDialogueEvent OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "Events|Dialogue")
	FOnDialogueEvent OnDialogueFinished;

	// --- Broadcast Functions ---

	UFUNCTION(BlueprintCallable, Category = "Events|Facility")
	void BroadcastFacilityPowerChanged(bool bPowered);

	UFUNCTION(BlueprintCallable, Category = "Events|Narrative")
	void BroadcastNarrativeBeat(FName BeatID);

	UFUNCTION(BlueprintCallable, Category = "Events|Player")
	void BroadcastSprintChanged(bool bIsSprinting);

	UFUNCTION(BlueprintCallable, Category = "Events|Player")
	void BroadcastInteractionTargetChanged(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Events|Player")
	void BroadcastStaminaChanged(float NormalizedStamina);

	UFUNCTION(BlueprintCallable, Category = "Events|Player")
	void BroadcastPeekChanged(int32 Direction, float Alpha);

	UFUNCTION(BlueprintCallable, Category = "Events|Dialogue")
	void BroadcastDialogueStarted(FName DialogueID);

	UFUNCTION(BlueprintCallable, Category = "Events|Dialogue")
	void BroadcastDialogueFinished(FName DialogueID);
};
