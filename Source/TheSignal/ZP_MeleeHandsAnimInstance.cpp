// Copyright The Signal. All Rights Reserved.

#include "ZP_MeleeHandsAnimInstance.h"
#include "ZP_GraceCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

void UZP_MeleeHandsAnimInstance::ShiftBoneTreeCS(TArray<FTransform>& CS, const FReferenceSkeleton& RefSkel, int32 RootBone, const FVector& DeltaCS)
{
	if (RootBone == INDEX_NONE) return;
	const int32 NumBones = RefSkel.GetNum();
	for (int32 i = 0; i < NumBones && i < CS.Num(); ++i)
	{
		// Is RootBone an ancestor of (or equal to) bone i?
		int32 b = i;
		bool bUnder = false;
		while (b != INDEX_NONE)
		{
			if (b == RootBone) { bUnder = true; break; }
			b = RefSkel.GetParentIndex(b);
		}
		if (bUnder)
		{
			CS[i].AddToTranslation(DeltaCS);
		}
	}
}

void UZP_MeleeHandsAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Mesh || !Mesh->GetSkeletalMeshAsset()) return;

	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Mesh->GetOwner());
	if (!Grace) return;

	const FVector OffL = Grace->GetActiveMeleeHandOffset(false);
	const FVector OffR = Grace->GetActiveMeleeHandOffset(true);
	if (OffL.IsNearlyZero() && OffR.IsNearlyZero()) return;

	TArray<FTransform>& CS = Mesh->GetEditableComponentSpaceTransforms();
	if (CS.Num() == 0) return;
	const FReferenceSkeleton& RefSkel = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	if (!OffL.IsNearlyZero())
	{
		ShiftBoneTreeCS(CS, RefSkel, Mesh->GetBoneIndex(FName("hand_l")), OffL);
	}
	if (!OffR.IsNearlyZero())
	{
		ShiftBoneTreeCS(CS, RefSkel, Mesh->GetBoneIndex(FName("hand_r")), OffR);
	}
}
