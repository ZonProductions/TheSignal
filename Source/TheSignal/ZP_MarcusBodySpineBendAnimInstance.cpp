// Copyright The Signal. All Rights Reserved.

#include "ZP_MarcusBodySpineBendAnimInstance.h"
#include "ZP_GraceCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

void UZP_MarcusBodySpineBendAnimInstance::RotateBoneTreeCS(
	TArray<FTransform>& CS, const FReferenceSkeleton& RefSkel,
	int32 RootBone, const FQuat& DeltaRotCS) const
{
	if (RootBone == INDEX_NONE || RootBone >= CS.Num()) return;
	const FVector Pivot = CS[RootBone].GetLocation();
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
			FTransform T = CS[i];
			const FVector Rel = T.GetLocation() - Pivot;
			T.SetLocation(Pivot + DeltaRotCS.RotateVector(Rel));
			T.SetRotation(DeltaRotCS * T.GetRotation());
			CS[i] = T;
		}
	}
}

void UZP_MarcusBodySpineBendAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Mesh || !Mesh->GetSkeletalMeshAsset()) return;

	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Mesh->GetOwner());
	if (!Grace || !Grace->FirstPersonCamera) return;

	// Look-down angle: positive when camera forward points below the horizon.
	// Camera forward Z = -1 when looking straight down -> LookDownDeg = +90.
	const float FwdZ = FMath::Clamp(Grace->FirstPersonCamera->GetForwardVector().Z, -1.f, 1.f);
	const float LookDownDeg = -FMath::RadiansToDegrees(FMath::Asin(FwdZ));

	const float Threshold = Grace->AZP_SpineBendThresholdDeg;
	const float MaxBend   = Grace->AZP_SpineBendMaxDeg;
	const float Speed     = FMath::Max(0.1f, Grace->AZP_SpineBendInterpSpeed);

	float TargetBend = 0.f;
	if (LookDownDeg > Threshold && MaxBend > KINDA_SMALL_NUMBER)
	{
		const float Range = FMath::Max(1.f, 90.f - Threshold);
		float t = FMath::Clamp((LookDownDeg - Threshold) / Range, 0.f, 1.f);
		t = t * t * (3.f - 2.f * t);  // smoothstep
		TargetBend = t * MaxBend;
	}

	// Suspended while grabbed: the body plays authored full-body struggle clips and the 3P
	// camera doesn't look through the chest — camera-derived bending only distorts the pose
	// (it kinked the neck of the leader-posed head, dev report 2026-07-02).
	if (Grace->GrabPhase != EZP_GrabPhase::None)
	{
		TargetBend = 0.f;
	}

	const float Dt = GetDeltaSeconds();
	SpineBendCurrent = FMath::FInterpTo(SpineBendCurrent, TargetBend, Dt, Speed);

	// Grab arm-clipping knobs (AZP_GraceCharacter -> Grab|Pose) apply during the visible 3P
	// grapple phases — dev dials out arm clipping against the Shambler live in PIE.
	const EZP_GrabPhase GP = Grace->GrabPhase;
	const bool b3PGrab =
		GP == EZP_GrabPhase::Entry || GP == EZP_GrabPhase::Munch || GP == EZP_GrabPhase::Wrestle ||
		GP == EZP_GrabPhase::EscapeKick || GP == EZP_GrabPhase::EscapePush;
	const bool bGrabPose = b3PGrab &&
		(!Grace->AZP_GrabArmLRotation.IsNearlyZero() || !Grace->AZP_GrabArmRRotation.IsNearlyZero());
	const bool bBend = !FMath::IsNearlyZero(SpineBendCurrent, 0.01f);
	if (!bBend && !bGrabPose) return;

	TArray<FTransform>& CS = Mesh->GetEditableComponentSpaceTransforms();
	if (CS.Num() == 0) return;
	const FReferenceSkeleton& RefSkel = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	if (bBend)
	{
		// CCMH has a 5-vertebra spine (spine_01..spine_05). Distribute the total bend
		// across them weighted toward the head so the chest closes to the camera faster
		// than the waist - looks natural and shuts the gap quickly.
		struct FSpineBend { const TCHAR* BoneName; float Weight; };
		const FSpineBend Bends[] = {
			{ TEXT("spine_01"), AZP_SpineBendWeights[0] },
			{ TEXT("spine_02"), AZP_SpineBendWeights[1] },
			{ TEXT("spine_03"), AZP_SpineBendWeights[2] },
			{ TEXT("spine_04"), AZP_SpineBendWeights[3] },
			{ TEXT("spine_05"), AZP_SpineBendWeights[4] },
		};

		const FVector LocalAxis = Grace->AZP_SpineBendBoneLocalAxis.GetSafeNormal();
		if (!LocalAxis.IsNearlyZero())
		{
			for (const FSpineBend& B : Bends)
			{
				const int32 Idx = Mesh->GetBoneIndex(FName(B.BoneName));
				if (Idx == INDEX_NONE || Idx >= CS.Num()) continue;
				const float Deg = SpineBendCurrent * B.Weight;
				if (FMath::IsNearlyZero(Deg)) continue;

				// Convert bone-LOCAL hinge axis to component space using this bone's CS
				// rotation, so the bend pivots around the bone's intrinsic right vector
				// regardless of where the body is rotated in the world.
				const FVector AxisCS = CS[Idx].GetRotation().RotateVector(LocalAxis);
				const FQuat Q(AxisCS, FMath::DegreesToRadians(Deg));
				RotateBoneTreeCS(CS, RefSkel, Idx, Q);
			}
		}
	}

	if (bGrabPose)
	{
		// Bone-LOCAL additive arm rotations: consistent knob behavior regardless of facing.
		struct FPoseFix { const TCHAR* BoneName; const FRotator* Rot; };
		const FPoseFix Fixes[] = {
			{ TEXT("upperarm_l"), &Grace->AZP_GrabArmLRotation },
			{ TEXT("upperarm_r"), &Grace->AZP_GrabArmRRotation },
		};
		for (const FPoseFix& F : Fixes)
		{
			if (F.Rot->IsNearlyZero()) continue;
			const int32 Idx = Mesh->GetBoneIndex(FName(F.BoneName));
			if (Idx == INDEX_NONE || Idx >= CS.Num()) continue;
			const FQuat BoneCS = CS[Idx].GetRotation();
			const FQuat DeltaCS = BoneCS * F.Rot->Quaternion() * BoneCS.Inverse();
			RotateBoneTreeCS(CS, RefSkel, Idx, DeltaCS);
		}
	}
}
