// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_PatrolComponent
 *
 * Purpose: Manages patrol waypoint cycling + chase state for creatures using
 *          Procedural Monsters v2's BPC_3D_Pathfinding system.
 *
 *          Spawns an invisible AZP_PatrolWaypoint actor and teleports it between
 *          PatrolPoints. Calls SetTargetLocation() on the pathfinding component
 *          directly via UE reflection (no BP wiring needed).
 *
 *          Auto-binds PawnSensingComponent.OnSeePawn for chase detection.
 *          Auto-calls InitializePatrol() from BeginPlay.
 *
 *          Movement is driven by the climbing system's CMC pipeline (FLYING mode).
 *          This component only manages TARGETING — it does NOT drive movement directly.
 *
 * Owner Subsystem: EnemyAI
 *
 * Blueprint Extension Points:
 *   - PatrolPoints array: set per-instance in editor
 *   - OnRequestSetTarget delegate: optional BP hook for additional behavior
 *   - StartChase / StopChase: callable from BP for custom triggers
 *   - bAutoInitialize: set false to defer InitializePatrol() call
 *
 * Dependencies:
 *   - AZP_PatrolWaypoint (spawned at runtime)
 *   - UPawnSensingComponent (auto-bound if present on owner)
 *   - BPC_3D_Pathfinding component (found by name on owner, SetTargetLocation via reflection)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_PatrolComponent.generated.h"

class AZP_PatrolWaypoint;
class UPawnSensingComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRequestSetTarget, AActor*, NewTarget);

UCLASS(ClassGroup=(TheSignal), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_PatrolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_PatrolComponent();

	// --- Configuration (set per-instance in editor) ---

	/** World-space patrol waypoints. Creature cycles through these in order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (MakeEditWidget = true))
	TArray<FVector> PatrolPoints;

	/** Distance threshold to consider "arrived" at a waypoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float ArrivalThreshold = 300.0f;

	/** How often (seconds) to check if creature arrived at waypoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float PatrolCheckInterval = 0.5f;

	/** If >= 0, overrides the creature's Monster Randomizer seed for deterministic appearance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	int32 PatrolSeed = -1;

	/** If true, InitializePatrol() is called automatically from BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bAutoInitialize = true;

	// --- State (read-only from BP) ---

	UPROPERTY(BlueprintReadOnly, Category = "Patrol|State")
	bool bIsChasing = false;

	UPROPERTY(BlueprintReadOnly, Category = "Patrol|State")
	int32 CurrentPatrolIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Patrol|State")
	TObjectPtr<AActor> PatrolTargetActor;

	// --- Delegates ---

	/** Fired when the patrol target changes. Optional BP hook for additional behavior. */
	UPROPERTY(BlueprintAssignable, Category = "Patrol|Events")
	FOnRequestSetTarget OnRequestSetTarget;

	// --- Public API ---

	/** Spawns waypoint actor, starts patrol timer, sets initial target. */
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void InitializePatrol();

	/** Switch to chasing ChaseTarget (usually the player). */
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void StartChase(AActor* ChaseTarget);

	/** Return to patrol behavior from current position. */
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void StopChase();

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle PatrolTimerHandle;

	/** Cached reference to the pathfinding component (found by name in BeginPlay). */
	UPROPERTY()
	TObjectPtr<UActorComponent> PathfindingComponent;

	/** Cached UFunction for SetTargetLocation on the pathfinding component. */
	UFunction* SetTargetLocationFunc = nullptr;

	/** Call SetTargetLocation on the pathfinding component via reflection. */
	void CallSetTargetLocation(AActor* TargetActor);

	/** Timer callback — checks distance to current waypoint, advances if arrived. */
	void CheckPatrolArrival();

	/** Teleport waypoint actor to PatrolPoints[CurrentPatrolIndex] and set target. */
	void MoveToNextWaypoint();

	/** PawnSensing callback — auto-bound in BeginPlay. */
	UFUNCTION()
	void OnSeePawn(APawn* SeenPawn);
};
