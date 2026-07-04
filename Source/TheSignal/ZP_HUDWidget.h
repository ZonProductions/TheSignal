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
 *   - AZP_HealthArcMaterial: set to M_HealthArc in WBP_HUD class defaults.
 *   - AZP_FullHealthColor / AZP_LowHealthColor: tunable in editor.
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
class UTexture2D;
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

	/** SignalSense waveform (the phone readout). Name MUST match the Image in WBP_HUD. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> SignalWave;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> AmmoText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> InteractionPrompt;

	// Optional: crosshair was removed (Grace is untrained). Keep the API +
	// guarded SetCrosshairVisible for future use; WBP_HUD needn't have the widget.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> Crosshair;

	/** Input glyph shown above the grab-struggle mash prompt (added to WBP_HUD 2026-07-02;
	 *  laid out at runtime in NativeConstruct relative to InteractionPrompt). */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> GrabPromptIcon;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> DamageVignette;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> HealVignette;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> DamageReductionVignette;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> InvincibilityVignette;

	// --- Weapon icons (designer-placed, one shown at a time) ---
	// Names MUST match the Image widgets in WBP_HUD.

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> Icon_Pistol;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> Icon_Rifle;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> Icon_Shotgun;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> Icon_Pipe;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> Icon_Grenade;

	// --- Weapon icon textures (assigned to the images by code at construct,
	//     so brushes never need hand-assignment in the designer). Set these in
	//     WBP_HUD class defaults (or via set_all_cdo.py). ---

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Weapon Icons")
	TObjectPtr<UTexture2D> AZP_PistolIconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Weapon Icons")
	TObjectPtr<UTexture2D> AZP_RifleIconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Weapon Icons")
	TObjectPtr<UTexture2D> AZP_ShotgunIconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Weapon Icons")
	TObjectPtr<UTexture2D> AZP_PipeIconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Weapon Icons")
	TObjectPtr<UTexture2D> AZP_GrenadeIconTexture;

	// --- Config ---

	/** Base material for the health arc. Set to M_HealthArc in WBP_HUD class defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health")
	TObjectPtr<UMaterialInterface> AZP_HealthArcMaterial;

	/** Base material for the SignalSense waveform. Set to M_SignalWaveform in WBP_HUD defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Signal")
	TObjectPtr<UMaterialInterface> AZP_SignalWaveMaterial;

	/** Color at full health. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health")
	FLinearColor AZP_FullHealthColor = FLinearColor(0.9f, 0.95f, 1.0f, 1.0f);

	/** Color at low health. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health")
	FLinearColor AZP_LowHealthColor = FLinearColor(0.8f, 0.1f, 0.1f, 1.0f);

	/** Health fraction below which color shifts toward AZP_LowHealthColor. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health")
	float AZP_LowHealthThreshold = 0.35f;

	/** Color of the stamina arc (green), set on the stamina arc material's ArcColor parameter at construct. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Stamina")
	FLinearColor AZP_StaminaArcColor = FLinearColor(0.2f, 0.9f, 0.3f, 1.0f);

	/** Render scale of the stamina arc so it nests inside the health arc ring (0.65 = 65%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Stamina")
	float AZP_StaminaArcScale = 0.65f;

	/** How fast the damage vignette fades out (higher = faster). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage")
	float AZP_DamageVignetteFadeSpeed = 3.0f;

	/** Max opacity the damage vignette reaches on hit. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Damage")
	float AZP_DamageVignetteMaxOpacity = 0.8f;

	/** Hardcoded LoadObject path for the damage vignette brush material; should become an asset-reference UPROPERTY. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Damage")
	TSoftObjectPtr<UMaterialInterface> AZP_DamageVignetteMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/Materials/UI/M_DamageVignette.M_DamageVignette")));

	/** How fast the heal vignette fades out (higher = faster). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float AZP_HealVignetteFadeSpeed = 5.0f;

	/** Max opacity the heal vignette reaches on heal. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float AZP_HealVignetteMaxOpacity = 0.6f;

	/** Hardcoded LoadObject path for the heal vignette brush material; should become an asset-reference UPROPERTY. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Effects")
	TSoftObjectPtr<UMaterialInterface> AZP_HealVignetteMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/Materials/UI/M_HealVignette.M_HealVignette")));

	/** How fast effect vignettes (damage reduction, invincibility) fade in/out. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float AZP_EffectVignetteFadeSpeed = 3.0f;

	/** Max opacity for damage reduction vignette. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float AZP_DamageReductionVignetteMaxOpacity = 0.4f;

	/** Hardcoded LoadObject path for the damage-reduction vignette brush material; should become an asset-reference UPROPERTY. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Effects")
	TSoftObjectPtr<UMaterialInterface> AZP_DamageReductionVignetteMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/Materials/UI/M_DamageReductionVignette.M_DamageReductionVignette")));

	/** Max opacity for invincibility vignette. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Effects")
	float AZP_InvincibilityVignetteMaxOpacity = 0.4f;

	/** Hardcoded LoadObject path for the invincibility vignette brush material; should become an asset-reference UPROPERTY. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Effects")
	TSoftObjectPtr<UMaterialInterface> AZP_InvincibilityVignetteMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/Materials/UI/M_InvincibilityVignette.M_InvincibilityVignette")));

	/** Attack-button glyph for keyboard/mouse (attack = LMB; Moonville KBM icon set). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Grab")
	TSoftObjectPtr<UTexture2D> AZP_GrabPromptGlyphTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Art/Textures/UI/Input/KeyboardMouse/T_IconMouse1.T_IconMouse1")));

	/** Attack-button glyph for gamepad (fire = Right Trigger; Moonville Xbox One icon set).
	 *  Selected automatically when the last input came from a controller. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Grab")
	TSoftObjectPtr<UTexture2D> AZP_GrabPromptGlyphGamepadTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Art/Textures/UI/Input/GamepadXboxOne/T_XB1_RT.T_XB1_RT")));

	/** On-screen size of the grab-prompt glyph, laid out at runtime because WBP_HUD has no authored slot for it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Grab")
	FVector2D AZP_GrabPromptIconSize = FVector2D(52.f, 52.f);

	/** Vertical gap between the grab-prompt glyph and the interaction prompt text it sits above. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Grab")
	float AZP_GrabPromptIconOffsetY = -8.f;

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

	/** Show the icon for the equipped weapon, collapse all the others. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetWeaponIcon(EZP_WeaponIcon WeaponIcon);

	/** Show interaction prompt with given text. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowInteractionPrompt(const FText& Text);

	/** Hide the interaction prompt. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideInteractionPrompt();

	/** Show or hide crosshair. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetCrosshairVisible(bool bVisible);

	/** Hold the damage vignette at (at least) this opacity — used while grabbed by an enemy.
	 *  Damage hits still pulse it to max on top; 0 releases the hold (normal fade-to-0). */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetDamageVignetteHold(float HoldOpacity);

	/** Show the grab-struggle prompt: attack-button glyph + text. bGamepad selects the
	 *  controller glyph (RT) over the KBM one (LMB). Safe to re-call to swap glyphs live. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowGrabPrompt(const FText& Text, bool bGamepad = false);

	/** Hide the grab-struggle prompt (glyph + text). */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideGrabPrompt();

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

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SignalWaveDMI;

	EZP_WeaponType CachedWeaponType = EZP_WeaponType::Ranged;

	// Last ammo values — lets SetWeaponType re-render the ammo line when the
	// weapon type changes. Equips broadcast ammo BEFORE type, so without this
	// the ammo is formatted with the *previous* weapon's type (melee hides it,
	// throwable mis-formats it, etc.).
	int32 LastCurrentAmmo = 0;
	int32 LastReserveAmmo = 0;

	float DamageVignetteOpacity = 0.f;

	/** Floor the damage vignette fades TOWARD instead of 0 (grab hold). */
	float DamageVignetteHoldOpacity = 0.f;

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
	void OnWeaponIconChangedHandler(EZP_WeaponIcon NewWeaponIcon);

	UFUNCTION()
	void OnHealthChangedHandler(float NewHealth, float MaxHealth, float DamageAmount);

	UFUNCTION()
	void OnInvincibilityChangedHandler(bool bActive);

	UFUNCTION()
	void OnDamageReductionChangedHandler(bool bActive);

	UFUNCTION()
	void OnStaminaChangedHandler(float NormalizedStamina);
};
