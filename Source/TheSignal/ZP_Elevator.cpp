// Copyright The Signal. All Rights Reserved.

#include "ZP_Elevator.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "UObject/ConstructorHelpers.h"

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

	// Default car sounds (overridable per BP/instance).
	static ConstructorHelpers::FObjectFinder<USoundBase> MoveSoundFinder(
		TEXT("/Game/Audio/Elevator/SFX_Elevator_Move.SFX_Elevator_Move"));
	if (MoveSoundFinder.Succeeded()) { AZP_MoveSound = MoveSoundFinder.Object; }

	static ConstructorHelpers::FObjectFinder<USoundBase> ArriveSoundFinder(
		TEXT("/Game/Audio/Elevator/SFX_Elevator_Beep.SFX_Elevator_Beep"));
	if (ArriveSoundFinder.Succeeded()) { AZP_ArriveSound = ArriveSoundFinder.Object; }
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

	if (GetActorLocation().Equals(TargetLocation, AZP_ArriveTolerance))
	{
		// Already there — fire the full arrival path so hooks (doors/lights/beep) still run.
		bMoving = false;
		SetActorTickEnabled(false);
		HandleArrived(RelativeZ);
		return;
	}

	const bool bWasMoving = bMoving;
	bMoving = true;
	SetActorTickEnabled(true);

	// Departure effects only on the not-moving -> moving edge (a mid-travel redirect keeps the
	// existing travel loop running and is not a new departure).
	if (!bWasMoving)
	{
		OnElevatorDeparted.Broadcast(GetCurrentRelativeZ());
		if (AZP_MoveSound)
		{
			MoveAudioComp = UZP_SFXStatics::PlaySFXAttached(
				AZP_MoveSound, PlatformMesh, AZP_MoveSoundCarry, AZP_MoveSoundVolume);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Elevator %s: moving to relative Z %.1f (world %s)"),
		*GetName(), RelativeZ, *TargetLocation.ToString());
}

void AZP_Elevator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bMoving) return;

	const FVector Cur = GetActorLocation();
	const FVector New = FMath::VInterpConstantTo(Cur, TargetLocation, DeltaTime, AZP_MoveSpeed);

	// bSweep = false: the platform is a kinematic mover — it must NOT be blocked by the rider's
	// capsule. The standing character is carried by the engine's based-movement instead.
	SetActorLocation(New, /*bSweep=*/false);

	if (New.Equals(TargetLocation, AZP_ArriveTolerance))
	{
		SetActorLocation(TargetLocation, /*bSweep=*/false);
		bMoving = false;
		SetActorTickEnabled(false);
		HandleArrived(GetCurrentRelativeZ());
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Elevator %s: arrived (relative Z %.1f)"),
			*GetName(), GetCurrentRelativeZ());
	}
}

void AZP_Elevator::HandleArrived(float RelativeZ)
{
	if (MoveAudioComp)
	{
		MoveAudioComp->FadeOut(AZP_MoveSoundFadeOut, 0.f);
		MoveAudioComp = nullptr;
	}

	if (AZP_ArriveSound)
	{
		UZP_SFXStatics::PlaySFXAttached(
			AZP_ArriveSound, PlatformMesh, AZP_ArriveSoundCarry, AZP_ArriveSoundVolume);
	}

	OnElevatorArrived.Broadcast(RelativeZ);
}

float AZP_Elevator::GetCurrentRelativeZ() const
{
	return GetActorLocation().Z - OriginLocation.Z;
}
