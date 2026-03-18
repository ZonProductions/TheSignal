// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_HUDWidget
 *
 * Purpose: C++ base class for the main gameplay HUD. Drives health arc,
 *          ammo counter, interaction prompt, and crosshair via BindWidget.
 *          WBP_HUD (UMG Blueprint) handles visual layout; this class owns
 *          all data flow and runtime updates.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - HealthArcMaterial: set to M_HealthArc in WBP_HUD class defaults.
 *   - FullHealthColor / LowHealthColor: tunable in editor.
 *   - All public Set/Show/Hide functions are BlueprintCallable.
 *
 * Dependencies:
 *   - UMG (UUserWidget, UImage, UTextBlock)
 *   - M_HealthArc material (UI domain, radial arc shader)
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_WeaponTypes.h"
#include "ZP_HUDWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class AZP_GraceCharacter;

UCLASS()
class THESIGNAL_API UZP_HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- BindWidget (names MUST match UMG designer widget names) ---

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> HealthArc;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> StaminaArc;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> AmmoText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> InteractionPrompt;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Crosshair;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> DamageVignette;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> HealVignette;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> DamageReductionVignette;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> InvincibilityVignette;

	// --- Config ---

	/** Base material for the health arc. Set to M_HealthArc in WBP_HUD class defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health")
	TObjectPtr<UMaterialInterface> HealthArcMaterial;

	/** Color at full health. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health")
	FLinearColor FullHealthColor = FLinearColor(0.9f, 0.95f, 1.0f, 1.0f);

	/** Color at low health. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health")
	FLinearColor LowHealthColor = FLinearColor(0.8f, 0.1f, 0.1f, 1.0f);

	/** Health fraction below which color shifts toward LowHealthColor. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health")
	float LowHealthThreshold = 0.35f;

	/** How fast the damage vignette fades out (higher = faster). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage")
	float DamageVignetteFadeSpeed = 3.0f;

	/** Max opacity the damage vignette reaches on hit. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage")
	float DamageVignetteMaxOpacity = 0.8f;

	/** How fast the heal vignette fades out (higher = faster). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float HealVignetteFadeSpeed = 5.0f;

	/** Max opacity the heal vignette reaches on heal. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float HealVignetteMaxOpacity = 0.6f;

	/** How fast effect vignettes (damage reduction, invincibility) fade in/out. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float EffectVignetteFadeSpeed = 3.0f;

	/** Max opacity for damage reduction vignette. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float DamageReductionVignetteMaxOpacity = 0.4f;

	/** Max opacity for invincibility vignette. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float InvincibilityVignetteMaxOpacity = 0.4f;

	// --- API ---

	/** Update health arc fill and color. 0.0 = dead, 1.0 = full. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetHealth(float HealthPercent);

	/** Update stamina arc fill. 0.0 = empty, 1.0 = full. Green interior arc. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetStamina(float StaminaPercent);

	/** Update ammo display. Shows "Current / Reserve". */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetAmmo(int32 CurrentAmmo, int32 ReserveAmmo);

	/** Update HUD for current weapon type. Controls ammo text visibility/format. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetWeaponType(EZP_WeaponType WeaponType);

	/** Show interaction prompt with given text. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowInteractionPrompt(const FText& Text);

	/** Hide the interaction prompt. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideInteractionPrompt();

	/** Show or hide crosshair. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetCrosshairVisible(bool bVisible);

	/** Bind this HUD to a character's components (ammo, health delegates). */
	void BindToCharacter(AZP_GraceCharacter* Character);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<AZP_GraceCharacter> BoundCharacter;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HealthArcDMI;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> StaminaArcDMI;

	EZP_WeaponType CachedWeaponType = EZP_WeaponType::Ranged;

	float DamageVignetteOpacity = 0.f;

	float HealVignetteOpacity = 0.f;

	float DamageReductionVignetteOpacity = 0.f;
	float DamageReductionVignetteTarget = 0.f;

	float InvincibilityVignetteOpacity = 0.f;
	float InvincibilityVignetteTarget = 0.f;

	UFUNCTION()
	void OnAmmoChangedHandler(int32 CurrentAmmo, int32 ReserveAmmo);

	UFUNCTION()
	void OnWeaponTypeChangedHandler(EZP_WeaponType NewWeaponType);

	UFUNCTION()
	void OnHealthChangedHandler(float NewHealth, float MaxHealth, float DamageAmount);

	UFUNCTION()
	void OnInvincibilityChangedHandler(bool bActive);

	UFUNCTION()
	void OnDamageReductionChangedHandler(bool bActive);

	UFUNCTION()
	void OnStaminaChangedHandler(float NormalizedStamina);
};
