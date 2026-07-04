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

	/** Skeleton bone at and above which bones are treated as Kinemation-owned upper body and skipped by the lower-body locomotion copy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	FName AZP_UpperBodyBoundaryBoneName = TEXT("spine_01");

	/** When true, copies ALL bones from source (full body) instead of just lower body.
	 *  Used during ladder climbing to override Kinemation's upper body with climb anim. */
	bool bCopyAllBones = false;

	/** Throwable grip spread — opens the right hand's pistol curl into a wide
	 *  grenade grip. Set by KinemationComponent while a throwable is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays")
	bool bThrowableGripSpread = false;


	/** Additive LOCAL-space rotation applied at every right finger joint while
	 *  the grip spread is active (negative pitch opens a UE-manny curl).
	 *  Tune live in PIE via MCP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays")
	FRotator AZP_GripSpreadPerJoint = FRotator(-20.f, 0.f, 0.f);

	/** Index finger gets this fraction of the spread (full spread splays it
	 *  too wide — dev verdict, session 63 screenshot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays")
	float AZP_GripSpreadIndexScale = 0.4f;

	/** Thumb gets this fraction of the spread (same reason as index). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays")
	float AZP_GripSpreadThumbScale = 0.3f;

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

	/** Cancel all active overlays — called on weapon switch to prevent carry-over. */
	void ResetOverlays();

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|Melee", meta = (AllowPrivateAccess = "true"))
	float AZP_MeleeSwingDuration = 0.35f;

	/** Peak pull-back/right angle (degrees) of the melee swing wind-up phase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|Melee", meta = (AllowPrivateAccess = "true"))
	float AZP_MeleeSwingWindupAngle = -35.f;

	/** Peak sweep-left angle (degrees) of the melee strike; follow-through overshoots this by 5 degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|Melee", meta = (AllowPrivateAccess = "true"))
	float AZP_MeleeSwingStrikeAngle = 60.f;

	/** Maximum downward arm dip in cm during the melee strike phase for weight feel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|Melee", meta = (AllowPrivateAccess = "true"))
	float AZP_MeleeSwingDropAmount = 8.f;

	/** Elapsed time into grenade throw (negative = inactive). */
	float GrenadeThrowTime = -1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|Grenade", meta = (AllowPrivateAccess = "true"))
	float AZP_GrenadeThrowDuration = 0.4f;

	/** Peak raise-arm-back angle (degrees) of the grenade throw wind-up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|Grenade", meta = (AllowPrivateAccess = "true"))
	float AZP_GrenadeThrowWindupAngle = -30.f;

	/** Peak forward sweep angle (degrees) of the grenade throw. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|Grenade", meta = (AllowPrivateAccess = "true"))
	float AZP_GrenadeThrowReleaseAngle = 25.f;

	/** Elapsed time into weapon switch (negative = inactive). */
	float WeaponSwitchTime = -1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|WeaponSwitch", meta = (AllowPrivateAccess = "true"))
	float AZP_WeaponSwitchDuration = 0.5f;

	/** How far (cm) both clavicles translate down at the bottom of the weapon-switch arm-lower overlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|WeaponSwitch", meta = (AllowPrivateAccess = "true"))
	float AZP_WeaponSwitchDropDistance = 20.f;

	/** How far (degrees) both upper arms tilt downward at the bottom of the weapon-switch arm-lower overlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlays|WeaponSwitch", meta = (AllowPrivateAccess = "true"))
	float AZP_WeaponSwitchDropRotation = 25.f;

	/** Apply additive rotation to a bone in component space. */
	void ApplyBoneRotationCS(TArray<FTransform>& CSTransforms, const FReferenceSkeleton& RefSkel,
		FName BoneName, const FRotator& AdditiveRotation);

	/** Apply additive translation to a bone in component space. */
	void ApplyBoneTranslationCS(TArray<FTransform>& CSTransforms, const FReferenceSkeleton& RefSkel,
		FName BoneName, const FVector& AdditiveTranslation);
};
