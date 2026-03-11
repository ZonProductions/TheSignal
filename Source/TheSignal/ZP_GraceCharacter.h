// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_GraceCharacter
 *
 * Purpose: First-person character for Grace Owens. Thin shell that owns
 *          components and routes input. All gameplay logic lives in
 *          UZP_GraceGameplayComponent, all Kinemation logic in
 *          UZP_KinemationComponent.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - MovementConfig DataAsset (propagated to GameplayComp).
 *   - WeaponClass (propagated to KinemationComp).
 *   - Input actions set via EditDefaultsOnly (configured in BP child).
 *   - OnInteract BlueprintImplementableEvent for interaction logic.
 *
 * Dependencies:
 *   - EnhancedInput
 *   - UZP_GraceGameplayComponent
 *   - UZP_KinemationComponent
 *   - UZP_GraceMovementConfig
 */

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ZP_GraceCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequenceBase;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
class UZP_GraceMovementConfig;
class UZP_GraceGameplayComponent;
class UZP_KinemationComponent;
class UPostProcessComponent;
class UZP_HealthComponent;
class UZP_MapComponent;
class UZP_FloorCullingComponent;
class UZP_RuntimeISMBatcher;
class UZP_NoteComponent;
class USpotLightComponent;
class UPointLightComponent;
class USoundBase;
class UZP_BriefcaseSubsystem;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_GraceCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZP_GraceCharacter();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** Visible first-person arms mesh. Attached to capsule like reference BP_TacticalShooterCharacter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USkeletalMeshComponent> PlayerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TObjectPtr<UZP_GraceGameplayComponent> GameplayComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kinemation")
	TObjectPtr<UZP_KinemationComponent> KinemationComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UZP_HealthComponent> HealthComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map")
	TObjectPtr<UZP_MapComponent> MapComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Notes")
	TObjectPtr<UZP_NoteComponent> NoteComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Culling")
	TObjectPtr<UZP_FloorCullingComponent> FloorCullingComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
	TObjectPtr<UZP_RuntimeISMBatcher> ISMBatcherComp;

	/** Post-process vignette driven by health percentage. Darkens screen edges below 50% HP. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UPostProcessComponent> DeathVignetteComp;

	// --- Flashlight (TLOU/SH2 style chest-mounted) ---

	/** Chest-mounted spotlight. Follows camera with slight lag for organic chest-mounted feel. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<USpotLightComponent> FlashlightComp;

	/** Ambient fill light — simulates light bouncing off surfaces near the flashlight beam.
	 *  Every horror game (TLOU, RE7, SH2) uses this trick: SpotLight for beam + PointLight for fill. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<UPointLightComponent> FlashlightFillComp;

	/** Whether the flashlight is currently on. */
	UPROPERTY(BlueprintReadWrite, Category = "Flashlight")
	bool bFlashlightOn = false;

	/** Sound played when toggling flashlight on/off. Defaults to CC pack flashlight click. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<USoundBase> FlashlightClickSound;

	/** How quickly the flashlight tracks the camera (higher = snappier, lower = more chest lag). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	float FlashlightInterpSpeed = 8.0f;

	/** Downward pitch offset from camera direction (chest naturally points slightly down). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	float FlashlightPitchOffset = -5.0f;

	// --- Configuration (propagated to components in PostInitializeComponents) ---

	/** DataAsset with all movement tuning values. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UZP_GraceMovementConfig> MovementConfig;

	/** Blueprint class of weapon to spawn. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	TSubclassOf<AActor> WeaponClass;

	/** Decal materials for bullet impacts. Set in BP child via Python. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TArray<TSoftObjectPtr<UMaterialInterface>> BulletDecalMaterials;

	/** Skeletal mesh for hidden locomotion Mesh. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<USkeletalMesh> LocomotionSkeletalMesh;

	/** Idle animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	/** Walk animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> WalkAnimation;

	/** Run animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> RunAnimation;

	/** Crouch idle animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> CrouchIdleAnimation;

	/** Crouch walk animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> CrouchWalkAnimation;

	/** Ladder climb up loop animation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> LadderClimbUpAnimation;

	/** Ladder climb down loop animation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> LadderClimbDownAnimation;

	/** Ladder idle animation (holding on, not moving). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> LadderIdleAnimation;

	// --- Input Actions (set in Blueprint child, e.g. BP_GraceCharacter) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	/** Q key — lean peek around cover (camera only, no weapon aim). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> PeekAction;

	/** RMB — aim down sights. Auto-peeks with weapon when near cover. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> ReloadAction;

	// --- BP Interface Compatibility ---
	// These properties are synced from components after init so that
	// BPI_TacticalShooterCharacter interface function implementations
	// (GetPrimaryWeapon, GetMainWeapon) can still read them as variables.
	// Will be removed when BP_GraceCharacter is deprecated.

	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	TObjectPtr<AActor> ActiveWeapon;

	// --- Map ---

	/** True when map overlay is open — blocks all gameplay input. DEPRECATED: Use IsMenuOpen() instead. */
	UPROPERTY(BlueprintReadWrite, Category = "Map")
	bool bMapOpen = false;

	/** Input action for toggling the map (M key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MapAction;

	/** Input action for cycling inventory tabs left (Q key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> TabCycleLeftAction;

	/** Input action for cycling inventory tabs right (E key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> TabCycleRightAction;

	// --- Inventory ---

	/** True when inventory UI is open — blocks all gameplay input. */
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	bool bInventoryMenuOpen = false;

	/** Runtime IMC that maps OwnSlotActions to Keys 1-4 at high priority. */
	UPROPERTY()
	TObjectPtr<UInputMappingContext> WeaponSlotIMC;

	/** Runtime-created input actions for weapon slots. NOT Moonville's actions —
	 *  avoids double-fire from conflicting IMC mappings. */
	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> OwnSlotActions;

	/** Auto-discovered Moonville BP_InteractionComponent (if present on this actor). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UActorComponent> MoonvilleInteractionComp;

	/** Auto-discovered Moonville BP_InventoryCharacterComponent (if present on this actor). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UActorComponent> MoonvilleInventoryComp;

	/** Input action for opening/closing inventory menu (Tab). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventoryMenuAction;

	/** Shortcut cross slot input actions (keys 1-4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventorySlot0Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventorySlot1Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventorySlot2Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventorySlot3Action;

	/** PDA_Item data asset for starting weapon. Granted to inventory at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSoftObjectPtr<UObject> StartingWeaponItem;

	// --- Interaction ---

	/** The IZP_Interactable actor currently in range (set by overlap on interactables). */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TWeakObjectPtr<AActor> CurrentInteractable;

	/** Called by interactable actors when player enters their trigger volume. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetCurrentInteractable(AActor* Interactable);

	/** Called by interactable actors when player leaves their trigger volume. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ClearCurrentInteractable(AActor* Interactable);

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	/** Override in Blueprint for interaction logic. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteract(AActor* InteractTarget);

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

private:
	// --- Death / Vignette ---
	UFUNCTION()
	void HandleDeath();

	UFUNCTION()
	void UpdateHealthVignette(float NewHealth, float MaxHealth, float DamageAmount);

	// --- Input Handlers ---
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_SprintStarted(const FInputActionValue& Value);
	void Input_SprintCompleted(const FInputActionValue& Value);
	void Input_Jump(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);
	void Input_CrouchStarted(const FInputActionValue& Value);
	void Input_CrouchCompleted(const FInputActionValue& Value);
	void Input_PeekStarted(const FInputActionValue& Value);
	void Input_PeekCompleted(const FInputActionValue& Value);
	void Input_AimStarted(const FInputActionValue& Value);
	void Input_AimCompleted(const FInputActionValue& Value);
	void Input_FireStarted(const FInputActionValue& Value);
	void Input_FireCompleted(const FInputActionValue& Value);
	void Input_ReloadStarted(const FInputActionValue& Value);
	void Input_InventoryMenu(const FInputActionValue& Value);
	void Input_Map(const FInputActionValue& Value);
	void Input_TabCycleLeft(const FInputActionValue& Value);
	void Input_TabCycleRight(const FInputActionValue& Value);
	void Input_InventorySlot0(const FInputActionValue& Value);
	void Input_InventorySlot1(const FInputActionValue& Value);
	void Input_InventorySlot2(const FInputActionValue& Value);
	void Input_InventorySlot3(const FInputActionValue& Value);

	/** Handles quickslot press: equips weapon OR uses consumable. SlotIndex is 0-based (Key 1 = slot 0). */
	void Input_InventorySlot(int32 SlotIndex);

	/** Calls Moonville's ExecuteItemActionByShortcut for consumable items. */
	void UseItemFromShortcutSlot(int32 SlotIndex);

	/** Adds StartingWeaponItem to inventory at BeginPlay. */
	void GrantStartingItems();

	/** Reads ObjectClass from a PDA_Item via reflection. Returns null if not a weapon. */
	TSubclassOf<AActor> GetWeaponClassFromItem(UObject* ItemDA);

	/** Reads weapon class from Moonville's ShortcutSlots[SlotIndex] via reflection. */
	TSubclassOf<AActor> GetWeaponFromShortcutSlot(int32 SlotIndex);

	/** Syncs ActiveWeapon from KinemationComp when weapon changes (equip/unequip). */
	UFUNCTION()
	void OnWeaponChangedHandler(AActor* NewWeapon);

	/** Removes 1 throwable from Moonville inventory when a grenade is consumed. */
	UFUNCTION()
	void OnThrowableConsumedHandler();

	/** Cached slot index of the last-used throwable for inventory removal. */
	int32 LastThrowableSlotIndex = -1;

	/** Cached PDA_Item data asset for the last-used throwable (for RemoveItemByDataAsset). */
	TObjectPtr<UObject> LastThrowableItemDA;

	// --- Briefcase / Container Tracking ---

	/** The briefcase actor currently being used by the player (for sync on close). */
	TWeakObjectPtr<AActor> ActiveBriefcaseActor;

	/** The container actor (any type) currently being used — for bPlayerIsUsingActor close detection. */
	TWeakObjectPtr<AActor> ActiveContainerActor;

	/** True when we called Interact() but haven't yet seen bPlayerIsUsingActor=true. */
	bool bWaitingForContainerOpen = false;

	/** Frames spent waiting for container to open. Timeout after N frames = container didn't have a UI (instant loot). */
	int32 ContainerOpenWaitFrames = 0;

	/** True once bPlayerIsUsingActor was confirmed true — now watching for it to go false. */
	bool bContainerWasOpen = false;

	/** Checks if an open container was closed, then syncs briefcase data and unequips removed weapons. */
	void CheckContainerClosed();

	/** Unequips the active weapon if it's no longer in the player's inventory. */
	void UnequipMissingWeapon();

	void ToggleFlashlight();

	/** Weapon classes that have been fully consumed (thrown grenades). Blocks re-equip. */
	TSet<TSubclassOf<AActor>> ConsumedWeaponClasses;

	/** Reads the PDA_Item UObject from Moonville's ShortcutSlots[SlotIndex] via reflection. */
	UObject* GetItemDAFromShortcutSlot(int32 SlotIndex);

	// --- Ladder Climbing ---

	/** True while the player is on a ladder. Blocks normal movement, aim, fire, etc. */
	UPROPERTY(BlueprintReadOnly, Category = "Ladder", meta = (AllowPrivateAccess = "true"))
	bool bOnLadder = false;

	/** The ladder actor being climbed. */
	TWeakObjectPtr<AActor> ActiveLadderActor;

	/** Current climb input: +1 = up, -1 = down, 0 = idle. Updated from Input_Move. */
	float LadderClimbInput = 0.f;


	/** Saved weapon class before entering ladder — re-equipped on exit. */
	TSubclassOf<UObject> PreLadderWeaponClass;

	/** Self-managed camera rotation during climbing. Controller rotation gets corrupted
	 *  by Kinemation camera modifiers, so we bypass it entirely on the ladder. */
	FRotator LadderCameraRotation = FRotator::ZeroRotator;

	/** Enter climbing state on the given ladder. Called from AZP_Ladder::OnInteract. */
	void EnterLadder(AActor* LadderActor);

	/** Exit climbing state. bExitTop = true → teleport to top, false → teleport to bottom. */
	void ExitLadder(bool bExitTop);

	friend class AZP_Ladder;

};
