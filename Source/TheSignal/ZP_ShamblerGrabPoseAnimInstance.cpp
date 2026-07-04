// Copyright The Signal. All Rights Reserved.

#include "ZP_ShamblerGrabPoseAnimInstance.h"
#include "ZP_ShamblerBehaviorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

void UZP_ShamblerGrabPoseAnimInstance::RotateBoneTreeCS(
	TArray<FTransform>& CS, const FReferenceSkeleton& RefSkel,
	int32 RootBone, const FQuat& DeltaRotCS) const
{
	if (RootBone == INDEX_NONE || RootBone >= CS.Num()) return;
	const FVector Pivot = CS[RootBone].GetLocation();
	const int32 NumBones = RefSkel.GetNum();
	for (int32 i = 0; i < NumBones && i < CS.Num(); ++i)
	{
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

void UZP_ShamblerGrabPoseAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Mesh || !Mesh->GetSkeletalMeshAsset()) return;

	if (!Behavior.IsValid())
	{
		if (AActor* Owner = Mesh->GetOwner())
		{
			Behavior = Owner->FindComponentByClass<UZP_ShamblerBehaviorComponent>();
		}
		if (!Behavior.IsValid()) return;
	}
	const bool bGrab = (Behavior->State == EShamblerState::Grab);
	const float HeadStab = FMath::Clamp(Behavior->AZP_RunHeadStabilize, 0.f, 1.f);
	const bool bRunStabilize = !bGrab && HeadStab > 0.f && Behavior->IsRunBurstActive();
	if (!bGrab && !bRunStabilize) return;

	const FRotator& RotL = Behavior->AZP_GrabArmLRotation;
	const FRotator& RotR = Behavior->AZP_GrabArmRRotation;
	const FRotator& RotH = Behavior->AZP_GrabHeadRotation;
	if (bGrab && RotL.IsNearlyZero() && RotR.IsNearlyZero() && RotH.IsNearlyZero()) return;

	const FReferenceSkeleton& RefSkel = Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	// Resolve the necromorph's upper-arm + head bones once — mixamorig naming, indices vary per
	// export (e.g. mixamorig_leftarm_09), so match by substring: "leftarm"/"rightarm" (not
	// "forearm"), and "head" excluding the "headtop_end" tip bone. Also cache the head's
	// ref-pose COMPONENT-space rotation — the run-burst stabilize target.
	if (!bResolved)
	{
		bResolved = true;
		for (int32 i = 0; i < RefSkel.GetNum(); ++i)
		{
			const FString Name = RefSkel.GetBoneName(i).ToString().ToLower();
			if (HeadBone == INDEX_NONE && Name.Contains(AZP_GrabBoneMatchHead) && !Name.Contains(AZP_GrabBoneMatchHeadExclude)) { HeadBone = i; }
			if (Name.Contains(AZP_GrabBoneMatchForearmExclude)) continue;
			if (ArmLBone == INDEX_NONE && Name.Contains(AZP_GrabBoneMatchArmL)) { ArmLBone = i; }
			if (ArmRBone == INDEX_NONE && Name.Contains(AZP_GrabBoneMatchArmR)) { ArmRBone = i; }
		}
		if (HeadBone != INDEX_NONE)
		{
			FTransform RefCS = FTransform::Identity;
			for (int32 b = HeadBone; b != INDEX_NONE; b = RefSkel.GetParentIndex(b))
			{
				RefCS = RefCS * RefSkel.GetRefBonePose()[b]; // CS(bone) = Local(bone) * CS(parent)
			}
			HeadRefCSRot = RefCS.GetRotation();
		}
	}

	TArray<FTransform>& CS = Mesh->GetEditableComponentSpaceTransforms();
	if (CS.Num() == 0) return;

	if (bRunStabilize)
	{
		// RUN BURST: hold the head at its ref-pose component-space orientation — a steady gaze
		// on whatever the body faces (the player, mid-chase) while the torso pounds underneath.
		// The baked local freeze couldn't do this: local-frozen bones still ride the parent.
		if (HeadBone != INDEX_NONE && HeadBone < CS.Num())
		{
			FQuat Delta = HeadRefCSRot * CS[HeadBone].GetRotation().Inverse();
			Delta.Normalize();
			Delta = FQuat::Slerp(FQuat::Identity, Delta, HeadStab);
			RotateBoneTreeCS(CS, RefSkel, HeadBone, Delta);
		}
		return;
	}

	struct FArmFix { int32 Bone; const FRotator* Rot; };
	const FArmFix Fixes[] = { { ArmLBone, &RotL }, { ArmRBone, &RotR }, { HeadBone, &RotH } };
	for (const FArmFix& F : Fixes)
	{
		if (F.Bone == INDEX_NONE || F.Bone >= CS.Num() || F.Rot->IsNearlyZero()) continue;
		// Bone-LOCAL additive rotation: consistent knob behavior regardless of world facing.
		const FQuat BoneCS = CS[F.Bone].GetRotation();
		const FQuat DeltaCS = BoneCS * F.Rot->Quaternion() * BoneCS.Inverse();
		RotateBoneTreeCS(CS, RefSkel, F.Bone, DeltaCS);
	}
}
