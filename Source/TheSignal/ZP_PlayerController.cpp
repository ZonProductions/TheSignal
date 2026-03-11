// Copyright The Signal. All Rights Reserved.

#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "ZP_DialogueManager.h"
#include "ZP_DialogueWidget.h"
#include "ZP_MapWidget.h"
#include "ZP_InventoryTabWidget.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

AZP_PlayerController::AZP_PlayerController()
{
	bShowMouseCursor = false;
	// Enable input processing during pause so ESC can close the pause menu
	bShouldPerformFullTickWhenPaused = true;

	// Create DialogueManager component
	DialogueManager = CreateDefaultSubobject<UZP_DialogueManager>(TEXT("DialogueManager"));

	// --- Baked CDO defaults (replaces set_all_cdo.py) ---

	// Default Input Mapping Context
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCFinder(TEXT("/Game/Core/Input/IMC_Grace"));
	if (IMCFinder.Succeeded()) DefaultMappingContext = IMCFinder.Object;

	// Widget classes (InventoryTabWidgetClass, MapWidgetClass) are set via BP CDO
	// because FClassFinder for Widget Blueprints causes cascading loads during CDO construction.
}

void AZP_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Runtime fallback: load widget classes if not set by BP CDO
	if (!InventoryTabWidgetClass)
	{
		InventoryTabWidgetClass = LoadClass<UZP_InventoryTabWidget>(nullptr,
			TEXT("/Game/Blueprints/UI/WBP_InventoryTab.WBP_InventoryTab_C"));
	}
	if (!MapWidgetClass)
	{
		MapWidgetClass = LoadClass<UZP_MapWidget>(nullptr,
			TEXT("/Game/User_Interface/WBP_Map.WBP_Map_C"));
	}

	if (DefaultMappingContext)
	{
		AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] PlayerController: Applied default IMC (priority %d)."), DefaultMappingPriority);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] PlayerController: No DefaultMappingContext set! Input won't work."));
	}

	// Create gameplay HUD
	if (HUDWidgetClass && IsLocalController())
	{
		HUDWidget = CreateWidget<UZP_HUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] PlayerController: Created HUD widget %s"), *HUDWidgetClass->GetName());
		}
	}
	else if (!HUDWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] PlayerController: No HUDWidgetClass set — no gameplay HUD."));
	}

	// Create dialogue widget
	if (DialogueWidgetClass && IsLocalController())
	{
		DialogueWidget = CreateWidget<UZP_DialogueWidget>(this, DialogueWidgetClass);
		if (DialogueWidget)
		{
			DialogueWidget->AddToViewport(50); // Above HUD (0), below pause menu (200)
			if (DialogueManager)
			{
				DialogueWidget->BindToDialogueManager(DialogueManager);
			}
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] PlayerController: Created DialogueWidget %s"), *DialogueWidgetClass->GetName());
		}
	}

	// Create map widget
	if (MapWidgetClass && IsLocalController())
	{
		MapWidget = CreateWidget<UZP_MapWidget>(this, MapWidgetClass);
		if (MapWidget)
		{
			MapWidget->AddToViewport(100); // Above HUD (0), above dialogue (50), below pause (200)
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] MAP DEBUG: Created MapWidget from %s"), *MapWidgetClass->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[TheSignal] MAP DEBUG: CreateWidget returned NULL for MapWidgetClass %s"), *MapWidgetClass->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] MAP DEBUG: MapWidgetClass is %s, IsLocal=%d — MapWidget NOT created"),
			MapWidgetClass ? TEXT("set") : TEXT("NULL"), IsLocalController() ? 1 : 0);
	}

	// Create inventory tab widget (Map / Inventory / Notes)
	if (InventoryTabWidgetClass && IsLocalController())
	{
		InventoryTabWidget = CreateWidget<UZP_InventoryTabWidget>(this, InventoryTabWidgetClass);
		if (InventoryTabWidget)
		{
			InventoryTabWidget->AddToViewport(100); // Same Z as old map widget
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] PlayerController: Created InventoryTabWidget from %s"), *InventoryTabWidgetClass->GetName());
		}
	}
	else if (!InventoryTabWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] PlayerController: No InventoryTabWidgetClass set — Tab menu won't work."));
	}

	// Fade from black on every level start (covers both initial spawn and respawn reload)
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.5f, FLinearColor::Black, /*bShouldFadeAudio=*/false, /*bHoldWhenFinished=*/false);
	}
}

void AZP_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PauseAction)
		{
			EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &AZP_PlayerController::TogglePauseMenu);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] PlayerController: Bound PauseAction to TogglePauseMenu."));
		}
	}
}

bool AZP_PlayerController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate)
{
	const bool Result = Super::SetPause(bPause, CanUnpauseDelegate);

	// On unpause: always restore game input, regardless of who triggered it
	// (our ESC handler, EGUI's Continue button, etc.)
	if (!bPause)
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ActivePauseMenu.Reset();
	}

	return Result;
}

void AZP_PlayerController::TogglePauseMenu()
{
	// Menu open → close it
	if (ActivePauseMenu.IsValid() && ActivePauseMenu->IsInViewport())
	{
		ActivePauseMenu->RemoveFromParent();
		SetPause(false); // SetPause override handles input restoration
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Pause menu closed via ESC."));
		return;
	}

	// Menu closed → open it
	if (!PauseMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] PauseMenuWidgetClass not set — cannot open pause menu."));
		return;
	}

	UUserWidget* Menu = CreateWidget<UUserWidget>(this, PauseMenuWidgetClass);
	if (Menu)
	{
		Menu->AddToViewport(200);
		ActivePauseMenu = Menu;

		// Hide EGUI buttons we don't need — Collapsed removes from layout (no gaps)
		static const FName ButtonsToHide[] = {
			FName("SaveSavegameButton"),
			FName("PhotoModeBtn"),
			FName("CreditsBtn")
		};
		for (const FName& Name : ButtonsToHide)
		{
			if (UWidget* W = Menu->GetWidgetFromName(Name))
			{
				W->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		SetPause(true);
		SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(Menu->TakeWidget()));
		SetShowMouseCursor(true);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Pause menu opened."));
	}
}

// --- Death / Respawn ---

void AZP_PlayerController::OnPawnDied()
{
	DisableInput(this);

	// Fade to black with audio fade, hold at black
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.5f, FLinearColor::Black, /*bShouldFadeAudio=*/true, /*bHoldWhenFinished=*/true);
	}

	// Respawn after 1 second
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AZP_PlayerController::ExecuteRespawn, 1.0f, false);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] PlayerController: Pawn died — fading to black, respawn in 1s."));
}

void AZP_PlayerController::ExecuteRespawn()
{
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] PlayerController: Reloading level '%s' for respawn."), *LevelName);
	UGameplayStatics::OpenLevel(GetWorld(), FName(*LevelName));
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
