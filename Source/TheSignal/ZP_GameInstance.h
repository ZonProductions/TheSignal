// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_GameInstance
 *
 * Purpose: Persistent game state across level loads. Owns subsystem lifecycle
 *          and will hold save data + narrative progression flags in v0.2+.
 *
 * Owner Subsystem: Core
 *
 * Blueprint Extension Points:
 *   - GI_TheSignal Blueprint extends this for prototype scripting.
 *   - Virtual Init/Shutdown for subsystem lifecycle.
 *
 * Dependencies:
 *   - UZP_EventBroadcaster (auto-registered GameInstanceSubsystem)
 */

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ZP_GameInstance.generated.h"

UCLASS(Blueprintable)
class THESIGNAL_API UZP_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;
};
