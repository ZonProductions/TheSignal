// Copyright The Signal. All Rights Reserved.

#include "ZP_SavePoint.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AZP_SavePoint::AZP_SavePoint()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root scene component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Desk mesh — set the actual mesh in BP child
	DeskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeskMesh"));
	DeskMesh->SetupAttachment(Root);

	// Interaction trigger volume
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	InteractionVolume->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);

	// Monitor glow light
	MonitorGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("MonitorGlow"));
	MonitorGlow->SetupAttachment(Root);
	MonitorGlow->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	MonitorGlow->SetIntensity(500.f);
	MonitorGlow->SetAttenuationRadius(200.f);
	MonitorGlow->SetLightColor(FLinearColor(0.6f, 0.8f, 1.0f)); // soft blue-white
}

void AZP_SavePoint::BeginPlay()
{
	Super::BeginPlay();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_SavePoint::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_SavePoint::OnOverlapEnd);
}

FText AZP_SavePoint::GetInteractionPrompt_Implementation()
{
	return PromptText;
}

void AZP_SavePoint::OnInteract_Implementation(ACharacter* Interactor)
{
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] SavePoint %s: Activated by %s"),
		*GetName(), *Interactor->GetName());

	// Open save menu widget
	if (SaveMenuWidgetClass)
	{
		APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
		if (PC)
		{
			UUserWidget* SaveMenu = CreateWidget<UUserWidget>(PC, SaveMenuWidgetClass);
			if (SaveMenu)
			{
				SaveMenu->AddToViewport(100);

				// Call InitWidget(OperationType) on the EGUI save manager widget.
				// InitWidget is a custom event that sets up OperationsManager, refreshes file list, etc.
				// OperationType enum: 0 = Save, 1 = Load
				UFunction* InitWidgetFunc = SaveMenu->FindFunction(FName("InitWidget"));
				if (InitWidgetFunc)
				{
					struct FInitWidgetParams
					{
						uint8 OperationType;
					};
					FInitWidgetParams Params;
					Params.OperationType = 0; // Save
					SaveMenu->ProcessEvent(InitWidgetFunc, &Params);
					UE_LOG(LogTemp, Log, TEXT("[TheSignal] SavePoint: Called InitWidget on save menu"));
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[TheSignal] SavePoint: InitWidget function not found on widget"));
				}

				// Switch to UI input so player can interact with save menu
				FInputModeGameAndUI InputMode;
				InputMode.SetWidgetToFocus(SaveMenu->TakeWidget());
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				PC->SetInputMode(InputMode);
				PC->SetShowMouseCursor(true);

				UE_LOG(LogTemp, Log, TEXT("[TheSignal] SavePoint: Opened save menu"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] SavePoint %s: No SaveMenuWidgetClass set!"), *GetName());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
				TEXT("SavePoint: No SaveMenuWidgetClass assigned in BP defaults"));
		}
	}

	// Call the BP event for any additional logic
	OnSavePointActivated(Interactor);
}

void AZP_SavePoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	// Register as current interactable on the character
	Grace->SetCurrentInteractable(this);

	// Show interaction prompt on HUD
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->ShowInteractionPrompt(PromptText);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] SavePoint %s: Player entered range"), *GetName());
}

void AZP_SavePoint::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	// Unregister from character
	Grace->ClearCurrentInteractable(this);

	// Hide interaction prompt
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->HideInteractionPrompt();
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] SavePoint %s: Player left range"), *GetName());
}
