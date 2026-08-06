// Copyright The Signal. All Rights Reserved.

#include "ZP_LockableDoor.h"
#include "Components/StaticMeshComponent.h"

AZP_LockableDoor::AZP_LockableDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // Enabled only during Opening

	DoorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot"));
	SetRootComponent(DoorRoot);

	// Frame mesh — non-moving
	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	FrameMesh->SetupAttachment(DoorRoot);

	// Pivot for door rotation — offset to door hinge position in BP
	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(DoorRoot);

	// Door mesh — rotates with pivot
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorPivot);
	// Dynamic nav obstacle, same contract as AZP_InteractDoor (2026-08-05 nav-aware doors):
	// closed leaf carves the doorway, open leaf releases it.
	DoorMesh->SetCanEverAffectNavigation(true);
}

void AZP_LockableDoor::BeginPlay()
{
	Super::BeginPlay();

	CurrentState = AZP_InitialState;

	if (AZP_OpenMode == EZP_DoorOpenMode::Rotate)
	{
		InitialYaw = DoorPivot->GetRelativeRotation().Yaw;
		TargetYaw = InitialYaw;
	}
	else
	{
		InitialLocation = DoorPivot->GetRelativeLocation();
		TargetLocation = InitialLocation;
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] LockableDoor %s: Initial state = %s, Mode = %s"),
		*GetName(), *UEnum::GetValueAsString(CurrentState),
		AZP_OpenMode == EZP_DoorOpenMode::Rotate ? TEXT("Rotate") : TEXT("Slide"));
}

void AZP_LockableDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState != EZP_DoorState::Opening) return;

	if (AZP_OpenMode == EZP_DoorOpenMode::Rotate)
	{
		FRotator CurrentRot = DoorPivot->GetRelativeRotation();
		float NewYaw = FMath::FInterpTo(CurrentRot.Yaw, TargetYaw, DeltaTime, AZP_OpenInterpSpeed);
		DoorPivot->SetRelativeRotation(FRotator(CurrentRot.Pitch, NewYaw, CurrentRot.Roll));

		if (FMath::IsNearlyEqual(NewYaw, TargetYaw, 0.5f))
		{
			DoorPivot->SetRelativeRotation(FRotator(CurrentRot.Pitch, TargetYaw, CurrentRot.Roll));
			SetState(EZP_DoorState::Open);
			SetActorTickEnabled(false);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] LockableDoor %s: Fully open"), *GetName());
		}
	}
	else // Slide
	{
		FVector CurrentLoc = DoorPivot->GetRelativeLocation();
		FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLocation, DeltaTime, AZP_OpenInterpSpeed);
		DoorPivot->SetRelativeLocation(NewLoc);

		if (FVector::Dist(NewLoc, TargetLocation) < 1.f)
		{
			DoorPivot->SetRelativeLocation(TargetLocation);
			SetState(EZP_DoorState::Open);
			SetActorTickEnabled(false);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] LockableDoor %s: Fully open (slid)"), *GetName());
		}
	}
}

void AZP_LockableDoor::Unlock()
{
	if (CurrentState != EZP_DoorState::Locked) return;

	SetState(EZP_DoorState::Closed);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] LockableDoor %s: Unlocked"), *GetName());
}

void AZP_LockableDoor::OpenDoor()
{
	if (CurrentState != EZP_DoorState::Closed) return;

	if (AZP_OpenMode == EZP_DoorOpenMode::Rotate)
	{
		TargetYaw = InitialYaw + AZP_OpenAngle;
	}
	else
	{
		TargetLocation = InitialLocation + AZP_SlideOffset;
	}

	SetState(EZP_DoorState::Opening);
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] LockableDoor %s: Opening (%s)"),
		*GetName(),
		AZP_OpenMode == EZP_DoorOpenMode::Rotate
			? *FString::Printf(TEXT("target yaw %.1f"), TargetYaw)
			: *FString::Printf(TEXT("slide to %s"), *TargetLocation.ToString()));
}

void AZP_LockableDoor::SetState(EZP_DoorState NewState)
{
	CurrentState = NewState;
	OnDoorStateChanged.Broadcast(NewState);
}
