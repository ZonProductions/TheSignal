// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_PatrolWaypoint
 *
 * Purpose: Invisible target actor for BPC_3D_Pathfinding.SetTargetLocation().
 *          Spawned and teleported by UZP_PatrolComponent to drive patrol behavior.
 *
 * Owner Subsystem: EnemyAI
 *
 * Blueprint Extension Points: None — purely internal.
 *
 * Dependencies: None
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_PatrolWaypoint.generated.h"

UCLASS(NotBlueprintable, NotPlaceable)
class THESIGNAL_API AZP_PatrolWaypoint : public AActor
{
	GENERATED_BODY()

public:
	AZP_PatrolWaypoint();
};
