// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_GrenadeProjectile
 *
 * Purpose: Throwable grenade projectile with fuse timer and two-tier radial damage.
 *          Spawned by UZP_KinemationComponent::ThrowProjectile().
 *          Bounces off surfaces, explodes after FuseTime seconds.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - GrenadeProjectileClass on UZP_KinemationComponent
 *   - InnerRadius/OuterRadius/InnerDamage/OuterDamage tunable in header
 *
 * Dependencies:
 *   - UProjectileMovementComponent
 *   - UGameplayStatics::ApplyRadialDamageWithFalloff
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_GrenadeProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class USoundBase;
class USoundCue;

UCLASS()
class THESIGNAL_API AZP_GrenadeProjectile : public AActor
{
	GENERATED_BODY()

public:
	AZP_GrenadeProjectile();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<UStaticMeshComponent> GrenadeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// --- Config ---

	/** Seconds after spawn before detonation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	float FuseTime = 2.5f;

	/** Inner radius — full damage zone (UU). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	float InnerRadius = 200.f;

	/** Outer radius — falloff damage zone (UU). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	float OuterRadius = 500.f;

	/** Damage at point blank (within InnerRadius). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	float InnerDamage = 100.f;

	/** Damage at outer edge of blast radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	float OuterDamage = 50.f;

	/** Niagara explosion effect. Loaded by path in constructor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	TObjectPtr<UNiagaraSystem> ExplosionFX;

	/** Explosion sound cue. Loaded by path in constructor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	TObjectPtr<USoundBase> ExplosionSound;

protected:
	virtual void BeginPlay() override;

private:
	void Explode();

	FTimerHandle FuseTimerHandle;
};
