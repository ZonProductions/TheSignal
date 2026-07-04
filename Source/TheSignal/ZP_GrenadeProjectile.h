// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_GrenadeProjectile
 *
 * Purpose: Throwable grenade projectile with fuse timer and two-tier radial damage.
 *          Spawned by UZP_KinemationComponent::ThrowProjectile().
 *          Bounces off surfaces, explodes after AZP_FuseTime seconds.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - AZP_GrenadeProjectileClass on UZP_KinemationComponent
 *   - AZP_InnerRadius/AZP_OuterRadius/AZP_InnerDamage/AZP_OuterDamage tunable in header
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
	float AZP_FuseTime = 5.0f;

	/** Inner radius — full damage zone (UU). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	float AZP_InnerRadius = 200.f;

	/** Outer radius — falloff damage zone (UU). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	float AZP_OuterRadius = 500.f;

	/** Damage at point blank (within AZP_InnerRadius). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	float AZP_InnerDamage = 100.f;

	/** Damage at outer edge of blast radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	float AZP_OuterDamage = 50.f;

	/** Niagara explosion effect. Loaded by path in constructor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	TObjectPtr<UNiagaraSystem> AZP_ExplosionFX;

	/** Explosion sound cue. Loaded by path in constructor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Explosion")
	TObjectPtr<USoundBase> AZP_ExplosionSound;

	/** Grenade launch speed — applied to ProjectileMovement InitialSpeed and MaxSpeed in the constructor; controls throw distance/arc feel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics")
	float AZP_GrenadeThrowSpeed = 800.f;

	/** How much velocity the grenade keeps on each surface bounce (ProjectileMovement->Bounciness). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics")
	float AZP_GrenadeBounciness = 0.3f;

	/** Surface friction applied to the grenade while sliding after bounces (ProjectileMovement->Friction). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics")
	float AZP_GrenadeFriction = 0.5f;

	/** Gravity multiplier giving the grenade a heavy lobbed arc instead of a rocket trajectory (ProjectileMovement->ProjectileGravityScale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics")
	float AZP_GrenadeGravityScale = 1.5f;

protected:
	virtual void BeginPlay() override;

private:
	void Explode();

	FTimerHandle FuseTimerHandle;
};
