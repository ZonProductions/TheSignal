// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_KinemationComponent
 *
 * Purpose: Reusable ActorComponent encapsulating Kinemation Tactical Shooter Pack
 *          integration: auto-detection of Kinemation BP components, camera API,
 *          weapon lifecycle, and fire/reload forwarding.
 *          Designed to work on any ACharacter (AZP_GraceCharacter or GASP BP).
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - CameraComponent ref (auto-discovered by name "FirstPersonCamera" if not set).
 *   - PlayerMeshComponent ref (auto-discovered by name "PlayerMesh" if not set).
 *   - WeaponClass to spawn at BeginPlay.
 *   - Kinemation component refs (auto-detected from owner's component list).
 *
 * Dependencies:
 *   - KinemationBridge (static reflection helper)
 *   - Kinemation Tactical Shooter Pack (Blueprint components on owner)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_WeaponTypes.h"
#include "ZP_KinemationComponent.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class UAnimSequenceBase;

UCLASS(ClassGroup=(TheSignal), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_KinemationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_KinemationComponent();

	// --- Configuration ---

	/** Camera to wire to Kinemation. Auto-discovered if not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
	TObjectPtr<UCameraComponent> CameraComponent;

	/** Player mesh for weapon attachment. Auto-discovered by name "PlayerMesh" if not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
	TObjectPtr<USkeletalMeshComponent> PlayerMeshComponent;

	/** Blueprint class of weapon to spawn at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	TSubclassOf<AActor> WeaponClass;

	/** If true, weapon spawns automatically during InitializeKinemation.
	 *  Set false when inventory system manages weapon lifecycle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Kinemation|Weapon")
	bool bAutoSpawnWeapon = true;

	// --- Hitscan Config ---

	/** Decal materials for bullet impacts. Randomly chosen per shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TArray<TObjectPtr<UMaterialInterface>> BulletDecalMaterials;

	/** Maximum range for hitscan traces (cm). Default 10000 = 100m. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float HitscanRange = 10000.0f;

	/** Size of bullet hole decals (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	FVector DecalSize = FVector(2.0f, 3.0f, 3.0f);

	/** How long decals remain before fading (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float DecalLifetime = 30.0f;

	/** Damage dealt on body hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float HitscanBodyDamage = 10.f;

	/** Damage dealt on weak point (center mass) hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float HitscanWeakPointDamage = 50.f;

	/** Distance (UU) from actor center that counts as a weak point hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float WeakPointRadius = 50.f;

	// --- Weapon Type ---

	/** Current weapon archetype — drives fire input routing. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	EZP_WeaponType CurrentWeaponType = EZP_WeaponType::None;

	/** Specific equipped-weapon identity — drives HUD weapon-icon selection. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	EZP_WeaponIcon CurrentWeaponIcon = EZP_WeaponIcon::None;

	// --- Melee Config ---

	/** Damage per melee swing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeDamage = 25.f;

	/** Maximum reach of melee sweep (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeRange = 200.f;

	/** Radius of the sphere sweep for melee hit detection (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeSweepRadius = 40.f;

	/** Minimum time between melee swings (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeCooldown = 0.6f;

	// --- Melee View Model (TICKET-054) ---
	// Kubold FPP Melee Animset plays on a dedicated camera-child mesh
	// (AZP_GraceCharacter::MeleeViewMesh) — never on PlayerMesh, because the
	// camera is socketed to PlayerMesh and any animation there moves the camera.

	/** Kubold view-model mesh — auto-discovered by name "MeleeViewMesh" on owner. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Melee")
	TObjectPtr<USkeletalMeshComponent> MeleeViewMeshComponent;

	/** Pipe mesh held by the view model. Created on first activation. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Melee")
	TObjectPtr<UStaticMeshComponent> MeleeWeaponMeshComp;

	/** Play-rate for the swing animation (1.42s source → ~1.0s at 1.4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeSwingRate = 1.4f;

	/** Play-rate for the equip (raise) animation (1.93s source → ~1.0s at 2.0). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeEquipRate = 2.0f;

	/** Play-rate for the unequip (lower) animation. Truncated by the swap window. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeUnequipRate = 2.5f;

	/** Seconds after swing start (post-rate) when the damage sweep fires — the impact frame. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float MeleeDamageDelay = 0.35f;

	// Grip: STATIC attach to hand_r — Kubold authors the anims with the weapon
	// rigid in the right hand ("parent weapons directly to hands" per their
	// docs). Proven in Reliquary (session4_weapons_final): zero offset + one
	// axis-correcting rotation holds across ALL anims. Do NOT compute alignment
	// from wrist positions — the wrist line wobbles; the hand bone axes don't.

	/** Pipe offset in hand_r bone space. STRAIGHT-LINE channel fit (session 63):
	 *  shaft dead along the line through both fists' grip channels, sampled
	 *  offline from A_MeleePipe_Idle across the whole loop (the longsword idle
	 *  holds the weapon straight up through both stacked fists). Channel center
	 *  per fist = centroid of the curled finger-joint ring (NOT the wrist→
	 *  knuckle line — the metacarpals run under the BACK of the hand and that
	 *  model laid the shaft against the back of the fist). Computed by
	 *  Scripts/Python/fit_pipe_grip_channel_offline.py — re-run it if the idle
	 *  anim or pipe mesh changes; it also writes the BP_GraceCharacter CDO.
	 *  Includes the dev-tuned POV trim (POV_TRIM_DEG in the script): shaft
	 *  swung 7.5° to the camera's right, pivoting on the left fist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	FVector MeleeGripOffset = FVector(-23.78f, -1.43f, 22.85f);

	/** Straight-line channel-fit rotation (see MeleeGripOffset). Re-tune for the
	 *  new CCMH hand. Applied EVERY frame so Details-panel edits show live in PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	FRotator MeleeGripRotation = FRotator(3.15f, 95.41f, 40.23f);

	/** Material painted on the melee view-model's bare-skin slots (forearm + hand) so the
	 *  FP melee arms read as Marcus skin. Defaults to MI_HandSkin (flat, tunable tone).
	 *  Swap or re-point this in BP_GraceCharacter → KinemationComp Details → Kinemation|Melee;
	 *  applied on melee init (set value + restart PIE to re-apply). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	TObjectPtr<UMaterialInterface> MeleeHandMaterial;

	// --- Throwable Config ---

	/** Held grenade offset in the KUBOLD VIEW MESH's hand_r bone space —
	 *  "one half of the pipe animation" (dev design): the view model's right
	 *  fist holds the grenade exactly where the pipe shaft passes through it.
	 *  Computed offline by Scripts/Python/fit_grenade_kubold_offline.py
	 *  (right-fist ring centroid on the pipe's channel line, incl. POV trim).
	 *  Re-run if the idle anim, grenade mesh, or ThrowableGripScale changes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|Throwable")
	FVector ThrowableGripOffset = FVector(-7.57f, 1.36f, -0.04f); // +3.5cm up the channel into the full grip (dev-tuned)

	/** Held grenade grip rotation (see ThrowableGripOffset). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|Throwable")
	FRotator ThrowableGripRotation = FRotator(3.15f, 95.41f, 40.23f);

	/** Held grenade scale (dev-tuned live in PIE: slimmer than the mesh ships,
	 *  stretched taller — Z is the grenade's long axis). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|Throwable")
	FVector ThrowableGripScale = FVector(0.6f, 0.6f, 1.01f);

	/** Grenade equip starts this far into the Kubold Equip anim — the first
	 *  half mimes drawing a pipe (a remnant with no pipe in hand); only the
	 *  second half, the simple rise from below, plays (dev call). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Throwable")
	float ThrowableEquipStartFraction = 0.5f;


	/** Blueprint class to spawn when throwing a grenade. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Throwable")
	TSubclassOf<AActor> GrenadeProjectileClass;

	/** Initial speed of thrown projectile (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Throwable")
	float ThrowSpeed = 800.f;

	// --- Animation Sequences (played as dynamic montages) ---

	/** Swing animations — cycled per swing (F → R → L variety, dev-approved).
	 *  Kubold Longsword retargeted onto the Operator skeleton
	 *  (Scripts/Python/retarget_melee_anims.py). Heavy/hold mechanic removed
	 *  by design (session 62) — swing fires immediately on press. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Animation")
	TArray<TObjectPtr<UAnimSequenceBase>> MeleeLightAnims;

	/** Melee view-model idle loop (Kubold FPP). */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> MeleeIdleAnim;

	/** Melee view-model equip/raise animation (Kubold FPP). */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> MeleeEquipAnim;

	/** Melee view-model unequip/lower animation (Kubold FPP). */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> MeleeUnequipAnim;

	/** Grenade throw animation — loaded by path in InitializeKinemation. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> GrenadeThrowAnim;

	// --- Weapon State ---

	/** The spawned weapon actor (if any). */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	TObjectPtr<AActor> ActiveWeapon;

	/** Weapon class that was equipped before a throwable. Used to auto-switch back after throw. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	TSubclassOf<UObject> PreviousWeaponClass;

	// --- Kinemation Component Refs (auto-detected or manually set) ---

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> TacticalCameraComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> TacticalAnimComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> RecoilAnimComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> IKMotionComp;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponChanged, AActor*, NewWeapon);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, CurrentAmmo, int32, ReserveAmmo);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponTypeChanged, EZP_WeaponType, NewWeaponType);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponIconChanged, EZP_WeaponIcon, NewWeaponIcon);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnThrowableConsumed);

	/** Broadcast when weapon is equipped or unequipped. NewWeapon is null on unequip. */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Weapon")
	FOnWeaponChanged OnWeaponChanged;

	// --- Ammo ---

	/** Rounds per magazine. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 MagSize = 12;

	/** Current rounds in the magazine. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 CurrentAmmo = 12;

	/** Reserve ammo (not in magazine). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 ReserveAmmo = 48;

	/** Broadcast when ammo count changes (fire, reload). */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Ammo")
	FOnAmmoChanged OnAmmoChanged;

	/** Broadcast when weapon type changes (ranged, melee, throwable). */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Weapon")
	FOnWeaponTypeChanged OnWeaponTypeChanged;

	/** Broadcast when the specific equipped weapon changes — drives HUD icon. */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Weapon")
	FOnWeaponIconChanged OnWeaponIconChanged;

	/** Broadcast when a throwable is consumed (thrown). Owner should remove 1 from inventory. */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Weapon")
	FOnThrowableConsumed OnThrowableConsumed;

	// --- ADS Config ---

	/** Default field of view (hip-fire). Set from MovementConfig in PostInitializeComponents. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|ADS")
	float DefaultFOV = 90.0f;

	/** Field of view when aiming down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|ADS")
	float AdsFOV = 65.0f;

	/** Per-weapon ADS field of view — the aim POSE pulls the sights to your eye (the
	 *  "zoom"); a WIDER FOV here counteracts it (higher = less zoom). Tune shotgun/rifle
	 *  up if they feel too zoomed. 90 = no FOV change (the pose's zoom only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|ADS")
	float AdsFOVPistol = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|ADS")
	float AdsFOVShotgun = 98.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|ADS")
	float AdsFOVRifle = 98.0f;

	/** Interpolation speed for FOV transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|ADS")
	float AdsFOVInterpSpeed = 10.0f;

	// --- Camera API ---

	/** Trigger a Kinemation camera shake. Requires TacticalCameraComp. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Kinemation")
	void TriggerCameraShake(UObject* ShakeData);

	/** Smoothly interpolate FOV to a new target. Requires TacticalCameraComp. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Kinemation")
	void SetTargetFOV(float NewFOV, float InterpSpeed = 5.0f);

	// --- Initialization ---

	/**
	 * Wire Kinemation components, spawn weapon, and complete initialization.
	 * MUST be called AFTER all SCS Blueprint components have had BeginPlay
	 * (i.e., from the owning actor's BeginPlay, after Super::BeginPlay()).
	 * This is critical — Kinemation BP components reset state in their BeginPlay,
	 * so wiring before that gets overwritten.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kinemation")
	void InitializeKinemation();

	// --- Weapon API ---

	/** Spawn and equip the weapon (if WeaponClass is set and no weapon is active).
	 *  Called by inventory system when player uses a weapon item. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	bool EquipWeapon();

	/** Equip a specific weapon class. Unequips current weapon first if one is active.
	 *  Applies per-weapon config (mag size, fire rate, damage) based on weapon type.
	 *  Accepts TSubclassOf<UObject> for Moonville compatibility (validates it's an Actor subclass).
	 *  Called by inventory system when player equips a weapon item with a specific class. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	bool EquipWeaponClass(TSubclassOf<UObject> NewWeaponClass);

	/** Destroy the active weapon, cancel pending fire/reload. Called on unequip. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void UnequipWeapon();

	/** Rearm the equipped throwable (inventory still has supply after a throw). */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void RestockThrowable();

	/** Hide/show the head region during reload/swap montages (see-own-head fix). */
	void SetCameraBonePinned(bool bPinned);

	/** Add ammo to reserve pool. Called by ammo pickup items. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Ammo")
	void AddReserveAmmo(int32 Amount);

	// --- ADS API ---

	/** Enter or exit aim-down-sights. Drives Kinemation aiming animation + FOV transition. */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|ADS")
	void SetAiming(bool bAiming);

	/** Current ADS state. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|ADS")
	bool bIsAiming = false;

	// --- Weapon API ---

	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void FirePressed();

	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void FireReleased();

	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void Reload();

	/** Returns true if Kinemation animation components are active. */
	UFUNCTION(BlueprintPure, Category = "Kinemation")
	bool IsKinemationActive() const { return TacticalAnimComp != nullptr; }

protected:
	virtual void BeginPlay() override;

private:
	void InitKinemationCamera();
	void InitKinemationAnimation();
	void SpawnAndEquipWeapon();
	void PerformHitscan();
	void PerformMeleeSwing();
	void ThrowProjectile();

	// --- Melee view model (TICKET-054) ---

	/** Show the Kubold view model: hide PlayerMesh arms, attach pipe, play Equip → Idle. */
	void ActivateMeleeViewModel();

	/** Show the view model's RIGHT arm only, grenade in its fist ("one half
	 *  of the pipe animation" — dev design). Shares the melee lifecycle. */
	void ActivateThrowableViewModel();

	/** Hide the view model and restore PlayerMesh arms.
	 *  bPlayUnequip: play the lower animation and hide after the swap drop window
	 *  (0.5s) instead of hiding immediately. */
	void DeactivateMeleeViewModel(bool bPlayUnequip);

	/** True while the view model is in throwable mode (left arm hidden). */
	bool bViewModelThrowable = false;

	/** SingleNode playback on the view mesh. */
	void PlayMeleeViewAnim(UAnimSequenceBase* Anim, bool bLoop, float Rate);

	/** The sphere-sweep damage check — runs on the impact frame, not at click time. */
	void DoMeleeDamageSweep();

	/** True while the Kubold view model is the visible melee representation. */
	bool bMeleeViewModelActive = false;

	/** Timer: equip animation finished → start idle loop. */
	FTimerHandle MeleeEquipIdleHandle;
	/** Timer: delayed damage sweep at the swing's impact frame. */
	FTimerHandle MeleeDamageHandle;
	/** Timer: hide view mesh after unequip lower animation. */
	FTimerHandle MeleeUnequipHideHandle;

	/** Cycle index into MeleeLightAnims (F → R → L). */
	int32 MeleeLightAnimIndex = 0;

	/** True while reload animation is playing — blocks firing. */
	bool bIsReloading = false;
	FTimerHandle ReloadTimerHandle;

	/** True while fire animation is cycling — blocks next shot (semi-auto lockout). */
	bool bFireCooldown = false;
	FTimerHandle FireCooldownHandle;

	/** True while melee swing is on cooldown. */
	bool bMeleeCooldown = false;
	FTimerHandle MeleeCooldownHandle;

	/** Timer for melee swing ADS return (strike → ready). */
	FTimerHandle MeleeSwingReturnHandle;
	/** Timer for melee wind-up → strike transition. */
	FTimerHandle MeleeWindupHandle;
public:
	/** True while melee swing ADS animation is in progress — blocks manual ADS. */
	bool bMeleeSwingActive = false;

	/** True while the melee view-model (Marcus arms) is up. The character reads this
	 *  to hide the body's own arms so they don't double with the view-model arms. */
	bool IsMeleeViewModelActive() const { return bMeleeViewModelActive; }

	/** Per-action state, read by the character to nudge the visible body off the camera
	 *  during each move (the upper body leans forward and clips the lens). Block is
	 *  tracked separately on the character. */
	bool IsReloadingState()  const { return bIsReloading; }
	bool IsSwingingState()   const { return bMeleeSwingActive; }
	bool IsSwitchingState()  const { return bWeaponSwitching; }
private:

	/** Releases the fire lock after the weapon Draw montage lands. */
	FTimerHandle WeaponSwitchAnimHandle;
	/** Restores the head region after the swap draw animation fully ends. */
	FTimerHandle HeadHideHandle;
	/** True while weapon switch animation is playing — blocks fire input. */
	bool bWeaponSwitching = false;
	/** Timer to re-enter ADS after melee weapon equip (ready stance). */
	FTimerHandle MeleeReadyStanceHandle;

	/** Apply per-weapon stats (mag size, fire rate, damage) based on weapon class name. */
	void ApplyWeaponConfig(TSubclassOf<AActor> InWeaponClass);

	/** Destroy old weapon, spawn + configure the new one, broadcast. Returns false if the spawn failed. */
	bool PerformWeaponSwap(TSubclassOf<AActor> ActorClass);

public:
	/** Minimum time between shots in seconds. Set per-weapon by ApplyWeaponConfig. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float FireCooldownTime = 0.25f;

	/** Fire-input lock while the weapon Draw montage plays after a swap. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float WeaponDrawLockTime = 0.6f;

	/** How long the head region stays hidden after a swap — must outlast the
	 *  longest draw animation (the body doesn't track the view during it).
	 *  1.8 flashed the head at the end of the pistol draw (dev-caught). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float SwapHeadHideTime = 2.5f;

	/** Mag-swap reload duration (pistol/rifles). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float ReloadTime = 3.0f;

	/** Shell loaders (shotguns) reload per shell: Start + shells*Loop + End.
	 *  Times measured from the Kinemation Herrington reload anims. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float ShellReloadEmptyStartTime = 2.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float ShellReloadTacStartTime = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float ShellReloadLoopTime = 0.92f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float ShellReloadEndTime = 0.85f;

	/** True for shell-by-shell reloaders (set per weapon in ApplyWeaponConfig). */
	bool bShellReload = false;
};
