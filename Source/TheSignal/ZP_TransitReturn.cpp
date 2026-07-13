// Copyright The Signal. All Rights Reserved.

#include "ZP_TransitReturn.h"
#include "ZP_Elevator.h"
#include "ZP_TransitLocation.h"
#include "ZP_GraceCharacter.h"
#include "ZP_ObjectiveSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"

AZP_TransitReturn::AZP_TransitReturn()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
	ButtonMesh->SetupAttachment(Root);
	ButtonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Default LARGE/tall so it covers the whole elevator-shaft opening at this floor with no tuning.
	// BoxExtent is half-size: 2000 Z half-extent = 40m of vertical reach up & down the shaft.
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetBoxExtent(FVector(250.f, 250.f, 2000.f));
	InteractionVolume->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);

	// Dim indicator at the button (same visual scale as the ObjectiveContainer status light).
	// Starts OFF — BeginPlay turns it on once the objective gate is met.
	IndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("IndicatorLight"));
	IndicatorLight->SetupAttachment(ButtonMesh);
	IndicatorLight->SetRelativeLocation(FVector(10.f, 0.f, 0.f));
	IndicatorLight->SetIntensity(300.f);
	IndicatorLight->SetAttenuationRadius(150.f);
	IndicatorLight->SetCastShadows(false);
	IndicatorLight->SetLightColor(FLinearColor::White);
	IndicatorLight->SetVisibility(false);

	// Default press beep (overridable per BP/instance).
	static ConstructorHelpers::FObjectFinder<USoundBase> PressSoundFinder(
		TEXT("/Game/Audio/Elevator/SFX_Elevator_Beep.SFX_Elevator_Beep"));
	if (PressSoundFinder.Succeeded()) { AZP_PressSound = PressSoundFinder.Object; }
}

void AZP_TransitReturn::BeginPlay()
{
	Super::BeginPlay();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_TransitReturn::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_TransitReturn::OnOverlapEnd);

	// Cache the nearest stop marker now (used only when AZP_ReturnLocation isn't explicitly set).
	if (!AZP_ReturnLocation && bAZP_AutoFindNearestLocation)
	{
		AZP_TransitLocation* Nearest = nullptr;
		float NearestDistSq = TNumericLimits<float>::Max();
		const FVector MyLoc = GetActorLocation();

		for (TActorIterator<AZP_TransitLocation> It(GetWorld()); It; ++It)
		{
			const float DistSq = FVector::DistSquared(MyLoc, It->GetActorLocation());
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = *It;
			}
		}

		CachedNearestLocation = Nearest;

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] TransitReturn %s: nearest TransitLocation = %s"),
			*GetName(), Nearest ? *Nearest->GetName() : TEXT("none found"));
	}

	// Track the car so the indicator can go green while it's parked at this floor.
	if (AZP_LinkedElevator)
	{
		AZP_LinkedElevator->OnElevatorArrived.AddDynamic(this, &AZP_TransitReturn::OnLinkedElevatorArrived);
		AZP_LinkedElevator->OnElevatorDeparted.AddDynamic(this, &AZP_TransitReturn::OnLinkedElevatorDeparted);
		bCarAtThisFloor = !AZP_LinkedElevator->IsMoving() && IsCarAtThisFloor();
	}

	InitObjectiveGate();
	UpdateIndicatorLight();
}

void AZP_TransitReturn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UZP_ObjectiveSubsystem* Obj = GI->GetSubsystem<UZP_ObjectiveSubsystem>())
		{
			Obj->OnFlagSet.RemoveDynamic(this, &AZP_TransitReturn::OnObjectiveGateFired);
			Obj->OnObjectiveCompleted.RemoveDynamic(this, &AZP_TransitReturn::OnObjectiveGateFired);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AZP_TransitReturn::InitObjectiveGate()
{
	if (AZP_RequiredObjective == NAME_None)
	{
		bPowered = true;
		return;
	}

	UGameInstance* GI = GetGameInstance();
	UZP_ObjectiveSubsystem* Obj = GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr;
	if (!Obj)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitReturn %s: gate '%s' set but no objective subsystem — staying unpowered."),
			*GetName(), *AZP_RequiredObjective.ToString());
		return;
	}

	// Matches EITHER a progression flag OR a completed main objective (same rule as
	// AZP_InteractDoor::AZP_ObjectiveOverride) — covers BP_ObjectiveContainer unlock flags.
	if (Obj->HasFlag(AZP_RequiredObjective) || Obj->IsObjectiveComplete(AZP_RequiredObjective))
	{
		bPowered = true;
		return;
	}

	Obj->OnFlagSet.AddDynamic(this, &AZP_TransitReturn::OnObjectiveGateFired);
	Obj->OnObjectiveCompleted.AddDynamic(this, &AZP_TransitReturn::OnObjectiveGateFired);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] TransitReturn %s: unpowered — armed on objective gate '%s'"),
		*GetName(), *AZP_RequiredObjective.ToString());
}

void AZP_TransitReturn::OnObjectiveGateFired(FName Id)
{
	if (Id != AZP_RequiredObjective || bPowered) return;

	bPowered = true;
	UpdateIndicatorLight();
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] TransitReturn %s: objective gate '%s' completed — elevator call POWERED"),
		*GetName(), *Id.ToString());
}

bool AZP_TransitReturn::IsCarAtThisFloor()
{
	if (!AZP_LinkedElevator) return false;
	AZP_TransitLocation* Loc = ResolveReturnLocation();
	if (!Loc) return false;
	return FMath::Abs(AZP_LinkedElevator->GetActorLocation().Z - Loc->GetActorLocation().Z) <= AZP_ArriveZMargin;
}

void AZP_TransitReturn::OnLinkedElevatorArrived(float RelativeZ)
{
	// Arrived at THIS floor -> parked color; arrived elsewhere -> back to idle.
	bCarAtThisFloor = IsCarAtThisFloor();
	UpdateIndicatorLight();
}

void AZP_TransitReturn::OnLinkedElevatorDeparted(float FromRelativeZ)
{
	bCarAtThisFloor = false;
	UpdateIndicatorLight();
}

void AZP_TransitReturn::UpdateIndicatorLight()
{
	if (!IndicatorLight) return;
	IndicatorLight->SetVisibility(bPowered);
	IndicatorLight->SetLightColor(bCarAtThisFloor ? AZP_ArrivedLightColor : AZP_ReadyLightColor);
}

AZP_TransitLocation* AZP_TransitReturn::ResolveReturnLocation()
{
	if (AZP_ReturnLocation) return AZP_ReturnLocation;
	return CachedNearestLocation.Get();
}

FText AZP_TransitReturn::GetInteractionPrompt_Implementation()
{
	return bPowered ? AZP_PromptText : AZP_UnpoweredPromptText;
}

void AZP_TransitReturn::OnInteract_Implementation(ACharacter* Interactor)
{
	CallElevatorHere();
}

void AZP_TransitReturn::CallElevatorHere()
{
	if (!bPowered)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] TransitReturn %s: pressed but UNPOWERED (gate '%s' not met) — ignoring."),
			*GetName(), *AZP_RequiredObjective.ToString());
		return;
	}

	if (!AZP_LinkedElevator)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitReturn %s: AZP_LinkedElevator is unset — cannot recall."), *GetName());
		return;
	}

	AZP_TransitLocation* Loc = ResolveReturnLocation();
	if (!Loc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitReturn %s: no AZP_ReturnLocation (and none auto-found) — cannot recall."), *GetName());
		return;
	}

	// Button acknowledge beep, at the button.
	if (AZP_PressSound)
	{
		UZP_SFXStatics::PlaySFXAttached(AZP_PressSound, ButtonMesh ? ButtonMesh.Get() : GetRootComponent(),
			AZP_PressSoundCarry, AZP_PressSoundVolume);
	}

	// Convert the floor marker's world Z into a move relative to the car's BeginPlay origin.
	const float RelZ = Loc->GetActorLocation().Z - AZP_LinkedElevator->GetOriginZ();
	AZP_LinkedElevator->MoveToRelativeZ(RelZ);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] TransitReturn %s: recalling elevator %s -> relative Z %.1f (loc=%s)"),
		*GetName(), *AZP_LinkedElevator->GetName(), RelZ, *Loc->GetName());
}

void AZP_TransitReturn::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	// Register so the E-press recalls the car. No HUD prompt — it's a physical call button.
	Grace->SetCurrentInteractable(this);
}

void AZP_TransitReturn::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->ClearCurrentInteractable(this);
}
