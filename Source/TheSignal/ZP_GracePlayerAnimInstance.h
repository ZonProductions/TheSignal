// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_GracePlayerAnimInstance
 *
 * Purpose: C++ AnimInstance for Grace's VISIBLE PlayerMesh. Copies locomotion
 *          bone transforms from hidden Mesh by name in C++.
 *          CopyBonesFromSource() called from Character Tick (post-animation).
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Dependencies:
 *   - AZP_GraceCharacter (reads hidden Mesh reference)
 */

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ZP_GracePlayerAnimInstance.generated.h"

class USkeletalMeshComponent;

UCLASS()
class THESIGNAL_API UZP_GracePlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** The hidden Mesh to copy locomotion FROM. Auto-discovered from ACharacter::GetMesh(). */
	UPROPERTY(BlueprintReadWrite, Category = "Locomotion")
	TObjectPtr<USkeletalMeshComponent> SourceMeshComponent;

	/** Copy matching bone transforms from source mesh to this mesh.
	 *  Call AFTER animation evaluation (e.g. from Character Tick). */
	void CopyBonesFromSource();

	/** Snapshot current lower body bones and begin blending to next pose.
	 *  Call BEFORE switching the hidden Mesh animation. */
	void StartBoneBlend(float InterpSpeed);

	/** Trigger a melee swing overlay (applies bone rotations post-Kinemation). */
	void StartMeleeSwing(float Duration = 0.35f);

	/** Trigger a grenade throw overlay (applies bone rotations post-Kinemation). */
	void StartGrenadeThrow(float Duration = 0.4f);

	/** Trigger weapon switch arm-lower animation (both arms drop then raise). */
	void StartWeaponSwitch(float Duration = 0.5f);

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;

private:
	/** Target bone index → source bone index. INDEX_NONE = skip (upper body or no match). */
	TArray<int32> BoneIndexMap;
	void BuildBoneMap();
	bool bBoneMapBuilt = false;

	/** Returns true if BoneIdx == AncestorIdx or is a child/grandchild/etc. of AncestorIdx. */
	bool IsDescendantOf(const FReferenceSkeleton& RefSkel, int32 BoneIdx, int32 AncestorIdx) const;

	// --- Crouch Bone Blending ---
	/** Cached lower body bone transforms from before an anim switch. */
	TArray<FTransform> CachedLowerBodyBones;
	/** 0 = fully cached (old pose), 1 = fully source (new pose). */
	float BoneBlendAlpha = 1.0f;
	/** Interp speed for bone blending — matched to camera transition. */
	float BoneBlendInterpSpeed = 8.0f;

	// --- Weapon Action Overlays ---
	// Applied in NativePostEvaluateAnimation AFTER Kinemation evaluation.
	// Uses sine curves for smooth motion that overlays on existing pose.

	/** Elapsed time into melee swing (negative = inactive). */
	float MeleeSwingTime = -1.f;
	float MeleeSwingDuration = 0.35f;

	/** Elapsed time into grenade throw (negative = inactive). */
	float GrenadeThrowTime = -1.f;
	float GrenadeThrowDuration = 0.4f;

	/** Elapsed time into weapon switch (negative = inactive). */
	float WeaponSwitchTime = -1.f;
	float WeaponSwitchDuration = 0.5f;

	/** Apply additive rotation to a bone in component space. */
	void ApplyBoneRotationCS(TArray<FTransform>& CSTransforms, const FReferenceSkeleton& RefSkel,
		FName BoneName, const FRotator& AdditiveRotation);

	/** Apply additive translation to a bone in component space. */
	void ApplyBoneTranslationCS(TArray<FTransform>& CSTransforms, const FReferenceSkeleton& RefSkel,
		FName BoneName, const FVector& AdditiveTranslation);
};
