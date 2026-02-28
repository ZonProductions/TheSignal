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

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GameMode constructed — DefaultPawn: ZP_GraceCharacter, Controller: ZP_PlayerController"));
}
