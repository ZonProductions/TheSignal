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
#include "ZP_ObjectiveSubsystem.h"
#include "Engine/GameInstance.h"

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
		// Re-arm the gameplay mapping context we suspended when the pause menu opened. Remove-then-add
		// guarantees exactly one instance no matter which path triggered the unpause (so the player is
		// never left stuck or with a duplicated context).
		if (DefaultMappingContext)
		{
			RemoveMappingContext(DefaultMappingContext);
			AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
		}
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		ActivePauseMenu.Reset();
	}

	return Result;
}

bool AZP_PlayerController::IsPauseMenuOpen() const
{
	return ActivePauseMenu.IsValid() && ActivePauseMenu->IsInViewport();
}

void AZP_PlayerController::ZP_ObjFlag(const FString& Flag)
{
	if (Flag.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ZP_ObjFlag usage: ZP_ObjFlag <flagName>  (e.g. reached_empty_floor)"));
		return;
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UZP_ObjectiveSubsystem* Obj = GI->GetSubsystem<UZP_ObjectiveSubsystem>())
		{
			Obj->SetFlag(FName(*Flag));
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_ObjFlag: set objective flag '%s'"), *Flag);
		}
	}
}

void AZP_PlayerController::ZP_ObjReset()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UZP_ObjectiveSubsystem* Obj = GI->GetSubsystem<UZP_ObjectiveSubsystem>())
		{
			Obj->ResetAllProgress();
		}
	}
}

void AZP_PlayerController::ZP_InvReset()
{
	if (UGameplayStatics::DoesSaveGameExist(TEXT("TheSignal_Inventory"), 0))
	{
		UGameplayStatics::DeleteGameInSlot(TEXT("TheSignal_Inventory"), 0);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_InvReset: cleared inventory snapshot — restart PIE for a fresh loadout."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_InvReset: no inventory snapshot to clear."));
	}
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

		// True pause for the player: SetPause alone left the pad's left-stick driving the character
		// "under" the menu and let gameplay actions steal the controller's menu-nav inputs. Removing the
		// gameplay mapping context (IMC_Grace) kills EVERY gameplay action regardless of input mode — the
		// exact fix the save menu uses. Restored in SetPause(false) on unpause.
		if (DefaultMappingContext)
		{
			RemoveMappingContext(DefaultMappingContext);
		}

		// GameAndUI (not UIOnly) and DON'T force focus to the root: the EGUI pause widget is a
		// CommonActivatableWidget that focuses its own nav target — clobbering it with the root breaks
		// controller navigation (proven on the save menu). Gameplay is already dead via the IMC removal.
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		SetShowMouseCursor(true);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Pause menu opened (gameplay input suspended)."));
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

void AZP_PlayerController::DumpUIInput()
{
	UEnhancedInputLocalPlayerSubsystem* Subsys =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsys)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UIProbe] No EnhancedInput subsystem"));
		return;
	}

	struct FProbe { const TCHAR* Name; const TCHAR* Path; };
	const FProbe Probes[] = {
		{ TEXT("CloseNotification"), TEXT("/Game/InventorySystemPro/Blueprints/Input/FirstTimePickup/IA_UI_InventoryCloseNotification.IA_UI_InventoryCloseNotification") },
		{ TEXT("Select(accept)"),    TEXT("/Game/InventorySystemPro/Blueprints/Input/InventoryMenu/IA_UI_InventorySelect.IA_UI_InventorySelect") },
		{ TEXT("Back"),              TEXT("/Game/InventorySystemPro/Blueprints/Input/InventoryMenu/IA_UI_InventoryBack.IA_UI_InventoryBack") },
		{ TEXT("Interact"),          TEXT("/Game/Core/Input/Actions/IA_Interact.IA_Interact") },
		// Gameplay actions: if these still have LIVE keys while a menu is open, IMC_Grace is still
		// active and stealing the pad — proving the gameplay context wasn't actually suppressed.
		{ TEXT("GAMEPLAY:Move"),     TEXT("/Game/Core/Input/Actions/IA_Move.IA_Move") },
		{ TEXT("GAMEPLAY:Jump/Dodge"), TEXT("/Game/Core/Input/Actions/IA_Jump.IA_Jump") },
	};

	UE_LOG(LogTemp, Warning, TEXT("[UIProbe] ===== active key mappings (a menu should be open) ====="));
	for (const FProbe& P : Probes)
	{
		UInputAction* IA = LoadObject<UInputAction>(nullptr, P.Path);
		if (!IA)
		{
			UE_LOG(LogTemp, Warning, TEXT("[UIProbe]   %-18s : ACTION ASSET NOT FOUND (%s)"), P.Name, P.Path);
			continue;
		}
		const TArray<FKey> Keys = Subsys->QueryKeysMappedToAction(IA);
		FString S;
		for (const FKey& K : Keys) { S += K.ToString() + TEXT(" "); }
		UE_LOG(LogTemp, Warning, TEXT("[UIProbe]   %-18s : %s"),
			P.Name, Keys.Num() ? *S : TEXT("(NONE — its mapping context is NOT active right now)"));
	}

	const bool bCursor = bShowMouseCursor;
	UE_LOG(LogTemp, Warning, TEXT("[UIProbe]   ShowMouseCursor=%d  InputModeNote=check whether UIOnly/GameAndUI"), bCursor ? 1 : 0);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, TEXT("[UIProbe] logged active UI key mappings — see Output Log"));
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
