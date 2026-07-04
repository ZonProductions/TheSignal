// Copyright The Signal. All Rights Reserved.

#include "ZP_HUDWidget.h"
#include "ZP_GraceCharacter.h"
#include "ZP_KinemationComponent.h"
#include "ZP_HealthComponent.h"
#include "ZP_GraceGameplayComponent.h"
#include "ZP_SignalSenseComponent.h"
#include "ZP_EventBroadcaster.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include <initializer_list>

void UZP_HUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Create dynamic material instance for health arc
	if (AZP_HealthArcMaterial && HealthArc)
	{
		HealthArcDMI = UMaterialInstanceDynamic::Create(AZP_HealthArcMaterial, this);
		HealthArc->SetBrushFromMaterial(HealthArcDMI);
		SetHealth(1.0f);
	}

	// Create stamina arc DMI — same material, green color, scaled down to fit inside health arc
	if (AZP_HealthArcMaterial && StaminaArc)
	{
		StaminaArcDMI = UMaterialInstanceDynamic::Create(AZP_HealthArcMaterial, this);
		StaminaArc->SetBrushFromMaterial(StaminaArcDMI);
		StaminaArcDMI->SetVectorParameterValue(FName("ArcColor"), AZP_StaminaArcColor);
		// Scale down to 65% — fits inside the health arc ring
		StaminaArc->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		StaminaArc->SetRenderScale(FVector2D(AZP_StaminaArcScale, AZP_StaminaArcScale));
		SetStamina(1.0f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ZP_HUDWidget: AZP_HealthArcMaterial or HealthArc widget missing."));
	}

	// SignalSense waveform DMI (the phone readout). Amplitude is driven each tick.
	if (AZP_SignalWaveMaterial && SignalWave)
	{
		SignalWaveDMI = UMaterialInstanceDynamic::Create(AZP_SignalWaveMaterial, this);
		SignalWave->SetBrushFromMaterial(SignalWaveDMI);
	}

	// Grab prompt glyph — hidden until a grab; laid out at runtime just above the interaction
	// prompt (the widget was added to WBP_HUD by tooling with no authored slot layout).
	if (GrabPromptIcon)
	{
		GrabPromptIcon->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* IconSlot = Cast<UCanvasPanelSlot>(GrabPromptIcon->Slot))
		{
			if (InteractionPrompt)
			{
				if (UCanvasPanelSlot* PromptSlot = Cast<UCanvasPanelSlot>(InteractionPrompt->Slot))
				{
					IconSlot->SetAnchors(PromptSlot->GetAnchors());
					IconSlot->SetAlignment(FVector2D(0.5f, 1.f)); // bottom-center pivot
					const FVector2D P = PromptSlot->GetPosition();
					const FVector2D S = PromptSlot->GetSize();
					IconSlot->SetPosition(FVector2D(P.X + S.X * 0.5f, P.Y + AZP_GrabPromptIconOffsetY)); // centered above the text
					IconSlot->SetAutoSize(false);
					IconSlot->SetSize(AZP_GrabPromptIconSize);
				}
			}
		}
	}

	// Initialize vignettes — load materials by path, start hidden
	auto InitVignette = [](UImage* Widget, const TSoftObjectPtr<UMaterialInterface>& MatAsset)
	{
		if (!Widget) return;
		UMaterialInterface* Mat = MatAsset.LoadSynchronous();
		if (Mat)
		{
			Widget->SetBrushFromMaterial(Mat);
		}
		Widget->SetRenderOpacity(0.f);
	};

	InitVignette(DamageVignette, AZP_DamageVignetteMaterialAsset);
	InitVignette(HealVignette, AZP_HealVignetteMaterialAsset);
	InitVignette(DamageReductionVignette, AZP_DamageReductionVignetteMaterialAsset);
	InitVignette(InvincibilityVignette, AZP_InvincibilityVignetteMaterialAsset);

	// Resolve the weapon-icon images. BindWidgetOptional catches exact-name
	// matches; for any that didn't bind (designer named it differently, e.g.
	// "Icon_AssualtRifle" after the icon file), fall back to GetWidgetFromName
	// across known aliases. FName compare is case-insensitive.
	auto Resolve = [this](TObjectPtr<UImage>& OutSlot, std::initializer_list<const TCHAR*> Names)
	{
		if (OutSlot) { return; }
		for (const TCHAR* N : Names)
		{
			if (UImage* Img = Cast<UImage>(GetWidgetFromName(FName(N))))
			{
				OutSlot = Img;
				break;
			}
		}
	};
	Resolve(Icon_Pistol,  {TEXT("Icon_Pistol")});
	Resolve(Icon_Rifle,   {TEXT("Icon_Rifle"), TEXT("Icon_AssaultRifle"), TEXT("Icon_AssualtRifle"), TEXT("Icon_AR")});
	Resolve(Icon_Shotgun, {TEXT("Icon_Shotgun"), TEXT("Icon_PolicShotgun"), TEXT("Icon_PoliceShotgun")});
	Resolve(Icon_Pipe,    {TEXT("Icon_Pipe")});
	Resolve(Icon_Grenade, {TEXT("Icon_Gernade"), TEXT("Icon_Grenade"), TEXT("Icon_Explosive"), TEXT("Icon_ExplosiveGrenade"), TEXT("Icon_ExplosiveGernade")});

	// Assign each resolved icon's brush from its texture (set in WBP_HUD class
	// defaults via set_all_cdo.py), so brushes never need hand-assignment.
	// bMatchSize=true guarantees a non-zero brush size even if never set.
	auto ApplyIcon = [](UImage* Img, UTexture2D* Tex)
	{
		if (Img && Tex)
		{
			Img->SetBrushFromTexture(Tex, true);
		}
	};
	ApplyIcon(Icon_Pistol, AZP_PistolIconTexture);
	ApplyIcon(Icon_Rifle, AZP_RifleIconTexture);
	ApplyIcon(Icon_Shotgun, AZP_ShotgunIconTexture);
	ApplyIcon(Icon_Pipe, AZP_PipeIconTexture);
	ApplyIcon(Icon_Grenade, AZP_GrenadeIconTexture);

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

	// Drive the SignalSense waveform from the player's component: live amplitude plus the
	// rolling history texture + write head, so the scope scrolls chronologically.
	if (SignalWaveDMI && BoundCharacter && BoundCharacter->SignalSenseComp)
	{
		UZP_SignalSenseComponent* SS = BoundCharacter->SignalSenseComp;
		SignalWaveDMI->SetScalarParameterValue(FName("Amplitude"), SS->GetWaveformAmplitude());
		SignalWaveDMI->SetScalarParameterValue(FName("WritePhase"), SS->GetAmpWritePhase());
		if (UTexture2D* Hist = SS->GetAmpHistoryTexture())
		{
			SignalWaveDMI->SetTextureParameterValue(FName("AmpHistory"), Hist);
		}
	}

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

	// Damage vignette — fades to the hold floor (0 normally; raised while grabbed so the
	// vignette stays up through the struggle, with hits still pulsing it to max on top).
	FadeVignette(DamageVignette, DamageVignetteOpacity, DamageVignetteHoldOpacity, AZP_DamageVignetteFadeSpeed);

	// Heal vignette — always fades to 0 (one-shot flash)
	FadeVignette(HealVignette, HealVignetteOpacity, 0.f, AZP_HealVignetteFadeSpeed);

	// Damage reduction vignette — holds at target while active, fades to 0 when cleared
	FadeVignette(DamageReductionVignette, DamageReductionVignetteOpacity, DamageReductionVignetteTarget, AZP_EffectVignetteFadeSpeed);

	// Invincibility vignette — holds at target while active, fades to 0 when cleared
	FadeVignette(InvincibilityVignette, InvincibilityVignetteOpacity, InvincibilityVignetteTarget, AZP_EffectVignetteFadeSpeed);
}

void UZP_HUDWidget::SetDamageVignetteHold(float HoldOpacity)
{
	DamageVignetteHoldOpacity = FMath::Clamp(HoldOpacity, 0.f, 1.f);
	// Raising the hold above the current opacity snaps up immediately — the grab just landed.
	if (DamageVignette && DamageVignetteOpacity < DamageVignetteHoldOpacity)
	{
		DamageVignetteOpacity = DamageVignetteHoldOpacity;
		DamageVignette->SetRenderOpacity(DamageVignetteOpacity);
	}
}

void UZP_HUDWidget::ShowGrabPrompt(const FText& Text, bool bGamepad)
{
	ShowInteractionPrompt(Text);
	if (GrabPromptIcon)
	{
		UTexture2D* Glyph = bGamepad ? AZP_GrabPromptGlyphGamepadTexture.LoadSynchronous()
		                             : AZP_GrabPromptGlyphTexture.LoadSynchronous();
		if (Glyph)
		{
			GrabPromptIcon->SetBrushFromTexture(Glyph, /*bMatchSize*/false);
		}
		GrabPromptIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UZP_HUDWidget::HideGrabPrompt()
{
	HideInteractionPrompt();
	if (GrabPromptIcon)
	{
		GrabPromptIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UZP_HUDWidget::SetHealth(float HealthPercent)
{
	HealthPercent = FMath::Clamp(HealthPercent, 0.0f, 1.0f);

	if (HealthArcDMI)
	{
		HealthArcDMI->SetScalarParameterValue(FName("HealthPercent"), HealthPercent);

		// Color shifts toward AZP_LowHealthColor below threshold
		float ColorAlpha = 1.0f;
		if (HealthPercent < AZP_LowHealthThreshold && AZP_LowHealthThreshold > 0.0f)
		{
			ColorAlpha = HealthPercent / AZP_LowHealthThreshold;
		}
		FLinearColor ArcColor = FMath::Lerp(AZP_LowHealthColor, AZP_FullHealthColor, ColorAlpha);
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
	LastCurrentAmmo = CurrentAmmo;
	LastReserveAmmo = ReserveAmmo;

	if (!AmmoText)
	{
		return;
	}

	switch (CachedWeaponType)
	{
	case EZP_WeaponType::None:
		// Unarmed — no weapon up, so no ammo line at all (same as melee/pipe).
		// Without this case, None fell through to the Ranged default and printed
		// the pistol demo defaults (12 / 48).
		AmmoText->SetVisibility(ESlateVisibility::Collapsed);
		break;

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
	// Re-render the ammo line for the new type — equips broadcast ammo BEFORE
	// type, so the first SetAmmo used the previous weapon's format. Fixes:
	// pipe->other hiding ammo, pipe showing ammo, grenade showing "1/0".
	SetAmmo(LastCurrentAmmo, LastReserveAmmo);
}

void UZP_HUDWidget::SetWeaponIcon(EZP_WeaponIcon WeaponIcon)
{
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] HUD SetWeaponIcon icon=%d | bound Pistol=%d Rifle=%d Shotgun=%d Pipe=%d Grenade=%d"),
		(int32)WeaponIcon, Icon_Pistol != nullptr, Icon_Rifle != nullptr,
		Icon_Shotgun != nullptr, Icon_Pipe != nullptr, Icon_Grenade != nullptr);

	// Reveal the matching designer-placed icon, collapse the rest. Each is
	// optional — a HUD variant may not have placed every icon.
	auto SetVis = [](UImage* Img, bool bShow)
	{
		if (Img)
		{
			Img->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	};
	SetVis(Icon_Pistol,  WeaponIcon == EZP_WeaponIcon::Pistol);
	SetVis(Icon_Rifle,   WeaponIcon == EZP_WeaponIcon::Rifle);
	SetVis(Icon_Shotgun, WeaponIcon == EZP_WeaponIcon::Shotgun);
	SetVis(Icon_Pipe,    WeaponIcon == EZP_WeaponIcon::Pipe);
	SetVis(Icon_Grenade, WeaponIcon == EZP_WeaponIcon::Grenade);
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
		Character->KinemationComp->OnWeaponIconChanged.AddDynamic(this, &UZP_HUDWidget::OnWeaponIconChangedHandler);
		SetWeaponIcon(Character->KinemationComp->CurrentWeaponIcon);
		SetAmmo(Character->KinemationComp->CurrentAmmo, Character->KinemationComp->ReserveAmmo);
	}

	// Bind health updates
	if (Character->HealthComp)
	{
		Character->HealthComp->OnHealthChanged.AddDynamic(this, &UZP_HUDWidget::OnHealthChangedHandler);
		Character->HealthComp->OnInvincibilityChanged.AddDynamic(this, &UZP_HUDWidget::OnInvincibilityChangedHandler);
		Character->HealthComp->OnDamageReductionChanged.AddDynamic(this, &UZP_HUDWidget::OnDamageReductionChangedHandler);
		if (Character->HealthComp->AZP_MaxHealth > 0.f)
		{
			SetHealth(Character->HealthComp->CurrentHealth / Character->HealthComp->AZP_MaxHealth);
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
		DamageVignetteOpacity = AZP_DamageVignetteMaxOpacity;
		DamageVignette->SetRenderOpacity(DamageVignetteOpacity);
	}

	// Flash heal vignette on heal (negative DamageAmount = healing)
	if (DamageAmount < 0.f && HealVignette)
	{
		HealVignetteOpacity = AZP_HealVignetteMaxOpacity;
		HealVignette->SetRenderOpacity(HealVignetteOpacity);
	}
}

void UZP_HUDWidget::OnInvincibilityChangedHandler(bool bActive)
{
	if (bActive)
	{
		InvincibilityVignetteTarget = AZP_InvincibilityVignetteMaxOpacity;
		InvincibilityVignetteOpacity = AZP_InvincibilityVignetteMaxOpacity;
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
		DamageReductionVignetteTarget = AZP_DamageReductionVignetteMaxOpacity;
		DamageReductionVignetteOpacity = AZP_DamageReductionVignetteMaxOpacity;
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

void UZP_HUDWidget::OnWeaponIconChangedHandler(EZP_WeaponIcon NewWeaponIcon)
{
	SetWeaponIcon(NewWeaponIcon);
}
