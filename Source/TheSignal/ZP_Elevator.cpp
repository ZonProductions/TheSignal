// Copyright The Signal. All Rights Reserved.

#include "ZP_Elevator.h"
#include "Components/StaticMeshComponent.h"

AZP_Elevator::AZP_Elevator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // ticks only while travelling

	ElevatorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ElevatorRoot"));
	SetRootComponent(ElevatorRoot);

	// The car. Movable + blocking so a standing character is carried by engine based-movement.
	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	PlatformMesh->SetupAttachment(ElevatorRoot);
	PlatformMesh->SetMobility(EComponentMobility::Movable);
	PlatformMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PlatformMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AZP_Elevator::BeginPlay()
{
	Super::BeginPlay();

	OriginLocation = GetActorLocation();
	TargetLocation = OriginLocation;

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Elevator %s: origin %s"), *GetName(), *OriginLocation.ToString());
}

void AZP_Elevator::MoveToRelativeZ(float RelativeZ)
{
	TargetLocation = OriginLocation + FVector(0.f, 0.f, RelativeZ);

	if (GetActorLocation().Equals(TargetLocation, ArriveTolerance))
	{
		// Already there — fire arrival so hooks (doors/audio) still run.
		bMoving = false;
		SetActorTickEnabled(false);
		OnElevatorArrived.Broadcast(RelativeZ);
		return;
	}

	bMoving = true;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Elevator %s: moving to relative Z %.1f (world %s)"),
		*GetName(), RelativeZ, *TargetLocation.ToString());
}

void AZP_Elevator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bMoving) return;

	const FVector Cur = GetActorLocation();
	const FVector New = FMath::VInterpConstantTo(Cur, TargetLocation, DeltaTime, MoveSpeed);

	// bSweep = false: the platform is a kinematic mover — it must NOT be blocked by the rider's
	// capsule. The standing character is carried by the engine's based-movement instead.
	SetActorLocation(New, /*bSweep=*/false);

	if (New.Equals(TargetLocation, ArriveTolerance))
	{
		SetActorLocation(TargetLocation, /*bSweep=*/false);
		bMoving = false;
		SetActorTickEnabled(false);
		OnElevatorArrived.Broadcast(GetCurrentRelativeZ());
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Elevator %s: arrived (relative Z %.1f)"),
			*GetName(), GetCurrentRelativeZ());
	}
}

float AZP_Elevator::GetCurrentRelativeZ() const
{
	return GetActorLocation().Z - OriginLocation.Z;
}
