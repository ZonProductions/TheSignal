// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_HealthComponent
 *
 * Purpose: Reusable health tracking component. Attach to any actor that can take damage.
 *          Creatures now, player later.
 *
 * Owner Subsystem: Core Gameplay
 *
 * Blueprint Extension Points:
 *   - OnHealthChanged: fires on every damage event with new health, max health, and damage dealt
 *   - OnDied: fires once when health reaches zero
 *   - MaxHealth: configurable per-instance in editor
 *   - ApplyInvincibility(Duration): grants temporary invulnerability
 *   - ApplyDamageReduction(Multiplier, Duration): scales incoming damage temporarily
 *
 * Dependencies: None — standalone component.
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, float, NewHealth, float, MaxHealth, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffectStateChanged, bool, bActive);

UCLASS(ClassGroup=(TheSignal), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_HealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_HealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "Health|Effects")
	bool bIsInvincible = false;

	UPROPERTY(BlueprintReadOnly, Category = "Health|Effects")
	float DamageMultiplier = 1.0f;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDied OnDied;

	UPROPERTY(BlueprintAssignable, Category = "Health|Effects")
	FOnEffectStateChanged OnInvincibilityChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health|Effects")
	FOnEffectStateChanged OnDamageReductionChanged;

	/** Called by owning actor's TakeDamage override. Clamps health, broadcasts events. */
	void ApplyDamage(float DamageAmount);

	/** Restore health by Amount, clamped to MaxHealth. Broadcasts OnHealthChanged.
	 *  Called by health item consume actions. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	/** Grants invulnerability for Duration seconds. Stacks reset the timer. */
	UFUNCTION(BlueprintCallable, Category = "Health|Effects")
	void ApplyInvincibility(float Duration);

	/** Resets health to max, clears death state, broadcasts OnHealthChanged. Used by in-place respawn. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	/** Scales incoming damage by Multiplier (0.5 = half damage) for Duration seconds. Stacks reset. */
	UFUNCTION(BlueprintCallable, Category = "Health|Effects")
	void ApplyDamageReduction(float Multiplier, float Duration);

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle InvincibilityTimerHandle;
	FTimerHandle DamageReductionTimerHandle;

	void ClearInvincibility();
	void ClearDamageReduction();
};
