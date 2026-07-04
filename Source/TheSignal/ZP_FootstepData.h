// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_FootstepData
 *
 * Purpose: THE footstep surface table — one DataAsset (DA_Footsteps) the dev authors ONCE and
 *          extends forever with zero code/rebuilds. Each row = one surface (Metal, Tile, Wood, ...)
 *          with its sound variations, volume/pitch character, and TWO ways to match the floor:
 *            1. Physical-material SurfaceType (the proper engine system — takes precedence the
 *               moment floor assets get PhysMats authored);
 *            2. Material-NAME keywords (works TODAY on purchased packs with no PhysMats).
 *          The player's stepper resolves the floor under it per step; unmatched floors use
 *          AZP_DefaultSounds. Footsteps are core plumbing, not per-asset sound design.
 *
 * Owner Subsystem: Audio / PlayerCharacter
 *
 * Blueprint extension points: edit DA_Footsteps rows in the editor (add surfaces, drop sounds).
 * Dependencies: PhysicsCore (UPhysicalMaterial SurfaceType).
 */

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "ZP_FootstepData.generated.h"

class USoundBase;

USTRUCT(BlueprintType)
struct FZP_FootstepSurface
{
	GENERATED_BODY()

	/** Label only (readability in the asset editor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	FString Name;

	/** Match by authored physical material (preferred, exact). SurfaceType_Default = not used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType_Default;

	/** Match by floor material NAME (case-insensitive substrings) — the zero-authoring path for
	 *  purchased packs. First row whose keyword matches wins; order rows accordingly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	TArray<FString> MaterialNameKeywords;

	/** Step variations for this surface, one picked at random per step. Empty = row is a
	 *  placeholder slot (falls through to AZP_DefaultSounds until sounds are dropped in). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	TArray<TObjectPtr<USoundBase>> Sounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float VolumeMul = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float PitchMin = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float PitchMax = 1.08f;
};

UCLASS(BlueprintType)
class THESIGNAL_API UZP_FootstepData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Surface rows, matched top-down (SurfaceType first, then name keywords). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TArray<FZP_FootstepSurface> AZP_Surfaces;

	/** Fallback set for unmatched floors (and rows whose Sounds are still empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TArray<TObjectPtr<USoundBase>> AZP_DefaultSounds;

	/** Best row for a floor: authored SurfaceType match wins, else first keyword match on the
	 *  material name, else nullptr (caller uses AZP_DefaultSounds). Rows with EMPTY Sounds never win. */
	const FZP_FootstepSurface* Resolve(EPhysicalSurface InSurfaceType, const FString& MaterialNameLower) const;
};
