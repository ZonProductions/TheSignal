// Copyright The Signal. All Rights Reserved.

#include "ZP_HUDWidget.h"
#include "ZP_GraceCharacter.h"
#include "ZP_KinemationComponent.h"
#include "ZP_HealthComponent.h"
#include "ZP_GraceGameplayComponent.h"
#include "ZP_EventBroadcaster.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

void UZP_HUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Create dynamic material instance for health arc
	if (HealthArcMaterial && HealthArc)
	{
		HealthArcDMI = UMaterialInstanceDynamic::Create(HealthArcMaterial, this);
		HealthArc->SetBrushFromMaterial(HealthArcDMI);
		SetHealth(1.0f);
	}

	// Create stamina arc DMI — same material, green color, scaled down to fit inside health arc
	if (HealthArcMaterial && StaminaArc)
	{
		StaminaArcDMI = UMaterialInstanceDynamic::Create(HealthArcMaterial, this);
		StaminaArc->SetBrushFromMaterial(StaminaArcDMI);
		StaminaArcDMI->SetVectorParameterValue(FName("ArcColor"), FLinearColor(0.2f, 0.9f, 0.3f, 1.0f));
		// Scale down to 65% — fits inside the health arc ring
		StaminaArc->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		StaminaArc->SetRenderScale(FVector2D(0.65f, 0.65f));
		SetStamina(1.0f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ZP_HUDWidget: HealthArcMaterial or HealthArc widget missing."));
	}

	// Initialize vignettes — load materials by path, start hidden
	auto InitVignette = [](UImage* Widget, const TCHAR* MatPath)
	{
		if (!Widget) return;
		UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, MatPath);
		if (Mat)
		{
			Widget->SetBrushFromMaterial(Mat);
		}
		Widget->SetRenderOpacity(0.f);
	};

	InitVignette(DamageVignette, TEXT("/Game/Materials/UI/M_DamageVignette"));
	InitVignette(HealVignette, TEXT("/Game/Materials/UI/M_HealVignette"));
	InitVignette(DamageReductionVignette, TEXT("/Game/Materials/UI/M_DamageReductionVignette"));
	InitVignette(InvincibilityVignette, TEXT("/Game/Materials/UI/M_InvincibilityVignette"));

	// Hide interaction prompt by default
	HideInteractionPrompt();

	// Default ammo
	SetAmmo(0, 0);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_HUDWidget: Constructed — Arc:%s Ammo:%s Prompt:%s Crosshair:%s"),
		HealthArc ? TEXT("OK") : TEXT("MISSING"),
		AmmoText ? TEXT("OK") : TEXT("MISSING"),
		InteractionPrompt ? TEXT("OK") : TEXT("MISSING"),
		Crosshair ? TEXT("OK") : TEXT("MISSING"));
}

void UZP_HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Helper lambda for fading vignettes
	auto FadeVignette = [InDeltaTime](UImage* Widget, float& Opacity, float Target, float Speed)
	{
		if (!Widget) return;
		if (FMath::IsNearlyEqual(Opacity, Target, 0.005f))
		{
			if (!FMath::IsNearlyZero(Target - Opacity))
			{
				Opacity = Target;
				Widget->SetRenderOpacity(Opacity);
			}
			return;
		}
		Opacity = FMath::FInterpTo(Opacity, Target, InDeltaTime, Speed);
		if (Opacity < 0.01f && Target <= 0.f)
		{
			Opacity = 0.f;
		}
		Widget->SetRenderOpacity(Opacity);
	};

	// Damage vignette — always fades to 0 (one-shot flash)
	FadeVignette(DamageVignette, DamageVignetteOpacity, 0.f, DamageVignetteFadeSpeed);

	// Heal vignette — always fades to 0 (one-shot flash)
	FadeVignette(HealVignette, HealVignetteOpacity, 0.f, HealVignetteFadeSpeed);

	// Damage reduction vignette — holds at target while active, fades to 0 when cleared
	FadeVignette(DamageReductionVignette, DamageReductionVignetteOpacity, DamageReductionVignetteTarget, EffectVignetteFadeSpeed);

	// Invincibility vignette — holds at target while active, fades to 0 when cleared
	FadeVignette(InvincibilityVignette, InvincibilityVignetteOpacity, InvincibilityVignetteTarget, EffectVignetteFadeSpeed);
}

void UZP_HUDWidget::SetHealth(float HealthPercent)
{
	HealthPercent = FMath::Clamp(HealthPercent, 0.0f, 1.0f);

	if (HealthArcDMI)
	{
		HealthArcDMI->SetScalarParameterValue(FName("HealthPercent"), HealthPercent);

		// Color shifts toward LowHealthColor below threshold
		float ColorAlpha = 1.0f;
		if (HealthPercent < LowHealthThreshold && LowHealthThreshold > 0.0f)
		{
			ColorAlpha = HealthPercent / LowHealthThreshold;
		}
		FLinearColor ArcColor = FMath::Lerp(LowHealthColor, FullHealthColor, ColorAlpha);
		HealthArcDMI->SetVectorParameterValue(FName("ArcColor"), ArcColor);
	}
}

void UZP_HUDWidget::SetStamina(float StaminaPercent)
{
	StaminaPercent = FMath::Clamp(StaminaPercent, 0.0f, 1.0f);

	if (StaminaArcDMI)
	{
		StaminaArcDMI->SetScalarParameterValue(FName("HealthPercent"), StaminaPercent);
	}
}

void UZP_HUDWidget::SetAmmo(int32 CurrentAmmo, int32 ReserveAmmo)
{
	if (!AmmoText)
	{
		return;
	}

	switch (CachedWeaponType)
	{
	case EZP_WeaponType::Melee:
		// No ammo display for melee
		AmmoText->SetVisibility(ESlateVisibility::Collapsed);
		break;

	case EZP_WeaponType::Throwable:
		// Count only (e.g. "3")
		AmmoText->SetVisibility(ESlateVisibility::HitTestInvisible);
		AmmoText->SetText(FText::FromString(
			FString::Printf(TEXT("%d"), CurrentAmmo)));
		break;

	case EZP_WeaponType::Ranged:
	default:
		// Standard "Current / Reserve"
		AmmoText->SetVisibility(ESlateVisibility::HitTestInvisible);
		AmmoText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), CurrentAmmo, ReserveAmmo)));
		break;
	}
}

void UZP_HUDWidget::SetWeaponType(EZP_WeaponType WeaponType)
{
	CachedWeaponType = WeaponType;
}

void UZP_HUDWidget::ShowInteractionPrompt(const FText& Text)
{
	if (InteractionPrompt)
	{
		InteractionPrompt->SetText(Text);
		InteractionPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UZP_HUDWidget::HideInteractionPrompt()
{
	if (InteractionPrompt)
	{
		InteractionPrompt->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UZP_HUDWidget::SetCrosshairVisible(bool bVisible)
{
	if (Crosshair)
	{
		Crosshair->SetVisibility(bVisible
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UZP_HUDWidget::NativeDestruct()
{
	// Unbind all delegates to prevent crash when widget is destroyed before character
	if (BoundCharacter)
	{
		if (BoundCharacter->KinemationComp)
		{
			BoundCharacter->KinemationComp->OnAmmoChanged.RemoveDynamic(this, &UZP_HUDWidget::OnAmmoChangedHandler);
			BoundCharacter->KinemationComp->OnWeaponTypeChanged.RemoveDynamic(this, &UZP_HUDWidget::OnWeaponTypeChangedHandler);
		}
		if (BoundCharacter->HealthComp)
		{
			BoundCharacter->HealthComp->OnHealthChanged.RemoveDynamic(this, &UZP_HUDWidget::OnHealthChangedHandler);
			BoundCharacter->HealthComp->OnInvincibilityChanged.RemoveDynamic(this, &UZP_HUDWidget::OnInvincibilityChangedHandler);
			BoundCharacter->HealthComp->OnDamageReductionChanged.RemoveDynamic(this, &UZP_HUDWidget::OnDamageReductionChangedHandler);
		}
		if (BoundCharacter->GameplayComp && BoundCharacter->GameplayComp->GetEventBroadcaster())
		{
			BoundCharacter->GameplayComp->GetEventBroadcaster()->OnStaminaChanged.RemoveDynamic(this, &UZP_HUDWidget::OnStaminaChangedHandler);
		}
		BoundCharacter = nullptr;
	}

	Super::NativeDestruct();
}

void UZP_HUDWidget::BindToCharacter(AZP_GraceCharacter* Character)
{
	if (!Character) return;

	BoundCharacter = Character;

	// Bind ammo and weapon type updates
	if (Character->KinemationComp)
	{
		Character->KinemationComp->OnAmmoChanged.AddDynamic(this, &UZP_HUDWidget::OnAmmoChangedHandler);
		Character->KinemationComp->OnWeaponTypeChanged.AddDynamic(this, &UZP_HUDWidget::OnWeaponTypeChangedHandler);
		SetWeaponType(Character->KinemationComp->CurrentWeaponType);
		SetAmmo(Character->KinemationComp->CurrentAmmo, Character->KinemationComp->ReserveAmmo);
	}

	// Bind health updates
	if (Character->HealthComp)
	{
		Character->HealthComp->OnHealthChanged.AddDynamic(this, &UZP_HUDWidget::OnHealthChangedHandler);
		Character->HealthComp->OnInvincibilityChanged.AddDynamic(this, &UZP_HUDWidget::OnInvincibilityChangedHandler);
		Character->HealthComp->OnDamageReductionChanged.AddDynamic(this, &UZP_HUDWidget::OnDamageReductionChangedHandler);
		if (Character->HealthComp->MaxHealth > 0.f)
		{
			SetHealth(Character->HealthComp->CurrentHealth / Character->HealthComp->MaxHealth);
		}
	}

	// Bind stamina updates via EventBroadcaster
	if (Character->GameplayComp && Character->GameplayComp->GetEventBroadcaster())
	{
		Character->GameplayComp->GetEventBroadcaster()->OnStaminaChanged.AddDynamic(this, &UZP_HUDWidget::OnStaminaChangedHandler);
		SetStamina(1.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_HUDWidget: Bound to %s"), *Character->GetName());
}

void UZP_HUDWidget::OnAmmoChangedHandler(int32 CurrentAmmo, int32 ReserveAmmo)
{
	SetAmmo(CurrentAmmo, ReserveAmmo);
}

void UZP_HUDWidget::OnHealthChangedHandler(float NewHealth, float MaxHealth, float DamageAmount)
{
	if (MaxHealth > 0.f)
	{
		SetHealth(NewHealth / MaxHealth);
	}

	// Flash damage vignette on hit
	if (DamageAmount > 0.f && DamageVignette)
	{
		DamageVignetteOpacity = DamageVignetteMaxOpacity;
		DamageVignette->SetRenderOpacity(DamageVignetteOpacity);
	}

	// Flash heal vignette on heal (negative DamageAmount = healing)
	if (DamageAmount < 0.f && HealVignette)
	{
		HealVignetteOpacity = HealVignetteMaxOpacity;
		HealVignette->SetRenderOpacity(HealVignetteOpacity);
	}
}

void UZP_HUDWidget::OnInvincibilityChangedHandler(bool bActive)
{
	if (bActive)
	{
		InvincibilityVignetteTarget = InvincibilityVignetteMaxOpacity;
		InvincibilityVignetteOpacity = InvincibilityVignetteMaxOpacity;
		if (InvincibilityVignette)
		{
			InvincibilityVignette->SetRenderOpacity(InvincibilityVignetteOpacity);
		}
	}
	else
	{
		InvincibilityVignetteTarget = 0.f;
	}
}

void UZP_HUDWidget::OnDamageReductionChangedHandler(bool bActive)
{
	if (bActive)
	{
		DamageReductionVignetteTarget = DamageReductionVignetteMaxOpacity;
		DamageReductionVignetteOpacity = DamageReductionVignetteMaxOpacity;
		if (DamageReductionVignette)
		{
			DamageReductionVignette->SetRenderOpacity(DamageReductionVignetteOpacity);
		}
	}
	else
	{
		DamageReductionVignetteTarget = 0.f;
	}
}

void UZP_HUDWidget::OnStaminaChangedHandler(float NormalizedStamina)
{
	SetStamina(NormalizedStamina);
}

void UZP_HUDWidget::OnWeaponTypeChangedHandler(EZP_WeaponType NewWeaponType)
{
	SetWeaponType(NewWeaponType);
}
