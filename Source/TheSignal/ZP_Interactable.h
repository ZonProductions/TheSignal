// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * IZP_Interactable
 *
 * Purpose: Minimal C++ interface for any actor the player can interact with.
 *          Save points, doors, pickups — anything that responds to E/X button.
 *          This is the foundation for TICKET-018 (Interaction System).
 *
 * Owner Subsystem: Gameplay
 *
 * Blueprint Extension Points:
 *   - GetInteractionPrompt: override to return custom prompt text per actor.
 *   - OnInteract: override to define what happens when player interacts.
 *
 * Dependencies: None — standalone interface.
 */

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ZP_Interactable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UZP_Interactable : public UInterface
{
	GENERATED_BODY()
};

class THESIGNAL_API IZP_Interactable
{
	GENERATED_BODY()

public:
	/** Text shown on HUD when player is in range. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt();

	/** Called when the player presses interact while in range. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteract(ACharacter* Interactor);
};
