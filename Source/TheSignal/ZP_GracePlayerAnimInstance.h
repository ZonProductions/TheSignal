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
};
