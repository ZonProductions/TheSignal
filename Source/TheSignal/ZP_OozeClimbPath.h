// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_OozeClimbPath
 *
 * Purpose: The Oozeling's patrol route — a placeable, distinctly-named alias of
 *          AZP_ScytheerClimbPath (spline + per-point wall normals). Everything is inherited:
 *          shape the spline with "Add Points To End", then click "Snap Wall Normals To
 *          Geometry" once so each point's normal faces OUT of the surface it rests on.
 *          Assign the placed actor to a BP_Oozeling's AZP_PatrolPath.
 *
 *          Exists as its own class so ooze routes are identifiable in the outliner / Place
 *          Actors panel and can grow ooze-specific authoring later without touching the
 *          Scytheer's paths.
 *
 * Owner Subsystem: EnemyAI / Oozeling
 *
 * Blueprint Extension Points: none — pure C++.
 *
 * Dependencies: AZP_ScytheerClimbPath (all behavior).
 */

#include "CoreMinimal.h"
#include "ZP_ScytheerClimbPath.h"
#include "ZP_OozeClimbPath.generated.h"

UCLASS()
class THESIGNAL_API AZP_OozeClimbPath : public AZP_ScytheerClimbPath
{
	GENERATED_BODY()

public:
	AZP_OozeClimbPath();
};
