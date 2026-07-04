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
 *          (AZP_SpineBendThresholdDeg / AZP_SpineBendMaxDeg / AZP_SpineBendInterpSpeed /
 *          AZP_SpineBendBoneLocalAxis) read from the owning AZP_GraceCharacter
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

	/** Per-vertebra distribution of the total look-down spine bend across CCMH spine_01..spine_05, weighted toward the head so the chest closes to the camera faster than the waist. */
	UPROPERTY(EditAnywhere, Category = "Appearance|SpineBend")
	float AZP_SpineBendWeights[5] = { 0.10f, 0.15f, 0.20f, 0.25f, 0.30f };

private:
	/** Smoothed bend (degrees) carried across frames so easing works. */
	float SpineBendCurrent = 0.f;

	/** Rotate every bone in RootBone's subtree (inclusive) about RootBone's CS
	 *  position by DeltaRotCS. Children's CS transforms are recomputed so the
	 *  whole subtree rigidly rotates with the parent. */
	void RotateBoneTreeCS(TArray<FTransform>& CS, const struct FReferenceSkeleton& RefSkel,
	                      int32 RootBone, const FQuat& DeltaRotCS) const;
};
