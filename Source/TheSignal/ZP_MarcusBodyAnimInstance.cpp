// Copyright The Signal. All Rights Reserved.

#include "ZP_MarcusBodyAnimInstance.h"
#include "ZP_GraceCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Controller.h"

namespace
{
	/** True if BoneIdx is AncestorIdx or sits anywhere under it. */
	bool IsInSubtree(const FReferenceSkeleton& Ref, int32 BoneIdx, int32 AncestorIdx)
	{
		int32 P = BoneIdx;
		while (P != INDEX_NONE)
		{
			if (P == AncestorIdx) { return true; }
			P = Ref.GetParentIndex(P);
		}
		return false;
	}

	/** Rotate RootIdx and every descendant about RootIdx's own pivot (component space). */
	void RotateSubtreeAboutPivot(TArray<FTransform>& CS, const FReferenceSkeleton& Ref,
		int32 RootIdx, const FQuat& Q)
	{
		const FVector Pivot = CS[RootIdx].GetLocation();
		// RefSkeleton is parent-before-child: descendants always index higher.
		for (int32 i = RootIdx; i < CS.Num(); ++i)
		{
			if (i != RootIdx && !IsInSubtree(Ref, i, RootIdx))
			{
				continue;
			}
			CS[i].SetRotation(Q * CS[i].GetRotation());
			CS[i].SetLocation(Pivot + Q.RotateVector(CS[i].GetLocation() - Pivot));
		}
	}
}

void UZP_MarcusBodyAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	USkeletalMeshComponent* M = GetSkelMeshComponent();
	AZP_GraceCharacter* Grace = M ? Cast<AZP_GraceCharacter>(M->GetOwner()) : nullptr;
	if (!Grace || !M->GetSkeletalMeshAsset() || !Grace->bAZP_MarcusChestBend)
	{
		return;
	}
	// The grab beat shows the head and plays 3P victim clips on this mesh — a camera-pitch bend
	// there would deform the victim body against its authored clip. MarcusHead visibility IS the
	// grab-beat marker ("stays hidden except during the grab beat").
	if (Grace->MarcusHead && Grace->MarcusHead->IsVisible())
	{
		return;
	}
	AController* C = Grace->GetController();
	if (!C)
	{
		return;
	}

	// Look-down amount: control pitch is negative looking down. Ease the curl in between the
	// start/end pitches so level view = zero bend = zero change to normal play.
	const float Pitch = FRotator::NormalizeAxis(C->GetControlRotation().Pitch);
	const float Down = -Pitch;
	const float Start = Grace->AZP_MarcusChestBendStartPitch;
	const float End = FMath::Max(Grace->AZP_MarcusChestBendEndPitch, Start + 1.0f);
	const float Alpha = FMath::Clamp((Down - Start) / (End - Start), 0.0f, 1.0f);
	if (Alpha <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float TotalDeg = Grace->AZP_MarcusChestBendMaxDegrees * Alpha;

	TArray<FTransform>& CS = M->GetEditableComponentSpaceTransforms();
	if (CS.Num() == 0)
	{
		return;
	}
	const FReferenceSkeleton& Ref = M->GetSkeletalMeshAsset()->GetRefSkeleton();

	const TArray<FName>& Bones = Grace->AZP_MarcusChestBendBones;
	const TArray<float>& Weights = Grace->AZP_MarcusChestBendWeights;
	const int32 NeckIdx = Ref.FindBoneIndex(Grace->AZP_MarcusChestNeckBone);

	// ---------------------------------------------------------------- A) SWAY DAMP
	// Try-out killer #2: the loco clip animates the spine, and with the chest right at the lens
	// that sway reads as "upper chest bobbing". While bent, blend the bend bones' (and neck
	// chain's) LOCAL transforms toward ref pose — the chest stiffens onto the pelvis exactly as
	// much as it is in view. Arms/clavicles keep their animated locals and are re-composed under
	// the damped parents, so nothing tears.
	const float Damp = FMath::Clamp(Grace->AZP_MarcusChestSwayDamping, 0.0f, 1.0f) * Alpha;
	const int32 SubtreeRoot = (Bones.Num() > 0) ? Ref.FindBoneIndex(Bones[0]) : INDEX_NONE;
	if (Damp > KINDA_SMALL_NUMBER && SubtreeRoot != INDEX_NONE && SubtreeRoot < CS.Num())
	{
		const TArray<FTransform>& RefLocal = Ref.GetRefBonePose();
		const TArray<FTransform> Orig = CS; // animated pose, pre-damp — locals derive from THIS

		// Which bones get their locals damped: the bend chain + the neck chain.
		TSet<int32> DampSet;
		for (const FName& B : Bones)
		{
			const int32 Idx = Ref.FindBoneIndex(B);
			if (Idx != INDEX_NONE) { DampSet.Add(Idx); }
		}
		if (NeckIdx != INDEX_NONE)
		{
			for (int32 i = NeckIdx; i < CS.Num(); ++i)
			{
				if (IsInSubtree(Ref, i, NeckIdx)) { DampSet.Add(i); }
			}
		}

		for (int32 i = SubtreeRoot; i < CS.Num(); ++i)
		{
			if (!IsInSubtree(Ref, i, SubtreeRoot))
			{
				continue;
			}
			const int32 Parent = Ref.GetParentIndex(i);
			if (Parent == INDEX_NONE || Parent >= CS.Num())
			{
				continue;
			}
			// Local from the ORIGINAL animated pose, then recompose under the NEW parent.
			FTransform Local = Orig[i].GetRelativeTransform(Orig[Parent]);
			if (DampSet.Contains(i) && RefLocal.IsValidIndex(i))
			{
				FTransform Damped = Local;
				Damped.SetRotation(FQuat::Slerp(Local.GetRotation(), RefLocal[i].GetRotation(), Damp));
				Damped.SetLocation(FMath::Lerp(Local.GetLocation(), RefLocal[i].GetLocation(), Damp));
				Local = Damped;
			}
			CS[i] = Local * CS[Parent];
		}
	}

	// ---------------------------------------------------------------- B) BEND
	// Bend axis = the character's RIGHT expressed in this component's space (the component is
	// yaw-rotated -90; never assume a local axis). Positive angle about RIGHT = curl forward.
	const FVector CSAxis = M->GetComponentTransform()
		.InverseTransformVectorNoScale(Grace->GetActorRightVector()).GetSafeNormal();
	if (CSAxis.IsNearlyZero())
	{
		return;
	}

	float AppliedDeg = 0.0f;
	int32 Applied = 0;
	for (int32 s = 0; s < Bones.Num(); ++s)
	{
		const int32 BoneIdx = Ref.FindBoneIndex(Bones[s]);
		if (BoneIdx == INDEX_NONE || BoneIdx >= CS.Num())
		{
			continue;
		}
		const float Weight = Weights.IsValidIndex(s) ? Weights[s] : (1.0f / Bones.Num());
		const float Deg = TotalDeg * Weight;
		RotateSubtreeAboutPivot(CS, Ref, BoneIdx, FQuat(CSAxis, FMath::DegreesToRadians(Deg)));
		AppliedDeg += Deg;
		++Applied;
	}

	// ---------------------------------------------------------------- C) NECK STAYS UP
	// Try-out killer #1: the headless body's neck stump rode the full curl straight into the
	// lens. Counter-rotate the neck chain about its own pivot by the applied bend — the stump
	// keeps FOLLOWING the chest (attached, no tear) but keeps POINTING up, out of frame.
	const float Upright = FMath::Clamp(Grace->AZP_MarcusChestNeckUpright, 0.0f, 1.0f);
	if (Upright > KINDA_SMALL_NUMBER && AppliedDeg > 0.0f
		&& NeckIdx != INDEX_NONE && NeckIdx < CS.Num())
	{
		RotateSubtreeAboutPivot(CS, Ref, NeckIdx,
			FQuat(CSAxis, FMath::DegreesToRadians(-AppliedDeg * Upright)));
	}

	if (Applied == 0 && !bWarnedNoBones)
	{
		bWarnedNoBones = true;
		UE_LOG(LogTemp, Warning,
			TEXT("[TheSignal] MarcusBodyAnimInstance: none of the AZP_MarcusChestBendBones resolve on %s — chest bend inert."),
			*GetNameSafe(M->GetSkeletalMeshAsset()));
	}
}
