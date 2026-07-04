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
 *   - AZP_DefaultMappingContext set in Blueprint (avoids hard C++ asset refs).
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
class UZP_InventoryTabWidget;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AZP_PlayerController();

	// --- Death / Respawn ---

	/** Called by Grace when health reaches zero. Fades to black, then respawns after delay. */
	void OnPawnDied();

	/** Seconds of camera fade from black on every level start (initial spawn and respawn reload). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fade")
	float AZP_LevelStartFadeInDuration = 0.5f;

	/** Seconds of the fade-to-black (with audio fade) when the pawn dies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
	float AZP_DeathFadeOutDuration = 0.5f;

	/** Seconds held at black after death before the level reloads for respawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
	float AZP_RespawnDelay = 1.0f;

	// --- HUD ---

	/** Widget class to spawn as the gameplay HUD. Set to WBP_HUD in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UZP_HUDWidget> AZP_HUDWidgetClass;

	/** Live HUD widget instance. Created in BeginPlay. */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UZP_HUDWidget> HUDWidget;

	// --- Dialogue ---

	/** Widget class for dialogue subtitles/choices. Set to WBP_DialogueBox in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UZP_DialogueWidget> AZP_DialogueWidgetClass;

	/** Live dialogue widget instance. Created in BeginPlay. */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UZP_DialogueWidget> DialogueWidget;

	/** Dialogue manager component. Auto-created. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UZP_DialogueManager> DialogueManager;

	// --- Map ---

	/** Widget class for the map overlay. Set to WBP_Map in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UZP_MapWidget> AZP_MapWidgetClass;

	/** Live map widget instance. Created in BeginPlay. */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UZP_MapWidget> MapWidget;

	// --- Inventory Tab Menu ---

	/** Widget class for the unified inventory tab menu (Map/Inventory/Notes). Set to WBP_InventoryTab in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UZP_InventoryTabWidget> AZP_InventoryTabWidgetClass;

	/** Live inventory tab widget instance. Created in BeginPlay. */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UZP_InventoryTabWidget> InventoryTabWidget;

	// --- Pause Menu ---

	/** Input action for toggling the pause menu. Set to IA_OpenPauseMenu in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UInputAction> AZP_PauseAction;

	/** Widget class to spawn as the pause menu. Set to WBP_EasyPauseMenu in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> AZP_PauseMenuWidgetClass;

	/** Named buttons inside the EGUI pause menu widget that get collapsed on open (unwanted menu entries). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|PauseMenu")
	TArray<FName> AZP_PauseMenuButtonsToHide = { "SaveSavegameButton", "PhotoModeBtn", "CreditsBtn" };

	/** True while the pause menu is up — gameplay/inventory input handlers suppress themselves under it. */
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsPauseMenuOpen() const;

	/** Dev console: set an objective progression flag (e.g. `ZP_ObjFlag reached_empty_floor` to complete
	 *  a step gated on that flag). Lets you verify objective progression without placing a trigger. */
	UFUNCTION(Exec)
	void ZP_ObjFlag(const FString& Flag);

	/** Dev console: wipe saved objective progress + in-memory state for a fresh run (`ZP_ObjReset`). */
	UFUNCTION(Exec)
	void ZP_ObjReset();

	/** Dev console: delete the persisted inventory snapshot so the NEXT PIE start grants the fresh
	 *  starting loadout (`ZP_InvReset`). Restart PIE after running it. */
	UFUNCTION(Exec)
	void ZP_InvReset();

	// --- Input ---

	/** Default mapping context applied on BeginPlay. Set in PC_Grace Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> AZP_DefaultMappingContext;

	/** Input mapping priority. Higher = takes precedence. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	int32 AZP_DefaultMappingPriority = 1;

	/** Add a mapping context at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void AddMappingContext(UInputMappingContext* Context, int32 Priority);

	/** Remove a mapping context at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void RemoveMappingContext(UInputMappingContext* Context);

	/** PROBE: type `DumpUIInput` in the console while a menu is open. Logs which keys are CURRENTLY
	 *  live for the CommonUI accept/back/close-notification actions (i.e. across all active mapping
	 *  contexts) + the focused widget — to diagnose why keyboard/gamepad don't drive CommonUI menus. */
	UFUNCTION(Exec)
	void DumpUIInput();

	/**
	 * Override SetPause to ensure input mode + cursor are always restored on unpause,
	 * regardless of who triggered it (our ESC handler or EGUI's Continue button).
	 */
	virtual bool SetPause(bool bPause, FCanUnpause CanUnpauseDelegate = FCanUnpause()) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	/** Toggle pause menu on/off. Bound to AZP_PauseAction. */
	void TogglePauseMenu();

	/** Tracked pause menu widget instance. Weak so self-close (Continue button) nulls automatically. */
	TWeakObjectPtr<UUserWidget> ActivePauseMenu;

	// --- Respawn ---
	void ExecuteRespawn();
	FTimerHandle RespawnTimerHandle;
};
