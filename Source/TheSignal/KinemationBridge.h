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

	/** If AC_TacticalShooterAnimation.ActiveSettings is null, installs a default-constructed
	 *  instance of its settings class. The UE5_ABP_IK_Pose layer on PlayerMesh reads
	 *  ActiveSettings EVERY FRAME independent of the component's tick — while unarmed/melee it
	 *  spammed "Accessed None ... ActiveSettings" per frame (message log + callstack format cost).
	 *  A neutral settings object silences it with default grip-pose values (2026-08-04). */
	static void AnimEnsureActiveSettings(UObject* AnimComp);

	/** FORCE-install a fresh default-constructed settings object, replacing whatever weapon's
	 *  settings are live. Ensure() early-returns on non-null, so after any gun its settings
	 *  stay installed forever — and the graph keeps rendering that gun's STANCE (head ~19 uu
	 *  lower than the unarmed rest pose, measured 2026-08-07) under the melee view model,
	 *  putting the FP camera inside the visible chest. Call on melee/throwable equips to
	 *  return PlayerMesh to the same class-default pose spawn-unarmed runs. */
	static void AnimResetActiveSettings(UObject* AnimComp);

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

	// ── Weapon ammo mirror (reflection into BP int variables) ──
	// The pack weapon BP keeps its own ActiveAmmo/MaxAmmo: its Fire path gates on
	// ActiveAmmo > 0 (and decrements per shot), and the shotgun's shell-reload chain
	// (ReloadStart → ReloadLoop → ReloadEnd) inserts shells until ActiveAmmo == MaxAmmo
	// EXACTLY. C++ owns the real ammo; these push/pull the BP mirror so the visuals
	// load/fire exactly the rounds the player actually has.

	/** Read an int variable off the weapon BP (ActiveAmmo/MaxAmmo). Fallback if absent. */
	static int32 WeaponGetInt(AActor* Weapon, FName VarName, int32 Fallback);

	/** Read a bool variable off the weapon BP (HasActiveAction). Fallback if absent.
	 *  The pack holds HasActiveAction for the FULL length of every action montage
	 *  (Draw/Reload/Inspect via StartAction) and its OnFirePressed dead-ends on it —
	 *  C++ fire must agree or it burns ammo with no montage/recoil/sound. */
	static bool WeaponGetBool(AActor* Weapon, FName VarName, bool bFallback);

	/** Write an int variable on the weapon BP. Returns false if the class lacks it. */
	static bool WeaponSetInt(AActor* Weapon, FName VarName, int32 Value);

	/** Clear the weapon's action lock (EndAction) after a reload finishes/cancels early. */
	static void WeaponEndAction(AActor* Weapon);
};
