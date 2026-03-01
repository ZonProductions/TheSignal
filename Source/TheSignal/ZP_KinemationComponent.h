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
#include "ZP_KinemationComponent.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UMaterialInterface;

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

	// --- Weapon State ---

	/** The spawned weapon actor (if any). */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	TObjectPtr<AActor> ActiveWeapon;

	// --- Kinemation Component Refs (auto-detected or manually set) ---

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> TacticalCameraComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> TacticalAnimComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> RecoilAnimComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> IKMotionComp;

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

	/** True while reload animation is playing — blocks firing. */
	bool bIsReloading = false;
	FTimerHandle ReloadTimerHandle;

	/** True while fire animation is cycling — blocks next shot (semi-auto lockout). */
	bool bFireCooldown = false;
	FTimerHandle FireCooldownHandle;

	/** Minimum time between shots in seconds. Semi-auto pistol ~0.25s. */
	float FireCooldownTime = 0.25f;
};
