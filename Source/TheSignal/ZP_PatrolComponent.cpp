// Copyright The Signal. All Rights Reserved.

#include "ZP_PatrolComponent.h"
#include "ZP_PatrolWaypoint.h"
#include "Perception/PawnSensingComponent.h"
#include "TimerManager.h"

UZP_PatrolComponent::UZP_PatrolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_PatrolComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Find BPC_3D_Pathfinding component by SCS name and cache SetTargetLocation function
	for (UActorComponent* Comp : Owner->GetComponents())
	{
		if (Comp && Comp->GetFName().ToString() == TEXT("BPI_3D_Pathfinding"))
		{
			PathfindingComponent = Comp;
			SetTargetLocationFunc = Comp->GetClass()->FindFunctionByName(TEXT("SetTargetLocation"));

			if (SetTargetLocationFunc)
			{
				UE_LOG(LogTemp, Log, TEXT("[ZP_Patrol] %s: Found pathfinding component, SetTargetLocation cached."),
					*Owner->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[ZP_Patrol] %s: Found pathfinding component but SetTargetLocation function not found!"),
					*Owner->GetName());
			}

			break;
		}
	}

	if (!PathfindingComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP_Patrol] %s: BPI_3D_Pathfinding component not found on owner."),
			*Owner->GetName());
	}

	// Auto-bind to PawnSensingComponent if present
	if (UPawnSensingComponent* Sensing = Owner->FindComponentByClass<UPawnSensingComponent>())
	{
		Sensing->OnSeePawn.AddDynamic(this, &UZP_PatrolComponent::OnSeePawn);
		UE_LOG(LogTemp, Log, TEXT("[ZP_Patrol] %s: Auto-bound to PawnSensingComponent."),
			*Owner->GetName());
	}

	// Auto-initialize patrol if configured
	if (bAutoInitialize)
	{
		InitializePatrol();
	}
}

void UZP_PatrolComponent::CallSetTargetLocation(AActor* TargetActor)
{
	if (PathfindingComponent && SetTargetLocationFunc)
	{
		struct
		{
			AActor* TargetActor;
		} Params;
		Params.TargetActor = TargetActor;

		PathfindingComponent->ProcessEvent(SetTargetLocationFunc, &Params);
	}

	// Also broadcast delegate for any BP hooks
	OnRequestSetTarget.Broadcast(TargetActor);
}

void UZP_PatrolComponent::OnSeePawn(APawn* SeenPawn)
{
	if (SeenPawn)
	{
		StartChase(SeenPawn);
	}
}

void UZP_PatrolComponent::InitializePatrol()
{
	if (PatrolPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP_Patrol] %s: No PatrolPoints set — patrol disabled."),
			*GetOwner()->GetName());
		return;
	}

	// Spawn invisible waypoint actor at first patrol point
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PatrolTargetActor = GetWorld()->SpawnActor<AZP_PatrolWaypoint>(
		AZP_PatrolWaypoint::StaticClass(),
		PatrolPoints[0],
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!PatrolTargetActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[ZP_Patrol] %s: Failed to spawn waypoint actor!"),
			*GetOwner()->GetName());
		return;
	}

	CurrentPatrolIndex = 0;
	bIsChasing = false;

	// Set initial target on pathfinding component
	CallSetTargetLocation(PatrolTargetActor);

	// Start periodic arrival check
	GetWorld()->GetTimerManager().SetTimer(
		PatrolTimerHandle,
		this,
		&UZP_PatrolComponent::CheckPatrolArrival,
		PatrolCheckInterval,
		true // looping
	);

	UE_LOG(LogTemp, Log, TEXT("[ZP_Patrol] %s: Patrol initialized with %d waypoints."),
		*GetOwner()->GetName(), PatrolPoints.Num());
}

void UZP_PatrolComponent::StartChase(AActor* ChaseTarget)
{
	if (!ChaseTarget)
	{
		return;
	}

	bIsChasing = true;
	CallSetTargetLocation(ChaseTarget);

	UE_LOG(LogTemp, Log, TEXT("[ZP_Patrol] %s: Chasing %s"),
		*GetOwner()->GetName(), *ChaseTarget->GetName());
}

void UZP_PatrolComponent::StopChase()
{
	bIsChasing = false;

	if (PatrolTargetActor && PatrolPoints.IsValidIndex(CurrentPatrolIndex))
	{
		PatrolTargetActor->SetActorLocation(PatrolPoints[CurrentPatrolIndex]);
		CallSetTargetLocation(PatrolTargetActor);

		UE_LOG(LogTemp, Log, TEXT("[ZP_Patrol] %s: Returning to patrol (waypoint %d)."),
			*GetOwner()->GetName(), CurrentPatrolIndex);
	}
}

void UZP_PatrolComponent::CheckPatrolArrival()
{
	if (bIsChasing || !PatrolTargetActor || PatrolPoints.Num() == 0)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const float Distance = FVector::Dist(Owner->GetActorLocation(), PatrolPoints[CurrentPatrolIndex]);

	if (Distance <= ArrivalThreshold)
	{
		// Advance to next waypoint (wrap around)
		CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
		MoveToNextWaypoint();
	}
}

void UZP_PatrolComponent::MoveToNextWaypoint()
{
	if (PatrolTargetActor && PatrolPoints.IsValidIndex(CurrentPatrolIndex))
	{
		PatrolTargetActor->SetActorLocation(PatrolPoints[CurrentPatrolIndex]);
		CallSetTargetLocation(PatrolTargetActor);

		UE_LOG(LogTemp, Log, TEXT("[ZP_Patrol] %s: Moving to waypoint %d at %s"),
			*GetOwner()->GetName(), CurrentPatrolIndex,
			*PatrolPoints[CurrentPatrolIndex].ToString());
	}
}
