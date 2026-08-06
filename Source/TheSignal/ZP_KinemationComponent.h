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
 *   - AZP_WeaponClass to spawn at BeginPlay.
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
class USoundBase;
class USkeletalMesh;
class UStaticMesh;

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
	TSubclassOf<AActor> AZP_WeaponClass;

	/** If true, weapon spawns automatically during InitializeKinemation.
	 *  Set false when inventory system manages weapon lifecycle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Kinemation|Weapon")
	bool bAZP_AutoSpawnWeapon = true;

	/** PlayerMesh socket the spawned ranged weapon attaches to (Kinemation virtual-bone gun socket). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Weapon")
	FName AZP_WeaponAttachSocketName = FName(TEXT("VB ik_hand_gun"));

	// --- Hitscan Config ---

	/** Decal materials for bullet impacts. Randomly chosen per shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TArray<TObjectPtr<UMaterialInterface>> AZP_BulletDecalMaterials;

	/** Maximum range for hitscan traces (cm). Default 10000 = 100m. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float AZP_HitscanRange = 10000.0f;

	/** Size of bullet hole decals (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	FVector AZP_DecalSize = FVector(2.0f, 3.0f, 3.0f);

	/** How long decals remain before fading (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float AZP_DecalLifetime = 30.0f;

	/** Seconds a bullet decal takes to fade out at the end of AZP_DecalLifetime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Hitscan")
	float AZP_DecalFadeDuration = 2.0f;

	/** Damage dealt on body hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float AZP_HitscanBodyDamage = 10.f;

	/** Damage dealt on weak point (center mass) hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float AZP_HitscanWeakPointDamage = 50.f;

	/** Distance (UU) from actor center that counts as a weak point hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float AZP_WeakPointRadius = 50.f;

	/** Radius in cm within which a gunshot alerts all nearby creatures via BroadcastGunshot (creatures can override per-instance with their own GunshotAlertRadius). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Hitscan")
	float AZP_GunshotAlertRadius = 8000.f;

	// --- Bullet impact sounds (played once per shot at the impact point, chosen by weapon icon) ---

	/** Pistol bullet-impact sound (icon == Pistol). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TObjectPtr<USoundBase> AZP_PistolImpactSound;

	/** Rifle bullet-impact sound (icon == Rifle). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TObjectPtr<USoundBase> AZP_RifleImpactSound;

	/** Shotgun bullet-impact sound (icon == Shotgun). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TObjectPtr<USoundBase> AZP_ShotgunImpactSound;

	// --- Shotgun spray (icon == Shotgun) ---
	// Fires a randomized burst of pellets instead of one trace. AZP_HitscanBodyDamage /
	// AZP_HitscanWeakPointDamage are the TOTAL per-shot damage (3x pistol), divided across
	// the pellets — a full spray that all connects deals the total, partial hits less.

	/** Pellets per shot, randomized in [Min,Max] each trigger pull. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	int32 AZP_ShotgunPelletMin = 15;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	int32 AZP_ShotgunPelletMax = 20;

	/** Spread cone half-angle (degrees) the pellets scatter within. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	float AZP_ShotgunSpreadDegrees = 4.0f;

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
	float AZP_MeleeDamage = 25.f;

	/** Maximum reach of melee sweep (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float AZP_MeleeRange = 200.f;

	/** Radius of the sphere sweep for melee hit detection (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float AZP_MeleeSweepRadius = 40.f;

	/** Minimum time between melee swings (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float AZP_MeleeCooldown = 0.6f;

	/** Radius in cm within which a melee impact alerts nearby creatures (quieter than gunshots). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	float AZP_MeleeNoiseAlertRadius = 2000.f;

	// --- Melee impact sounds ---
	// The pipe is metal: a hit ALWAYS makes the metal sound. Striking an enemy
	// ALSO layers a flesh impact on top (dev: "impact + hit at the same time").
	// Striking a wall/hard surface plays the dedicated pipe-on-surface sound instead.

	/** Metal sound of the pipe itself — plays on any enemy connect (SFX_PIPE_HIT). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	TObjectPtr<USoundBase> AZP_PipeMetalSound;

	/** Flesh impacts, randomized — layered over the metal sound on enemy hits (SFX_MELEE_IMPACT1/2). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	TArray<TObjectPtr<USoundBase>> AZP_MeleeFleshImpactSounds;

	/** Pipe striking a wall / hard non-enemy surface — the CONCRETE/default family
	 *  (SFX_PIPE_SURFACE_WALL_IMPACT). Also the fallback whenever a per-surface slot below is
	 *  null. Surface family comes from UZP_SFXStatics::ClassifySurface on the swing hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	TObjectPtr<USoundBase> AZP_PipeWallImpactSound;

	/** Pipe striking METAL (vents, lockers, machines, doors, railings) — SFX_PIPE_SURFACE_METAL_IMPACT.
	 *  Ships as a duplicate of the wall impact; replace the asset's wave with the sourced sound. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	TObjectPtr<USoundBase> AZP_PipeImpactMetal;

	/** Pipe striking WOOD (crates, pallets, furniture) — SFX_PIPE_SURFACE_WOOD_IMPACT. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	TObjectPtr<USoundBase> AZP_PipeImpactWood;

	/** Pipe striking GLASS/TILE (windows, lab tile, screens) — SFX_PIPE_SURFACE_GLASS_IMPACT. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	TObjectPtr<USoundBase> AZP_PipeImpactGlass;

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
	float AZP_MeleeSwingRate = 1.4f;

	/** Fraction of the swing after which a HELD block may cancel the remaining follow-through.
	 *  The strike and most of the follow-through always play (never reads as clipping); only the
	 *  return-to-idle dead frames get replaced by the guard coming up (never reads as a pause).
	 *  1.0 = block always waits for the full clip. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float AZP_MeleeBlockCancelFraction = 0.75f;

	/** Whoosh played at every swing start (own-body foley, 2D). Defaults to SFX_MELEE_SWING. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	TObjectPtr<USoundBase> AZP_MeleeSwingSound;

	/** Volume of the swing whoosh. Knob lives on BP_GraceCharacter -> KinemationComp Details. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	float AZP_MeleeSwingVolume = 1.f;

	/** Play-rate for the equip (raise) animation (1.93s source → ~1.0s at 2.0). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float AZP_MeleeEquipRate = 2.0f;

	/** Play-rate for the unequip (lower) animation. Truncated by the swap window. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float AZP_MeleeUnequipRate = 2.5f;

	/** Seconds after the melee unequip/lower anim starts before the view model is hidden (the swap drop window; matches EquipWeaponClass phase 2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	float AZP_MeleeUnequipHideDelay = 0.5f;

	/** Seconds after swing start (post-rate) when the damage sweep fires — the impact frame. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Melee")
	float AZP_MeleeDamageDelay = 0.35f;

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
	FVector AZP_MeleeGripOffset = FVector(-23.78f, -1.43f, 22.85f);

	/** Straight-line channel-fit rotation (see AZP_MeleeGripOffset). Re-tune for the
	 *  new CCMH hand. Applied EVERY frame so Details-panel edits show live in PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	FRotator AZP_MeleeGripRotation = FRotator(3.15f, 95.41f, 40.23f);

	/** Block grip as an ADDITIVE delta on top of the idle grip (AZP_MeleeGripOffset/Rotation),
	 *  in hand_r-LOCAL space. Default zero => block grip == idle grip (pipe stays exactly
	 *  where idle holds it, just carried into the guard pose by hand_r). Dial these if the
	 *  block guard wants the pipe held differently — they ONLY affect block, never idle, so
	 *  the two grips stop fighting. Because the blend is hand-local, no value can ever
	 *  swing the pipe out of the hand (worst case shifts the grip slightly within it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	FVector AZP_BlockGripDeltaLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	FRotator AZP_BlockGripDeltaRotation = FRotator::ZeroRotator;

	/** Initial melee weapon attach to hand_r at the idle grip. UpdateMeleeGrip then eases
	 *  the hand_r-RELATIVE grip toward the block delta while blocking. */
	void SetMeleeWeaponBlockGrip(bool bBlocking);

	/** Ease the pipe's hand_r-relative grip between idle (AZP_MeleeGripOffset) and block
	 *  (AZP_MeleeGripOffset + BlockGripDelta*) by MeleeGripBlend, setting its RELATIVE
	 *  transform. Hand-local blend on a hand_r-parented pipe => it tracks the hand every
	 *  frame and can never swing out on un/block. Reads offsets live so Details edits tune
	 *  in PIE. */
	void UpdateMeleeGrip(float DeltaSeconds, bool bBlocking);

	/** Current idle<->block grip blend (0 = idle grip, 1 = block grip), both relative to
	 *  hand_r. Eased toward the block state at AZP_BlockGripBlendSpeed. */
	float MeleeGripBlend = 0.f;

	/** How fast the grip eases between idle and block (FInterpTo speed). Higher = snappier.
	 *  Tune in KinemationComp Details if the transition feels too slow/fast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	float AZP_BlockGripBlendSpeed = 8.0f;

	/** Material painted on the view-model hand/forearm skin slots — MELEE (Skin+Gloves slots)
	 *  AND RANGED (RangedArms/RangedHands, since 2026-08-05) so the hands never change material
	 *  when swapping stances. Defaults to MI_HandSkin (flat, tunable tone). Swap or re-point in
	 *  BP_GraceCharacter → KinemationComp Details → Kinemation|Melee; applied at init
	 *  (set value + restart PIE to re-apply). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	TObjectPtr<UMaterialInterface> AZP_MeleeHandMaterial;

	/** Skeletal mesh assigned to the melee/throwable view model (Operator body, proven pose; CCMH swap reverted 2026-06-20 — keep the default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	TSoftObjectPtr<USkeletalMesh> AZP_MeleeViewModelMeshAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono.SKM_Operator_Mono")));

	/** Static mesh of the pipe held by the melee view model (Moonville example-content pipe). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Melee")
	TSoftObjectPtr<UStaticMesh> AZP_MeleeWeaponMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/InventorySystemPro/ExampleContent/Common/Art/Pipe/SM_Pipe.SM_Pipe")));

	// --- Throwable Config ---

	/** Held grenade offset in the KUBOLD VIEW MESH's hand_r bone space —
	 *  "one half of the pipe animation" (dev design): the view model's right
	 *  fist holds the grenade exactly where the pipe shaft passes through it.
	 *  Computed offline by Scripts/Python/fit_grenade_kubold_offline.py
	 *  (right-fist ring centroid on the pipe's channel line, incl. POV trim).
	 *  Re-run if the idle anim, grenade mesh, or AZP_ThrowableGripScale changes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|Throwable")
	FVector AZP_ThrowableGripOffset = FVector(-7.57f, 1.36f, -0.04f); // +3.5cm up the channel into the full grip (dev-tuned)

	/** Held grenade grip rotation (see AZP_ThrowableGripOffset). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|Throwable")
	FRotator AZP_ThrowableGripRotation = FRotator(3.15f, 95.41f, 40.23f);

	/** Held grenade scale (dev-tuned live in PIE: slimmer than the mesh ships,
	 *  stretched taller — Z is the grenade's long axis). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|Throwable")
	FVector AZP_ThrowableGripScale = FVector(0.6f, 0.6f, 1.01f);

	/** Grenade equip starts this far into the Kubold Equip anim — the first
	 *  half mimes drawing a pipe (a remnant with no pipe in hand); only the
	 *  second half, the simple rise from below, plays (dev call). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Throwable")
	float AZP_ThrowableEquipStartFraction = 0.5f;


	/** Blueprint class to spawn when throwing a grenade. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Throwable")
	TSubclassOf<AActor> AZP_GrenadeProjectileClass;

	/** Initial speed of thrown projectile (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Throwable")
	float AZP_ThrowSpeed = 800.f;

	/** Cooldown in seconds after throwing a grenade before the next throw is allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Throwable")
	float AZP_ThrowCooldownTime = 0.8f;

	/** Distance in cm in front of the camera where the thrown grenade spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Throwable")
	float AZP_ThrowSpawnForwardOffset = 100.f;

	// --- Animation Sequences (played as dynamic montages) ---

	/** Swing animations — cycled per swing (F → R → L variety, dev-approved).
	 *  Kubold Longsword retargeted onto the Operator skeleton
	 *  (Scripts/Python/retarget_melee_anims.py). Heavy/hold mechanic removed
	 *  by design (session 62) — swing fires immediately on press. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Animation")
	TArray<TObjectPtr<UAnimSequenceBase>> AZP_MeleeLightAnims;

	/** Melee view-model idle loop (Kubold FPP). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> AZP_MeleeIdleAnim;

	/** Melee view-model equip/raise animation (Kubold FPP). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> AZP_MeleeEquipAnim;

	/** Melee view-model unequip/lower animation (Kubold FPP). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> AZP_MeleeUnequipAnim;

	/** Grenade throw animation — loaded by path in InitializeKinemation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|Animation")
	TObjectPtr<UAnimSequenceBase> AZP_GrenadeThrowAnim;

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
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReserveConsumed, int32, Rounds, EZP_WeaponIcon, AmmoIcon);

	/** Broadcast when weapon is equipped or unequipped. NewWeapon is null on unequip. */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Weapon")
	FOnWeaponChanged OnWeaponChanged;

	// --- Ammo ---

	/** Rounds per magazine. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 AZP_MagSize = 12;

	/** Current rounds in the magazine. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 CurrentAmmo = 12;

	/** Reserve ammo — a MIRROR of the matching ammo the player is carrying in the
	 *  Moonville inventory for the equipped weapon. The character syncs this from
	 *  inventory (SyncReserveFromInventory); the component never stores ammo itself.
	 *  Ammo always lives in the inventory grid (it's the survival challenge). */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 ReserveAmmo = 0;

	/** Maximum reserve (inventory ammo of this type) the player may carry — the pickup
	 *  cap. Set per-weapon in ApplyWeaponConfig from GetReserveCapForIcon.
	 *  Pistol 48 / shotgun 24 / rifle 90. */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Ammo")
	int32 MaxReserveAmmo = 48;

	/** Reserve cap for a weapon icon. Pistol 48 / Shotgun 24 / Rifle 90; 0 for
	 *  melee/throwable/none (no reserve). */
	static int32 GetReserveCapForIcon(EZP_WeaponIcon Icon);

	/** Map an ammo item name to its weapon icon: "9mm"→Pistol, "Buckshot"→Shotgun,
	 *  "556"→Rifle. None if the name matches no known ammo type. */
	static EZP_WeaponIcon AmmoNameToIcon(const FString& AmmoItemName);

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

	/** Broadcast when a reload pulls rounds from reserve — the character removes that
	 *  many ammo items of AmmoIcon from the Moonville inventory (inventory IS the reserve). */
	UPROPERTY(BlueprintAssignable, Category = "Kinemation|Ammo")
	FOnReserveConsumed OnReserveConsumed;

	// --- ADS Config ---

	/** Default field of view (hip-fire). Set from AZP_MovementConfig in PostInitializeComponents. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|ADS")
	float AZP_DefaultFOV = 90.0f;

	/** Field of view when aiming down sights. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|ADS")
	float AZP_AdsFOV = 65.0f;

	/** Per-weapon ADS field of view — the aim POSE pulls the sights to your eye (the
	 *  "zoom"); a WIDER FOV here counteracts it (higher = less zoom). Tune shotgun/rifle
	 *  up if they feel too zoomed. 90 = no FOV change (the pose's zoom only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|ADS")
	float AZP_AdsFOVPistol = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|ADS")
	float AZP_AdsFOVShotgun = 98.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kinemation|ADS")
	float AZP_AdsFOVRifle = 98.0f;

	/** Interpolation speed for FOV transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kinemation|ADS")
	float AZP_AdsFOVInterpSpeed = 10.0f;

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

	/** Spawn and equip the weapon (if AZP_WeaponClass is set and no weapon is active).
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

	/** Kill the queued impact-frame melee sweep (and the swing-active flag). Called when a grab
	 *  latches mid-swing: the sweep landing a frame after the latch deals damage mid-grab, which
	 *  by design BREAKS the grab — the "latch glitch" (dev 2026-07-03). Visual timers stay. */
	void CancelPendingMeleeSwing();

	/** Rearm the equipped throwable (inventory still has supply after a throw). */
	UFUNCTION(BlueprintCallable, Category = "Kinemation|Weapon")
	void RestockThrowable();

	/** Hide/show the head region during reload/swap montages (see-own-head fix). */
	void SetCameraBonePinned(bool bPinned);

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

	/** Cancel an in-progress reload.
	 *  bImmediate (weapon swap / stow): hard stop — the draw montage or actor
	 *  destruction covers the visual; every shell already inserted stays counted.
	 *  Soft (fire press, shell loaders only): the shell in motion still seats, the
	 *  end animation plays, and the gun unlocks with exactly the inserted rounds.
	 *  Mag-swap reloads only support the immediate form (the mag is out of the gun —
	 *  nothing transfers). */
	void CancelReload(bool bImmediate);

	/** True while a shell-by-shell reload is still inserting rounds (not the end anim). */
	bool IsShellReloadInProgress() const { return bIsReloading && bShellReloadActive; }

	/** Snapshot of the per-class loaded-round memory INCLUDING the in-hand weapon's live
	 *  count, keyed by class path — for the character's EGUI save hook. Without this a
	 *  load hands back full mags the inventory already paid for at reload time. */
	TMap<FString, int32> ExportMagazineState() const;

	/** REPLACE the per-class loaded-round memory from a save file (class path → rounds)
	 *  and correct the in-hand weapon immediately (the deferred save-file re-equip
	 *  early-outs when the saved class is already equipped, so a same-class load would
	 *  otherwise keep the pre-load magazine). */
	void ImportMagazineState(const TMap<FString, int32>& State);

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

	/** Cycle index into AZP_MeleeLightAnims (F → R → L). */
	int32 MeleeLightAnimIndex = 0;

	/** Control rotation captured at swing start. The damage sweep uses THIS committed aim (not the live,
	 *  whipped control rotation), so the committed-swing camera whip can move the view without making the
	 *  swing miss what the player aimed at. */
	FRotator MeleeSwingAimRot = FRotator::ZeroRotator;

	/** Swing variant for THIS swing: 0=Forward, 1=Right, 2=Left (F→R→L). Drives the camera whip/kick. */
	int32 MeleeCurrentSwingDir = 0;

	/** True while reload animation is playing — blocks firing. */
	bool bIsReloading = false;
	FTimerHandle ReloadTimerHandle;

	/** Loaded rounds remembered per ranged weapon class — swapping away and back must
	 *  NOT refill the magazine (dev spec 2026-07-12). Session-runtime state; the
	 *  Moonville inventory grid remains the persistent ammo store. */
	UPROPERTY(Transient)
	TMap<TSubclassOf<AActor>, int32> SavedMagAmmo;

	/** Stash the active ranged weapon's loaded rounds into SavedMagAmmo (swap/stow). */
	void SaveMagazineForCurrentWeapon();

	/** Set CurrentAmmo for a freshly equipped ranged class (saved count, else full mag)
	 *  and mirror it into the pack weapon BP's ActiveAmmo/MaxAmmo. */
	void RestoreMagazineForClass(TSubclassOf<AActor> ActorClass);

	// --- Shell-by-shell reload runtime (shotguns) ---
	// The pack BP's ReloadStart→ReloadLoop→ReloadEnd chain owns the visuals and the
	// per-shell timing; its loop inserts one shell per iteration into its OWN ActiveAmmo
	// and terminates only on ActiveAmmo == MaxAmmo (exact equality). C++ presents it a
	// target via the ammo mirror, then reads each seated shell back into CurrentAmmo /
	// the inventory. Mirror is a 0.05s timer scoped to the reload window ONLY — the
	// chain is a latent Delay loop inside sealed pack content with no event to bind to
	// (documented exception to the no-polling rule).

	/** True while the BP shell chain is inserting rounds (poll mirror running). */
	bool bShellReloadActive = false;
	/** BP-side reload began on the empty-start path (chambers a shell before its first loop). */
	bool bShellBPStartedEmpty = false;
	/** CurrentAmmo value this reload stops at (reserve- and mag-clamped). */
	int32 ShellReloadTarget = 0;
	/** BP ActiveAmmo minus real CurrentAmmo. 1 only for the one-shell-from-empty load,
	 *  which must be presented to the BP as a tactical reload (from empty its chain
	 *  always inserts >= 2 shells before the first equality check). */
	int32 ShellBPOffset = 0;
	/** BP ActiveAmmo as presented at reload start (pending-increment detection on cancel). */
	int32 ShellInitialBPActive = 0;
	/** World seconds after which the mirror force-finishes (BP chain died/desynced). */
	float ShellReloadFailsafeAt = 0.f;
	FTimerHandle ShellPollHandle;

	/** Kick off a shell reload: present target to the BP, start the mirror. */
	void StartShellReload();
	/** Mirror tick: absorb seated shells; close out at target or failsafe. */
	void ShellPollTick();
	/** Pull the BP's shell count into CurrentAmmo, consuming inventory per shell. */
	void AbsorbBPShellCount();
	/** Soft cancel: lower the BP's MaxAmmo to the next shell boundary (equality-safe). */
	void LowerShellTargetToPending();
	/** Target reached/canceled — hold the lock through the end animation. */
	void BeginShellReloadEnd();
	/** Shared release: unlock, unpin head, re-true the BP ammo mirror. */
	void FinishReloadLock();

	/** True while fire animation is cycling — blocks next shot (semi-auto lockout). */
	bool bFireCooldown = false;
	FTimerHandle FireCooldownHandle;

	/** True while melee swing is on cooldown. */
	bool bMeleeCooldown = false;
	FTimerHandle MeleeCooldownHandle;

	/** Timer for melee swing ADS return (strike → ready). */
	FTimerHandle MeleeSwingReturnHandle;
	/** Flips bMeleeSwingTailCancelable at AZP_MeleeBlockCancelFraction of the swing. */
	FTimerHandle MeleeTailCancelHandle;
	/** Timer for melee wind-up → strike transition. */
	FTimerHandle MeleeWindupHandle;
public:
	/** True while melee swing ADS animation is in progress — blocks manual ADS. */
	bool bMeleeSwingActive = false;

	/** True once the swing has played past AZP_MeleeBlockCancelFraction: the strike + follow-through
	 *  are visibly done and only return-to-idle frames remain — a held block may cancel into them. */
	bool bMeleeSwingTailCancelable = false;

	/** Block-cancel the swing's return-to-idle tail: kills the idle-return timer and clears the
	 *  swing state WITHOUT touching the view anim — the caller (block start) takes over the hands
	 *  the same frame. Only valid once bMeleeSwingTailCancelable is set. */
	void CancelMeleeSwingRecovery()
	{
		if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(MeleeSwingReturnHandle);
			W->GetTimerManager().ClearTimer(MeleeTailCancelHandle);
		}
		bMeleeSwingActive = false;
		bMeleeSwingTailCancelable = false;
	}

	/** Seconds of the current swing's return-to-idle still queued (0 when no swing is active). The
	 *  character reads this so a block press can cancel the tail once the swing is within its grace
	 *  window (AZP_MeleeToBlockGracePeriod on AZP_GraceCharacter). */
	float GetMeleeSwingTimeRemaining() const
	{
		if (!bMeleeSwingActive) { return 0.f; }
		if (const UWorld* W = GetWorld())
		{
			const float R = W->GetTimerManager().GetTimerRemaining(MeleeSwingReturnHandle);
			return R > 0.f ? R : 0.f;
		}
		return 0.f;
	}

	/** True while the melee view-model (Marcus arms) is up. The character reads this
	 *  to hide the body's own arms so they don't double with the view-model arms. */
	bool IsMeleeViewModelActive() const { return bMeleeViewModelActive; }

	/** Per-action state, read by the character to nudge the visible body off the camera
	 *  during each move (the upper body leans forward and clips the lens). Block is
	 *  tracked separately on the character. */
	bool IsReloadingState()  const { return bIsReloading; }
	bool IsSwingingState()   const { return bMeleeSwingActive; }
	bool IsSwitchingState()  const { return bWeaponSwitching; }

	/** [LatchProbe] — is the impact-frame damage sweep still queued? Read at the grab latch so the
	 *  trace shows whether a swing's damage was in flight at that exact moment. */
	bool IsMeleeSweepPending() const
	{
		const UWorld* W = GetWorld();
		return W && W->GetTimerManager().IsTimerActive(MeleeDamageHandle);
	}
	/** Seconds until the queued sweep fires (-1 = none). Same probe family. */
	float MeleeSweepRemaining() const
	{
		const UWorld* W = GetWorld();
		return W ? W->GetTimerManager().GetTimerRemaining(MeleeDamageHandle) : -1.f;
	}
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
	float AZP_FireCooldownTime = 0.25f;

	/** Fire-input lock while the weapon Draw montage plays after a swap. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float AZP_WeaponDrawLockTime = 0.6f;

	/** How long the head region stays hidden after a swap — must outlast the
	 *  longest draw animation (the body doesn't track the view during it).
	 *  1.8 flashed the head at the end of the pistol draw (dev-caught). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float AZP_SwapHeadHideTime = 2.5f;

	/** Mag-swap reload duration (pistol/rifles). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float AZP_ReloadTime = 3.0f;

	/** Shell loaders (shotguns) reload per shell: Start + shells*Loop + End.
	 *  Times measured from the Kinemation Herrington reload anims. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float AZP_ShellReloadEmptyStartTime = 2.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float AZP_ShellReloadTacStartTime = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float AZP_ShellReloadLoopTime = 0.92f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float AZP_ShellReloadEndTime = 0.85f;

	/** True for shell-by-shell reloaders (set per weapon in ApplyWeaponConfig). */
	bool bShellReload = false;
};
