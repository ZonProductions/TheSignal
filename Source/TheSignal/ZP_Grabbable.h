// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * IZP_Grabbable / IZP_Grabber
 *
 * Purpose: The two sides of the grab/struggle handshake (Docs/Plan_GrabStruggle.md).
 *          - IZP_Grabbable: the VICTIM (player). An enemy calls TryBeginGrab; on Grabbed
 *            the victim freezes itself, snaps to face the grabber, and runs the phase
 *            machine (entry -> munch -> wrestle -> escape/fail).
 *          - IZP_Grabber: the ATTACKER (enemy behavior component). The victim's phase
 *            machine calls OnVictimGrabPhase at every transition so the enemy plays the
 *            matching paired clip / knockback / stun. Phase None = aborted (clean release,
 *            no outcome anims — victim died or the grab was broken by damage).
 *
 *          C++-only interfaces (CannotImplementInterfaceInBlueprint) — both sides are
 *          native. New enemy types get grabs by implementing IZP_Grabber and calling
 *          the victim's TryBeginGrab; no per-enemy code on the player.
 *
 * Owner Subsystem: Combat
 */

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ZP_GrabTypes.h"
#include "ZP_Grabbable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UZP_Grabbable : public UInterface
{
	GENERATED_BODY()
};

class THESIGNAL_API IZP_Grabbable
{
	GENERATED_BODY()

public:
	/** An enemy in range asks to latch on. Deflected = victim was blocking (stagger yourself,
	 *  start your cooldown). Unavailable = try again later (immunity/dodge/menus). */
	virtual EZP_GrabAttemptResult TryBeginGrab(AActor* Grabber) = 0;

	/** Grabber died / the grab was broken externally mid-grab: release immediately,
	 *  restore all state, play no outcome anims. */
	virtual void AbortGrab() = 0;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UZP_Grabber : public UInterface
{
	GENERATED_BODY()
};

class THESIGNAL_API IZP_Grabber
{
	GENERATED_BODY()

public:
	/** The victim's phase machine advanced — mirror it (paired clip, knockback, stun).
	 *  Receives Munch/Wrestle/EscapeKick/EscapePush/FailKnockdown, and None on abort. */
	virtual void OnVictimGrabPhase(EZP_GrabPhase NewPhase) = 0;
};
