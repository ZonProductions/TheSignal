// Copyright The Signal. All Rights Reserved.

#include "ZP_InteractDoor.h"
#include "ZP_GraceCharacter.h"
#include "Components/BoxComponent.h"

AZP_InteractDoor::AZP_InteractDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	InteractionVolume->SetGenerateOverlapEvents(true);

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_InteractDoor::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_InteractDoor::OnOverlapEnd);
}

void AZP_InteractDoor::BeginPlay()
{
	Super::BeginPlay();

	if (!DoorActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] InteractDoor %s: No DoorActor linked!"), *GetName());
		return;
	}

	// Ensure door meshes are Movable so they can be moved at runtime.
	// BigCompany pack doors ship as Static — override to Movable.
	TArray<UStaticMeshComponent*> MeshComps;
	DoorActor->GetComponents(MeshComps);
	for (UStaticMeshComponent* MC : MeshComps)
	{
		MC->SetMobility(EComponentMobility::Movable);
	}

	if (OpenMode == EZP_InteractDoorMode::Rotate)
	{
		ClosedRotation = DoorActor->GetActorRotation();
		OpenRotation = ClosedRotation;
		OpenRotation.Yaw += OpenAngle;
	}
	else
	{
		ClosedLocation = DoorActor->GetActorLocation();
		OpenLocation = ClosedLocation + SlideOffset;
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: Linked to %s (%s mode)"),
		*GetName(), *DoorActor->GetName(),
		OpenMode == EZP_InteractDoorMode::Rotate ? TEXT("Rotate") : TEXT("Slide"));
}

void AZP_InteractDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!DoorActor || !bIsAnimating)
	{
		SetActorTickEnabled(false);
		bIsAnimating = false;
		return;
	}

	if (OpenMode == EZP_InteractDoorMode::Rotate)
	{
		const FRotator& Target = bIsOpen ? OpenRotation : ClosedRotation;
		FRotator Current = DoorActor->GetActorRotation();
		FRotator NewRot = FMath::RInterpTo(Current, Target, DeltaTime, InterpSpeed);
		DoorActor->SetActorRotation(NewRot);

		if (NewRot.Equals(Target, 0.5f))
		{
			DoorActor->SetActorRotation(Target);
			bIsAnimating = false;
			SetActorTickEnabled(false);
		}
	}
	else // Slide
	{
		const FVector& Target = bIsOpen ? OpenLocation : ClosedLocation;
		FVector Current = DoorActor->GetActorLocation();
		FVector NewLoc = FMath::VInterpTo(Current, Target, DeltaTime, InterpSpeed);
		DoorActor->SetActorLocation(NewLoc);

		if (FVector::Dist(NewLoc, Target) < 1.f)
		{
			DoorActor->SetActorLocation(Target);
			bIsAnimating = false;
			SetActorTickEnabled(false);
		}
	}
}

// --- IZP_Interactable ---

FText AZP_InteractDoor::GetInteractionPrompt_Implementation()
{
	// No prompt — doors are self-evident
	return FText::GetEmpty();
}

void AZP_InteractDoor::OnInteract_Implementation(ACharacter* Interactor)
{
	if (!DoorActor)
	{
		return;
	}

	bIsOpen = !bIsOpen;
	bIsAnimating = true;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: %s"),
		*GetName(), bIsOpen ? TEXT("Opening") : TEXT("Closing"));
}

// --- Overlap ---

void AZP_InteractDoor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->SetCurrentInteractable(this);
}

void AZP_InteractDoor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->ClearCurrentInteractable(this);
}
