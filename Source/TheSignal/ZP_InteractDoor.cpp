// Copyright The Signal. All Rights Reserved.

#include "ZP_InteractDoor.h"
#include "ZP_GraceCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
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

// --- Unified door target accessors (external actor vs built-in mesh) ---

FRotator AZP_InteractDoor::GetDoorRotation() const
{
	if (bSelfContained) return DoorMesh->GetRelativeRotation();
	return DoorActor ? DoorActor->GetActorRotation() : FRotator::ZeroRotator;
}

void AZP_InteractDoor::SetDoorRotation(const FRotator& InRot)
{
	if (bSelfContained) DoorMesh->SetRelativeRotation(InRot);
	else if (DoorActor) DoorActor->SetActorRotation(InRot);
}

FVector AZP_InteractDoor::GetDoorLocation() const
{
	if (bSelfContained) return DoorMesh->GetRelativeLocation();
	return DoorActor ? DoorActor->GetActorLocation() : FVector::ZeroVector;
}

void AZP_InteractDoor::SetDoorLocation(const FVector& InLoc)
{
	if (bSelfContained) DoorMesh->SetRelativeLocation(InLoc);
	else if (DoorActor) DoorActor->SetActorLocation(InLoc);
}

AZP_InteractDoor::AZP_InteractDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Built-in door mesh for self-contained doors. Empty by default — only used
	// when no external DoorActor is linked (assign a mesh in the BP).
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Root);
	DoorMesh->SetMobility(EComponentMobility::Movable);
	DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));

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

	// Self-contained mode: no external actor, but a built-in mesh is assigned.
	bSelfContained = (DoorActor == nullptr) && DoorMesh && (DoorMesh->GetStaticMesh() != nullptr);

	if (bSelfContained)
	{
		DoorMesh->SetMobility(EComponentMobility::Movable);

		if (OpenMode == EZP_InteractDoorMode::Rotate)
		{
			ClosedRotation = DoorMesh->GetRelativeRotation();
			OpenRotation = ClosedRotation;
			OpenRotation.Yaw += OpenAngle;
		}
		else
		{
			ClosedLocation = DoorMesh->GetRelativeLocation();
			OpenLocation = ClosedLocation + SlideOffset;
		}

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: Self-contained (%s mode, mesh %s)"),
			*GetName(),
			OpenMode == EZP_InteractDoorMode::Rotate ? TEXT("Rotate") : TEXT("Slide"),
			*DoorMesh->GetStaticMesh()->GetName());
		return;
	}

	if (!DoorActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] InteractDoor %s: No DoorActor linked and no DoorMesh set!"), *GetName());
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

	if (!HasDoorTarget() || !bIsAnimating)
	{
		SetActorTickEnabled(false);
		bIsAnimating = false;
		return;
	}

	if (OpenMode == EZP_InteractDoorMode::Rotate)
	{
		const FRotator& Target = bIsOpen ? OpenRotation : ClosedRotation;
		FRotator Current = GetDoorRotation();
		FRotator NewRot = FMath::RInterpTo(Current, Target, DeltaTime, InterpSpeed);
		SetDoorRotation(NewRot);

		if (NewRot.Equals(Target, 0.5f))
		{
			SetDoorRotation(Target);
			bIsAnimating = false;
			SetActorTickEnabled(false);
		}
	}
	else // Slide
	{
		const FVector& Target = bIsOpen ? OpenLocation : ClosedLocation;
		FVector Current = GetDoorLocation();
		FVector NewLoc = FMath::VInterpTo(Current, Target, DeltaTime, InterpSpeed);
		SetDoorLocation(NewLoc);

		if (FVector::Dist(NewLoc, Target) < 1.f)
		{
			SetDoorLocation(Target);
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
	if (!HasDoorTarget()) return;

	if (bLocked)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: LOCKED — ignoring interact"), *GetName());
		return;
	}

	bIsOpen = !bIsOpen;

	// Hinged doors always swing AWAY from whoever opens them (dev call:
	// never hit the player). Pick the yaw sign from which side of the door
	// plane the interactor stands on. Decided fresh on every open.
	if (bIsOpen && OpenMode == EZP_InteractDoorMode::Rotate && Interactor)
	{
		const FVector DoorLoc = bSelfContained
			? DoorMesh->GetComponentLocation()
			: (DoorActor ? DoorActor->GetActorLocation() : GetActorLocation());
		const FVector DoorFwd = bSelfContained
			? DoorMesh->GetForwardVector()
			: (DoorActor ? DoorActor->GetActorForwardVector() : GetActorForwardVector());
		const float Side = FVector::DotProduct(DoorFwd, Interactor->GetActorLocation() - DoorLoc);
		// Sign flipped after testing: these door assets swing INTO the player
		// with the textbook convention (dev-caught).
		OpenRotation = ClosedRotation;
		OpenRotation.Yaw += (Side >= 0.f) ? -OpenAngle : OpenAngle;
	}

	bIsAnimating = true;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: %s"),
		*GetName(), bIsOpen ? TEXT("Opening") : TEXT("Closing"));
}

void AZP_InteractDoor::Unlock()
{
	bLocked = false;

	// Auto-open the door when unlocked (same behavior as AZP_LockableDoor)
	if (!bIsOpen && HasDoorTarget())
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
