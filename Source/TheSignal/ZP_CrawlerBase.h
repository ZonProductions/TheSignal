// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_CrawlerBase
 *
 * Purpose: Base Character class for all creatures. Swaps in UZP_CrawlerMovementComponent
 *          as the default CMC via ObjectInitializer. All creature Blueprints inherit from
 *          this (via BP_ClimbingSystem_Character reparenting) and get the custom CMC
 *          automatically.
 *
 *          Owns a UZP_HealthComponent for damage tracking. Overrides TakeDamage to forward
 *          to health component. On death: ragdoll → freeze → cleanup.
 *
 * Owner Subsystem: EnemyAI
 *
 * Blueprint Extension Points:
 *   - HealthComp: adjust AZP_MaxHealth per creature instance
 *   - OnDied delegate on HealthComp: hook additional death VFX/SFX
 *
 * Dependencies:
 *   - UZP_CrawlerMovementComponent (custom CMC subclass)
 *   - UZP_HealthComponent (damage tracking)
 *   - UZP_CrawlerBehaviorComponent (disabled on death)
 */

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZP_CrawlerBase.generated.h"

class UZP_HealthComponent;
class UZP_CrawlerBehaviorComponent;
class UMaterialInterface;

UCLASS()
class THESIGNAL_API AZP_CrawlerBase : public ACharacter
{
	GENERATED_BODY()

public:
	AZP_CrawlerBase(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UZP_HealthComponent> HealthComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Behavior")
	TObjectPtr<UZP_CrawlerBehaviorComponent> BehaviorComp;

	// ── Creature Configuration ───────────────────────────────────────
	// Override these per variant BP to control appearance.
	// Leg count and scale feed into Monster Randomizer (wired in BP_Monster_Pawn).
	// AZP_TentacleMaterial is applied post-spawn via timer.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config")
	int32 AZP_CreatureSeed = 125;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config")
	int32 AZP_MinLegCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config")
	int32 AZP_MaxLegCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config")
	float AZP_MinGeneralScale = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config")
	float AZP_MaxGeneralScale = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config")
	float AZP_MinLegScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config")
	float AZP_MaxLegScale = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config")
	float AZP_CreatureSpeedMultiplier = 0.75f;

	/** Material override for tentacles. If set, replaces MI_Flesh on all legs after spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config")
	TObjectPtr<UMaterialInterface> AZP_TentacleMaterial;

	/** Seconds after BeginPlay before TentacleMaterial is applied; must exceed BP_Monster_Pawn's 3s Monster Randomizer delay so legs exist when the swap runs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config|Material Swap")
	float AZP_TentacleMaterialSwapDelay = 4.5f;

	/** Runtime roughness override to reduce SSR floor reflection bleed on glossy materials.
	 *  0 = use material's original roughness unchanged. >0 = override roughness on a Dynamic Material Instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AZP_RuntimeRoughnessOverride = 0.12f;

	/** Seconds the frozen corpse and its orphaned leg/body/eye actors persist before destruction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float AZP_CorpseLifeSpan = 30.0f;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	/** Toggle BPC_Climbing's Enabled property. Controls wall/ceiling climbing. */
	void SetClimbingEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnDied();

	/** Apply AZP_TentacleMaterial to all leg skeletal meshes + body. Called via timer after Monster Randomizer completes. */
	void ApplyTentacleMaterial();
	FTimerHandle MaterialSwapTimerHandle;
};
