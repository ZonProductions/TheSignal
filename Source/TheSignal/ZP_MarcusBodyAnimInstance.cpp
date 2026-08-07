// Copyright The Signal. All Rights Reserved.

#include "ZP_MarcusBodyAnimInstance.h"
#include "ZP_GraceCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Controller.h"

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
	// start/end pitches so level view = zero bend (no change to normal play).
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

	// Bend axis = the character's RIGHT expressed in this component's space (the component is
	// yaw-rotated -90; never assume a local axis). Positive angle about RIGHT = curl forward/down.
	const FVector CSAxis = M->GetComponentTransform()
		.InverseTransformVectorNoScale(Grace->GetActorRightVector()).GetSafeNormal();
	if (CSAxis.IsNearlyZero())
	{
		return;
	}

	const TArray<FName>& Bones = Grace->AZP_MarcusChestBendBones;
	const TArray<float>& Weights = Grace->AZP_MarcusChestBendWeights;
	int32 Applied = 0;

	for (int32 s = 0; s < Bones.Num(); ++s)
	{
		const int32 BoneIdx = Ref.FindBoneIndex(Bones[s]);
		if (BoneIdx == INDEX_NONE || BoneIdx >= CS.Num())
		{
			continue;
		}
		const float Weight = Weights.IsValidIndex(s) ? Weights[s] : (1.0f / Bones.Num());
		const FQuat Q(CSAxis, FMath::DegreesToRadians(TotalDeg * Weight));
		const FVector Pivot = CS[BoneIdx].GetLocation();

		// Rotate the bone AND every descendant about the bone's own pivot — component-space
		// transforms do not propagate on their own; without this the chest bends while the
		// shoulders/arms stay behind (the exact "not joined together" artifact).
		// RefSkeleton is parent-before-child, so descendants always index higher.
		for (int32 i = BoneIdx; i < CS.Num(); ++i)
		{
			if (i != BoneIdx)
			{
				int32 P = i;
				bool bDescendant = false;
				while (P != INDEX_NONE)
				{
					if (P == BoneIdx) { bDescendant = true; break; }
					P = Ref.GetParentIndex(P);
				}
				if (!bDescendant)
				{
					continue;
				}
			}
			CS[i].SetRotation(Q * CS[i].GetRotation());
			CS[i].SetLocation(Pivot + Q.RotateVector(CS[i].GetLocation() - Pivot));
		}
		++Applied;
	}

	if (Applied == 0 && !bWarnedNoBones)
	{
		bWarnedNoBones = true;
		UE_LOG(LogTemp, Warning,
			TEXT("[TheSignal] MarcusBodyAnimInstance: none of the AZP_MarcusChestBendBones resolve on %s — chest bend inert."),
			*GetNameSafe(M->GetSkeletalMeshAsset()));
	}
}
