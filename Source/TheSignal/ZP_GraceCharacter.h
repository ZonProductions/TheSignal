// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_GraceCharacter
 *
 * Purpose: First-person character for Grace Owens. Thin shell that owns
 *          components and routes input. All gameplay logic lives in
 *          UZP_GraceGameplayComponent, all Kinemation logic in
 *          UZP_KinemationComponent.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - MovementConfig DataAsset (propagated to GameplayComp).
 *   - WeaponClass (propagated to KinemationComp).
 *   - Input actions set via EditDefaultsOnly (configured in BP child).
 *   - OnInteract BlueprintImplementableEvent for interaction logic.
 *
 * Dependencies:
 *   - EnhancedInput
 *   - UZP_GraceGameplayComponent
 *   - UZP_KinemationComponent
 *   - UZP_GraceMovementConfig
 */

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/SkeletalMeshComponent.h"
#include "ZP_GraceCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequenceBase;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
class UZP_GraceMovementConfig;
class UZP_GraceGameplayComponent;
class UZP_KinemationComponent;
class UPostProcessComponent;
class UZP_HealthComponent;
class UZP_MapComponent;
class UZP_FloorCullingComponent;
class UZP_RuntimeISMBatcher;
class UZP_NoteComponent;
class UZP_SignalSenseComponent;
class USpotLightComponent;
class UPointLightComponent;
class USoundBase;
class UZP_BriefcaseSubsystem;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_GraceCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZP_GraceCharacter();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** Visible first-person arms mesh. Attached to capsule like reference BP_TacticalShooterCharacter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USkeletalMeshComponent> PlayerMesh;

	/** First-person melee view model (Kubold FPP Melee Animset, UE4 skeleton).
	 *  Child of FirstPersonCamera: the camera drives it, it can never move the
	 *  camera — the constraint that killed montage/retarget approaches on
	 *  PlayerMesh (TICKET-054). SingleNode playback, same path as the hidden
	 *  locomotion mesh. ZP_KinemationComponent owns its lifecycle. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USkeletalMeshComponent> MeleeViewMesh;

	// --- Marcus appearance (Character Customizer CCMH body as the visible shell) ---
	/** Visible body. CCMH_Body_Male driven by CC_Retarget_AnimBP retargeting from
	 *  PlayerMesh (Manny pose) via the pack's MH_Retargeter. Replaces the Operator
	 *  body as the visible FP shell; PlayerMesh stays (hidden) to drive camera + pose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> MarcusBody;

	/** Upper-body apparel (Lab_Coat per Ommei preset) — leader-posed to MarcusBody. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> MarcusOveralls;

	/** Lower-body apparel (Tracksuit/Nurse_Pants per Ommei preset) — leader-posed to MarcusBody. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> MarcusPants;

	/** Footwear — leader-posed to MarcusBody. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> MarcusSneakers;

	/** Overalls sleeves for the weapon-arm view-model — leader-posed to MeleeViewMesh
	 *  so the FP arms wear Marcus's clothing (not bare skin). Shown with the view-model. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> MeleeViewOveralls;

	/** Bare Marcus-skin hands for the melee view-model — SK_Hand_01a (Operator skeleton)
	 *  leader-posed to MeleeViewMesh, reskinned to CCMH skin. The Operator mesh's own
	 *  gloved hands are bone-hidden so these show through. Same bare-hand mesh the ranged
	 *  arms use, so hands read identically on every weapon. Shown only while melee is up. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> MeleeHands;

	/** RANGED view-model: arms-only meshes (SK_Arm/SK_Hand, Operator skeleton) leader-
	 *  posed to PlayerMesh so they follow the LIVE Kinemation aim pose. Shown only while
	 *  a ranged weapon is up — just arms + gun, no body (no clothing mismatch). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> RangedArms;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> RangedHands;

	/** FP shirt sleeve (Operator skeleton) leader-posed over the ranged arms so they're
	 *  clothed, not bare. Reskinned toward Marcus's shirt. Shown only while ranged. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> RangedSleeve;

	/** Live-tunable offset for the RANGED arm view-model (RangedArms + RangedHands +
	 *  RangedSleeve, moved together) on top of the leader-posed Kinemation aim. Dial in
	 *  Details → Appearance; applied every frame so PIE edits show live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector RangedArmsOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FRotator RangedArmsRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float RangedArmsScale = 1.0f;

	/** Native CCMH locomotion clips (retargeted offline from the player's Manny clips),
	 *  played on MarcusBody via SingleNode — no live retarget node, no shuffle. */
	UPROPERTY() TObjectPtr<UAnimSequenceBase> MarcusIdle;
	UPROPERTY() TObjectPtr<UAnimSequenceBase> MarcusWalk;
	UPROPERTY() TObjectPtr<UAnimSequenceBase> MarcusRun;
	UPROPERTY() TObjectPtr<UAnimSequenceBase> MarcusCrouchIdle;
	UPROPERTY() TObjectPtr<UAnimSequenceBase> MarcusCrouchWalk;

	/** Live-tunable facing offset for Marcus's visible body (on top of the -90 base
	 *  yaw). Dial in BP_GraceCharacter → Details → Appearance to correct the angle the
	 *  body faces / tilt. Applied every frame so edits show live in PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float MarcusBodyYaw = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float MarcusBodyPitch = 0.0f;

	/** Camera look-down angle (degrees) below which MarcusBody's spine starts
	 *  bending forward so the camera never sees inside the chest cavity during
	 *  Kinemation reload/switch dives. No bend above this threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|SpineBend")
	float SpineBendThresholdDeg = 45.f;

	/** Maximum total spine bend (degrees) when looking straight down (-90 pitch).
	 *  Distributed across spine_01..spine_05 with weights .10/.15/.20/.25/.30,
	 *  more toward the head. 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|SpineBend")
	float SpineBendMaxDeg = 55.f;

	/** Easing speed (higher = snappier) for SpineBend toward target each frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|SpineBend")
	float SpineBendInterpSpeed = 12.f;

	/** Bone-LOCAL hinge axis for forward bend. Default (0,1,0) = bone-local +Y
	 *  (right vector) which bends forward on UE5/MetaHuman/CCMH spine convention.
	 *  Flip sign or swap component if the body leans the wrong way. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|SpineBend")
	FVector SpineBendBoneLocalAxis = FVector(0.f, 1.f, 0.f);

	/** Per-action CAMERA offset applied during that move so the leaning body doesn't clip
	 *  the lens — the camera is moved instead of the body. Capsule space: +X = forward
	 *  (lens away from the body), +Z = up. Lerped in/out at WeaponActionOffsetSpeed. One
	 *  each for reload / switch / swing / block — dial independently in Details → Camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector ReloadCamOffset = FVector(3.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector SwitchCamOffset = FVector(3.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector SwingCamOffset = FVector(4.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (DisplayName = "Block Camera Offset (moves VIEW)"))
	FVector BlockCamOffset = FVector(4.0f, 0.0f, 0.0f);

	/** Maximum degrees the camera may pitch DOWN from horizon (positive value).
	 *  Clamps both controller look input AND animation-driven camera dives
	 *  (Kinemation reload/equip socket pose), so the camera never passes through
	 *  the visible body when looking down. Default 55 = comfortable, eyes can
	 *  still see the floor a couple of meters ahead but never the chest cavity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pitch")
	float CameraPitchDownLimitDeg = 55.f;

	/** Maximum degrees the camera may pitch UP from horizon (positive value).
	 *  Standard FPS max-look-up cap. Default 80. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pitch")
	float CameraPitchUpLimitDeg = 80.f;

	/** How fast the camera slides to/from the active offset (higher = snappier). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float WeaponActionOffsetSpeed = 10.0f;


	/** Current interpolated weapon-action camera offset (runtime; fed to GameplayComp). */
	FVector CurrentWeaponActionOffset = FVector::ZeroVector;

	/** Per-hand grip offsets (component space) for the melee view-model arms. The
	 *  post-process hand layer (UZP_MeleeHandsAnimInstance) applies them live; dial
	 *  in Details → Appearance. MeleeHand?Offset is the base grip for ALL melee states
	 *  (idle/swing AND block) — block inherits this exact connection. BlockHand?Offset
	 *  is an OPTIONAL extra added ON TOP only while blocking; leave it zero and block
	 *  grips identically to non-block pipe wielding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector MeleeHandLOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector MeleeHandROffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (DisplayName = "Block Hand L Offset (extra, on top)"))
	FVector BlockHandLOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (DisplayName = "Block Hand R Offset (extra, on top)"))
	FVector BlockHandROffset = FVector::ZeroVector;

	/** Active melee hand grip offset, chosen by block state — read by the post-process
	 *  hand anim layer. bRight selects hand_r, else hand_l. */
	FVector GetActiveMeleeHandOffset(bool bRight) const;

	/** True while the player is holding a melee block. Read by the post-process hand
	 *  anim layer so it can replay the non-block grip pose during block. */
	bool IsBlockingNow() const { return bIsBlocking; }

	/** Tracks whether Marcus's body arms are currently hidden (weapon view-model up). */
	bool bMarcusArmsHidden = false;

	/** Tracks ranged-weapon state: when armed ranged, the Operator PlayerMesh is shown
	 *  (its Kinemation arms hold the gun) and Marcus is hidden. */
	bool bRangedArmedState = false;

	/** Live-tunable framing for the Marcus weapon-arm view-model (CCMH proportions
	 *  differ from the Operator the -155/-90 was tuned for). Dial in Details →
	 *  Appearance, applied every frame so PIE edits show live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector MeleeViewOffset = FVector(0.0f, 0.0f, -155.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FRotator MeleeViewRotation = FRotator(0.0f, -90.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float MeleeViewScale = 1.0f;

	/** Extra placement applied to the melee weapon-arm view-model (the ARM RIG —
	 *  MeleeViewMesh + sleeves + hands) ONLY while blocking — added on top of
	 *  MeleeViewOffset. Moves the ARMS, NOT the camera. X = forward/back,
	 *  Y = left/right, Z = up/down. Dial in Details → Appearance; applied every frame
	 *  so PIE edits show live — tune it WHILE holding a block. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (DisplayName = "Block Arms Offset (moves arms)"))
	FVector BlockViewOffset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TObjectPtr<UZP_GraceGameplayComponent> GameplayComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kinemation")
	TObjectPtr<UZP_KinemationComponent> KinemationComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UZP_HealthComponent> HealthComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map")
	TObjectPtr<UZP_MapComponent> MapComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Notes")
	TObjectPtr<UZP_NoteComponent> NoteComp;

	/** "The Phone" — proximity warning that drives layered audio + HUD waveform + rumble. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SignalSense")
	TObjectPtr<UZP_SignalSenseComponent> SignalSenseComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Culling")
	TObjectPtr<UZP_FloorCullingComponent> FloorCullingComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
	TObjectPtr<UZP_RuntimeISMBatcher> ISMBatcherComp;

	/** Post-process vignette driven by health percentage. Darkens screen edges below 50% HP. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UPostProcessComponent> DeathVignetteComp;

	// --- Flashlight (TLOU/SH2 style chest-mounted) ---

	/** Chest-mounted spotlight. Follows camera with slight lag for organic chest-mounted feel. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<USpotLightComponent> FlashlightComp;

	/** Ambient fill light — simulates light bouncing off surfaces near the flashlight beam.
	 *  Every horror game (TLOU, RE7, SH2) uses this trick: SpotLight for beam + PointLight for fill. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<UPointLightComponent> FlashlightFillComp;

	/** Whether the flashlight is currently on. */
	UPROPERTY(BlueprintReadWrite, Category = "Flashlight")
	bool bFlashlightOn = false;

	/** Sound played when toggling flashlight on/off. Defaults to CC pack flashlight click. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<USoundBase> FlashlightClickSound;

	/** How quickly the flashlight tracks the camera (higher = snappier, lower = more chest lag). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	float FlashlightInterpSpeed = 8.0f;

	/** Downward pitch offset from camera direction (chest naturally points slightly down). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	float FlashlightPitchOffset = -5.0f;

	// --- Configuration (propagated to components in PostInitializeComponents) ---

	/** DataAsset with all movement tuning values. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UZP_GraceMovementConfig> MovementConfig;

	/** Blueprint class of weapon to spawn. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	TSubclassOf<AActor> WeaponClass;

	/** How far the gun reaches ahead of the camera. The capsule is held this
	 *  far off any wall the player faces so the muzzle never clips through
	 *  geometry (the body capsule alone stops short of the gun's reach).
	 *  Moves the whole capsule (camera rides it  normal); NEVER PlayerMesh.
	 *  0 disables. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float GunCollisionReach = 75.0f;

	/** Radius of the muzzle clearance probe. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float GunCollisionRadius = 6.0f;

	/** Decal materials for bullet impacts. Set in BP child via Python. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TArray<TSoftObjectPtr<UMaterialInterface>> BulletDecalMaterials;

	/** Skeletal mesh for hidden locomotion Mesh. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<USkeletalMesh> LocomotionSkeletalMesh;

	/** Idle animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	/** Walk animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> WalkAnimation;

	/** Run animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> RunAnimation;

	/** Crouch idle animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> CrouchIdleAnimation;

	/** Crouch walk animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> CrouchWalkAnimation;

	/** Ladder climb up loop animation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> LadderClimbUpAnimation;

	/** Ladder climb down loop animation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> LadderClimbDownAnimation;

	/** Ladder idle animation (holding on, not moving). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> LadderIdleAnimation;

	/** Animated climb-over-the-top exit (root motion carries the body onto the
	 *  upper floor). Default A_Climb_Up_out_Right_UE5, set via set_all_cdo.py. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> LadderTopExitAnimation;

	/** Playback speed of the climb-over-the-top exit (anim + camera). 2.0 = double. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	float LadderTopExitPlayRate = 2.0f;

	// --- Input Actions (set in Blueprint child, e.g. BP_GraceCharacter) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	// --- Dodge (replaces Jump on space bar) ---

	/** Horizontal launch impulse (cm/s) applied on dodge. Below sprint speed
	 *  on purpose: a 700 spike feeds the locomotion blend a "running" signal
	 *  in one frame, which yanks the spine and the FPCamera-socketed view. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float DodgeImpulse = 400.f;

	/** Cooldown between dodges (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float DodgeCooldown = 0.8f;

	/** FPP_sns_Dodge — played on MeleeViewMesh. Doesn't drive PlayerMesh so the
	 *  firearm grip on hand_r is never disturbed; visible only when the melee
	 *  view model is active (pipe up). Movement impulse fires either way. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TSoftObjectPtr<UAnimSequenceBase> DodgeAnim;

	// --- Block (RMB hold while pipe equipped) ---

	/** True while RMB is held with the pipe active. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Block")
	bool bIsBlocking = false;

	/** True from RMB-release until the BlockStop clip finishes. Keeps the block
	 *  camera/arm offsets alive through the pose-out so the body doesn't swing into
	 *  the lens during the resolve. */
	bool bBlockResolving = false;

	/** Incoming damage is multiplied by this while blocking (0.25 = 75% off). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	float BlockDamageReductionMul = 0.25f;

	/** Seconds the attacker is staggered after a blocked hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	float BlockStaggerDuration = 1.0f;

	/** Kubold FPP_Longs_BlockLoop — held block pose, standing still. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> BlockLoopAnim;

	/** Kubold FPP_Longs_BlockWalk — block pose while walking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> BlockWalkAnim;

	/** Kubold FPP_Longs_BlockStart — pose-in motion before the loop. Gives the
	 *  eye visible motion so the held BlockLoop doesn't read as a freeze-frame. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> BlockStartAnim;

	/** Kubold FPP_Longs_BlockStop — pose-out motion after release. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> BlockStopAnim;

	/** Kubold FPP_Longs_BlockImpact 1/2/3 — random reaction on a blocked hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> BlockImpact1Anim;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> BlockImpact2Anim;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> BlockImpact3Anim;

	/** FPP_Longs_Idle — return-to-hold pose after dodge/block-release. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> MeleeIdleHoldAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	/** Q key — lean peek around cover (camera only, no weapon aim). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> PeekAction;

	/** RMB — aim down sights. Auto-peeks with weapon when near cover. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> ReloadAction;

	// --- BP Interface Compatibility ---
	// These properties are synced from components after init so that
	// BPI_TacticalShooterCharacter interface function implementations
	// (GetPrimaryWeapon, GetMainWeapon) can still read them as variables.
	// Will be removed when BP_GraceCharacter is deprecated.

	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	TObjectPtr<AActor> ActiveWeapon;

	// --- Map ---

	/** True when map overlay is open — blocks all gameplay input. DEPRECATED: Use IsMenuOpen() instead. */
	UPROPERTY(BlueprintReadWrite, Category = "Map")
	bool bMapOpen = false;

	/** Input action for toggling the map (M key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MapAction;

	/** Input action for cycling inventory tabs left (Q key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> TabCycleLeftAction;

	/** Input action for cycling inventory tabs right (E key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> TabCycleRightAction;

	// --- Inventory ---

	/** True when inventory UI is open — blocks all gameplay input. */
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	bool bInventoryMenuOpen = false;

	/** Runtime IMC that maps OwnSlotActions to Keys 1-4 at high priority. */
	UPROPERTY()
	TObjectPtr<UInputMappingContext> WeaponSlotIMC;

	/** Runtime-created input actions for weapon slots. NOT Moonville's actions —
	 *  avoids double-fire from conflicting IMC mappings. */
	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> OwnSlotActions;

	/** Auto-discovered Moonville BP_InteractionComponent (if present on this actor). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UActorComponent> MoonvilleInteractionComp;

	/** Auto-discovered Moonville BP_InventoryCharacterComponent (if present on this actor). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UActorComponent> MoonvilleInventoryComp;

	/** Input action for opening/closing inventory menu (Tab). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventoryMenuAction;

	/** Shortcut cross slot input actions (keys 1-4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventorySlot0Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventorySlot1Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventorySlot2Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> InventorySlot3Action;

	/** PDA_Item data asset for starting weapon. Granted to inventory at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSoftObjectPtr<UObject> StartingWeaponItem;

	// --- Interaction ---

	/** The IZP_Interactable actor currently in range (set by overlap on interactables). */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TWeakObjectPtr<AActor> CurrentInteractable;

	/** Called by interactable actors when player enters their trigger volume. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetCurrentInteractable(AActor* Interactable);

	/** Called by interactable actors when player leaves their trigger volume. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ClearCurrentInteractable(AActor* Interactable);

	/** Scan Moonville inventory for Item.Note tagged items and add them to NoteComponent.
	 *  Called on-demand when Notes tab is opened (delegate binding may not always fire). */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	void ScanInventoryForNotes();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	/** Override in Blueprint for interaction logic. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteract(AActor* InteractTarget);

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

private:
	// --- Death / Vignette ---
	UFUNCTION()
	void HandleDeath();

	UFUNCTION()
	void UpdateHealthVignette(float NewHealth, float MaxHealth, float DamageAmount);

	// --- Input Handlers ---
	void Input_Move(const FInputActionValue& Value);
	void Input_MoveCompleted(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_SprintStarted(const FInputActionValue& Value);
	void Input_SprintCompleted(const FInputActionValue& Value);
	void Input_Jump(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);
	void Input_CrouchStarted(const FInputActionValue& Value);
	void Input_CrouchCompleted(const FInputActionValue& Value);
	void Input_PeekStarted(const FInputActionValue& Value);
	void Input_PeekCompleted(const FInputActionValue& Value);
	void Input_AimStarted(const FInputActionValue& Value);
	void Input_AimCompleted(const FInputActionValue& Value);
	void Input_FireStarted(const FInputActionValue& Value);
	void Input_FireCompleted(const FInputActionValue& Value);
	void Input_ReloadStarted(const FInputActionValue& Value);
	void Input_InventoryMenu(const FInputActionValue& Value);
	void Input_Map(const FInputActionValue& Value);
	void Input_TabCycleLeft(const FInputActionValue& Value);
	void Input_TabCycleRight(const FInputActionValue& Value);
	void Input_InventorySlot0(const FInputActionValue& Value);
	void Input_InventorySlot1(const FInputActionValue& Value);
	void Input_InventorySlot2(const FInputActionValue& Value);
	void Input_InventorySlot3(const FInputActionValue& Value);

	/** Handles quickslot press: equips weapon OR uses consumable. SlotIndex is 0-based (Key 1 = slot 0). */
	void Input_InventorySlot(int32 SlotIndex);

	/** Calls Moonville's ExecuteItemActionByShortcut for consumable items. */
	void UseItemFromShortcutSlot(int32 SlotIndex);

	/** Adds StartingWeaponItem to inventory at BeginPlay. */
	void GrantStartingItems();

	/** Directional dash on space bar (replaces Jump). Uses CurrentMoveInput;
	 *  defaults to backward when stationary. Plays FPP_sns_Dodge on
	 *  MeleeViewMesh (separate from PlayerMesh — firearm grip untouched). */
	void PerformDodge();

	/** Live WASD input (X=right, Y=forward). Set in Input_Move, cleared in
	 *  Input_MoveCompleted. Read by PerformDodge for dash direction. */
	FVector2D CurrentMoveInput = FVector2D::ZeroVector;

	/** Seconds remaining on dodge cooldown. Decremented in Tick. */
	float DodgeCooldownRemaining = 0.f;

	/** Seconds remaining where the dodge holds extra forward camera clearance
	 *  (covers the launch lean). Set in PerformDodge, decremented in Tick. */
	float DodgeClearanceRemaining = 0.f;

	/** Duration of the dodge forward-clearance window (seconds). */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeClearanceWindow = 0.5f;

	/** Seconds remaining where the block forward-clearance nudge is active. Set on
	 *  block start, decremented in Tick. A transient window — NOT the whole hold —
	 *  so the camera settles to neutral during a sustained block instead of floating
	 *  forward. Drives GameplayComp->SetForwardClearanceActive with DodgeClearanceRemaining. */
	float BlockClearanceRemaining = 0.f;

	/** Duration of the block forward-clearance nudge (seconds) — long enough to
	 *  cover the stance lean-in, then it eases out even while block is held. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Block")
	float BlockClearanceWindow = 0.4f;


	/** True when the block walk anim is currently loaded (vs BlockLoop). */
	bool bBlockWalkActive = false;

	/** Seconds remaining where a BlockImpact reaction owns the view mesh playback. */
	float BlockImpactLockRemaining = 0.f;

	/** Seconds remaining where the BlockStart transition owns the view mesh playback.
	 *  UpdateBlockAnimation bails while this is > 0 so it can't yank the Start
	 *  clip out and snap straight to BlockLoop. */
	float BlockStartLockRemaining = 0.f;

	/** Switch MeleeViewMesh between BlockLoop / BlockWalk based on motion. */
	void UpdateBlockAnimation();

	/** Assemble the Marcus CCMH body as the visible shell: load meshes, set the
	 *  retarget AnimBP (source = PlayerMesh) on MarcusBody, leader-pose apparel,
	 *  hide the Operator visible body. Called once in BeginPlay. */
	void SetupMarcusAppearance();

	/** Play a random FPP_Longs_BlockImpact1/2/3 on MeleeViewMesh. */
	void PlayBlockImpactAnim();

	/** Camera diagnostic burst — frames remaining (decremented in Tick). Set on
	 *  dodge/block. Each tick while > 0, dump camera world/relative position,
	 *  FPCamera socket world pos, PlayerMesh world pos, velocity, control
	 *  rotation. Lets us see exactly when the camera shifts. */
	int32 CameraProbeFramesLeft = 0;
	/** Tag for the current probe burst ("DODGE" / "BLOCK" / "BLOCK-HIT"). */
	FString CameraProbeTag;
	/** Dump a single-line camera diagnostic. */
	void LogCameraProbe(const TCHAR* Phase);

	/** Per-bone clip diagnostic. For PlayerMesh + MeleeViewMesh, lists any
	 *  bone within 50cm of the camera with its distance, forward-axis position
	 *  (positive = in front of camera), and lateral offset from the camera ray.
	 *  A bone with fwd>0 and lat<10cm is literally inside the camera's
	 *  near-frustum — that's POV-clipping. Runs on the same probe burst as
	 *  LogCameraProbe so the dev can correlate body bones with RMB/dodge events. */
	void LogBoneClipProbe();

	/** Reads ObjectClass from a PDA_Item via reflection. Returns null if not a weapon. */
	TSubclassOf<AActor> GetWeaponClassFromItem(UObject* ItemDA);

	/** Reads weapon class from Moonville's ShortcutSlots[SlotIndex] via reflection. */
	TSubclassOf<AActor> GetWeaponFromShortcutSlot(int32 SlotIndex);

	/** Syncs ActiveWeapon from KinemationComp when weapon changes (equip/unequip). */
	UFUNCTION()
	void OnWeaponChangedHandler(AActor* NewWeapon);

	/** Removes 1 throwable from Moonville inventory when a grenade is consumed. */
	UFUNCTION()
	void OnThrowableConsumedHandler();

	/** Bridge: Moonville OnInventoryUpdate → scan items for notes → NoteComponent. */
	UFUNCTION()
	void HandleInventoryUpdate();

	/** Bind to Moonville's OnInventoryUpdate dispatcher via reflection. */
	void BindInventoryUpdateDelegate();

	/** Cached slot index of the last-used throwable for inventory removal. */
	int32 LastThrowableSlotIndex = -1;

	/** Cached PDA_Item data asset for the last-used throwable (for RemoveItemByDataAsset). */
	TObjectPtr<UObject> LastThrowableItemDA;

	// --- Briefcase / Container Tracking ---

	/** The briefcase actor currently being used by the player (for sync on close). */
	TWeakObjectPtr<AActor> ActiveBriefcaseActor;

	/** The container actor (any type) currently being used — for bPlayerIsUsingActor close detection. */
	TWeakObjectPtr<AActor> ActiveContainerActor;

	/** True when we called Interact() but haven't yet seen bPlayerIsUsingActor=true. */
	bool bWaitingForContainerOpen = false;

	/** Frames spent waiting for container to open. Timeout after N frames = container didn't have a UI (instant loot). */
	int32 ContainerOpenWaitFrames = 0;

	/** True once bPlayerIsUsingActor was confirmed true — now watching for it to go false. */
	bool bContainerWasOpen = false;

	/** Checks if an open container was closed, then syncs briefcase data and unequips removed weapons. */
	void CheckContainerClosed();

	/** Unequips the active weapon if it's no longer in the player's inventory. */
	void UnequipMissingWeapon();

	void ToggleFlashlight();

	/** Total amount of items in Moonville ItemSlots whose weapon class matches
	 *  (sums stack amounts). Stack-aware supply check for throwables. */
	int32 CountWeaponClassInInventory(TSubclassOf<AActor> InWeaponClass);

	/** Move every item from the open Moonville container into the player
	 *  inventory ("E again to loot all"). False if no container menu is open. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TransferAllFromOpenContainer();

	/** One-line dump of Moonville's interaction state (menu flag, closest
	 *  interactable, cooldown) — session 63 diagnostics. */
	FString GetMoonvilleInteractionStateString() const;

	/** Item count + names inside a container's inventory — diagnostics. */
	FString DescribeLockerContents(AActor* LockerActor);

	/** Last logged interaction state — change detection in Tick. */
	FString LastInteractStateDump;
	float InteractStateLogAccum = 0.f;

	// --- Loot Locker Filtering ---

	/** Lockers that have been opened and emptied — no longer interactable. */
	TSet<FName> LootedEmptyLockers;

	/** Remove ammo items from a locker for weapons the player doesn't own. */
	void FilterLockerAmmo(AActor* LockerActor);

	/** Returns true if the player has a weapon matching this ammo type. */
	bool PlayerHasWeaponForAmmo(const FString& AmmoItemName);

	/** Returns true if locker's inventory is empty. */
	bool IsLockerInventoryEmpty(AActor* LockerActor);

	/** Disable a locker's interaction sphere so it can't be reopened. */
	void DisableLockerInteraction(AActor* LockerActor);

	/** Reads the PDA_Item UObject from Moonville's ShortcutSlots[SlotIndex] via reflection. */
	UObject* GetItemDAFromShortcutSlot(int32 SlotIndex);

	// --- Ladder Climbing ---

	/** True while the player is on a ladder. Blocks normal movement, aim, fire, etc. */
	UPROPERTY(BlueprintReadOnly, Category = "Ladder", meta = (AllowPrivateAccess = "true"))
	bool bOnLadder = false;

	/** The ladder actor being climbed. */
	TWeakObjectPtr<AActor> ActiveLadderActor;

	/** Current climb input: +1 = up, -1 = down, 0 = idle. Updated from Input_Move. */
	float LadderClimbInput = 0.f;

	/** World Z of the rung we're currently moving toward. */
	float LadderTargetRungZ = 0.f;

	/** True while interpolating between rungs — blocks new input until arrival. */
	bool bLadderMovingToRung = false;

	/** True while the animated climb-over-the-top exit is playing. */
	bool bLadderTopExiting = false;
	/** Seconds elapsed into the top-exit animation. */
	float LadderTopExitElapsed = 0.f;
	/** Cached length of the top-exit animation. */
	float LadderTopExitDuration = 0.f;
	/** Camera eye lerp endpoints during the top-exit (ladder eye -> floor eye). */
	FVector LadderTopExitCamStart = FVector::ZeroVector;
	FVector LadderTopExitCamEnd = FVector::ZeroVector;
	/** Capsule resting spot snapped to when the top-exit animation finishes. */
	FVector LadderTopExitEndLoc = FVector::ZeroVector;

	/** Saved weapon class before entering ladder — re-equipped on exit. */
	TSubclassOf<UObject> PreLadderWeaponClass;

	/** Self-managed camera rotation during climbing. Controller rotation gets corrupted
	 *  by Kinemation camera modifiers, so we bypass it entirely on the ladder. */
	FRotator LadderCameraRotation = FRotator::ZeroRotator;

	/** Enter climbing state on the given ladder. Called from AZP_Ladder::OnInteract. */
	void EnterLadder(AActor* LadderActor);

	/** Exit climbing state. bExitTop = true → teleport to top, false → teleport to bottom. */
	void ExitLadder(bool bExitTop);

	/** Begin the animated climb-over-the-top exit. Plays LadderTopExitAnimation;
	 *  the body's baked root motion carries it onto the floor while the camera
	 *  lerps up; capsule snaps to the resting spot when the anim completes. */
	void BeginLadderTopExit(class AZP_Ladder* Ladder);

	/** Advance the animated top-exit each tick; finalizes when the anim ends. */
	void UpdateLadderTopExit(float DeltaTime);

	friend class AZP_Ladder;

};
