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
 *   - AZP_MovementConfig DataAsset (propagated to GameplayComp).
 *   - AZP_WeaponClass (propagated to KinemationComp).
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
#include "JsonObjectWrapper.h"
#include "ZP_WeaponTypes.h"
#include "ZP_Grabbable.h"
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
class UZP_FootstepData;
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
class UTexture;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_GraceCharacter : public ACharacter, public IZP_Grabbable
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

	/** Marcus's head (CCMH_Head_Male — a SEPARATE mesh; the CCMH body asset is headless).
	 *  Leader-posed to MarcusBody, hidden in normal FP play (it would sit in the camera),
	 *  shown ONLY during the grab-struggle 3P beat. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> MarcusHead;

	/** Marcus's authored hair (SidePart_02 per the "Marcus" CC_SaveGame entry, dark-brown
	 *  Hair Tint). Leader-posed like the head; shown only with it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> MarcusHair;

	/** Marcus's eyebrows (Eyebrows_01 per the "Marcus" CC_SaveGame entry). No beard —
	 *  the preset says Beard_Default (clean-shaven). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USkeletalMeshComponent> MarcusBrows;

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
	FVector AZP_RangedArmsOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FRotator AZP_RangedArmsRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float AZP_RangedArmsScale = 1.0f;

	/** Native CCMH locomotion clips (retargeted offline from the player's Manny clips),
	 *  played on MarcusBody via SingleNode — no live retarget node, no shuffle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|MarcusClips") TObjectPtr<UAnimSequenceBase> AZP_MarcusIdle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|MarcusClips") TObjectPtr<UAnimSequenceBase> AZP_MarcusWalk;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|MarcusClips") TObjectPtr<UAnimSequenceBase> AZP_MarcusRun;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|MarcusClips") TObjectPtr<UAnimSequenceBase> AZP_MarcusCrouchIdle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|MarcusClips") TObjectPtr<UAnimSequenceBase> AZP_MarcusCrouchWalk;

	/** Live-tunable facing offset for Marcus's visible body (on top of the -90 base
	 *  yaw). Dial in BP_GraceCharacter → Details → Appearance to correct the angle the
	 *  body faces / tilt. Applied every frame so edits show live in PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float AZP_MarcusBodyYaw = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float AZP_MarcusBodyPitch = 0.0f;

	/** Uniform scale of the CCMH Marcus body that drops his eyes to the FP camera height (measured 124/143). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float AZP_MarcusBodyScale = 0.869f;

	// --- Chest-on-look-down try-out (dev 2026-08-07: "I kinda like it") ---
	// The camera rides the Operator's BENT spine while Marcus stays straight, so the camera
	// slides forward past his chest and look-down shows only legs. This curls Marcus's spine
	// with the camera pitch so his (janitor) chest comes into view — one mesh with the unarmed
	// arms, so it joins correctly by construction. UNHOOK = set bAZP_MarcusChestBend false
	// (Class Defaults, no rebuild needed).

	/** Master switch for the look-down chest bend on the Marcus body.
	 *  Round 2 (dev: "attempt at real version needs") — the two try-out killers are addressed:
	 *  the neck chain is counter-rotated to stay upright (no stump curling into the lens) and the
	 *  loco torso sway is damped toward ref pose while bent (no chest bobbing at close range). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	bool bAZP_MarcusChestBend = true;

	/** How much of the walk/idle clip's torso animation is flattened while the bend is active
	 *  (scales with the bend alpha). 1 = chest rides the pelvis rigidly at full look-down —
	 *  which is how your own chest behaves relative to your eyes; 0 = raw clip sway (the
	 *  try-out's "upper chest bobbing"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	float AZP_MarcusChestSwayDamping = 0.85f;

	/** Fraction of the applied bend counter-rotated back out at the neck, keeping the headless
	 *  body's neck stump pointing UP instead of curling into the camera. 1 = stump stays fully
	 *  upright (out of frame above the lens); 0 = the try-out behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	float AZP_MarcusChestNeckUpright = 1.0f;

	/** Root of the neck chain the counter-rotation (and its sway damp) applies from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	FName AZP_MarcusChestNeckBone = TEXT("neck_01");

	/** Total forward spine curl (degrees) at full look-down, spread across the bend bones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	float AZP_MarcusChestBendMaxDegrees = 32.0f;

	/** Look-down pitch (degrees) where the bend starts easing in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	float AZP_MarcusChestBendStartPitch = 12.0f;

	/** Look-down pitch (degrees) of full bend. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	float AZP_MarcusChestBendEndPitch = 72.0f;

	/** Spine chain the curl distributes across (CCMH = UE5-style spine_01..spine_05, verified). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	TArray<FName> AZP_MarcusChestBendBones = { FName("spine_01"), FName("spine_02"), FName("spine_03"), FName("spine_04"), FName("spine_05") };

	/** Bones hidden on the Marcus body + overalls while WEAPON arms are shown. Default = the
	 *  UPPERARMS: hiding at the clavicles collapsed every arm/sleeve vertex into pinch-lumps ON
	 *  the chest silhouette — invisible while the chest was hidden, but the 2026-08-07 "blending
	 *  issue with the pipe hand" once the bend put the chest in frame. Upperarm hides keep the
	 *  chest's own shoulder caps; the view-model arms draw over the stumps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	TArray<FName> AZP_MarcusWeaponArmHideBones = { FName("upperarm_l"), FName("upperarm_r") };

	/** RANGED body anchor — UNHOOKED (default false), 2026-08-07, after BOTH mechanisms failed
	 *  in PIE and proved the conflict is geometric: the ranged camera rides the pack gun socket
	 *  ~70 cm AHEAD of the torso the arms connect to. Slide the body under the camera -> arms
	 *  detach ("arms aren't attached to my body"); pin the body to the arms' torso (this anchor)
	 *  -> chest stays behind the lens (invisible) and swaps drag the offset through the melee
	 *  camera ("camera is now in the chest", "I see into my inners"). NO body placement satisfies
	 *  both while the camera sits out there. The real lever is the CAMERA: pull
	 *  GameplayComp->AZP_CameraRangedForward toward -70 (live knob) so ranged frames from
	 *  Marcus's head like unarmed — chest in frame, arms attached for free — then re-tune gun
	 *  framing via AZP_RangedArmsOffset / weapon offsets by eye. Do NOT re-enable this bool
	 *  except for experiments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	bool bAZP_MarcusRangedBodyAnchor = false;

	/** Bone matched between PlayerMesh (Operator) and MarcusBody (CCMH) — both skeletons are
	 *  UE5-style, spine_05 = chest top where the clavicles hang. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	FName AZP_MarcusRangedAnchorBone = FName("spine_05");

	/** Interp speed for the anchor offset (both engaging and returning to 0). Live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	float AZP_MarcusRangedAnchorInterpSpeed = 10.0f;

	/** Safety clamp (cm) on the forward anchor offset. Live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	float AZP_MarcusRangedAnchorMaxSlide = 120.0f;

	/** Per-bone fraction of the total curl, index-matched to the bones array (sums ~1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|ChestBend")
	TArray<float> AZP_MarcusChestBendWeights = { 0.10f, 0.15f, 0.20f, 0.25f, 0.30f };

	/** Camera look-down angle (degrees) below which MarcusBody's spine starts
	 *  bending forward so the camera never sees inside the chest cavity during
	 *  Kinemation reload/switch dives. No bend above this threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|SpineBend")
	float AZP_SpineBendThresholdDeg = 45.f;

	/** Maximum total spine bend (degrees) when looking straight down (-90 pitch).
	 *  Distributed across spine_01..spine_05 with weights .10/.15/.20/.25/.30,
	 *  more toward the head. 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|SpineBend")
	float AZP_SpineBendMaxDeg = 55.f;

	/** Easing speed (higher = snappier) for SpineBend toward target each frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|SpineBend")
	float AZP_SpineBendInterpSpeed = 12.f;

	/** Bone-LOCAL hinge axis for forward bend. Default (0,1,0) = bone-local +Y
	 *  (right vector) which bends forward on UE5/MetaHuman/CCMH spine convention.
	 *  Flip sign or swap component if the body leans the wrong way. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance|SpineBend")
	FVector AZP_SpineBendBoneLocalAxis = FVector(0.f, 1.f, 0.f);

	/** Per-action CAMERA offset applied during that move so the leaning body doesn't clip
	 *  the lens — the camera is moved instead of the body. Capsule space: +X = forward
	 *  (lens away from the body), +Z = up. Lerped in/out at AZP_WeaponActionOffsetSpeed. One
	 *  each for reload / switch / swing / block — dial independently in Details → Camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector AZP_ReloadCamOffset = FVector(3.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector AZP_SwitchCamOffset = FVector(3.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector AZP_SwingCamOffset = FVector(4.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (DisplayName = "Block Camera Offset (moves VIEW)"))
	FVector AZP_BlockCamOffset = FVector(4.0f, 0.0f, 0.0f);

	/** Camera offset held during a dodge — split by weapon so the melee (pipe) dash,
	 *  where the body leans hard toward the lens, can push the view clear of the body
	 *  independently of the ranged dash. Capsule space: +X = forward (lens away from
	 *  body), +Z = up. Applied while the dodge clearance window is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (DisplayName = "Dodge Camera Offset (Melee)"))
	FVector AZP_DodgeCamOffsetMelee = FVector(15.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (DisplayName = "Dodge Camera Offset (Ranged)"))
	FVector AZP_DodgeCamOffsetRanged = FVector(0.0f, 0.0f, 0.0f);

	/** Maximum degrees the camera may pitch DOWN from horizon (positive value).
	 *  Clamps both controller look input AND animation-driven camera dives
	 *  (Kinemation reload/equip socket pose), so the camera never passes through
	 *  the visible body when looking down. Default 55 = comfortable, eyes can
	 *  still see the floor a couple of meters ahead but never the chest cavity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pitch")
	float AZP_CameraPitchDownLimitDeg = 55.f;

	/** Maximum degrees the camera may pitch UP from horizon (positive value).
	 *  Standard FPS max-look-up cap. Default 80. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pitch")
	float AZP_CameraPitchUpLimitDeg = 80.f;

	/** How fast the camera slides to/from the active offset (higher = snappier). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float AZP_WeaponActionOffsetSpeed = 10.0f;

	/** Camera sits 20cm forward of the FPCamera socket so melee/dodge/block spine leans never put the view inside the body (dev-proven). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float AZP_CameraForwardOffset = 20.0f;

	/** SH2-style darkness: negative exposure bias on the player post-process (darker without crushing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Exposure")
	float AZP_AutoExposureBias = -0.5f;

	/** Exposure floor — lets eyes adapt to moonlit night while sealed dark rooms stay dark (session-64 tuned). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Exposure")
	float AZP_AutoExposureMinBrightness = 0.2f;

	/** Exposure ceiling of the player's clamped auto-exposure window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Exposure")
	float AZP_AutoExposureMaxBrightness = 1.2f;

	/** Bloom on the player post-process — kept low so darkness stays crisp. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Exposure")
	float AZP_PlayerBloomIntensity = 0.2f;


	/** Current interpolated weapon-action camera offset (runtime; fed to GameplayComp). */
	FVector CurrentWeaponActionOffset = FVector::ZeroVector;

	/** Per-hand grip offsets (component space) for the melee view-model arms. The
	 *  post-process hand layer (UZP_MeleeHandsAnimInstance) applies them live; dial
	 *  in Details → Appearance. MeleeHand?Offset is the base grip for ALL melee states
	 *  (idle/swing AND block) — block inherits this exact connection. BlockHand?Offset
	 *  is an OPTIONAL extra added ON TOP only while blocking; leave it zero and block
	 *  grips identically to non-block pipe wielding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector AZP_MeleeHandLOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector AZP_MeleeHandROffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (DisplayName = "Block Hand L Offset (extra, on top)"))
	FVector AZP_BlockHandLOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (DisplayName = "Block Hand R Offset (extra, on top)"))
	FVector AZP_BlockHandROffset = FVector::ZeroVector;

	/** Active melee hand grip offset, chosen by block state — read by the post-process
	 *  hand anim layer. bRight selects hand_r, else hand_l. */
	FVector GetActiveMeleeHandOffset(bool bRight) const;

	/** True while the player is holding a melee block. Read by the post-process hand
	 *  anim layer so it can replay the non-block grip pose during block. */
	bool IsBlockingNow() const { return bIsBlocking; }

	/** Tracks whether Marcus's body arms are currently hidden (weapon view-model up). */
	bool bMarcusArmsHidden = false;

	/** The exact bones currently hidden by the weapon-arms hide (snapshot of the
	 *  AZP_MarcusWeaponArmHideBones knob at hide time, so live knob edits unhide cleanly). */
	TArray<FName> MarcusArmHiddenBones;

	/** Tracks ranged-weapon state: when armed ranged, the Operator PlayerMesh is shown
	 *  (its Kinemation arms hold the gun) and Marcus is hidden. */
	bool bRangedArmedState = false;

	/** Live-tunable framing for the Marcus weapon-arm view-model (CCMH proportions
	 *  differ from the Operator the -155/-90 was tuned for). Dial in Details →
	 *  Appearance, applied every frame so PIE edits show live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector AZP_MeleeViewOffset = FVector(0.0f, 0.0f, -155.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FRotator AZP_MeleeViewRotation = FRotator(0.0f, -90.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float AZP_MeleeViewScale = 1.0f;

	/** View-model bob lag (dev 2026-08-07: "the body bobs but the pipe doesn't when moving").
	 *  A camera-child mesh rides the head bob screen-static; subtracting this fraction of the
	 *  bob offsets makes the pipe+arms LAG the camera like a held object — reads as moving
	 *  with the body. 0 = off (screen-locked, old behavior); 1 = fully world-stable. Live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	float AZP_MeleeViewBobScale = 0.6f;

	/** Extra placement applied to the melee weapon-arm view-model (the ARM RIG —
	 *  MeleeViewMesh + sleeves + hands) ONLY while blocking — added on top of
	 *  AZP_MeleeViewOffset. Moves the ARMS, NOT the camera. X = forward/back,
	 *  Y = left/right, Z = up/down. Dial in Details → Appearance; applied every frame
	 *  so PIE edits show live — tune it WHILE holding a block. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (DisplayName = "Block Arms Offset (moves arms)"))
	FVector AZP_BlockViewOffset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TObjectPtr<UZP_GraceGameplayComponent> GameplayComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kinemation")
	TObjectPtr<UZP_KinemationComponent> KinemationComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<UZP_HealthComponent> HealthComp;

	/** Marcus's blood identity — NORMAL red (the enemies' components run the dark-purple class
	 *  defaults). Drives the grab-bite spurts; anything that ever calls PlayHitBloodFor on the
	 *  player resolves this too. Colors/sizes are knobs on the component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	TObjectPtr<class UZP_BloodFXComponent> BloodFXComp;

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

	/** Health fraction below which the low-health vignette starts (also the map range start). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Vignette")
	float AZP_VignetteHealthThreshold = 0.5f;

	/** Vignette intensity at 0% HP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Vignette")
	float AZP_VignetteMaxIntensity = 1.5f;

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
	TObjectPtr<USoundBase> AZP_FlashlightClickSound;

	// --- Footsteps (own-body foley; distance-based — the SingleNode locomotion has no anim notifies) ---

	/** THE footstep surface table (DA_Footsteps): one row per surface with sounds + volume/pitch +
	 *  matching (PhysMat SurfaceType, or material-name keywords for unauthored pack floors).
	 *  Author it ONCE in the editor and extend forever — no code, no rebuilds. Lazily defaults to
	 *  /Game/Core/Data/DA_Footsteps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TObjectPtr<UZP_FootstepData> AZP_FootstepData;

	/** Hard fallback set used only when AZP_FootstepData is missing/unmatched AND its AZP_DefaultSounds
	 *  are empty. Defaults to the Moonville hard-surface set (SW_Footstep_1..6). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Footsteps")
	TArray<TObjectPtr<USoundBase>> AZP_FootstepSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float AZP_FootstepVolume = 0.4f;

	/** Extra volume on sprint steps (heavier footfalls). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float AZP_SprintFootstepVolumeMul = 1.25f;

	/** Distance (UU) between steps while walking. Crouch steps at 0.85x this, half volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float AZP_FootstepWalkStride = 170.f;

	/** Distance (UU) between steps while sprinting. MUST be well under WalkStride x (Sprint/Walk
	 *  speed ratio) or sprint cadence sounds identical to walking — at 260 walk / 390 sprint,
	 *  150 gives ~2.6 steps/s vs ~1.5 walking (clearly a run). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float AZP_FootstepSprintStride = 150.f;

	/** Speed below which no footsteps play (standing/drifting). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float AZP_FootstepMinSpeed = 60.f;

	/** Stride multiplier for crouch steps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float AZP_FootstepCrouchStrideMul = 0.85f;

	/** Volume multiplier for crouch steps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float AZP_FootstepCrouchVolumeMul = 0.5f;

	/** Default footstep pitch-jitter floor when no surface row matches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float AZP_FootstepPitchMinDefault = 0.92f;

	/** Default footstep pitch-jitter ceiling when no surface row matches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	float AZP_FootstepPitchMaxDefault = 1.08f;

	/** How quickly the flashlight tracks the camera (higher = snappier, lower = more chest lag). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	float AZP_FlashlightInterpSpeed = 8.0f;

	/** Downward pitch offset from camera direction (chest naturally points slightly down). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight")
	float AZP_FlashlightPitchOffset = -5.0f;

	/** iPhone-torch beam intensity — corridor throw of the chest spotlight (dev-tuned profile). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flashlight")
	float AZP_FlashlightIntensity = 25000.0f;

	/** Concentrated hotspot cone of the flashlight beam (distance punch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flashlight")
	float AZP_FlashlightInnerConeAngle = 16.0f;

	/** Wide phone-LED spill cone for close range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flashlight")
	float AZP_FlashlightOuterConeAngle = 40.0f;

	/** Potential reach (50 m) of the flashlight beam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flashlight")
	float AZP_FlashlightAttenuationRadius = 5000.0f;

	/** Ambient fill point-light intensity simulating flashlight bounce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flashlight")
	float AZP_FlashlightFillIntensity = 670.0f;

	/** Radius of the fill light's ambient coverage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flashlight")
	float AZP_FlashlightFillAttenuationRadius = 600.0f;

	// --- Configuration (propagated to components in PostInitializeComponents) ---

	/** DataAsset with all movement tuning values. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UZP_GraceMovementConfig> AZP_MovementConfig;

	/** Blueprint class of weapon to spawn. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	TSubclassOf<AActor> AZP_WeaponClass;

	/** How far the gun reaches ahead of the camera. The capsule is held this
	 *  far off any wall the player faces so the muzzle never clips through
	 *  geometry (the body capsule alone stops short of the gun's reach).
	 *  Moves the whole capsule (camera rides it  normal); NEVER PlayerMesh.
	 *  0 disables. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float AZP_GunCollisionReach = 75.0f;

	/** Radius of the muzzle clearance probe. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	float AZP_GunCollisionRadius = 6.0f;

	/** Decal materials for bullet impacts. Set in BP child via Python. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TArray<TSoftObjectPtr<UMaterialInterface>> AZP_BulletDecalMaterials;

	/** Skeletal mesh for hidden locomotion Mesh. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<USkeletalMesh> AZP_LocomotionSkeletalMesh;

	/** Idle animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> AZP_IdleAnimation;

	/** Walk animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> AZP_WalkAnimation;

	/** Run animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> AZP_RunAnimation;

	/** Crouch idle animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> AZP_CrouchIdleAnimation;

	/** Crouch walk animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> AZP_CrouchWalkAnimation;

	/** Ground speed above which idle switches to walk clips (also the Marcus body mirror). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float AZP_LocoWalkSpeedThreshold = 10.0f;

	/** Ground speed above which walk switches to run clips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	float AZP_LocoRunSpeedThreshold = 150.0f;

	/** Ladder climb up loop animation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> AZP_LadderClimbUpAnimation;

	/** Ladder climb down loop animation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> AZP_LadderClimbDownAnimation;

	/** Ladder idle animation (holding on, not moving). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> AZP_LadderIdleAnimation;

	/** Animated climb-over-the-top exit (root motion carries the body onto the
	 *  upper floor). Default A_Climb_Up_out_Right_UE5, set via set_all_cdo.py. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	TObjectPtr<UAnimSequenceBase> AZP_LadderTopExitAnimation;

	/** Playback speed of the climb-over-the-top exit (anim + camera). 2.0 = double. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion|Ladder")
	float AZP_LadderTopExitPlayRate = 2.0f;

	/** UU per ladder rung; drives climb anim scrubbing and rung snapping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Ladder")
	float AZP_LadderRungSpacing = 23.5f;

	/** Distance the capsule stands off the ladder center while climbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Ladder")
	float AZP_LadderStandoffDist = 75.f;

	/** UU pushed toward/past the ladder center onto the upper floor at top exit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Ladder")
	float AZP_LadderTopExitPush = 120.f;

	/** Offset below the ladder top that caps the highest climbable position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Ladder")
	float AZP_LadderTopClearance = 80.f;

	// --- Input Actions (set in Blueprint child, e.g. BP_GraceCharacter) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AZP_MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AZP_LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AZP_SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AZP_JumpAction;

	// --- Movement feel ---

	/** Backpedal speed multiplier — backward input is scaled by this, and the CMC reduces max
	 *  speed proportionally to input magnitude, so walking backwards moves at this fraction of
	 *  forward speed. Deliberately harsh (dev 2026-07-03: backing away from the Shambler sprint
	 *  must not be a free escape — turn and run, or dodge). Was a hardcoded 0.55. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AZP_BackpedalSpeedMul = 0.4f;

	// --- Dodge (replaces Jump on space bar) ---

	/** Horizontal launch velocity (cm/s) applied on dodge. With ground braking
	 *  this overrides the CMC velocity and decelerates to a stop, so the dash
	 *  COVERS roughly DodgeImpulse²/(2·braking) cm — ~1200 ≈ a 3m lunge. Tuned
	 *  up from 400 (which was below sprint speed and felt like a walk). If the
	 *  spine/camera yanks at high values, that's the locomotion blend reading a
	 *  one-frame "running" spike — tune down rather than re-architecting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float AZP_DodgeImpulse = 1200.f;

	/** Stamina consumed per dodge, as a PERCENT of max stamina (0-100). The
	 *  dodge is blocked if the player doesn't have at least this much. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float AZP_DodgeStaminaCostPercent = 20.f;

	/** Seconds after a dodge fires during which sprinting and weapon swapping
	 *  are locked out (the player is committed to the dash). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float AZP_DodgeLockWindow = 0.5f;

	/** Cooldown between dodges (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float AZP_DodgeCooldown = 0.8f;

	/** FPP_sns_Dodge — played on MeleeViewMesh. Doesn't drive PlayerMesh so the
	 *  firearm grip on hand_r is never disturbed; visible only when the melee
	 *  view model is active (pipe up). Movement impulse fires either way. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	TSoftObjectPtr<UAnimSequenceBase> AZP_DodgeAnim;

	// --- Fall Damage ---

	/** Drop distance (cm, peak-to-landing) below which a fall deals NO damage —
	 *  normal jumps and short hops are free. At/just above this, damage starts at
	 *  AZP_FallDamageMinAmount. Tune to ~1 story so a one-floor drop hurts (30 HP) but
	 *  stepping off a curb doesn't. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Damage")
	float AZP_FallDamageMinDistance = 300.f;

	/** Drop distance (cm) at/above which a fall deals AZP_FallDamageMaxAmount (lethal).
	 *  Tune to ~2 stories. Between min and max, damage lerps min→max amount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Damage")
	float AZP_FallDamageMaxDistance = 600.f;

	/** Damage dealt at exactly AZP_FallDamageMinDistance (1-story drop). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Damage")
	float AZP_FallDamageMinAmount = 30.f;

	/** Damage dealt at/above AZP_FallDamageMaxDistance (2-story drop = lethal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Damage")
	float AZP_FallDamageMaxAmount = 100.f;

	// --- Block (RMB hold while pipe equipped) ---

	/** True while RMB is held with the pipe active. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Block")
	bool bIsBlocking = false;

	/** True from RMB-release until the BlockStop clip finishes. Keeps the block
	 *  camera/arm offsets alive through the pose-out so the body doesn't swing into
	 *  the lens during the resolve. */
	bool bBlockResolving = false;

	/** True while RMB is physically held with the pipe (the INTENT to block). Decoupled from
	 *  bIsBlocking so a press during a swing is BUFFERED: Tick engages the guard the instant the
	 *  swing allows (post-contact recovery cancel) instead of discarding the press. */
	bool bBlockWanted = false;

	/** Incoming damage is multiplied by this while blocking (0.25 = 75% off). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	float AZP_BlockDamageReductionMul = 0.25f;

	/** Sound of an attack being CAUGHT ON THE GUARD — plays with the block-impact reaction.
	 *  Ships as SFX_MELEE_BLOCK, a duplicate of SFX_MELEE_IMPACT1; replace the asset's wave with
	 *  the real block sound (same pattern as the per-surface pipe impacts). Set it on
	 *  BP_GraceCharacter -> Class Defaults -> Combat|Block. Null = silent, no fallback: a wrong
	 *  sound here would read as a hit landing on Marcus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Block")
	TObjectPtr<USoundBase> AZP_BlockImpactSound;

	/** Volume multiplier for AZP_BlockImpactSound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Block")
	float AZP_BlockImpactVolume = 1.0f;

	/** Ground speed above which the held block switches from BlockLoop to BlockWalk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Block")
	float AZP_BlockWalkSpeedThreshold = 50.f;

	// ── Block stamina (blocking is a resource, not a free wall) ─────────
	/** Percent of max stamina a BLOCKED HIT costs (~1/3 per dev direction). The ONLY stamina cost
	 *  of blocking — holding the pose is free. Not enough left = GUARD BREAK: the hit lands at
	 *  full damage and the guard drops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Block")
	float AZP_BlockHitStaminaPercent = 33.3f;

	/** Minimum stamina fraction (0..1) required to RAISE (or re-raise) the guard — you cannot put
	 *  up a guard you can't pay for (default = one blocked hit's worth). Guard comes back as
	 *  stamina recovers past this while RMB is still held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Block")
	float AZP_BlockMinStaminaFractionToStart = 0.334f;

	// ── Stagger (exposed knobs) ──────────────────────────────────────
	// MELEE DESIGN (RE/SH2R-style): your swings do damage but do NOT stop the enemy — staggering
	// is the BLOCK reward. Spam-trading loses (enemies swing into your combo); block/dodge wins.
	/** Seconds the attacker is staggered after a successful BLOCK — the counter window (1-2 free hits). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stagger")
	float AZP_BlockStaggerDuration = 1.2f;

	/** Minimum seconds between block-staggers. Kept BELOW enemy swing cadence (~1.9s) so every
	 *  well-executed block rewards; it only exists as a safety floor against stagger spam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stagger")
	float AZP_BlockStaggerCooldown = 1.5f;

	/** Seconds the enemy is staggered by a landed melee (pipe) HIT. 0 = hits NEVER stagger — that
	 *  is the intended design (0.5 let the player stun-lock enemies by spamming; they died without
	 *  ever swinging back). Raise above 0 only to deliberately re-enable hit-staggers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stagger")
	float AZP_HitStaggerDuration = 0.f;

	/** Minimum seconds between melee-hit staggers on the SAME enemy chain. Irrelevant while
	 *  AZP_HitStaggerDuration = 0 (hits never stagger). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stagger")
	float AZP_HitStaggerCooldown = 0.0f;

	// --- Committed melee swing (camera / feel, 2026-07-05) ---
	// A pipe swing COMMITS: movement + aim lock for a short window, a small forward step-in, the view
	// WHIPS through the arc (direction matched to the F/R/L swing variant), and a directional camera
	// KICK punctuates contact. All camera motion is applied to the CONTROL ROTATION (Kinemation's camera
	// follows it) — PlayerMesh is NEVER moved (FPCamera-socket dead-end). The damage sweep uses the aim
	// captured at swing start, so the whip can move the view without making the swing miss. Degrees.

	/** Master toggle for the committed-swing camera/lock behaviour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	bool bAZP_MeleeCommitEnabled = true;

	/** Freeze movement input during the commit window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	bool bAZP_MeleeLockMove = true;

	/** Freeze look/aim input during the commit window (the swing owns the view). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	bool bAZP_MeleeLockAim = true;

	/** Seconds the commit window (lock + camera whip) lasts — kept near the strike, not the whole
	 *  return-to-idle, so control returns quickly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	float AZP_MeleeCommitDuration = 0.5f;

	/** Peak yaw (deg) the view whips for a LEFT/RIGHT swing — the perspective sweeps with the pipe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	float AZP_MeleeWhipYaw = 13.f;

	/** Peak pitch (deg) the view dips for a FORWARD/overhead swing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	float AZP_MeleeWhipPitch = 10.f;

	/** Forward step-in speed (cm/s) launched at swing start — a small committed lunge (collision-safe). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	float AZP_MeleeLungeSpeed = 250.f;

	/** Impact KICK peak yaw (deg), signed by swing direction — the directional snap on contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	float AZP_MeleeKickYaw = 6.f;

	/** Impact KICK peak pitch (deg) — the view jolts up on contact (all swing directions). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	float AZP_MeleeKickPitch = 7.f;

	/** Impact KICK decay time (seconds) back to zero — sharp = small. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	float AZP_MeleeKickDuration = 0.14f;

	/** Grace window (seconds) at the END of a pipe swing during which a pending block press may cancel
	 *  the remaining return-to-idle TAIL and raise the guard instead of waiting the whole clip out. The
	 *  strike + follow-through still play; only the dead frames left when the swing has <= this many
	 *  seconds remaining get replaced by the guard coming up. Larger = block sooner after a swing
	 *  (0 = must wait out the full clip). Opens the block window earlier than KinemationComp's
	 *  AZP_MeleeBlockCancelFraction — whichever opens first wins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee Feel")
	float AZP_MeleeToBlockGracePeriod = 0.5f;

	/** Kubold FPP_Longs_BlockLoop — held block pose, standing still. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> AZP_BlockLoopAnim;

	/** Kubold FPP_Longs_BlockWalk — block pose while walking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> AZP_BlockWalkAnim;

	/** Kubold FPP_Longs_BlockStart — pose-in motion before the loop. Gives the
	 *  eye visible motion so the held BlockLoop doesn't read as a freeze-frame. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> AZP_BlockStartAnim;

	/** Kubold FPP_Longs_BlockStop — pose-out motion after release. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> AZP_BlockStopAnim;

	/** Kubold FPP_Longs_BlockImpact 1/2/3 — random reaction on a blocked hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> AZP_BlockImpact1Anim;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> AZP_BlockImpact2Anim;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> AZP_BlockImpact3Anim;

	/** FPP_Longs_Idle — return-to-hold pose after dodge/block-release. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Block")
	TSoftObjectPtr<UAnimSequenceBase> AZP_MeleeIdleHoldAnim;

	// --- Grab / Struggle (zombie grapple — Docs/Plan_GrabStruggle.md) ---

	/** Current grab phase. None = not grabbed. Every input handler gates on this;
	 *  LMB becomes the mash-to-escape input while != None. */
	UPROPERTY(BlueprintReadOnly, Category = "Grab")
	EZP_GrabPhase GrabPhase = EZP_GrabPhase::None;

	// IZP_Grabbable — an enemy asks to latch on / releases us.
	virtual EZP_GrabAttemptResult TryBeginGrab(AActor* Grabber) override;
	virtual void AbortGrab() override;
	virtual bool IsGrabRecovering() const override
	{
		return GrabPhase == EZP_GrabPhase::FailKnockdown || GrabPhase == EZP_GrabPhase::GetUp;
	}

	/** Each LMB press adds this to the escape meter (threshold 1.0 wins). ~9 clean presses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Struggle")
	float AZP_MashGainPerPress = 0.12f;

	/** MINIMUM bite beat (s, from Munch starting) before a mash press may flip the pair to the
	 *  Wrestle clips. ROOT-CAUSED 2026-07-03 ([LatchProbe]): a player who ran in fighting is
	 *  still clicking at the latch, the first press landed 0.02-0.07s into the bite, and Marcus
	 *  took two hard single-node pose cuts within ~3 frames while the shambler was 0.3s of
	 *  blend behind — the "latch glitch". Presses inside this window still count their meter
	 *  gain; only the visual pair-switch waits. 0 = old instant behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Struggle")
	float AZP_GrabMinMunchTime = 0.5f;

	/** Escape meter drain per second (framerate-independent — presses are counted, not polled). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Struggle")
	float AZP_MashDecayPerSecond = 0.25f;

	/** Seconds (from the bite phase starting) to reach the escape threshold before the
	 *  knockdown fail state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Struggle")
	float AZP_StruggleTimeLimit = 4.0f;

	/** Accessibility: true = HOLD attack to fill the meter at AZP_HoldFillPerSecond instead of
	 *  tapping (TLOU/RE4R "Repeated Input Type: Hold"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Struggle")
	bool bAZP_EscapeHoldMode = false;

	/** Meter fill per second while attack is held (Hold accessibility mode only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Struggle")
	float AZP_HoldFillPerSecond = 0.5f;

	/** Ticking damage while HELD (munch + wrestle), landing once per second. HALVED 6.25 ->
	 *  3.125 with the Shambler's AZP_AttackDamage (dev 2026-07-03: everything it deals, halved —
	 *  "feels very overpowered"). The 2026-07-02 ratio derivation still holds at the new
	 *  scale: fastest escape (2s) = 2 ticks = half an attack (12.5); riding the full
	 *  AZP_StruggleTimeLimit (4s) to failure = one full attack, then the knockdown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Damage")
	float AZP_GrabTickDamagePerSecond = 3.125f;

	/** MINIMUM seconds trapped (from the bite phase starting) before an escape can complete —
	 *  even a perfect mash eats this long (and its damage ticks). The meter can be full
	 *  earlier; the break fires the moment this gate opens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Struggle")
	float AZP_GrabMinTrappedTime = 2.f;

	/** EXTRA damage chunk on struggle failure, on top of the accumulated ticks (which already
	 *  total one full attack by the fail point). 0 = ticks only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Damage")
	float AZP_FailDamageChunk = 0.f;

	/** Seconds after breaking free during which NO enemy may grab (anti-chain-grab). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Rules")
	float AZP_PostEscapeGrabImmunity = 3.0f;

	/** Over-the-right-shoulder camera frame during the struggle: behind / right / above the head.
	 *  THE distance knob — live-tunable in PIE (BP_GraceCharacter → Details → Grab|Camera). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Camera")
	float AZP_GrabCamBack = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Camera")
	float AZP_GrabCamRight = 55.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Camera")
	float AZP_GrabCamUp = 25.f;

	/** 1P→3P blend seconds (fits inside the 0.6s grab entry). Raised 0.35 -> 0.5 with the
	 *  smootherstep curve (dev 2026-07-03: "smoother in and out"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Camera")
	float AZP_GrabCamBlendIn = 0.5f;

	/** Seconds the VIEW swings onto the grabber at the latch. The old code TELEPORTED the
	 *  control rotation in one frame — the residual "camera jerk" no blend curve could hide
	 *  (dev 2026-07-03: "cutscene-like"). Smootherstep-eased; look input is gated for the
	 *  whole grab so nothing fights it. 0 = the old instant snap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Camera")
	float AZP_GrabFaceBlendTime = 0.35f;

	/** 3P→1P blend seconds (tail of the kick/push escape, or the start of the knockdown).
	 *  Raised 0.3 -> 0.45 with the smootherstep curve (dev 2026-07-03). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Camera")
	float AZP_GrabCamBlendOut = 0.45f;

	/** Damage-vignette floor held while grabbed (bites still pulse it to max on top). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|HUD")
	float AZP_GrabVignetteHold = 0.45f;

	/** Flashlight intensity multiplier while grabbed — dims the beam+fill to a silhouette-read
	 *  level instead of killing them (a pitch-dark room must not go fully black for the whole
	 *  grapple). 1 = unchanged, 0 = off. Restored on release. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Flashlight")
	float AZP_GrabFlashlightDimMul = 0.35f;

	/** Additive BONE-LOCAL rotations on Marcus's upper arms during the 3P grapple — dial out
	 *  arm clipping against the Shambler. Applied by the MarcusBody post-process layer on top
	 *  of the paired clips; live-tunable in PIE, zero = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Pose")
	FRotator AZP_GrabArmLRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Pose")
	FRotator AZP_GrabArmRRotation = FRotator::ZeroRotator;

	/** Player-facing mash prompt shown while grabbed — PLACEHOLDER: author the real wording in
	 *  BP_GraceCharacter → Details → Grab|HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|HUD")
	FText AZP_GrabPromptText = NSLOCTEXT("TheSignal", "GrabMashPrompt", "PRESS TO BREAK FREE");

	/** True when the most recent input came from a gamepad (lightweight per-tick poll of common
	 *  pad buttons/sticks vs mouse/movement keys). Drives which button glyph UI prompts show. */
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	bool bLastInputGamepad = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AZP_InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AZP_CrouchAction;

	/** Q key — lean peek around cover (camera only, no weapon aim). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> AZP_PeekAction;

	/** RMB — aim down sights. Auto-peeks with weapon when near cover. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> AZP_AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> AZP_FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> AZP_ReloadAction;

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
	TObjectPtr<UInputAction> AZP_MapAction;

	/** Input action for cycling inventory tabs left (Q key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> AZP_TabCycleLeftAction;

	/** Input action for cycling inventory tabs right (E key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> AZP_TabCycleRightAction;

	// --- Inventory ---

	/** True when inventory UI is open — blocks all gameplay input. */
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	bool bInventoryMenuOpen = false;

	/** True while a save-point menu is open. Routes gamepad to the UI (UIOnly) and gates the raw
	 *  weapon-slot polling so the D-pad navigates the menu instead of swapping weapons. */
	UPROPERTY(BlueprintReadOnly, Category = "SavePoint")
	bool bSaveMenuOpen = false;

	/** True while Moonville's first-time-pickup notification is open — we pull IMC_Grace + go
	 *  GameAndUI so keyboard/gamepad can dismiss it (Moonville leaves it in GameOnly = mouse-only). */
	bool bPickupMenuActive = false;

	/** The open save-point widget — watched in Tick to restore game input when it closes. */
	UPROPERTY()
	TWeakObjectPtr<UUserWidget> ActiveSaveMenu;

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
	TObjectPtr<UInputAction> AZP_InventoryMenuAction;

	/** Shortcut cross slot input actions (keys 1-4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> AZP_InventorySlot0Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> AZP_InventorySlot1Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> AZP_InventorySlot2Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Inventory")
	TObjectPtr<UInputAction> AZP_InventorySlot3Action;

	/** Flashlight toggle action (IA_InventoryFlashlight). Mapped in IMC_Grace to F + Right Shoulder. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AZP_FlashlightAction;

	/** PDA_Item data asset for starting weapon. Granted to inventory at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSoftObjectPtr<UObject> AZP_StartingWeaponItem;

	/** Frames the grab-the-whole-pile sweep keeps grabbing next-closest pickups after one E press. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Pickup")
	int32 AZP_GrabAllSweepTicks = 20;

	/** Max distance for an in-use container to count as arm's reach (guards against stale in-use flags hijacking E). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Container")
	float AZP_ContainerReachDistance = 500.f;

	// --- Interaction ---

	/** Crosshair line-trace reach (UU) for pickups/containers/doors on E press. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float AZP_InteractTraceDistance = 300.0f;

	/** Through-wall interact protection source (2026-08-05). Moonville's own
	 *  PreventInteractionThroughWall traces from the PAWN'S ACTOR LOCATION (waist height) to
	 *  the item — that ray clips the tier board under any EYE-LEVEL shelf pickup and silently
	 *  kills the E press (popup shows, nothing happens; log-proven on a second-tier item).
	 *  true = turn the pack's waist ray OFF at BeginPlay and gate the press with a
	 *  CAMERA->item visibility trace instead (if you can see it, you can grab it; real walls
	 *  still block, and the pack still requires the InteractionArea overlap, so range is
	 *  unchanged). false = stock Moonville waist ray. Applied at BeginPlay — restart PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bAZP_CameraInteractLOS = true;

	/** Hide under-outfit FP body sections at BeginPlay (dev 2026-08-05: "my actor's skin is
	 *  breaking through the pants" on look-down). The Operator body carries full skin geometry
	 *  BENEATH the outfit, and the FBX import also left an unassigned leftover slot
	 *  (Fbx_Default_Material_1) of under-body geometry wearing the pants material — both poke
	 *  through the clothes in the FP spine-bend. false = restore everything instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	bool bAZP_HideBodySkin = true;

	/** Material SLOT names whose mesh sections get hidden by bAZP_HideBodySkin. Operator slot
	 *  map: 0 Boots, 1 Gloves, 2 Googles, 3 Glass, 4 Skin, 5 Eye, 6 Headset, 7 Mask,
	 *  8 Pouches, 9 Mask_001, 10 Pants, 11 Fbx_Default_Material_1, 12 Tshirt, 13 Vest,
	 *  14 Pouches_001 — add/remove names here live if anything still pokes through/goes
	 *  missing (applied at BeginPlay; restart PIE). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	TArray<FName> AZP_HiddenBodySections = { FName(TEXT("MI_Skin")), FName(TEXT("Fbx_Default_Material_1")) };

	/** PIE console diagnostic (` console while possessing): "BodySlotToggle <index>" flips every
	 *  mesh section of that material slot on the FP body live — toggle slots until the offending
	 *  geometry vanishes, then bake the answer into AZP_HiddenBodySections. "BodySlotList"
	 *  prints every slot index/name and its current state to the log/screen. */
	UFUNCTION(Exec)
	void BodySlotToggle(int32 SlotIndex);

	/** Companion of BodySlotToggle — prints the FP body's slot map + hidden states. */
	UFUNCTION(Exec)
	void BodySlotList();

	/** PIE console: dump every FP-relevant mesh's visibility state (asset, Visible,
	 *  OnlyOwnerSee, OwnerNoSee). Run it while seeing a wrong body — the one with
	 *  OwnerNoSee=0 (or an unexpected Visible=1) is the mesh you are looking at. */
	UFUNCTION(Exec)
	void FPBodyDump();

	/** Apply one slot's section visibility on the FP body across all LODs. */
	void SetBodySlotVisible(int32 SlotIndex, bool bVisible);

	/** Runtime-hidden slot indices on PlayerMesh (BeginPlay list + console toggles). */
	TSet<int32> HiddenBodySlots;

	/** PACK-NATIVE apparel skin masking (dev 2026-08-06: "pants are not covering my legs...
	 *  skin breaking through" — keep the pants mesh, keep the legs). The Character Customizer
	 *  skin material (M_Skin_CCMH) WPO-SHRINKS every body vertex painted into an
	 *  ApparelMask_<slot> texture so covered skin tucks INSIDE the apparel — the exact dress-up
	 *  step the pack's customizer runs and our manual SetupMarcusAppearance always skipped
	 *  (the MI defaults sit on the neutral T_DefaultWPO_Mask = zero shrink). Nothing is hidden
	 *  or removed; exposed skin (hands/neck/head) is untouched. Applied at BeginPlay via a
	 *  DYNAMIC MI on MarcusBody only — the shared MI_Skin_Body_CCMH asset is never edited, so
	 *  the ranged-arm view models that borrow it keep full skin. false = raw skin (old look). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	bool bAZP_ApparelSkinMask = true;

	/** ApparelMask_0 (Upper Body apparel slot). Marcus wears Overalls_01, whose own
	 *  DT_Apparel_M_UpperBody+ row assigns T_ApparelMask_FullBody (full-body suit: arms,
	 *  torso, legs shrunk under the cloth; hands stay). Empty = leave the MI's neutral mask. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	TSoftObjectPtr<UTexture> AZP_ApparelMaskUpper = TSoftObjectPtr<UTexture>(FSoftObjectPath(
		TEXT("/Game/CharacterCustomizer/Characters/CCMH/Apparel/ApparelMasks/T_ApparelMask_FullBody.T_ApparelMask_FullBody")));

	/** ApparelMask_1 (Lower Body apparel slot). Marcus has no lower-only garment — the
	 *  overalls row even hides that slot — so this stays empty (neutral mask, no shrink). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	TSoftObjectPtr<UTexture> AZP_ApparelMaskLower;

	/** ApparelMask_2 (Footwear apparel slot). Sneaker_02 -> T_ApparelMask_Shoes per its
	 *  DT_Apparel_M_Footwear row (feet shrunk inside the sneakers). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	TSoftObjectPtr<UTexture> AZP_ApparelMaskFootwear = TSoftObjectPtr<UTexture>(FSoftObjectPath(
		TEXT("/Game/CharacterCustomizer/Characters/CCMH/Apparel/ApparelMasks/T_ApparelMask_Shoes.T_ApparelMask_Shoes")));

	/** 'Neck Shrink' scalar on the body skin MI — the Overalls_01 row sets 1 (tuck the body's
	 *  neck stub under the collar; the visible neck belongs to the head mesh). Negative =
	 *  leave the MI default (0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	float AZP_ApparelNeckShrink = 1.0f;

	/** 'Upper Body_Mask_Mult' — shrink strength for skin under ApparelMask_0 (the overalls'
	 *  FullBody mask: torso+arms+legs). MI default is 3; raised to 5 after the pipe-walk
	 *  right-leg poke (2026-08-06 round 2: "just barely" clipping at leg-swing extremes).
	 *  The wrist/collar boundary sits under the sleeve/collar so a deeper tuck stays
	 *  invisible. Negative = leave the MI default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	float AZP_ApparelMaskMultUpper = 5.0f;

	/** 'Lower Body_Mask_Mult' — shrink strength under ApparelMask_1 (unused on Marcus).
	 *  Negative = leave the MI default (3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	float AZP_ApparelMaskMultLower = -1.0f;

	/** 'Footwear_Mask_Mult' — shrink strength under ApparelMask_2 (T_ApparelMask_Shoes).
	 *  Negative = leave the MI default (1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	float AZP_ApparelMaskMultFootwear = -1.0f;

	/** GLOVE match for the unarmed hands (dev 2026-08-06: "unarmed hands don't use the same
	 *  gloves as the other weapons"). The weapon view models are bare-hand geometry painted
	 *  with AZP_MeleeHandMaterial (M_Gloves) — a flat dark-leather tone. The CCMH body is one
	 *  material section, so its hands get the same tone via the skin material's TATTOO layer
	 *  (base-color lerp by alpha, body branch only): this texture paints ONLY the hand UV
	 *  islands (built from the mesh's own hand/finger skin weights, glove tone averaged from
	 *  the real T_Gloves_BaseColor) and is fully transparent elsewhere. Applied on the same
	 *  MarcusBody dynamic MI with identity placement (offset 0 / scale 1 / rotation 0).
	 *  Empty = bare skin hands (old look). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Body")
	TSoftObjectPtr<UTexture> AZP_UnarmedGloveOverlay = TSoftObjectPtr<UTexture>(FSoftObjectPath(
		TEXT("/Game/Marcus/Textures/T_Marcus_GloveOverlay.T_Marcus_GloveOverlay")));

	/** Cosine of the look-at cone (~60 deg) required for a door overlap to consume the E press. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float AZP_DoorLOSDotThreshold = 0.5f;

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

	/** Stagger an enemy for Duration seconds via the IZP_Staggerable interface (checks the actor
	 *  AND its components, so component-based AI like the Shambler works too). No-op if the target
	 *  doesn't implement the interface — new enemies become staggerable just by implementing it. */
	void StaggerEnemy(AActor* Enemy, float Duration);

	/** Stagger an enemy using AZP_HitStaggerDuration — called by the melee component on a landed pipe hit. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Stagger")
	void MeleeStaggerEnemy(AActor* Enemy);

	/** Begin the committed-swing camera/lock for one pipe swing. Called by the melee component at swing
	 *  start. SwingDir: 0 = Forward/overhead, 1 = Right, 2 = Left (matches the F->R->L anim cycle). */
	void BeginMeleeCommitSwing(int32 SwingDir);

	/** Fire the directional impact kick — called by the melee component when a swing connects. */
	void MeleeCommitImpact(int32 SwingDir);

	/** Called by AZP_SavePoint after it spawns + inits the save widget: takes UI-only input, focuses
	 *  the menu for controller navigation, and starts watching (in Tick) for it to close so game
	 *  input is restored. */
	void OpenSaveMenu(UUserWidget* Menu);

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
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

private:
	// --- Death / Vignette ---
	UFUNCTION()
	void HandleDeath();

	UFUNCTION()
	void UpdateHealthVignette(float NewHealth, float MaxHealth, float DamageAmount);

	/** Cancel any active sprint because the player committed to an action (dodge, weapon use, aim,
	 *  reload, weapon swap, peek, menu, etc.). Called from every action handler EXCEPT movement,
	 *  look, and door/item interact — those must not break a sprint. */
	void CancelSprintFromAction();

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

	/** True while a modal menu owns the screen (save point OR pause menu). Menu-open input handlers
	 *  early-out on this so the player can't pull up the inventory/notes/map tabs UNDER those menus. */
	bool IsModalMenuOpen() const;

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
	void Input_Flashlight(const FInputActionValue& Value);

	/** Calls Moonville's ExecuteItemActionByShortcut for consumable items. */
	void UseItemFromShortcutSlot(int32 SlotIndex);

	/** Consume-action classes (PDA_Item.ItemActionActor) refused when health is already FULL —
	 *  the quickslot press does nothing and the item is NOT consumed. Class-name prefix match
	 *  ("BP_ConsumeHealthAction" matches the _C runtime class). Pure heals only — buff actions
	 *  (invincibility/damage reduction) stay usable at full HP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Consumables", meta = (AllowPrivateAccess = "true"))
	TArray<FString> AZP_FullHealthRefuseActions = { TEXT("BP_ConsumeHealthAction"), TEXT("BP_ConsumeHydrationAction") };

	/** Optional 2D cue played when a heal item is refused at full HP. None = silent refuse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Consumables", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> AZP_HealRefuseSound;

	/** Adds AZP_StartingWeaponItem to inventory at BeginPlay. */
	void GrantStartingItems();

	// --- Inventory persistence via EasyGameUI's per-slot save FILE ---
	// The player carries a BP_EasySaveGameComponent (added in BP_GraceCharacter). On SAVE/LOAD it fires its
	// LoadingOrSavingVariables dispatcher with the chosen slot's JSON; we hook that to serialize/restore the
	// Moonville grid FAITHFULLY (UE text export keeps stack amounts/durability/positions). Tied to the save
	// FILE — a fresh PIE / new game just runs GrantStartingItems (no auto-restore).

	/** Bind OnEguiSaveLoadVariables to the BP_EasySaveGameComponent's LoadingOrSavingVariables dispatcher. */
	void BindEguiSaveComponent();

	/** EGUI save/load hook. OperationType: 0 = Save (write inventory text into Json), 1 = Load (read + apply). */
	UFUNCTION()
	void OnEguiSaveLoadVariables(uint8 OperationType, FJsonObjectWrapper JsonObject);

	/** Export the Moonville grid (ItemSlots) to UE text + read InventorySizeExpansion. Empty if no inv comp. */
	FString ExportInventoryText(FVector2D& OutSizeExpansion) const;

	/** Apply previously-exported inventory text via Moonville LoadInventoryFromSavegame (faithful: keeps
	 *  stack amounts, durability, grid positions, rotation). */
	void ApplyInventoryFromText(const FString& Text, FVector2D SizeExpansion);

	/** Re-equip the weapon that was in-hand at save time (by actor class path) after a load restore. */
	void ReEquipWeaponByPath(const FString& WeaponClassPath);

	/** Directional dash on space bar (replaces Jump). Uses CurrentMoveInput;
	 *  defaults to backward when stationary. Plays FPP_sns_Dodge on
	 *  MeleeViewMesh (separate from PlayerMesh — firearm grip untouched). */
	void PerformDodge();

	/** Live WASD input (X=right, Y=forward). Set in Input_Move, cleared in
	 *  Input_MoveCompleted. Read by PerformDodge for dash direction. */
	FVector2D CurrentMoveInput = FVector2D::ZeroVector;

	/** Seconds remaining on dodge cooldown. Decremented in Tick. */
	float DodgeCooldownRemaining = 0.f;

	/** Seconds remaining where sprint + weapon swap are locked out by an active
	 *  dodge. Set to AZP_DodgeLockWindow in PerformDodge, decremented in Tick. */
	float DodgeLockRemaining = 0.f;

	/** Seconds remaining where the dodge holds extra forward camera clearance
	 *  (covers the launch lean). Set in PerformDodge, decremented in Tick. */
	float DodgeClearanceRemaining = 0.f;

	/** Duration of the dodge forward-clearance window (seconds). */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float AZP_DodgeClearanceWindow = 0.5f;

	// --- Fall Damage tracking ---
	/** Highest Z reached since the current fall began (the apex). Drop distance =
	 *  FallPeakZ - landing Z, so a jump-then-fall measures from the top, not from
	 *  takeoff. */
	float FallPeakZ = 0.f;
	/** True while airborne in MOVE_Falling — gates the apex tracking in Tick. */
	bool bTrackingFall = false;

	/** Seconds remaining where the block forward-clearance nudge is active. Set on
	 *  block start, decremented in Tick. A transient window — NOT the whole hold —
	 *  so the camera settles to neutral during a sustained block instead of floating
	 *  forward. Drives GameplayComp->SetForwardClearanceActive with DodgeClearanceRemaining. */
	float BlockClearanceRemaining = 0.f;

	/** Duration of the block forward-clearance nudge (seconds) — long enough to
	 *  cover the stance lean-in, then it eases out even while block is held. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Block")
	float AZP_BlockClearanceWindow = 0.4f;


	/** True when the block walk anim is currently loaded (vs BlockLoop). */
	bool bBlockWalkActive = false;

	/** Seconds remaining where a BlockImpact reaction owns the view mesh playback. */
	float BlockImpactLockRemaining = 0.f;

	/** Seconds remaining where the BlockStart transition owns the view mesh playback.
	 *  UpdateBlockAnimation bails while this is > 0 so it can't yank the Start
	 *  clip out and snap straight to BlockLoop. */
	float BlockStartLockRemaining = 0.f;

	/** Last time a blocked hit staggered the attacker — gates re-stagger by AZP_BlockStaggerCooldown. */
	double LastBlockStaggerTime = -1000.0;

	/** Last time a melee hit staggered an enemy — gates re-stagger by AZP_HitStaggerCooldown. */
	double LastHitStaggerTime = -1000.0;

	// --- Committed-swing camera state (see bAZP_MeleeCommitEnabled) ---
	bool bMeleeCommitActive = false;
	float MeleeCommitElapsed = 0.f;
	int32 MeleeCommitDir = 0;          // 0=Forward,1=Right,2=Left
	float MeleeKickElapsed = 1000.f;   // >= AZP_MeleeKickDuration => kick inactive
	int32 MeleeKickDir = 0;
	/** Camera offset (deg) applied to control rotation LAST frame — the whip/kick is applied as a delta
	 *  off this so it layers on player look and always unwinds to net-zero. */
	float MeleeLastYaw = 0.f;
	float MeleeLastPitch = 0.f;
	/** Drives the committed-swing camera whip + impact kick each frame (called from Tick). */
	void UpdateMeleeCommit(float DeltaTime);

	/** Switch MeleeViewMesh between BlockLoop / BlockWalk based on motion. */
	void UpdateBlockAnimation();

	/** Distance-accumulating footstep player (called every Tick). */
	void UpdateFootsteps(float DeltaTime);

	/** Ground distance travelled since the last footstep. */
	float FootstepDistanceAccum = 0.f;

	/** Raise the guard NOW (anims, offsets, camera nudge). Callers must have validated context. */
	void StartBlockNow();

	/** Drop the guard (BlockStop pose-out → melee idle). Shared by RMB release, guard break
	 *  (blocked hit with insufficient stamina), and hold-drain exhaustion. */
	void ReleaseBlock();

	/** If RMB is held (bBlockWanted) and the guard isn't up, raise it the moment context allows:
	 *  melee equipped, no menu/ladder, stamina above the raise floor, and the swing animation fully
	 *  finished (the swing is NEVER clipped). Called from input AND Tick — this is the buffering
	 *  that makes swing→block seamless without re-pressing. */
	void TryEngageBufferedBlock();

	/** Attack > Block > Idle: a swing pressed while the guard is up drops the guard INSTANTLY with
	 *  no BlockStop pose-out (the swing clip owns the view mesh the same frame). bBlockWanted stays
	 *  set, so the guard re-raises by itself the moment the swing animation ends. */
	void InterruptBlockForSwing();

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

	/** Reads the configured Item DataAsset off a Moonville BP_ItemPickup (the "Item"
	 *  property, falling back to "AZP_ItemDataAsset"). Null if the actor isn't a pickup. */
	UObject* GetPickupItemDA(AActor* PickupActor);

	/** True if the player must NOT be able to pick up this pickup: a firearm/melee
	 *  weapon already owned (2d — throwables excepted, they stack), or ammo whose
	 *  reserve is already at the cap (2b). Used to gate both interaction paths. */
	bool ShouldBlockPickupInteraction(AActor* PickupActor);

	/** Turn a blocked pickup's InteractionArea/InteractionCollision spheres OFF so
	 *  Moonville neither shows its "Press E" popup nor registers it — and back ON
	 *  when it's no longer blocked. Throttled from Tick over CachedPickups. */
	void RefreshPickupBlockStates();

	/** (Re)collect the level's BP_ItemPickup actors into CachedPickups. */
	void GatherLevelPickups();

	/** Cached level pickups for the block refresh (weak — destroyed when picked up). */
	TArray<TWeakObjectPtr<AActor>> CachedPickups;

	/** Tick throttle for RefreshPickupBlockStates. */
	float PickupBlockRefreshAccum = 0.f;

	/** Owner of Moonville's current ClosestInteractable IF it's a world pickup, else
	 *  null. Drives the grab-all sweep + pickup detection. */
	AActor* GetClosestMoonvillePickupOwner();

	/** Frames left in a "grab the whole pile" sweep after E is pressed on a pickup —
	 *  each tick grabs the next-closest pickup so bunched items come up in one press
	 *  instead of repositioning per item. */
	int32 GrabAllTicksRemaining = 0;

	/** Actors already grabbed during the current grab-all sweep. Prevents the sweep from
	 *  re-grabbing the same pickup while it lingers as Moonville's ClosestInteractable before
	 *  being destroyed — a re-grab spawned duplicate inventory slots for non-stacking items
	 *  (e.g. one pistol pickup yielded 5). Reset when a new sweep starts. */
	TSet<TWeakObjectPtr<AActor>> GrabAllGrabbedActors;

	/** Reads weapon class from Moonville's ShortcutSlots[SlotIndex] via reflection. */
	TSubclassOf<AActor> GetWeaponFromShortcutSlot(int32 SlotIndex);

	/** Reads the Item DataAsset (PDA_Item) from Moonville's ShortcutSlots[SlotIndex] via reflection. */
	UObject* GetShortcutSlotItemDA(int32 SlotIndex);

	/** True if this item's consume action is a pure heal (AZP_FullHealthRefuseActions) AND the
	 *  player's health is already full — using it would waste the item. */
	bool ShouldRefuseConsumableAtFullHealth(UObject* ItemDA) const;

	/** True if the weapon class is actually in the GRID (ItemSlots) — the source of truth. A quick-slot can
	 *  keep a stale ref after the item moves to a briefcase, so we verify the grid before equipping. */
	bool IsWeaponClassInGrid(TSubclassOf<AActor> InWeaponClass);

	/** Syncs ActiveWeapon from KinemationComp when weapon changes (equip/unequip). */
	UFUNCTION()
	void OnWeaponChangedHandler(AActor* NewWeapon);

	/** Removes 1 throwable from Moonville inventory when a grenade is consumed. */
	UFUNCTION()
	void OnThrowableConsumedHandler();

	/** Bridge: Moonville OnInventoryUpdate → scan items for notes → NoteComponent. */
	UFUNCTION()
	void HandleInventoryUpdate();

	/** Total rounds of the given ammo type carried in the Moonville inventory (sum of
	 *  DA_Ammo_* stacks whose name maps to Icon). This IS the reserve. */
	int32 GetInventoryAmmoCount(EZP_WeaponIcon Icon);

	/** Trim any ammo type over its cap back down to the cap (a whole-stack pickup can
	 *  push you past max; the overflow is discarded). Runs on every inventory change. */
	void EnforceAmmoCaps();

	/** Push the equipped weapon's inventory ammo count into KinemationComp->ReserveAmmo
	 *  and broadcast OnAmmoChanged so the HUD reserve mirrors what's carried. */
	void SyncReserveFromInventory();

	/** Remove Rounds of the given ammo type from the Moonville inventory (called when a
	 *  reload consumes reserve). Spreads across stacks of that ammo. */
	void RemoveInventoryAmmo(EZP_WeaponIcon Icon, int32 Rounds);

	/** Bound to KinemationComp->OnReserveConsumed: a reload pulled Rounds into the mag →
	 *  spend that many inventory ammo items, then re-sync the reserve mirror. */
	UFUNCTION()
	void OnReserveConsumedHandler(int32 Rounds, EZP_WeaponIcon Icon);

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

	// --- Grab / Struggle runtime (Docs/Plan_GrabStruggle.md) ---

	/** The enemy currently latched on. */
	TWeakObjectPtr<AActor> GrabberActor;

	/** Seconds left in the current fixed-length phase (entry / escape / knockdown / get-up). */
	float GrabPhaseTimeRemaining = 0.f;

	/** Escape meter 0→1. Presses add, decay drains, 1.0 breaks free. */
	float EscapeProgress = 0.f;

	/** Seconds left to reach the escape threshold before the knockdown fail. */
	float StruggleTimeRemaining = 0.f;

	/** Seconds held so far (from the bite phase starting) — gates the min-trapped-time escape. */
	float GrabHeldTime = 0.f;

	/** Countdown to the next once-per-second damage tick while held. */
	float GrabNextTickIn = 1.f;

	/** [LatchProbe] 2s post-latch window ticker — logs what each body/camera is doing every 0.1s. */
	FTimerHandle GrabLatchProbeTimer;
	FVector GrabLatchProbeOrigin = FVector::ZeroVector;
	double GrabLatchProbeStart = 0.0;

	/** Eased view swing onto the grabber (replaces the latch-frame control-rotation snap). */
	FRotator GrabFaceStartRot = FRotator::ZeroRotator;
	FRotator GrabFaceTargetRot = FRotator::ZeroRotator;
	float GrabFaceAlpha = 1.f; // 1 = settled/no blend running

	/** True while attack is physically held (drives Hold accessibility fill). */
	bool bGrabEscapeHeld = false;

	/** A mash press landed during the AZP_GrabMinMunchTime window — the Wrestle switch is buffered
	 *  and fires from UpdateGrab the moment the minimum bite beat has played. */
	bool bWrestleQueued = false;

	/** Device the currently-shown grab prompt glyph was picked for — re-shown on change. */
	bool bGrabPromptGamepad = false;

	/** Flashlight intensities captured at grab start for restore (-1 = nothing to restore). */
	float PreGrabFlashlightIntensity = -1.f;
	float PreGrabFlashlightFillIntensity = -1.f;

	/** Per-tick input-device poll (sets bLastInputGamepad). */
	void UpdateInputDeviceTracking();

	/** Seconds during which NO enemy may grab (post-escape / post-knockdown). */
	float GrabImmunityRemaining = 0.f;

	/** 0 = live FP camera, 1 = full over-the-shoulder frame. Lerped in UpdateGrab; CalcCamera
	 *  blends the two views by this, so entry and return are smooth on every exit path. */
	float GrabCamWeight = 0.f;

	/** Weapon stowed at grab start — re-equipped at the end (the ladder recipe). */
	TSubclassOf<UObject> PreGrabWeaponClass;

	/** Defers restoring pawn-vs-pawn collision after a grab until the capsules are clear —
	 *  restoring while interpenetrated depenetrates them (random shove/slide at release). */
	FTimerHandle GrabCollisionRestoreTimer;

	// Retargeted grab clips, lazily loaded on first grab (LoadAnimDefaults pattern).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Anims", meta = (AllowPrivateAccess = "true")) TObjectPtr<UAnimSequenceBase> AZP_GrabAnimEntry;      // CCMH, MarcusBody
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Anims", meta = (AllowPrivateAccess = "true")) TObjectPtr<UAnimSequenceBase> AZP_GrabAnimMunch;      // CCMH, MarcusBody
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Anims", meta = (AllowPrivateAccess = "true")) TObjectPtr<UAnimSequenceBase> AZP_GrabAnimWrestle;    // CCMH, MarcusBody
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Anims", meta = (AllowPrivateAccess = "true")) TObjectPtr<UAnimSequenceBase> AZP_GrabAnimKick;       // CCMH, MarcusBody
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Anims", meta = (AllowPrivateAccess = "true")) TObjectPtr<UAnimSequenceBase> AZP_GrabAnimPush;       // CCMH, MarcusBody
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Anims", meta = (AllowPrivateAccess = "true")) TObjectPtr<UAnimSequenceBase> AZP_GrabAnimKnockdownFP;// UEFN, hidden Mesh (camera rides)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab|Anims", meta = (AllowPrivateAccess = "true")) TObjectPtr<UAnimSequenceBase> AZP_GrabAnimGetUpBack;  // UEFN, hidden Mesh
	bool bGrabAnimsLoaded = false;

	void LoadGrabAnims();
	void SetGrabPhase(EZP_GrabPhase NewPhase);
	/** Per-tick grab machine: camera weight, meter decay/hold-fill, bite ticks, phase timers. */
	void UpdateGrab(float DeltaTime);
	/** LMB pressed while grabbed — the mash. */
	void GrabMashPressed();
	/** Tear down the grab on ANY exit path (escape completed, get-up done, abort, death). */
	void EndGrab(bool bAborted);
	/** SingleNode a grab clip on the visible 3P body (MarcusBody). */
	void PlayGrabClipOnBody(UAnimSequenceBase* Clip, bool bLoop);
	/** SingleNode a clip on the hidden Mesh with full-body copy → PlayerMesh → camera rides
	 *  the FPCamera socket (the ladder mechanism). Knockdown/get-up path. */
	void PlayGrabClipOnFPRig(UAnimSequenceBase* Clip);
	/** Tell the grabber (IZP_Grabber, actor or component) the phase machine advanced. */
	void NotifyGrabberPhase(EZP_GrabPhase Phase);

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

	/** Begin the animated climb-over-the-top exit. Plays AZP_LadderTopExitAnimation;
	 *  the body's baked root motion carries it onto the floor while the camera
	 *  lerps up; capsule snaps to the resting spot when the anim completes. */
	void BeginLadderTopExit(class AZP_Ladder* Ladder);

	/** Advance the animated top-exit each tick; finalizes when the anim ends. */
	void UpdateLadderTopExit(float DeltaTime);

	friend class AZP_Ladder;
	friend class UZP_MarcusBodyAnimInstance; // chest-bend instance reads knobs + MarcusHead grab marker

};
