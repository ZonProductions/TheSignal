// Copyright The Signal. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZP_WeaponTypes.generated.h"

/**
 * EZP_WeaponType
 *
 * Weapon archetype for routing fire input to the correct action
 * (hitscan, melee sweep, or projectile throw).
 */
UENUM(BlueprintType)
enum class EZP_WeaponType : uint8
{
	Ranged     UMETA(DisplayName = "Ranged"),
	Melee      UMETA(DisplayName = "Melee"),
	Throwable  UMETA(DisplayName = "Throwable"),
	None       UMETA(DisplayName = "None")
};
