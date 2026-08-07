// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_MarcusBodyAnimInstance
 *
 * Purpose: SingleNode-compatible AnimInstance for the visible Marcus (CCMH) body that adds a
 *          procedural LOOK-DOWN SPINE CURL in NativePostEvaluateAnimation, so the janitor chest
 *          comes into view under the camera (dev 2026-08-07 try-out: "I kinda like it"). Derives
 *          from UAnimSingleNodeInstance so every existing GetSingleNodeInstance() call site
 *          (locomotion clip swaps, the grab machine) keeps working unchanged.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points: none — all knobs live on AZP_GraceCharacter (Appearance|ChestBend);
 *          bAZP_MarcusChestBend=false disables the bend AND the class install (unhook switch).
 *
 * Dependencies: AZP_GraceCharacter (knob source + grab-beat marker via MarcusHead visibility).
 */

#include "CoreMinimal.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "ZP_MarcusBodyAnimInstance.generated.h"

UCLASS(Transient)
class THESIGNAL_API UZP_MarcusBodyAnimInstance : public UAnimSingleNodeInstance
{
	GENERATED_BODY()

protected:
	virtual void NativePostEvaluateAnimation() override;

private:
	/** Warn once (not per frame) if none of the configured bend bones resolve. */
	bool bWarnedNoBones = false;
};
