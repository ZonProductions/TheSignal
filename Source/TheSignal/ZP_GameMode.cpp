// Copyright The Signal. All Rights Reserved.

#include "ZP_GameMode.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_GameState.h"

AZP_GameMode::AZP_GameMode()
{
	DefaultPawnClass = AZP_GraceCharacter::StaticClass();
	PlayerControllerClass = AZP_PlayerController::StaticClass();
	GameStateClass = AZP_GameState::StaticClass();

	// BP classes (BP_GraceCharacter, PC_Grace) are set via GM_TheSignal Blueprint CDO.
	// FClassFinder for Blueprint classes in constructors causes cascading loads during CDO construction.

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GameMode constructed — DefaultPawn: ZP_GraceCharacter, Controller: ZP_PlayerController"));
}
