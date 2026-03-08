// Copyright The Signal. All Rights Reserved.

#include "ZP_HealthComponent.h"

UZP_HealthComponent::UZP_HealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_HealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

void UZP_HealthComponent::ApplyDamage(float DamageAmount)
{
	if (bIsDead || DamageAmount <= 0.f)
	{
		return;
	}

	if (bIsInvincible)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] HealthComponent on %s: %.0f damage blocked by invincibility"),
			*GetOwner()->GetName(), DamageAmount);
		return;
	}

	const float EffectiveDamage = DamageAmount * DamageMultiplier;
	CurrentHealth = FMath::Clamp(CurrentHealth - EffectiveDamage, 0.f, MaxHealth);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] HealthComponent on %s: took %.0f damage (%.0f raw * %.2f mult) — %.0f/%.0f HP remaining"),
		*GetOwner()->GetName(), EffectiveDamage, DamageAmount, DamageMultiplier, CurrentHealth, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, EffectiveDamage);

	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] HealthComponent on %s: DIED"), *GetOwner()->GetName());
		OnDied.Broadcast();
	}
}

void UZP_HealthComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.f)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
	const float ActualHeal = CurrentHealth - OldHealth;

	if (ActualHeal > 0.f)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] HealthComponent on %s: healed %.0f — %.0f/%.0f HP"),
			*GetOwner()->GetName(), ActualHeal, CurrentHealth, MaxHealth);

		// Broadcast with negative "damage" to indicate healing
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, -ActualHeal);
	}
}

void UZP_HealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] HealthComponent on %s: health reset to %.0f/%.0f"),
		*GetOwner()->GetName(), CurrentHealth, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0.f);
}

void UZP_HealthComponent::ApplyInvincibility(float Duration)
{
	bIsInvincible = true;

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] HealthComponent on %s: invincibility ON for %.1fs"),
		*GetOwner()->GetName(), Duration);

	OnInvincibilityChanged.Broadcast(true);

	GetWorld()->GetTimerManager().SetTimer(
		InvincibilityTimerHandle,
		this,
		&UZP_HealthComponent::ClearInvincibility,
		Duration,
		false
	);
}

void UZP_HealthComponent::ApplyDamageReduction(float Multiplier, float Duration)
{
	DamageMultiplier = FMath::Clamp(Multiplier, 0.f, 1.f);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] HealthComponent on %s: damage multiplier set to %.2f for %.1fs"),
		*GetOwner()->GetName(), DamageMultiplier, Duration);

	OnDamageReductionChanged.Broadcast(true);

	GetWorld()->GetTimerManager().SetTimer(
		DamageReductionTimerHandle,
		this,
		&UZP_HealthComponent::ClearDamageReduction,
		Duration,
		false
	);
}

void UZP_HealthComponent::ClearInvincibility()
{
	bIsInvincible = false;
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] HealthComponent on %s: invincibility OFF"),
		*GetOwner()->GetName());
	OnInvincibilityChanged.Broadcast(false);
}

void UZP_HealthComponent::ClearDamageReduction()
{
	DamageMultiplier = 1.0f;
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] HealthComponent on %s: damage multiplier reset to 1.0"),
		*GetOwner()->GetName());
	OnDamageReductionChanged.Broadcast(false);
}
