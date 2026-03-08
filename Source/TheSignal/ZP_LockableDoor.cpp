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
}

void AZP_LockableDoor::BeginPlay()
{
	Super::BeginPlay();

	CurrentState = InitialState;

	if (OpenMode == EZP_DoorOpenMode::Rotate)
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
		OpenMode == EZP_DoorOpenMode::Rotate ? TEXT("Rotate") : TEXT("Slide"));
}

void AZP_LockableDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState != EZP_DoorState::Opening) return;

	if (OpenMode == EZP_DoorOpenMode::Rotate)
	{
		FRotator CurrentRot = DoorPivot->GetRelativeRotation();
		float NewYaw = FMath::FInterpTo(CurrentRot.Yaw, TargetYaw, DeltaTime, OpenInterpSpeed);
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
		FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLocation, DeltaTime, OpenInterpSpeed);
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

	if (OpenMode == EZP_DoorOpenMode::Rotate)
	{
		TargetYaw = InitialYaw + OpenAngle;
	}
	else
	{
		TargetLocation = InitialLocation + SlideOffset;
	}

	SetState(EZP_DoorState::Opening);
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] LockableDoor %s: Opening (%s)"),
		*GetName(),
		OpenMode == EZP_DoorOpenMode::Rotate
			? *FString::Printf(TEXT("target yaw %.1f"), TargetYaw)
			: *FString::Printf(TEXT("slide to %s"), *TargetLocation.ToString()));
}

void AZP_LockableDoor::SetState(EZP_DoorState NewState)
{
	CurrentState = NewState;
	OnDoorStateChanged.Broadcast(NewState);
}
