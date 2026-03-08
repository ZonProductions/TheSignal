// Copyright The Signal. All Rights Reserved.

#include "ZP_KeyPickup.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"

AZP_KeyPickup::AZP_KeyPickup()
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
	PickupGlow->SetLightColor(FLinearColor(0.9f, 0.85f, 0.5f)); // warm yellow
}

void AZP_KeyPickup::BeginPlay()
{
	Super::BeginPlay();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_KeyPickup::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_KeyPickup::OnOverlapEnd);
}

FText AZP_KeyPickup::GetInteractionPrompt_Implementation()
{
	return PromptText;
}

void AZP_KeyPickup::OnInteract_Implementation(ACharacter* Interactor)
{
	if (bPickedUp) return;

	UObject* ItemDA = ItemDataAsset.LoadSynchronous();
	if (!ItemDA)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KeyPickup %s: ItemDataAsset failed to load!"), *GetName());
		return;
	}

	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Interactor);
	if (!Grace || !Grace->MoonvilleInventoryComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KeyPickup %s: No Grace or no MoonvilleInventoryComp"),
			*GetName());
		return;
	}

	// Call AddItemSimple(ItemDA, Amount) on Moonville inventory
	UFunction* AddFunc = Grace->MoonvilleInventoryComp->FindFunction(FName("AddItemSimple"));
	if (AddFunc)
	{
		struct { UObject* ItemToAdd; int32 Amount; } Params;
		Params.ItemToAdd = ItemDA;
		Params.Amount = ItemAmount;
		Grace->MoonvilleInventoryComp->ProcessEvent(AddFunc, &Params);

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] KeyPickup %s: Added %s x%d to inventory"),
			*GetName(), *ItemDA->GetName(), ItemAmount);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KeyPickup %s: AddItemSimple NOT FOUND on inventory component"),
			*GetName());
		return;
	}

	bPickedUp = true;

	// Clear interaction prompt
	Grace->ClearCurrentInteractable(this);
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->HideInteractionPrompt();
	}

	Destroy();
}

void AZP_KeyPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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

void AZP_KeyPickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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
