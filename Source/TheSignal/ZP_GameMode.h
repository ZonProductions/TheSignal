// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_GameMode
 *
 * Purpose: Base game mode for The Signal. Sets default pawn, player controller,
 *          and game state classes. Extended by GM_TheSignal Blueprint for
 *          prototype-level configuration.
 *
 * Owner Subsystem: Core
 *
 * Blueprint Extension Points:
 *   - Default class assignments overridable in GM_TheSignal Blueprint.
 *   - Virtual functions for match flow (future: narrative progression gates).
 *
 * Dependencies:
 *   - AZP_GraceCharacter (default pawn)
 *   - AZP_PlayerController (default controller)
 *   - AZP_GameState (default game state)
 */

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZP_GameMode.generated.h"

UCLASS(Blueprintable)
class THESIGNAL_API AZP_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZP_GameMode();
};
