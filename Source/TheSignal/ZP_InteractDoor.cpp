// Copyright The Signal. All Rights Reserved.

#include "ZP_InteractDoor.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "ZP_ObjectiveSubsystem.h"
#include "ZP_SFXStatics.h"
#include "ZP_Elevator.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

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

	// Default key-unlock sound (overridable per BP/instance).
	static ConstructorHelpers::FObjectFinder<USoundBase> UnlockSoundFinder(
		TEXT("/Game/Audio/SFX_Door_Unlock.SFX_Door_Unlock"));
	if (UnlockSoundFinder.Succeeded()) { AZP_UnlockSound = UnlockSoundFinder.Object; }

	// Default handle/knob sound (overridable per BP/instance).
	static ConstructorHelpers::FObjectFinder<USoundBase> HandleSoundFinder(
		TEXT("/Game/Audio/SFX_Door_Handle.SFX_Door_Handle"));
	if (HandleSoundFinder.Succeeded()) { AZP_HandleSound = HandleSoundFinder.Object; }
}

void AZP_InteractDoor::BeginPlay()
{
	Super::BeginPlay();

	// A required key item implies the door starts locked — no need to also tick bAZP_Locked.
	if (!AZP_RequiredItem.IsNull())
	{
		bAZP_Locked = true;
	}

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
		InitElevatorLink();
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
	InitElevatorLink();
}

// --- Elevator link (landing/shaft doors) ---

void AZP_InteractDoor::InitElevatorLink()
{
	if (!AZP_LinkedElevator || !HasDoorTarget()) { return; }

	AZP_LinkedElevator->OnElevatorArrived.AddDynamic(this, &AZP_InteractDoor::OnLinkedElevatorArrived);
	AZP_LinkedElevator->OnElevatorDeparted.AddDynamic(this, &AZP_InteractDoor::OnLinkedElevatorDeparted);

	// Car already parked at this floor when the level starts -> begin in the OPEN pose.
	// Otherwise a linked landing door starts closed; locked too if it guards the shaft.
	if (!AZP_LinkedElevator->IsMoving() && IsElevatorAtMyFloor())
	{
		OpenDoor(/*bInstant*/ true);
	}
	else if (bAZP_CloseWhenElevatorLeaves)
	{
		bAZP_Locked = true;
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: elevator-linked to %s (Z margin %.0f, car %s)"),
		*GetName(), *AZP_LinkedElevator->GetName(), AZP_ElevatorZMargin,
		bIsOpen ? TEXT("AT this floor — starting open") : TEXT("away — starting closed"));
}

bool AZP_InteractDoor::IsElevatorAtMyFloor() const
{
	if (!AZP_LinkedElevator) { return false; }
	return FMath::Abs(AZP_LinkedElevator->GetActorLocation().Z - GetActorLocation().Z) <= AZP_ElevatorZMargin;
}

void AZP_InteractDoor::OnLinkedElevatorArrived(float RelativeZ)
{
	if (IsElevatorAtMyFloor())
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: elevator parked at this floor — opening"), *GetName());
		OpenDoor(/*bInstant*/ false);
	}
}

void AZP_InteractDoor::OnLinkedElevatorDeparted(float FromRelativeZ)
{
	if (!bAZP_CloseWhenElevatorLeaves || !bIsOpen || !AZP_LinkedElevator) { return; }

	// Only react when the car left THIS floor.
	const float FromWorldZ = AZP_LinkedElevator->GetOriginZ() + FromRelativeZ;
	if (FMath::Abs(FromWorldZ - GetActorLocation().Z) > AZP_ElevatorZMargin) { return; }

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: elevator left this floor — closing + locking"), *GetName());
	CloseDoor();
	bAZP_Locked = true;
}

void AZP_InteractDoor::CloseDoor()
{
	if (!HasDoorTarget() || !bIsOpen) { return; }
	bIsOpen = false;
	StartDoorAnimation();
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
	if (AZP_LinkedElevator)
	{
		AZP_LinkedElevator->OnElevatorArrived.RemoveDynamic(this, &AZP_InteractDoor::OnLinkedElevatorArrived);
		AZP_LinkedElevator->OnElevatorDeparted.RemoveDynamic(this, &AZP_InteractDoor::OnLinkedElevatorDeparted);
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

	// Rate-limit interact presses — inside the window a press does nothing at all
	// (no toggle, no handle rattle, no messages). Kills E-mash spam.
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now - LastInteractTime < AZP_InteractCooldown) { return; }
	LastInteractTime = Now;

	// The handle always turns/rattles — every accepted attempt is audible, open or not.
	PlayHandleSound();

	if (bAZP_Locked)
	{
		// Free-side unlock first (never consumes the key), then the key-item gate. Either way
		// fall through to the normal open flow below.
		const bool bSideUnlock = bAZP_UnlockFromOtherSide && Interactor && IsOnUnlockSide(Interactor);
		const bool bKeyUnlock = !bSideUnlock && !AZP_RequiredItem.IsNull() && PlayerHasRequiredItem(Interactor);
		if (bSideUnlock || bKeyUnlock)
		{
			UnlockWithKey(Interactor, /*bViaKeyItem*/ bKeyUnlock);
		}
		else
		{
			// Cannot open — surface the locked prompt for ANY blocked reason (manual lock,
			// missing key item, elevator away).
			ShowTimedHudMessage(Interactor, AZP_LockedPromptText);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: LOCKED — ignoring interact%s"),
				*GetName(), AZP_RequiredItem.IsNull() ? TEXT("") : TEXT(" (missing required item)"));
			return;
		}
	}

	bIsOpen = !bIsOpen;

	// Hinged doors always swing AWAY from whoever opens them (dev call:
	// never hit the player). No axis convention — those broke on mirrored
	// placements twice: instead, simulate the panel center's end position for
	// BOTH yaw signs and keep the one that lands farther from the opener. The
	// two candidates are mirror images across the closed-door plane, so the
	// farther one is the away-swing for any mesh axes, hinge side, or actor
	// rotation the level uses. Decided fresh on every open.
	if (bIsOpen && AZP_OpenMode == EZP_InteractDoorMode::Rotate && Interactor)
	{
		// A yaw delta rotates external actors about world Z; the built-in
		// mesh's RELATIVE yaw spins about its attach parent's Z axis.
		FVector Axis = FVector::UpVector;
		FVector Pivot;
		FVector PanelCenter;
		if (bSelfContained)
		{
			if (const USceneComponent* Parent = DoorMesh->GetAttachParent())
			{
				Axis = Parent->GetComponentQuat().GetUpVector();
			}
			Pivot = DoorMesh->GetComponentLocation();
			PanelCenter = DoorMesh->Bounds.Origin;
		}
		else
		{
			Pivot = AZP_DoorActor->GetActorLocation();
			PanelCenter = AZP_DoorActor->GetComponentsBoundingBox(true).GetCenter();
		}

		// Hinge→panel arm at the CLOSED pose (re-derived if the player
		// re-opens mid-close, when the panel sits at a partial angle).
		FVector Arm = PanelCenter - Pivot;
		Arm = Arm.RotateAngleAxis(ClosedRotation.Yaw - GetDoorRotation().Yaw, Axis);

		float Swing = AZP_OpenAngle; // center-pivot doors keep the authored sign
		if (!FVector::VectorPlaneProject(Arm, Axis).IsNearlyZero(1.f))
		{
			const FVector PlayerLoc = Interactor->GetActorLocation();
			const float DistPlus = FVector::DistSquared(Pivot + Arm.RotateAngleAxis(AZP_OpenAngle, Axis), PlayerLoc);
			const float DistMinus = FVector::DistSquared(Pivot + Arm.RotateAngleAxis(-AZP_OpenAngle, Axis), PlayerLoc);
			Swing = (DistPlus >= DistMinus) ? AZP_OpenAngle : -AZP_OpenAngle;
		}
		OpenRotation = ClosedRotation;
		OpenRotation.Yaw += Swing;
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
		UZP_SFXStatics::PlaySFXAttached(S, GetSFXAttachComp(), AZP_OpenSoundCarry);
	}
}

// --- Key-item lock (Moonville reflection, mirrors ZP_VentDoor/CardReaderPanel) ---

/** True if TargetDA sits in the named Moonville slot array (ItemSlots / ShortcutSlots). */
static bool HasItemInSlotArray(UActorComponent* InvComp, const FName& ArrayName, UObject* TargetDA)
{
	if (!InvComp || !TargetDA) return false;

	FProperty* SlotsProp = InvComp->GetClass()->FindPropertyByName(ArrayName);
	if (!SlotsProp) return false;

	FArrayProperty* ArrayProp = CastField<FArrayProperty>(SlotsProp);
	if (!ArrayProp) return false;

	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(InvComp));
	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return false;

	FProperty* ItemProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("Item_")))
		{
			ItemProp = *It;
			break;
		}
	}
	if (!ItemProp) return false;

	FObjectProperty* ObjProp = CastField<FObjectProperty>(ItemProp);
	if (!ObjProp) return false;

	for (int32 i = 0; i < ArrayHelper.Num(); i++)
	{
		void* ElementData = ArrayHelper.GetRawPtr(i);
		UObject* SlotItem = ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<void>(ElementData));
		if (SlotItem == TargetDA) return true;
	}
	return false;
}

bool AZP_InteractDoor::PlayerHasRequiredItem(ACharacter* Character) const
{
	UObject* TargetDA = AZP_RequiredItem.LoadSynchronous();
	if (!TargetDA || !Character) return false;

	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Character);
	UActorComponent* InvComp = Grace ? Grace->MoonvilleInventoryComp.Get() : nullptr;
	if (!InvComp) return false;

	return HasItemInSlotArray(InvComp, FName("ItemSlots"), TargetDA)
		|| HasItemInSlotArray(InvComp, FName("ShortcutSlots"), TargetDA);
}

bool AZP_InteractDoor::IsOnUnlockSide(const ACharacter* Character) const
{
	if (!Character) { return false; }

	// Side = which half-space of THIS trigger actor's forward (+X) plane the player stands in.
	// The dev flips AZP_UnlockSide instead of rotating the door.
	const float Dot = FVector::DotProduct(GetActorForwardVector(),
		Character->GetActorLocation() - GetActorLocation());
	return (AZP_UnlockSide == EZP_DoorUnlockSide::Front) ? (Dot >= 0.f) : (Dot < 0.f);
}

USceneComponent* AZP_InteractDoor::GetSFXAttachComp() const
{
	USceneComponent* At = bSelfContained
		? Cast<USceneComponent>(DoorMesh)
		: (AZP_DoorActor ? AZP_DoorActor->GetRootComponent() : GetRootComponent());
	return At ? At : GetRootComponent();
}

void AZP_InteractDoor::PlayHandleSound()
{
	if (AZP_HandleSound)
	{
		UZP_SFXStatics::PlaySFXAttached(AZP_HandleSound, GetSFXAttachComp(),
			AZP_HandleSoundCarry, AZP_HandleSoundVolume);
	}
}

void AZP_InteractDoor::ShowTimedHudMessage(ACharacter* Interactor, const FText& Message)
{
	AZP_PlayerController* PC = Interactor ? Cast<AZP_PlayerController>(Interactor->GetController()) : nullptr;
	if (!PC || !PC->HUDWidget) { return; }

	PC->HUDWidget->ShowInteractionPrompt(Message);

	TWeakObjectPtr<UZP_HUDWidget> HUD = PC->HUDWidget;
	GetWorldTimerManager().SetTimer(HudMessageTimer, FTimerDelegate::CreateWeakLambda(this,
		[HUD]()
		{
			if (HUD.IsValid()) { HUD->HideInteractionPrompt(); }
		}),
		FMath::Max(AZP_UnlockedMessageDuration, 0.1f), false);
}

void AZP_InteractDoor::UnlockWithKey(ACharacter* Interactor, bool bViaKeyItem)
{
	bAZP_Locked = false;

	if (AZP_UnlockSound)
	{
		UZP_SFXStatics::PlaySFXAttached(AZP_UnlockSound, GetSFXAttachComp(),
			AZP_UnlockSoundCarry, AZP_UnlockSoundVolume);
	}

	if (bViaKeyItem && bAZP_ConsumeKeyOnUnlock)
	{
		ConsumeKeyItem(Interactor);
	}

	ShowTimedHudMessage(Interactor, AZP_UnlockedMessage);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InteractDoor %s: UNLOCKED %s%s"),
		*GetName(),
		bViaKeyItem ? TEXT("with key item") : TEXT("from the free side"),
		(bViaKeyItem && bAZP_ConsumeKeyOnUnlock) ? TEXT(" (consumed)") : TEXT(""));
}

void AZP_InteractDoor::ConsumeKeyItem(ACharacter* Interactor)
{
	UObject* TargetDA = AZP_RequiredItem.LoadSynchronous();
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Interactor);
	UActorComponent* InvComp = Grace ? Grace->MoonvilleInventoryComp.Get() : nullptr;
	if (!TargetDA || !InvComp) return;

	UFunction* RemoveFunc = InvComp->FindFunction(FName("RemoveItemByDataAsset"));
	if (RemoveFunc)
	{
		struct { UObject* AZP_ItemDataAsset; int32 AmountToRemove; } Params;
		Params.AZP_ItemDataAsset = TargetDA;
		Params.AmountToRemove = 1;
		InvComp->ProcessEvent(RemoveFunc, &Params);
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
