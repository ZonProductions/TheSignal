// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_PlayerController
 *
 * Purpose: Player controller handling Enhanced Input setup, input mapping
 *          context application, and HUD lifecycle. Extended by PC_Grace
 *          Blueprint for prototype input bindings.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - DefaultMappingContext set in Blueprint (avoids hard C++ asset refs).
 *   - Additional mapping contexts can be added/removed at runtime.
 *
 * Dependencies:
 *   - EnhancedInput (UEnhancedInputLocalPlayerSubsystem)
 */

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ZP_PlayerController.generated.h"

class UInputMappingContext;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AZP_PlayerController();

	/** Default mapping context applied on BeginPlay. Set in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Input mapping priority. Higher = takes precedence. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	int32 DefaultMappingPriority = 0;

	/** Add a mapping context at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void AddMappingContext(UInputMappingContext* Context, int32 Priority);

	/** Remove a mapping context at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void RemoveMappingContext(UInputMappingContext* Context);

protected:
	virtual void BeginPlay() override;
};
