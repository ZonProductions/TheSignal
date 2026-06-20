// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_MarcusBodySpineBendAnimInstance
 *
 * Purpose: Post-process AnimInstance for the visible CCMH body (MarcusBody).
 *          When the camera pitches below a threshold (typically during a
 *          Kinemation reload/switch dive), bends the spine forward in component
 *          space so the camera always meets the OUTSIDE of the chest and never
 *          reveals the interior cavity. Bend distributes across CCMH's 5
 *          spine vertebrae weighted .10/.15/.20/.25/.30 toward the head.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points: assigned as MarcusBody's override post-process
 *          AnimBP via USkeletalMeshComponent::SetOverridePostProcessAnimBP in
 *          AZP_GraceCharacter::SetupMarcusAppearance. Tunables
 *          (SpineBendThresholdDeg / SpineBendMaxDeg / SpineBendInterpSpeed /
 *          SpineBendBoneLocalAxis) read from the owning AZP_GraceCharacter
 *          each frame so dev can dial in Details -> Appearance|SpineBend.
 *
 * Dependencies: AZP_GraceCharacter, UCameraComponent, FReferenceSkeleton.
 */

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ZP_MarcusBodySpineBendAnimInstance.generated.h"

UCLASS()
class THESIGNAL_API UZP_MarcusBodySpineBendAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativePostEvaluateAnimation() override;

private:
	/** Smoothed bend (degrees) carried across frames so easing works. */
	float SpineBendCurrent = 0.f;

	/** Rotate every bone in RootBone's subtree (inclusive) about RootBone's CS
	 *  position by DeltaRotCS. Children's CS transforms are recomputed so the
	 *  whole subtree rigidly rotates with the parent. */
	void RotateBoneTreeCS(TArray<FTransform>& CS, const struct FReferenceSkeleton& RefSkel,
	                      int32 RootBone, const FQuat& DeltaRotCS) const;
};
