// Copyright The Signal. All Rights Reserved.

#include "ZP_PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

AZP_PlayerController::AZP_PlayerController()
{
	bShowMouseCursor = false;
}

void AZP_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (DefaultMappingContext)
	{
		AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] PlayerController: Applied default IMC (priority %d)."), DefaultMappingPriority);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] PlayerController: No DefaultMappingContext set! Input won't work."));
	}
}

void AZP_PlayerController::AddMappingContext(UInputMappingContext* Context, int32 Priority)
{
	if (!Context) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(Context, Priority);
	}
}

void AZP_PlayerController::RemoveMappingContext(UInputMappingContext* Context)
{
	if (!Context) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(Context);
	}
}
