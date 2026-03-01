// Copyright The Signal. All Rights Reserved.

#include "ZP_GracePlayerAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

void UZP_GracePlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Auto-discover source mesh (hidden Mesh) from owning character
	if (!SourceMeshComponent)
	{
		if (ACharacter* Char = Cast<ACharacter>(TryGetPawnOwner()))
		{
			SourceMeshComponent = Char->GetMesh();
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] GracePlayerAnimInstance: Source mesh: %s"),
				SourceMeshComponent ? *SourceMeshComponent->GetName() : TEXT("NONE"));
		}
	}

	BuildBoneMap();
}

void UZP_GracePlayerAnimInstance::BuildBoneMap()
{
	bBoneMapBuilt = false;
	BoneIndexMap.Empty();

	USkeletalMeshComponent* TargetMesh = GetSkelMeshComponent();
	if (!SourceMeshComponent || !TargetMesh)
	{
		return;
	}

	// Skeletal mesh assets may not be assigned yet during early initialization
	if (!SourceMeshComponent->GetSkeletalMeshAsset() || !TargetMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	const FReferenceSkeleton& SourceRefSkel = SourceMeshComponent->GetSkeletalMeshAsset()->GetRefSkeleton();
	const FReferenceSkeleton& TargetRefSkel = TargetMesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	const int32 TargetBoneCount = TargetRefSkel.GetNum();
	BoneIndexMap.SetNum(TargetBoneCount);

	// Find spine_01 in target skeleton — everything at or above spine_01 is upper body.
	// Upper body is driven by Kinemation; we only copy lower body (pelvis, legs).
	const int32 SpineBoneIdx = TargetRefSkel.FindBoneIndex(FName(TEXT("spine_01")));

	int32 MappedCount = 0;
	int32 SkippedUpper = 0;
	for (int32 TargetIdx = 0; TargetIdx < TargetBoneCount; ++TargetIdx)
	{
		// Skip spine_01 and all its descendants (upper body = Kinemation territory)
		if (SpineBoneIdx != INDEX_NONE && IsDescendantOf(TargetRefSkel, TargetIdx, SpineBoneIdx))
		{
			BoneIndexMap[TargetIdx] = INDEX_NONE;
			++SkippedUpper;
			continue;
		}

		const FName BoneName = TargetRefSkel.GetBoneName(TargetIdx);
		const int32 SourceIdx = SourceRefSkel.FindBoneIndex(BoneName);
		BoneIndexMap[TargetIdx] = SourceIdx;
		if (SourceIdx != INDEX_NONE)
		{
			++MappedCount;
		}
	}

	bBoneMapBuilt = true;
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] GracePlayerAnimInstance: Bone map built — %d/%d mapped, %d skipped (upper body)"),
		MappedCount, TargetBoneCount, SkippedUpper);
}

void UZP_GracePlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Re-discover source if lost
	if (!SourceMeshComponent)
	{
		if (ACharacter* Char = Cast<ACharacter>(TryGetPawnOwner()))
		{
			SourceMeshComponent = Char->GetMesh();
			if (SourceMeshComponent)
			{
				BuildBoneMap();
			}
		}
	}
}

void UZP_GracePlayerAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	// This runs AFTER AnimGraph evaluation, BEFORE bone transform finalization.
	// Modifications here are part of the normal animation pipeline — no double-buffer issues.
	CopyBonesFromSource();
}

void UZP_GracePlayerAnimInstance::CopyBonesFromSource()
{
	if (!SourceMeshComponent)
	{
		return;
	}

	// Lazy-build bone map on first call (NativeInitializeAnimation fires too early)
	if (!bBoneMapBuilt)
	{
		BuildBoneMap();
		if (!bBoneMapBuilt)
		{
			return;
		}
	}

	USkeletalMeshComponent* TargetMesh = GetSkelMeshComponent();
	if (!TargetMesh)
	{
		return;
	}

	const TArray<FTransform>& SourceCSTransforms = SourceMeshComponent->GetComponentSpaceTransforms();
	if (SourceCSTransforms.Num() == 0)
	{
		return;
	}

	TArray<FTransform>& TargetCSTransforms = TargetMesh->GetEditableComponentSpaceTransforms();
	if (TargetCSTransforms.Num() == 0)
	{
		return;
	}

	int32 CopiedCount = 0;
	for (int32 TargetIdx = 0; TargetIdx < BoneIndexMap.Num() && TargetIdx < TargetCSTransforms.Num(); ++TargetIdx)
	{
		const int32 SourceIdx = BoneIndexMap[TargetIdx];
		if (SourceIdx != INDEX_NONE && SourceIdx < SourceCSTransforms.Num())
		{
			TargetCSTransforms[TargetIdx] = SourceCSTransforms[SourceIdx];
			++CopiedCount;
		}
	}

	// NO FinalizeBoneTransform() — the engine does it after NativePostEvaluateAnimation.
	// Our modifications are included in the normal finalization pass.

	// Log every 300 frames
	static int32 FrameCounter = 0;
	if (++FrameCounter % 300 == 1)
	{
		const int32 TestBoneIdx = SourceMeshComponent->GetBoneIndex(FName(TEXT("foot_l")));
		FVector SrcFootPos = (TestBoneIdx != INDEX_NONE && TestBoneIdx < SourceCSTransforms.Num())
			? SourceCSTransforms[TestBoneIdx].GetTranslation() : FVector::ZeroVector;

		UE_LOG(LogTemp, Log, TEXT("[BoneCopy] PostEval: Copied %d lower-body bones | Source foot_l: (%.1f,%.1f,%.1f)"),
			CopiedCount, SrcFootPos.X, SrcFootPos.Y, SrcFootPos.Z);
	}
}

bool UZP_GracePlayerAnimInstance::IsDescendantOf(const FReferenceSkeleton& RefSkel, int32 BoneIdx, int32 AncestorIdx) const
{
	int32 Current = BoneIdx;
	while (Current != INDEX_NONE)
	{
		if (Current == AncestorIdx) return true;
		Current = RefSkel.GetParentIndex(Current);
	}
	return false;
}
