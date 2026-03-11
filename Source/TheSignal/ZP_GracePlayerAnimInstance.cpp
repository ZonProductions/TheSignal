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

	// Advance weapon action timers
	if (MeleeSwingTime >= 0.f)
	{
		MeleeSwingTime += DeltaSeconds;
		if (MeleeSwingTime > MeleeSwingDuration)
		{
			MeleeSwingTime = -1.f;
		}
	}
	if (GrenadeThrowTime >= 0.f)
	{
		GrenadeThrowTime += DeltaSeconds;
		if (GrenadeThrowTime > GrenadeThrowDuration)
		{
			GrenadeThrowTime = -1.f;
		}
	}
	if (WeaponSwitchTime >= 0.f)
	{
		WeaponSwitchTime += DeltaSeconds;
		if (WeaponSwitchTime > WeaponSwitchDuration)
		{
			WeaponSwitchTime = -1.f;
		}
	}
}

void UZP_GracePlayerAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	// Advance bone blend alpha
	if (BoneBlendAlpha < 1.0f)
	{
		BoneBlendAlpha = FMath::FInterpTo(BoneBlendAlpha, 1.0f, GetDeltaSeconds(), BoneBlendInterpSpeed);
		if (BoneBlendAlpha > 0.99f)
		{
			BoneBlendAlpha = 1.0f;
		}
	}

	// This runs AFTER AnimGraph evaluation, BEFORE bone transform finalization.
	// Modifications here are part of the normal animation pipeline — no double-buffer issues.
	CopyBonesFromSource();

	// --- Weapon action overlays (post-Kinemation) ---
	// Applied AFTER CopyBonesFromSource so they layer on top of the final pose.
	USkeletalMeshComponent* TargetMesh = GetSkelMeshComponent();
	if (!TargetMesh || !TargetMesh->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] PostEval: TargetMesh=%s, SkeletalAsset=%s — BAILING"),
			TargetMesh ? TEXT("OK") : TEXT("NULL"),
			(TargetMesh && TargetMesh->GetSkeletalMeshAsset()) ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	TArray<FTransform>& CSTransforms = TargetMesh->GetEditableComponentSpaceTransforms();
	if (CSTransforms.Num() == 0) return;
	const FReferenceSkeleton& RefSkel = TargetMesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	// Debug: log active overlay states every 60 frames
	static int32 OverlayDebugCounter = 0;
	if (++OverlayDebugCounter % 60 == 1)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] PostEval overlays: Melee=%.3f, Grenade=%.3f, Switch=%.3f, CSTransforms=%d"),
			MeleeSwingTime, GrenadeThrowTime, WeaponSwitchTime, CSTransforms.Num());
	}

	// --- Melee swing: big horizontal sweep across body ---
	// Wind-up (pull right) → strike (sweep left across body) → follow-through → return
	if (MeleeSwingTime >= 0.f)
	{
		const float T = FMath::Clamp(MeleeSwingTime / MeleeSwingDuration, 0.f, 1.f);

		// Primary swing angle — drives the horizontal sweep
		float SwingAngle; // degrees
		// Drop amount — arms dip down during strike for weight feel
		float DropAmount; // unreal units (cm)

		if (T < 0.2f)
		{
			// Wind-up: pull back/right
			const float Phase = T / 0.2f;
			const float Ease = FMath::Sin(Phase * PI * 0.5f);
			SwingAngle = -35.f * Ease;
			DropAmount = -3.f * Ease; // slight raise during wind-up
		}
		else if (T < 0.55f)
		{
			// Strike: sweep left across body (fast, aggressive)
			const float Phase = (T - 0.2f) / 0.35f;
			const float Ease = FMath::Sin(Phase * PI * 0.5f);
			SwingAngle = FMath::Lerp(-35.f, 60.f, Ease);
			DropAmount = FMath::Lerp(-3.f, 8.f, Ease); // arms dip during strike
		}
		else if (T < 0.75f)
		{
			// Follow-through overshoot
			const float Phase = (T - 0.55f) / 0.2f;
			SwingAngle = FMath::Lerp(60.f, 65.f, Phase); // slight overshoot
			DropAmount = FMath::Lerp(8.f, 5.f, Phase);
		}
		else
		{
			// Return to rest
			const float Phase = (T - 0.75f) / 0.25f;
			const float Ease = Phase * Phase; // ease-in for snappy return
			SwingAngle = FMath::Lerp(65.f, 0.f, Ease);
			DropAmount = FMath::Lerp(5.f, 0.f, Ease);
		}

		// Spine twist — body rotates into the swing
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("spine_01"),
			FRotator(0.f, SwingAngle * 0.08f, 0.f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("spine_02"),
			FRotator(0.f, SwingAngle * 0.08f, 0.f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("spine_03"),
			FRotator(SwingAngle * 0.05f, SwingAngle * 0.12f, 0.f));

		// Right arm — drives the main arc
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("clavicle_r"),
			FRotator(SwingAngle * 0.15f, SwingAngle * 0.1f, 0.f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("upperarm_r"),
			FRotator(SwingAngle * 0.5f, SwingAngle * -0.3f, SwingAngle * 0.15f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("lowerarm_r"),
			FRotator(SwingAngle * 0.35f, 0.f, SwingAngle * 0.1f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("hand_r"),
			FRotator(SwingAngle * 0.15f, SwingAngle * -0.1f, 0.f));

		// Left arm follows — both hands on the pipe
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("clavicle_l"),
			FRotator(SwingAngle * 0.1f, SwingAngle * 0.08f, 0.f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("upperarm_l"),
			FRotator(SwingAngle * 0.4f, SwingAngle * -0.2f, SwingAngle * 0.1f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("lowerarm_l"),
			FRotator(SwingAngle * 0.25f, 0.f, SwingAngle * 0.08f));

		// Drop both arms during strike for weight
		if (FMath::Abs(DropAmount) > 0.1f)
		{
			ApplyBoneTranslationCS(CSTransforms, RefSkel, FName("upperarm_r"),
				FVector(0.f, 0.f, -DropAmount));
			ApplyBoneTranslationCS(CSTransforms, RefSkel, FName("upperarm_l"),
				FVector(0.f, 0.f, -DropAmount));
		}
	}

	// --- Grenade throw: raise arm → throw forward → follow-through ---
	if (GrenadeThrowTime >= 0.f)
	{
		const float T = GrenadeThrowTime / GrenadeThrowDuration;

		float ThrowAngle;
		if (T < 0.35f)
		{
			// Wind-up: raise arm back
			const float Phase = T / 0.35f;
			ThrowAngle = -30.f * FMath::Sin(Phase * PI * 0.5f);
		}
		else if (T < 0.65f)
		{
			// Throw: arm sweeps forward
			const float Phase = (T - 0.35f) / 0.3f;
			ThrowAngle = FMath::Lerp(-30.f, 25.f, FMath::Sin(Phase * PI * 0.5f));
		}
		else
		{
			// Follow-through
			const float Phase = (T - 0.65f) / 0.35f;
			ThrowAngle = FMath::Lerp(25.f, 0.f, Phase);
		}

		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("upperarm_r"),
			FRotator(ThrowAngle * 0.7f, ThrowAngle * 0.2f, 0.f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("lowerarm_r"),
			FRotator(ThrowAngle * 0.5f, 0.f, 0.f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("hand_r"),
			FRotator(ThrowAngle * 0.3f, 0.f, ThrowAngle * 0.1f));
	}

	// --- Weapon switch: arms drop down then raise back up ---
	if (WeaponSwitchTime >= 0.f)
	{
		const float T = FMath::Clamp(WeaponSwitchTime / WeaponSwitchDuration, 0.f, 1.f);

		// Drop curve: fast down (0-0.4), hold briefly (0.4-0.6), raise back (0.6-1.0)
		float DropFactor; // 0 = normal, 1 = fully lowered
		if (T < 0.4f)
		{
			// Drop down
			const float Phase = T / 0.4f;
			DropFactor = FMath::Sin(Phase * PI * 0.5f);
		}
		else if (T < 0.6f)
		{
			// Hold at bottom
			DropFactor = 1.0f;
		}
		else
		{
			// Raise back up
			const float Phase = (T - 0.6f) / 0.4f;
			DropFactor = 1.0f - FMath::Sin(Phase * PI * 0.5f);
		}

		const float DropDist = 20.f * DropFactor; // 20 cm drop
		const float DropRot = 25.f * DropFactor;  // rotation to tilt arms down

		// Both arms drop down
		ApplyBoneTranslationCS(CSTransforms, RefSkel, FName("clavicle_r"),
			FVector(0.f, 0.f, -DropDist));
		ApplyBoneTranslationCS(CSTransforms, RefSkel, FName("clavicle_l"),
			FVector(0.f, 0.f, -DropDist));

		// Tilt arms downward
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("upperarm_r"),
			FRotator(-DropRot, 0.f, 0.f));
		ApplyBoneRotationCS(CSTransforms, RefSkel, FName("upperarm_l"),
			FRotator(-DropRot, 0.f, 0.f));
	}
}

void UZP_GracePlayerAnimInstance::StartBoneBlend(float InterpSpeed)
{
	// Snapshot current lower body bones before the anim switch
	USkeletalMeshComponent* TargetMesh = GetSkelMeshComponent();
	if (!TargetMesh || !bBoneMapBuilt) return;

	const TArray<FTransform>& CurrentCS = TargetMesh->GetComponentSpaceTransforms();
	if (CurrentCS.Num() == 0) return;

	CachedLowerBodyBones.SetNum(BoneIndexMap.Num());
	for (int32 TargetIdx = 0; TargetIdx < BoneIndexMap.Num() && TargetIdx < CurrentCS.Num(); ++TargetIdx)
	{
		if (BoneIndexMap[TargetIdx] != INDEX_NONE)
		{
			CachedLowerBodyBones[TargetIdx] = CurrentCS[TargetIdx];
		}
	}

	BoneBlendAlpha = 0.0f;
	BoneBlendInterpSpeed = InterpSpeed;
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

	const bool bBlending = BoneBlendAlpha < 1.0f && CachedLowerBodyBones.Num() == BoneIndexMap.Num();

	// When bCopyAllBones is true (ladder climbing), copy every bone by name,
	// ignoring the BoneIndexMap's spine_01 upper-body filter.
	// This overrides Kinemation's upper body with the climbing animation.
	const FReferenceSkeleton& SourceRefSkel = SourceMeshComponent->GetSkeletalMeshAsset()->GetRefSkeleton();
	const FReferenceSkeleton& TargetRefSkel = TargetMesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	// Compensate for component orientation difference (PlayerMesh has -90° yaw offset from hidden Mesh).
	// During bCopyAllBones, transform bones: SourceCS → World → TargetCS.
	// During normal lower-body copy, direct CS copy works fine (gimbal lock vicinity makes yaw offset invisible).
	const bool bNeedCompensation = bCopyAllBones;
	FTransform SourceComponentTF, TargetComponentTF;
	if (bNeedCompensation)
	{
		SourceComponentTF = SourceMeshComponent->GetComponentTransform();
		TargetComponentTF = TargetMesh->GetComponentTransform();
	}

	int32 CopiedCount = 0;
	for (int32 TargetIdx = 0; TargetIdx < BoneIndexMap.Num() && TargetIdx < TargetCSTransforms.Num(); ++TargetIdx)
	{
		int32 SourceIdx = BoneIndexMap[TargetIdx];

		// Full-body mode: find source bone by name even for upper body bones
		if (bCopyAllBones && SourceIdx == INDEX_NONE)
		{
			const FName BoneName = TargetRefSkel.GetBoneName(TargetIdx);
			SourceIdx = SourceRefSkel.FindBoneIndex(BoneName);
		}

		if (SourceIdx != INDEX_NONE && SourceIdx < SourceCSTransforms.Num())
		{
			FTransform FinalBone;
			if (bNeedCompensation)
			{
				// SourceCS → World → TargetCS (compensates for PlayerMesh -90° yaw offset)
				FTransform WorldBone = SourceCSTransforms[SourceIdx] * SourceComponentTF;
				FinalBone = WorldBone.GetRelativeTransform(TargetComponentTF);
			}
			else
			{
				FinalBone = SourceCSTransforms[SourceIdx];
			}

			if (bBlending)
			{
				FTransform Blended;
				Blended.Blend(CachedLowerBodyBones[TargetIdx], FinalBone, BoneBlendAlpha);
				TargetCSTransforms[TargetIdx] = Blended;
			}
			else
			{
				TargetCSTransforms[TargetIdx] = FinalBone;
			}
			++CopiedCount;
		}
	}

	// NO FinalizeBoneTransform() — the engine does it after NativePostEvaluateAnimation.
	// Our modifications are included in the normal finalization pass.

#if !UE_BUILD_SHIPPING
	// Periodic diagnostic — only in debug/development builds
	static int32 FrameCounter = 0;
	if (++FrameCounter % 300 == 1)
	{
		const int32 TestBoneIdx = SourceMeshComponent->GetBoneIndex(FName(TEXT("foot_l")));
		FVector SrcFootPos = (TestBoneIdx != INDEX_NONE && TestBoneIdx < SourceCSTransforms.Num())
			? SourceCSTransforms[TestBoneIdx].GetTranslation() : FVector::ZeroVector;

		UE_LOG(LogTemp, Log, TEXT("[BoneCopy] PostEval: Copied %d lower-body bones | Source foot_l: (%.1f,%.1f,%.1f)"),
			CopiedCount, SrcFootPos.X, SrcFootPos.Y, SrcFootPos.Z);
	}
#endif
}

void UZP_GracePlayerAnimInstance::StartMeleeSwing(float Duration)
{
	MeleeSwingTime = 0.f;
	MeleeSwingDuration = Duration;
	UE_LOG(LogTemp, Warning, TEXT("[TheSignal] StartMeleeSwing: Duration=%.2f"), Duration);
}

void UZP_GracePlayerAnimInstance::StartGrenadeThrow(float Duration)
{
	GrenadeThrowTime = 0.f;
	GrenadeThrowDuration = Duration;
}

void UZP_GracePlayerAnimInstance::StartWeaponSwitch(float Duration)
{
	WeaponSwitchTime = 0.f;
	WeaponSwitchDuration = Duration;
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] StartWeaponSwitch: Duration=%.2f"), Duration);
}

void UZP_GracePlayerAnimInstance::ResetOverlays()
{
	MeleeSwingTime = -1.f;
	GrenadeThrowTime = -1.f;
	WeaponSwitchTime = -1.f;
}

void UZP_GracePlayerAnimInstance::ApplyBoneRotationCS(TArray<FTransform>& CSTransforms,
	const FReferenceSkeleton& RefSkel, FName BoneName, const FRotator& AdditiveRotation)
{
	const int32 BoneIdx = RefSkel.FindBoneIndex(BoneName);
	if (BoneIdx == INDEX_NONE || BoneIdx >= CSTransforms.Num()) return;

	// Apply rotation in component space (additive on top of current pose)
	const FQuat AdditiveQuat = AdditiveRotation.Quaternion();
	CSTransforms[BoneIdx].SetRotation(AdditiveQuat * CSTransforms[BoneIdx].GetRotation());
}

void UZP_GracePlayerAnimInstance::ApplyBoneTranslationCS(TArray<FTransform>& CSTransforms,
	const FReferenceSkeleton& RefSkel, FName BoneName, const FVector& AdditiveTranslation)
{
	const int32 BoneIdx = RefSkel.FindBoneIndex(BoneName);
	if (BoneIdx == INDEX_NONE || BoneIdx >= CSTransforms.Num()) return;

	CSTransforms[BoneIdx].AddToTranslation(AdditiveTranslation);
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
