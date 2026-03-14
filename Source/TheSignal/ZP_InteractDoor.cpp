// Copyright The Signal. All Rights Reserved.

#include "ZP_InteractDoor.h"
#include "ZP_GraceCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"

// --- Static door lookup map ---
TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AZP_InteractDoor>> AZP_InteractDoor::DoorActorMap;

AZP_InteractDoor* AZP_InteractDoor::FindDoorForActor(AActor* Actor)
{
	if (!Actor) return nullptr;
	auto* Found = DoorActorMap.Find(Actor);
	if (Found && Found->IsValid()) return Found->Get();
	return nullptr;
}

AZP_InteractDoor::AZP_InteractDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetBoxExtent(FVector(250.f, 250.f, 120.f));
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

	// Register mesh → trigger mapping for trace-based interaction
	DoorActorMap.Add(DoorActor, this);

	// Disable collision on co-located actors (door frames).
	// BigCompany pack frames have convex hulls that fill the doorway opening.
	// Walls handle structural collision — frames are purely visual.
	FVector DoorLoc = DoorActor->GetActorLocation();
	AActor* DoorActorRaw = DoorActor.Get(); // Raw pointer for reliable comparison
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AActor* ItActor = Cast<AActor>(*It);
		if (ItActor == DoorActorRaw) continue; // Skip the door panel itself
		if (FVector::Dist(It->GetActorLocation(), DoorLoc) < 10.f)
		{
			UStaticMeshComponent* SMC = It->GetStaticMeshComponent();
			if (SMC)
			{
				SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: Disabled frame collision on %s (door panel is %s)"),
					*GetName(), *It->GetName(), *DoorActorRaw->GetName());
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: Linked to %s (%s mode)"),
		*GetName(), *DoorActor->GetName(),
		OpenMode == EZP_InteractDoorMode::Rotate ? TEXT("Rotate") : TEXT("Slide"));
}

void AZP_InteractDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DoorActor)
	{
		DoorActorMap.Remove(DoorActor);
	}
	Super::EndPlay(EndPlayReason);
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
	if (bLocked)
	{
		return FText::FromString(TEXT("Locked"));
	}
	return FText::GetEmpty();
}

void AZP_InteractDoor::OnInteract_Implementation(ACharacter* Interactor)
{
	if (!DoorActor) return;

	if (bLocked)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: LOCKED — ignoring interact"), *GetName());
		return;
	}

	bIsOpen = !bIsOpen;
	bIsAnimating = true;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: %s"),
		*GetName(), bIsOpen ? TEXT("Opening") : TEXT("Closing"));
}

void AZP_InteractDoor::Unlock()
{
	bLocked = false;

	// Auto-open the door when unlocked (same behavior as AZP_LockableDoor)
	if (!bIsOpen && DoorActor)
	{
		bIsOpen = true;
		bIsAnimating = true;
		SetActorTickEnabled(true);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: UNLOCKED + OPENED"), *GetName());
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
