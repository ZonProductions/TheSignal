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

/**
 * EZP_WeaponIcon
 *
 * Specific equipped-weapon identity for HUD icon display. Finer-grained than
 * EZP_WeaponType (which can't tell Rifle from Shotgun — both Ranged). Set per
 * weapon in UZP_KinemationComponent::ApplyWeaponConfig.
 */
UENUM(BlueprintType)
enum class EZP_WeaponIcon : uint8
{
	None     UMETA(DisplayName = "None"),
	Pistol   UMETA(DisplayName = "Pistol"),
	Rifle    UMETA(DisplayName = "Rifle"),
	Shotgun  UMETA(DisplayName = "Shotgun"),
	Pipe     UMETA(DisplayName = "Pipe"),
	Grenade  UMETA(DisplayName = "Grenade")
};
