// Copyright The Signal. All Rights Reserved.
#pragma once

/**
 * FKinemationBridge
 *
 * Static utility for calling Kinemation Blueprint component functions from C++.
 * Uses FindFunction + ProcessEvent (UE reflection) since Kinemation components
 * are Blueprint classes with no C++ API.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Dependencies:
 *   - Kinemation Tactical Shooter Pack (Blueprint components)
 */

#include "CoreMinimal.h"

class UCameraComponent;
class USkeletalMeshComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogKinemation, Log, All);

struct FKinemationBridge
{
	// ── Helpers ──────────────────────────────────────────────

	/** Call a no-param Blueprint function by name. */
	static void CallNoParams(UObject* Target, FName FuncName);

	// ── AC_FirstPersonCamera ────────────────────────────────

	static void UpdateTargetCamera(UObject* CameraComp, UCameraComponent* Camera);
	static void UpdatePlayerMesh(UObject* CameraComp, USkeletalMeshComponent* Mesh);
	/** Force-update OwnerAnimInstance on AC_FirstPersonCamera from the current Mesh. */
	static void RefreshOwnerAnimInstance(UObject* CameraComp, USkeletalMeshComponent* Mesh);
	static void UpdateTargetFOV(UObject* CameraComp, float NewFOV, float InterpSpeed);
	static void EnableFreeLook(UObject* CameraComp);
	static void DisableFreeLook(UObject* CameraComp);
	static void AddFreeLookInput(UObject* CameraComp, float InputX, float InputY);
	static void PlayCameraShake(UObject* CameraComp, UObject* ShakeData);

	// ── AC_TacticalShooterAnimation ─────────────────────────

	static void AnimSetAiming(UObject* AnimComp, bool bIsAiming);
	static void AnimSetActiveSettings(UObject* AnimComp, UObject* Settings);
	static void AnimToggleReadyPose(UObject* AnimComp, bool bUseHighReady);

	// ── AC_RecoilAnimation ──────────────────────────────────

	static void RecoilPlay(UObject* RecoilComp);
	static void RecoilStop(UObject* RecoilComp);
	static void RecoilSetAiming(UObject* RecoilComp, bool bIsAiming);

	// ── AC_IKMotionPlayer ───────────────────────────────────

	static void IKPlay(UObject* IKComp, UObject* MotionData);

	// ── Weapon (BP_TacticalShooterWeapon) ────────────────────

	static void WeaponDraw(AActor* Weapon);
	static void WeaponOnFirePressed(AActor* Weapon);
	static void WeaponOnFireReleased(AActor* Weapon);
	static void WeaponOnReload(AActor* Weapon);
	static void WeaponChangeFireMode(AActor* Weapon);
	static void WeaponOnInspect(AActor* Weapon);
	static void WeaponOnMagCheck(AActor* Weapon);
	static void WeaponOnToggleAttachment(AActor* Weapon);
	static UObject* WeaponGetSettings(AActor* Weapon);
};
