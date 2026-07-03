// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_ShamblerGrabPoseAnimInstance
 *
 * Purpose: C++ PARENT class of ABP_Shambler (the main anim instance). While the behavior
 *          component is in the Grab state, NativePostEvaluateAnimation applies the dev-tunable
 *          bone-local rotations (UZP_ShamblerBehaviorComponent::GrabArmLRotation /
 *          GrabArmRRotation / GrabHeadRotation) on top of the paired grapple clips so arm
 *          clipping and the bite angle against Marcus can be dialed live in PIE. Inert in every
 *          other state.
 *
 * Owner Subsystem: EnemyAI
 *
 * Blueprint Extension Points: ABP_Shambler's graph (BS_Shambler -> DefaultSlot -> Output) is
 *          untouched — this class only adds the native post-evaluate overlay, the exact
 *          UZP_GracePlayerAnimInstance pattern. Knobs: component Details -> Shambler|Grab.
 *          DO NOT use this (or any graph-less C++ instance) as an override POST-PROCESS AnimBP:
 *          that replaced the pose path with ref pose and, at the necromorph mesh's 0.01 base
 *          scale, rendered the Shambler INVISIBLE (2026-07-02, DEAD ENDS).
 *
 * Dependencies: UZP_ShamblerBehaviorComponent (state + knobs), FReferenceSkeleton.
 *          Necromorph bones are mixamorig_* — the upper-arm bones are resolved once by name
 *          substring (contains "leftarm"/"rightarm", not "forearm").
 */

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ZP_ShamblerGrabPoseAnimInstance.generated.h"

class UZP_ShamblerBehaviorComponent;

UCLASS()
class THESIGNAL_API UZP_ShamblerGrabPoseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativePostEvaluateAnimation() override;

private:
	TWeakObjectPtr<UZP_ShamblerBehaviorComponent> Behavior;
	bool bResolved = false;
	int32 ArmLBone = INDEX_NONE;
	int32 ArmRBone = INDEX_NONE;
	int32 HeadBone = INDEX_NONE;

	/** Rigidly rotate RootBone's subtree (inclusive) about its CS position. */
	void RotateBoneTreeCS(TArray<FTransform>& CS, const struct FReferenceSkeleton& RefSkel,
	                      int32 RootBone, const FQuat& DeltaRotCS) const;
};
