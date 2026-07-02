// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_MeleeDamageType
 *
 * Purpose: Marker damage type for the player's MELEE (pipe) hits. Enemies that judge head-vs-body by
 *          hit height (Shambler, Scytheer) check for this so a blunt melee strike is NEVER counted as
 *          a headshot — it deals its own flat damage instead. Also lets future systems react to melee
 *          vs gunfire differently (impact FX, aggro weighting) without a per-enemy hack.
 *
 * Owner Subsystem: Combat.
 *
 * Usage: pass UZP_MeleeDamageType::StaticClass() as the DamageTypeClass of the melee ApplyPointDamage.
 */

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "ZP_MeleeDamageType.generated.h"

UCLASS()
class THESIGNAL_API UZP_MeleeDamageType : public UDamageType
{
	GENERATED_BODY()
};
