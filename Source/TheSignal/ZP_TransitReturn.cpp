// Copyright The Signal. All Rights Reserved.

#include "ZP_TransitReturn.h"
#include "ZP_Elevator.h"
#include "ZP_TransitLocation.h"
#include "ZP_GraceCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
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
}

void AZP_TransitReturn::BeginPlay()
{
	Super::BeginPlay();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_TransitReturn::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_TransitReturn::OnOverlapEnd);

	// Cache the nearest stop marker now (used only when ReturnLocation isn't explicitly set).
	if (!ReturnLocation && bAutoFindNearestLocation)
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
}

AZP_TransitLocation* AZP_TransitReturn::ResolveReturnLocation()
{
	if (ReturnLocation) return ReturnLocation;
	return CachedNearestLocation.Get();
}

FText AZP_TransitReturn::GetInteractionPrompt_Implementation()
{
	return PromptText;
}

void AZP_TransitReturn::OnInteract_Implementation(ACharacter* Interactor)
{
	CallElevatorHere();
}

void AZP_TransitReturn::CallElevatorHere()
{
	if (!LinkedElevator)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitReturn %s: LinkedElevator is unset — cannot recall."), *GetName());
		return;
	}

	AZP_TransitLocation* Loc = ResolveReturnLocation();
	if (!Loc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitReturn %s: no ReturnLocation (and none auto-found) — cannot recall."), *GetName());
		return;
	}

	// Convert the floor marker's world Z into a move relative to the car's BeginPlay origin.
	const float RelZ = Loc->GetActorLocation().Z - LinkedElevator->GetOriginZ();
	LinkedElevator->MoveToRelativeZ(RelZ);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] TransitReturn %s: recalling elevator %s -> relative Z %.1f (loc=%s)"),
		*GetName(), *LinkedElevator->GetName(), RelZ, *Loc->GetName());
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
