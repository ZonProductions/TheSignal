// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * ZP_GrabTypes
 *
 * Purpose: Shared types for the zombie grab/struggle system (Docs/Plan_GrabStruggle.md).
 *          The VICTIM (player) owns the phase machine; the GRABBER (enemy) mirrors each
 *          phase with its paired clip via IZP_Grabber.
 *
 * Owner Subsystem: Combat
 */

#include "CoreMinimal.h"
#include "ZP_GrabTypes.generated.h"

UENUM(BlueprintType)
enum class EZP_GrabPhase : uint8
{
	None,          // not grabbed
	Entry,         // paired Idle_To_Grab (0.6s) — the latch; zero damage (free-escape window)
	Munch,         // paired Grab_To_Munching loop — biting; damage ticks once per loop
	Wrestle,       // paired Grab_To_Wrestle loop — active mash struggle
	EscapeKick,    // paired Kick/Kicked (2.3s) — escape success outcome A
	EscapePush,    // paired Push/Pushed (2.3s) — escape success outcome B
	FailKnockdown, // FPP knockdown on the FP rig, camera rides the fall (1P again)
	GetUp          // get-up clip on the FP rig, then back to normal play
};

UENUM()
enum class EZP_GrabAttemptResult : uint8
{
	Grabbed,     // latched — victim froze itself and snapped to face the grabber
	Deflected,   // victim was BLOCKING — grabber should stagger and start its grab cooldown
	Unavailable  // can't be grabbed right now (immunity window, mid-dodge, menus, ladder, already grabbed, dead)
};
