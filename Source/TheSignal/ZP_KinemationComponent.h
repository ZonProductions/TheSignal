// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_KinemationComponent
 *
 * Purpose: Reusable ActorComponent encapsulating Kinemation Tactical Shooter Pack
 *          integration: auto-detection of Kinemation BP components, camera API,
 *          weapon lifecycle, and fire/reload forwarding.
 *          Designed to work on any ACharacter (AZP_GraceCharacter or GASP BP).
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - CameraComponent ref (auto-discovered by name "FirstPersonCamera" if not set).
 *   - PlayerMeshComponent ref (auto-discovered by name "PlayerMesh" if not set).
 *   - WeaponClass to spawn at BeginPlay.
 *   - Kinemation component refs (auto-detected from owner's component list).
 *
 * Dependencies:
 *   - KinemationBridge (static reflection helper)
 *   - Kinemation Tactical Shooter Pack (Blueprint components on owner)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_WeaponTypes.h"
#include "ZP_KinemationComponent.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class UAnimSequenceBase;

UCLASS(ClassGroup=(TheSignal), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_KinemationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_KinemationComponent();

	// --- Configuration ---

	/** Camera to wire to Kinemation. Auto-discovered if not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
	TObjectPtr<UCameraComponent> CameraComponent;

	/** Player mesh for weapon attachment. Auto-discovered by name "PlayerMesh" if not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
	TObjectPtr<USkeletalMeshComponent> PlayerMeshComponent;

	/** Blueprint class of weapon to spawn at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	TSubclassOf<AActor> WeaponClass;

	/** If true, weapon spawns automatically during InitializeKinemation.
	 *  Set false when inventory system manages weapon lifecycle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Kinemation|Weapon")
	bool bAutoSpawnWeapon = true;

	// --- Hitscan Config ---

	/** Decal materials for bullet impacts. Randomly chosen per shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TArray<TObjectPtr<UMaterialInterface>> BulletDecalMaterials;

	/** Maximum range for hitscan traces (cm). Default 10000 = 100m. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float HitscanRange = 10000.0f;

	/** Size of bullet hole decals (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	FVector DecalSize = FVector(2.0f, 3.0f, 3.0f);

	/** How long decals remain before fading (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float DecalLifetime = 30.0f;

	/** Damage dealt on body hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float HitscanBodyDamage = 10.f;

	/** Damage dealt on weak point (center mass) hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float HitscanWeakPointDamage = 50.f;

	/** Distance (UU) from actor center that counts as a weak point hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float WeakPointRadius = 50.f;

	// --- Weapon Type ---

	/** Current weapon archetype — drives fire input routing. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	EZP_WeaponType CurrentWeaponType = EZP_WeaponType::None;

	// --- Melee Config ---

	/** Damage per melee swing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeDamage = 25.f;

	/** Maximum reach of melee sweep (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeRange = 200.f;

	/** Radius of the sphere sweep for melee hit detection (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeSweepRadius = 40.f;

	/** Minimum time between melee swings (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeCooldown = 0.6f;

	// --- Throwable Config ---

	/** Blueprint class to spawn when throwing a grenade. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Throwable")
	TSubclassOf<AActor> GrenadeProjectileClass;

	/** Initial speed of thrown projectile (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Throwable")
	float ThrowSpeed = 800.f;

	// --- Animation Sequences (played as dynamic montages) ---

	/** Melee swing animation — loaded by path in InitializeKinemation. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> MeleeSwingAnim;

	/** Grenade throw animation — loaded by path in InitializeKinemation. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> GrenadeThrowAnim;

	// --- Weapon State ---

	/** The spawned weapon actor (if any). */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	TObjectPtr<AActor> ActiveWeapon;

	/** Weapon class that was equipped before a throwable. Used to auto-switch back after throw. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	TSubclassOf<UObject> PreviousWeaponClass;

	// --- Kinemation Component Refs (auto-detected or manually set) ---

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> TacticalCameraComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> TacticalAnimComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> RecoilAnimComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> IKMotionComp;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponChanged, AActor*, NewWeapon);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, CurrentAmmo, int32, ReserveAmmo);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponTypeChanged, EZP_WeaponType, NewWeaponType);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnThrowableConsumed);

	/** Broadcast when weapon is equipped or unequipped. NewWeapon is null on unequip. */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Weapon")
	FOnWeaponChanged OnWeaponChanged;

	// --- Ammo ---

	/** Rounds per magazine. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 MagSize = 12;

	/** Current rounds in the magazine. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 CurrentAmmo = 12;

	/** Reserve ammo (not in magazine). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 ReserveAmmo = 48;

	/** Broadcast when ammo count changes (fire, reload). */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Ammo")
	FOnAmmoChanged OnAmmoChanged;

	/** Broadcast when weapon type changes (ranged, melee, throwable). */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Weapon")
	FOnWeaponTypeChanged OnWeaponTypeChanged;

	/** Broadcast when a throwable is consumed (thrown). Owner should remove 1 from inventory. */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Weapon")
	FOnThrowableConsumed OnThrowableConsumed;

	// --- ADS Config ---

	/** Default field of view (hip-fire). Set from MovementConfig in PostInitializeComponents. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|ADS")
	float DefaultFOV = 90.0f;

	/** Field of view when aiming down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|ADS")
	float AdsFOV = 65.0f;

	/** Interpolation speed for FOV transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|ADS")
	float AdsFOVInterpSpeed = 10.0f;

	// --- Camera API ---

	/** Trigger a Kinemation camera shake. Requires TacticalCameraComp. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Kinemation")
	void TriggerCameraShake(UObject* ShakeData);

	/** Smoothly interpolate FOV to a new target. Requires TacticalCameraComp. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Kinemation")
	void SetTargetFOV(float NewFOV, float InterpSpeed = 5.0f);

	// --- Initialization ---

	/**
	 * Wire Kinemation components, spawn weapon, and complete initialization.
	 * MUST be called AFTER all SCS Blueprint components have had BeginPlay
	 * (i.e., from the owning actor's BeginPlay, after Super::BeginPlay()).
	 * This is critical — Kinemation BP components reset state in their BeginPlay,
	 * so wiring before that gets overwritten.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kinemation")
	void InitializeKinemation();

	// --- Weapon API ---

	/** Spawn and equip the weapon (if WeaponClass is set and no weapon is active).
	 *  Called by inventory system when player uses a weapon item. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	bool EquipWeapon();

	/** Equip a specific weapon class. Unequips current weapon first if one is active.
	 *  Applies per-weapon config (mag size, fire rate, damage) based on weapon type.
	 *  Accepts TSubclassOf<UObject> for Moonville compatibility (validates it's an Actor subclass).
	 *  Called by inventory system when player equips a weapon item with a specific class. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	bool EquipWeaponClass(TSubclassOf<UObject> NewWeaponClass);

	/** Destroy the active weapon, cancel pending fire/reload. Called on unequip. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void UnequipWeapon();

	/** Add ammo to reserve pool. Called by ammo pickup items. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Ammo")
	void AddReserveAmmo(int32 Amount);

	// --- ADS API ---

	/** Enter or exit aim-down-sights. Drives Kinemation aiming animation + FOV transition. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|ADS")
	void SetAiming(bool bAiming);

	/** Current ADS state. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|ADS")
	bool bIsAiming = false;

	// --- Weapon API ---

	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void FirePressed();

	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void FireReleased();

	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void Reload();

	/** Returns true if Kinemation animation components are active. */
	UFUNCTION(BlueprintPure, Category = "Kinemation")
	bool IsKinemationActive() const { return TacticalAnimComp != nullptr; }

protected:
	virtual void BeginPlay() override;

private:
	void InitKinemationCamera();
	void InitKinemationAnimation();
	void SpawnAndEquipWeapon();
	void PerformHitscan();
	void PerformMeleeSwing();
	void ThrowProjectile();

	/** True while reload animation is playing — blocks firing. */
	bool bIsReloading = false;
	FTimerHandle ReloadTimerHandle;

	/** True while fire animation is cycling — blocks next shot (semi-auto lockout). */
	bool bFireCooldown = false;
	FTimerHandle FireCooldownHandle;

	/** True while melee swing is on cooldown. */
	bool bMeleeCooldown = false;
	FTimerHandle MeleeCooldownHandle;

	/** Timer for melee swing ADS return (strike → ready). */
	FTimerHandle MeleeSwingReturnHandle;
	/** Timer for melee wind-up → strike transition. */
	FTimerHandle MeleeWindupHandle;
public:
	/** True while melee swing ADS animation is in progress — blocks manual ADS. */
	bool bMeleeSwingActive = false;
private:

	/** Timer for weapon switch drop phase (arms going off screen). */
	FTimerHandle WeaponSwitchAnimHandle;
	/** Timer for deferred weapon swap (spawn new weapon while arms off screen). */
	FTimerHandle WeaponSwapDeferredHandle;
	/** Timer for weapon switch rise phase (arms coming back on screen). */
	FTimerHandle WeaponSwitchRiseHandle;
	/** True while weapon switch animation is playing — blocks fire input. */
	bool bWeaponSwitching = false;
	/** Pending weapon class for deferred swap. */
	TSubclassOf<AActor> PendingSwapWeaponClass;
	/** Timer to re-enter ADS after melee weapon equip (ready stance). */
	FTimerHandle MeleeReadyStanceHandle;
	/** True while swap camera offset is applied — prevents accumulation on rapid swaps. */
	bool bSwapCameraOffset = false;

	/** Apply per-weapon stats (mag size, fire rate, damage) based on weapon class name. */
	void ApplyWeaponConfig(TSubclassOf<AActor> InWeaponClass);

public:
	/** Minimum time between shots in seconds. Set per-weapon by ApplyWeaponConfig. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float FireCooldownTime = 0.25f;
};
