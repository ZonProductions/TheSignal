// Copyright The Signal. All Rights Reserved.

#include "ZP_MapPickup.h"
#include "ZP_MapComponent.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"

AZP_MapPickup::AZP_MapPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(Root);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetBoxExtent(FVector(100.f, 100.f, 80.f));
	InteractionVolume->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);

	PickupGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("PickupGlow"));
	PickupGlow->SetupAttachment(Root);
	PickupGlow->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	PickupGlow->SetIntensity(300.f);
	PickupGlow->SetAttenuationRadius(150.f);
	PickupGlow->SetLightColor(FLinearColor(0.4f, 0.7f, 0.9f)); // cool blue for maps
}

void AZP_MapPickup::BeginPlay()
{
	Super::BeginPlay();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_MapPickup::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_MapPickup::OnOverlapEnd);
}

FText AZP_MapPickup::GetInteractionPrompt_Implementation()
{
	return PromptText;
}

void AZP_MapPickup::OnInteract_Implementation(ACharacter* Interactor)
{
	if (bPickedUp) return;

	UZP_MapComponent* MapComp = Interactor ? Interactor->FindComponentByClass<UZP_MapComponent>() : nullptr;
	if (!MapComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MapPickup %s: No MapComponent on interactor"), *GetName());
		return;
	}

	MapComp->DiscoverMap(AreaID);
	bPickedUp = true;

	// Clear interaction prompt
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Interactor);
	if (Grace)
	{
		Grace->ClearCurrentInteractable(this);
		AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
		if (PC && PC->HUDWidget)
		{
			PC->HUDWidget->HideInteractionPrompt();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] MapPickup %s: Discovered map for area '%s'"),
		*GetName(), *AreaID.ToString());

	Destroy();
}

void AZP_MapPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bPickedUp) return;

	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->SetCurrentInteractable(this);

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->ShowInteractionPrompt(PromptText);
	}
}

void AZP_MapPickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (bPickedUp) return;

	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->ClearCurrentInteractable(this);

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->HideInteractionPrompt();
	}
}
