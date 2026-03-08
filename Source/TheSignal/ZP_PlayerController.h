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

class UInputAction;
class UInputMappingContext;
class UZP_HUDWidget;
class UZP_DialogueManager;
class UZP_DialogueWidget;
class UZP_MapWidget;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AZP_PlayerController();

	// --- Death / Respawn ---

	/** Called by Grace when health reaches zero. Fades to black, then respawns after delay. */
	void OnPawnDied();

	// --- HUD ---

	/** Widget class to spawn as the gameplay HUD. Set to WBP_HUD in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UZP_HUDWidget> HUDWidgetClass;

	/** Live HUD widget instance. Created in BeginPlay. */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UZP_HUDWidget> HUDWidget;

	// --- Dialogue ---

	/** Widget class for dialogue subtitles/choices. Set to WBP_DialogueBox in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UZP_DialogueWidget> DialogueWidgetClass;

	/** Live dialogue widget instance. Created in BeginPlay. */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UZP_DialogueWidget> DialogueWidget;

	/** Dialogue manager component. Auto-created. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UZP_DialogueManager> DialogueManager;

	// --- Map ---

	/** Widget class for the map overlay. Set to WBP_Map in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UZP_MapWidget> MapWidgetClass;

	/** Live map widget instance. Created in BeginPlay. */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UZP_MapWidget> MapWidget;

	// --- Pause Menu ---

	/** Input action for toggling the pause menu. Set to IA_OpenPauseMenu in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UInputAction> PauseAction;

	/** Widget class to spawn as the pause menu. Set to WBP_EasyPauseMenu in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	// --- Input ---

	/** Default mapping context applied on BeginPlay. Set in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Input mapping priority. Higher = takes precedence. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	int32 DefaultMappingPriority = 1;

	/** Add a mapping context at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void AddMappingContext(UInputMappingContext* Context, int32 Priority);

	/** Remove a mapping context at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void RemoveMappingContext(UInputMappingContext* Context);

	/**
	 * Override SetPause to ensure input mode + cursor are always restored on unpause,
	 * regardless of who triggered it (our ESC handler or EGUI's Continue button).
	 */
	virtual bool SetPause(bool bPause, FCanUnpause CanUnpauseDelegate = FCanUnpause()) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	/** Toggle pause menu on/off. Bound to PauseAction. */
	void TogglePauseMenu();

	/** Tracked pause menu widget instance. Weak so self-close (Continue button) nulls automatically. */
	TWeakObjectPtr<UUserWidget> ActivePauseMenu;

	// --- Respawn ---
	void ExecuteRespawn();
	FTimerHandle RespawnTimerHandle;
};
