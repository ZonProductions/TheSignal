// Copyright The Signal. All Rights Reserved.

#include "ZP_MeleeHandsAnimInstance.h"
#include "ZP_GraceCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "Engine/Engine.h"

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

	TArray<FTransform>& CS = Mesh->GetEditableComponentSpaceTransforms();
	if (CS.Num() == 0) return;
	const FReferenceSkeleton& RefSkel = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	// Build the hand_l / hand_r subtrees once (ascending index == parent-first).
	if (HandSubtree.Num() == 0)
	{
		const int32 HL = Mesh->GetBoneIndex(FName("hand_l"));
		const int32 HR = Mesh->GetBoneIndex(FName("hand_r"));
		for (int32 i = 0; i < RefSkel.GetNum() && i < CS.Num(); ++i)
		{
			int32 b = i;
			while (b != INDEX_NONE)
			{
				if (b == HL || b == HR) { HandSubtree.Add(i); break; }
				b = RefSkel.GetParentIndex(b);
			}
		}
	}

	const bool bBlocking = Grace->IsBlockingNow();

	// --- PROBE: this layer is now confirmed running on MeleeViewMesh. Log hand_r CS
	// (the bone the pipe hangs off) so we can see if/when it goes bad. Throttled. ---
	const int32 HandRIdx = Mesh->GetBoneIndex(FName("hand_r"));
	const bool bProbeTick = (GFrameCounter % 30 == 0);
	if (bProbeTick && HandRIdx != INDEX_NONE && HandRIdx < CS.Num())
	{
		const FTransform& HR = CS[HandRIdx];
		UE_LOG(LogTemp, Warning,
			TEXT("[HANDPROBE] run=1 blocking=%d subtree=%d cache=%d hand_r_loc=%s scale=%s nan=%d"),
			bBlocking ? 1 : 0, HandSubtree.Num(), bHasGripCache ? 1 : 0,
			*HR.GetLocation().ToString(), *HR.GetScale3D().ToString(), HR.ContainsNaN() ? 1 : 0);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage((uint64)1001, 1.0f,
				bBlocking ? FColor::Orange : FColor::Green,
				FString::Printf(TEXT("[HANDPROBE] block=%d cache=%d hand_r=%s"),
					bBlocking ? 1 : 0, bHasGripCache ? 1 : 0, *CS[HandRIdx].GetLocation().ToString()));
		}
	}

	if (!bBlocking)
	{
		// Capture the live non-block grip (parent-relative). The last snapshot before a
		// block is the idle pose gripping the pipe.
		if (CachedLocal.Num() != CS.Num()) CachedLocal.SetNum(CS.Num());
		bool bClean = true;
		for (int32 i : HandSubtree)
		{
			const int32 p = RefSkel.GetParentIndex(i);
			const FTransform L = (p != INDEX_NONE && p < CS.Num())
				? CS[i].GetRelativeTransform(CS[p]) : CS[i];
			if (L.ContainsNaN()) { bClean = false; break; }
			CachedLocal[i] = L;
		}
		if (bClean) bHasGripCache = true;
	}
	else if (bHasGripCache && CachedLocal.Num() == CS.Num())
	{
		// Replay the cached grip onto the block pose. Compute the whole subtree into a
		// temp buffer and ONLY commit if every transform is finite — a single NaN would
		// otherwise fly hand_r (and the pipe attached to it) off-screen.
		TArray<FTransform> NewCS = CS;
		bool bClean = true;
		for (int32 i : HandSubtree)
		{
			const int32 p = RefSkel.GetParentIndex(i);
			const FTransform T = (p != INDEX_NONE && p < NewCS.Num())
				? CachedLocal[i] * NewCS[p] : CachedLocal[i];
			if (T.ContainsNaN()) { bClean = false; break; }
			NewCS[i] = T;
		}
		if (bClean)
		{
			for (int32 i : HandSubtree) CS[i] = NewCS[i];
		}
		if (bProbeTick)
		{
			UE_LOG(LogTemp, Warning, TEXT("[HANDPROBE] BLOCK replay committed=%d hand_r_after=%s"),
				bClean ? 1 : 0,
				(HandRIdx != INDEX_NONE && HandRIdx < CS.Num()) ? *CS[HandRIdx].GetLocation().ToString() : TEXT("?"));
		}
	}

	// Dev-tunable rigid translation on top (block = base grip + optional block extra).
	const FVector OffL = Grace->GetActiveMeleeHandOffset(false);
	const FVector OffR = Grace->GetActiveMeleeHandOffset(true);
	if (!OffL.IsNearlyZero())
	{
		ShiftBoneTreeCS(CS, RefSkel, Mesh->GetBoneIndex(FName("hand_l")), OffL);
	}
	if (!OffR.IsNearlyZero())
	{
		ShiftBoneTreeCS(CS, RefSkel, Mesh->GetBoneIndex(FName("hand_r")), OffR);
	}
}
