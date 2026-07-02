// Copyright The Signal. All Rights Reserved.

#include "ZP_FootstepData.h"

const FZP_FootstepSurface* UZP_FootstepData::Resolve(EPhysicalSurface InSurfaceType, const FString& MaterialNameLower) const
{
	// Pass 1: authored physical-material SurfaceType — exact and intentional, always wins.
	if (InSurfaceType != SurfaceType_Default)
	{
		for (const FZP_FootstepSurface& Row : Surfaces)
		{
			if (Row.SurfaceType == InSurfaceType && Row.Sounds.Num() > 0)
			{
				return &Row;
			}
		}
	}

	// Pass 2: material-name keywords — the zero-authoring path for purchased packs.
	for (const FZP_FootstepSurface& Row : Surfaces)
	{
		if (Row.Sounds.Num() == 0) { continue; } // placeholder slot — not ready yet
		for (const FString& Key : Row.MaterialNameKeywords)
		{
			const FString Trimmed = Key.TrimStartAndEnd().ToLower();
			if (!Trimmed.IsEmpty() && MaterialNameLower.Contains(Trimmed))
			{
				return &Row;
			}
		}
	}
	return nullptr;
}
