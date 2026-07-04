// Copyright The Signal. All Rights Reserved.

#include "ZP_InteractDoor.h"
#include "ZP_GraceCharacter.h"
#include "ZP_ObjectiveSubsystem.h"
#include "ZP_SFXStatics.h"
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
	return AZP_DoorActor ? AZP_DoorActor->GetActorRotation() : FRotator::ZeroRotator;
}

void AZP_InteractDoor::SetDoorRotation(const FRotator& InRot)
{
	if (bSelfContained) DoorMesh->SetRelativeRotation(InRot);
	else if (AZP_DoorActor) AZP_DoorActor->SetActorRotation(InRot);
}

FVector AZP_InteractDoor::GetDoorLocation() const
{
	if (bSelfContained) return DoorMesh->GetRelativeLocation();
	return AZP_DoorActor ? AZP_DoorActor->GetActorLocation() : FVector::ZeroVector;
}

void AZP_InteractDoor::SetDoorLocation(const FVector& InLoc)
{
	if (bSelfContained) DoorMesh->SetRelativeLocation(InLoc);
	else if (AZP_DoorActor) AZP_DoorActor->SetActorLocation(InLoc);
}

AZP_InteractDoor::AZP_InteractDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Built-in door mesh for self-contained doors. Empty by default — only used
	// when no external AZP_DoorActor is linked (assign a mesh in the BP).
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
	bSelfContained = (AZP_DoorActor == nullptr) && DoorMesh && (DoorMesh->GetStaticMesh() != nullptr);

	if (bSelfContained)
	{
		DoorMesh->SetMobility(EComponentMobility::Movable);

		if (AZP_OpenMode == EZP_InteractDoorMode::Rotate)
		{
			ClosedRotation = DoorMesh->GetRelativeRotation();
			OpenRotation = ClosedRotation;
			OpenRotation.Yaw += AZP_OpenAngle;
		}
		else
		{
			ClosedLocation = DoorMesh->GetRelativeLocation();
			OpenLocation = ClosedLocation + AZP_SlideOffset;
		}

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: Self-contained (%s mode, mesh %s)"),
			*GetName(),
			AZP_OpenMode == EZP_InteractDoorMode::Rotate ? TEXT("Rotate") : TEXT("Slide"),
			*DoorMesh->GetStaticMesh()->GetName());
		InitObjectiveOverride();
		return;
	}

	if (!AZP_DoorActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] InteractDoor %s: No AZP_DoorActor linked and no DoorMesh set!"), *GetName());
		return;
	}

	// Ensure door meshes are Movable so they can be moved at runtime.
	// BigCompany pack doors ship as Static — override to Movable.
	TArray<UStaticMeshComponent*> MeshComps;
	AZP_DoorActor->GetComponents(MeshComps);
	for (UStaticMeshComponent* MC : MeshComps)
	{
		MC->SetMobility(EComponentMobility::Movable);
	}

	if (AZP_OpenMode == EZP_InteractDoorMode::Rotate)
	{
		ClosedRotation = AZP_DoorActor->GetActorRotation();
		OpenRotation = ClosedRotation;
		OpenRotation.Yaw += AZP_OpenAngle;
	}
	else
	{
		ClosedLocation = AZP_DoorActor->GetActorLocation();
		OpenLocation = ClosedLocation + AZP_SlideOffset;
	}

	// Register mesh → trigger mapping for trace-based interaction
	DoorActorMap.Add(AZP_DoorActor, this);

	// Disable collision on co-located actors (door frames).
	// BigCompany pack frames have convex hulls that fill the doorway opening.
	// Walls handle structural collision — frames are purely visual.
	FVector DoorLoc = AZP_DoorActor->GetActorLocation();
	AActor* DoorActorRaw = AZP_DoorActor.Get(); // Raw pointer for reliable comparison
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AActor* ItActor = Cast<AActor>(*It);
		if (ItActor == DoorActorRaw) continue; // Skip the door panel itself
		if (FVector::Dist(It->GetActorLocation(), DoorLoc) < AZP_FrameCollisionSilenceRadius)
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
		*GetName(), *AZP_DoorActor->GetName(),
		AZP_OpenMode == EZP_InteractDoorMode::Rotate ? TEXT("Rotate") : TEXT("Slide"));

	InitObjectiveOverride();
}

void AZP_InteractDoor::InitObjectiveOverride()
{
	if (AZP_ObjectiveOverride == NAME_None || !HasDoorTarget()) { return; }
	UGameInstance* GI = GetGameInstance();
	UZP_ObjectiveSubsystem* Obj = GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr;
	if (!Obj)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] InteractDoor %s: objective override '%s' set but no objective subsystem."),
			*GetName(), *AZP_ObjectiveOverride.ToString());
		return;
	}

	// Already completed before this level loaded (save-load / cross-level) — apply the END
	// state instantly. Reactions never replay.
	if (Obj->HasFlag(AZP_ObjectiveOverride) || Obj->IsObjectiveComplete(AZP_ObjectiveOverride))
	{
		OpenDoor(/*bInstant*/ true);
		return;
	}
	Obj->OnFlagSet.AddDynamic(this, &AZP_InteractDoor::OnObjectiveOverrideFired);
	Obj->OnObjectiveCompleted.AddDynamic(this, &AZP_InteractDoor::OnObjectiveOverrideFired);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: armed on objective override '%s'"),
		*GetName(), *AZP_ObjectiveOverride.ToString());
}

void AZP_InteractDoor::OnObjectiveOverrideFired(FName Id)
{
	if (Id == AZP_ObjectiveOverride)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: objective override '%s' completed — opening"),
			*GetName(), *Id.ToString());
		OpenDoor(/*bInstant*/ false);
	}
}

void AZP_InteractDoor::OpenDoor(bool bInstant)
{
	bAZP_Locked = false;
	if (!HasDoorTarget() || bIsOpen) { return; }
	bIsOpen = true;
	if (bInstant)
	{
		if (AZP_OpenMode == EZP_InteractDoorMode::Rotate) { SetDoorRotation(OpenRotation); }
		else { SetDoorLocation(OpenLocation); }
		bIsAnimating = false;
	}
	else
	{
		StartDoorAnimation();
		PlayOpenSound();
	}
}

void AZP_InteractDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AZP_DoorActor)
	{
		DoorActorMap.Remove(AZP_DoorActor);
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UZP_ObjectiveSubsystem* Obj = GI->GetSubsystem<UZP_ObjectiveSubsystem>())
		{
			Obj->OnFlagSet.RemoveDynamic(this, &AZP_InteractDoor::OnObjectiveOverrideFired);
			Obj->OnObjectiveCompleted.RemoveDynamic(this, &AZP_InteractDoor::OnObjectiveOverrideFired);
		}
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

	// EXACT-TIME path: AZP_OpenDuration > 0 = the full move takes exactly that many seconds
	// (eased in/out). Start pose captured in StartDoorAnimation.
	if (AZP_OpenDuration > 0.f)
	{
		AnimElapsed += DeltaTime;
		const float T = FMath::Clamp(AnimElapsed / AZP_OpenDuration, 0.f, 1.f);
		const float Alpha = FMath::InterpEaseInOut(0.f, 1.f, T, 2.f);
		if (AZP_OpenMode == EZP_InteractDoorMode::Rotate)
		{
			const FRotator& Target = bIsOpen ? OpenRotation : ClosedRotation;
			SetDoorRotation(FQuat::Slerp(AnimStartRotation.Quaternion(), Target.Quaternion(), Alpha).Rotator());
		}
		else
		{
			const FVector& Target = bIsOpen ? OpenLocation : ClosedLocation;
			SetDoorLocation(FMath::Lerp(AnimStartLocation, Target, Alpha));
		}
		if (T >= 1.f)
		{
			bIsAnimating = false;
			SetActorTickEnabled(false);
		}
		return;
	}

	if (AZP_OpenMode == EZP_InteractDoorMode::Rotate)
	{
		const FRotator& Target = bIsOpen ? OpenRotation : ClosedRotation;
		FRotator Current = GetDoorRotation();
		FRotator NewRot = FMath::RInterpTo(Current, Target, DeltaTime, AZP_InterpSpeed);
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
		FVector NewLoc = FMath::VInterpTo(Current, Target, DeltaTime, AZP_InterpSpeed);
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
	if (bAZP_Locked)
	{
		return AZP_LockedPromptText;
	}
	return FText::GetEmpty();
}

void AZP_InteractDoor::OnInteract_Implementation(ACharacter* Interactor)
{
	if (!HasDoorTarget()) return;

	if (bAZP_Locked)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: LOCKED — ignoring interact"), *GetName());
		return;
	}

	bIsOpen = !bIsOpen;

	// Hinged doors always swing AWAY from whoever opens them (dev call:
	// never hit the player). Pick the yaw sign from which side of the door
	// plane the interactor stands on. Decided fresh on every open.
	if (bIsOpen && AZP_OpenMode == EZP_InteractDoorMode::Rotate && Interactor)
	{
		const FVector DoorLoc = bSelfContained
			? DoorMesh->GetComponentLocation()
			: (AZP_DoorActor ? AZP_DoorActor->GetActorLocation() : GetActorLocation());
		const FVector DoorFwd = bSelfContained
			? DoorMesh->GetForwardVector()
			: (AZP_DoorActor ? AZP_DoorActor->GetActorForwardVector() : GetActorForwardVector());
		const float Side = FVector::DotProduct(DoorFwd, Interactor->GetActorLocation() - DoorLoc);
		// Sign flipped AGAIN (dev-caught): the door assets were being placed
		// upside-down, which inverts the mesh forward vector. The previous sign was
		// tuned to that wrong orientation and swung INTO the player. Flipped back to
		// the textbook convention now that the doors are used right-side up.
		OpenRotation = ClosedRotation;
		OpenRotation.Yaw += (Side >= 0.f) ? AZP_OpenAngle : -AZP_OpenAngle;
	}

	if (bIsOpen) { PlayOpenSound(); }
	StartDoorAnimation();

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: %s"),
		*GetName(), bIsOpen ? TEXT("Opening") : TEXT("Closing"));
}

void AZP_InteractDoor::StartDoorAnimation()
{
	AnimElapsed = 0.f;
	AnimStartRotation = GetDoorRotation();
	AnimStartLocation = GetDoorLocation();
	bIsAnimating = true;
	SetActorTickEnabled(true);
}

void AZP_InteractDoor::PlayOpenSound()
{
	if (USoundBase* S = AZP_OpenSound)
	{
		USceneComponent* At = bSelfContained
			? Cast<USceneComponent>(DoorMesh)
			: (AZP_DoorActor ? AZP_DoorActor->GetRootComponent() : GetRootComponent());
		UZP_SFXStatics::PlaySFXAttached(S, At ? At : GetRootComponent(), AZP_OpenSoundCarry);
	}
}

void AZP_InteractDoor::Unlock()
{
	bAZP_Locked = false;

	// Auto-open the door when unlocked (same behavior as AZP_LockableDoor)
	if (!bIsOpen && HasDoorTarget())
	{
		bIsOpen = true;
		StartDoorAnimation();
		PlayOpenSound();
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
