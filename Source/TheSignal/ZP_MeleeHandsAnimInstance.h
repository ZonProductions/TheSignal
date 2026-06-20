// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_MeleeHandsAnimInstance
 *
 * Purpose: Post-process AnimInstance for the melee weapon-arm view-model. Runs AFTER
 *          the SingleNode melee clip and rigidly shifts hand_l / hand_r (and their
 *          finger children) by dev-tunable offsets so each hand grips the weapon
 *          cleanly. Different offsets for the idle/swing pose vs the block pose.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points: assign as the MeleeViewMesh post-process AnimBP
 *          (SetOverridePostProcessAnimBP). Offsets are read from the owning
 *          AZP_GraceCharacter each frame (GetActiveMeleeHandOffset).
 *
 * Dependencies: AZP_GraceCharacter.
 */

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ZP_MeleeHandsAnimInstance.generated.h"

UCLASS()
class THESIGNAL_API UZP_MeleeHandsAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativePostEvaluateAnimation() override;

private:
	/** Shift a bone + all its descendants by DeltaCS (component space) so the hand
	 *  moves rigidly (fingers keep their shape). */
	void ShiftBoneTreeCS(TArray<FTransform>& CS, const struct FReferenceSkeleton& RefSkel, int32 RootBone, const FVector& DeltaCS);

	/** hand_l/hand_r + finger descendants, ascending index (parent-first). Built once. */
	TArray<int32> HandSubtree;

	/** Live snapshot of the non-block grip as parent-relative (local) transforms,
	 *  captured every non-block frame and replayed during a block. */
	TArray<FTransform> CachedLocal;

	/** True once CachedLocal holds a valid non-block grip snapshot. */
	bool bHasGripCache = false;
};
