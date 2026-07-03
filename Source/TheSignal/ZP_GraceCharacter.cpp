// Copyright The Signal. All Rights Reserved.

#include "ZP_GraceCharacter.h"
#include "ZP_GraceGameplayComponent.h"
#include "ZP_KinemationComponent.h"
#include "ZP_FootstepData.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ZP_WeaponTypes.h"
#include "ZP_HealthComponent.h"
#include "ZP_ShamblerBehaviorComponent.h"
#include "ZP_Staggerable.h"
#include "ZP_GraceMovementConfig.h"
#include "ZP_GracePlayerAnimInstance.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "ZP_Interactable.h"
#include "ZP_InteractDoor.h"
#include "ZP_ObjectiveDepositLibrary.h"
#include "ZP_MapComponent.h"
#include "ZP_NoteComponent.h"
#include "ZP_SignalSenseComponent.h"
#include "ZP_InventoryTabTypes.h"
#include "ZP_InventoryTabWidget.h"
#include "GameplayTagContainer.h"
#include "UObject/UnrealType.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ZP_FloorCullingComponent.h"
#include "ZP_RuntimeISMBatcher.h"
#include "ZP_MapWidget.h"
#include "ZP_NPCInteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/PostProcessComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ZP_BriefcaseSubsystem.h"
#include "ZP_ObjectiveSubsystem.h"
#include "ZP_SaveGame.h"
#include "UObject/SoftObjectPath.h"
#include "Dom/JsonObject.h"
#include "ZP_Ladder.h"
#include "Components/ArrowComponent.h"

AZP_GraceCharacter::AZP_GraceCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule — radius sized so weapon tip is at collision boundary
	GetCapsuleComponent()->InitCapsuleSize(55.0f, 88.0f);

	// --- Hidden inherited Mesh (GASP motion-matching source) ---
	// This mesh runs ABP_GraceLocomotion (duplicated from GASP's ABP_SandboxCharacter)
	// to generate full-body motion-matching poses. It is invisible — PlayerMesh
	// retargets its lower body and Kinemation drives its upper body.
	// Skeletal mesh + AnimClass are set via MCP/Blueprint (can't reference BP assets from C++).
	GetMesh()->SetVisibility(false);
	GetMesh()->SetComponentTickEnabled(true);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));

	// Visible first-person arms — attached to capsule.
	// Bone transforms copied from hidden Mesh in C++ (ZP_GracePlayerAnimInstance).
	// Must be created BEFORE camera so camera can attach to its FPCamera socket.
	PlayerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerMesh"));
	PlayerMesh->SetupAttachment(GetCapsuleComponent());
	PlayerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));

	// First-person camera — attached to PlayerMesh at FPCamera socket.
	// This makes the camera MOVE with the skeleton (spine rotation, aim offset).
	// AC_FirstPersonCamera drives ROTATION each tick (K2_SetWorldRotation).
	// Without socket attachment, looking up/down causes parallax (gun leaves view),
	// and the camera detaches from the SKM_Operator_Mono mesh's headless eye spot
	// — looking down lets you see your own neck/feet. Capsule-anchored = dead end.
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(PlayerMesh, FName(TEXT("FPCamera")));
	// 20cm forward of the FPCamera socket. Kinemation's melee/dodge/block
	// stances pitch the spine forward and tuck the head bone into the chest;
	// with the camera AT the bone origin, the view ended up INSIDE the body
	// (rendering the inside-facing chest armor — confirmed by dev screenshot).
	// 20cm forward keeps the camera ahead of the body geometry through normal
	// spine lean. BaseCameraX is captured in GameplayComp so peek/bob preserves
	// the offset each tick.
	FirstPersonCamera->SetRelativeLocation(FVector(20.0f, 0.0f, 0.0f));
	FirstPersonCamera->SetRelativeRotation(FRotator::ZeroRotator);
	FirstPersonCamera->bUsePawnControlRotation = false;

	// --- Melee view model (TICKET-054) ---
	// Kubold FPP melee anims play here, never on PlayerMesh (camera is socketed
	// to PlayerMesh — animating it swings the camera). Mesh/anims loaded by
	// ZP_KinemationComponent at init. Offset: mannequin eye height below camera,
	// yawed -90 so the mannequin faces camera forward (+X).
	MeleeViewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeleeViewMesh"));
	MeleeViewMesh->SetupAttachment(FirstPersonCamera);
	MeleeViewMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -155.0f));
	MeleeViewMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	MeleeViewMesh->SetVisibility(false);
	MeleeViewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeleeViewMesh->SetOnlyOwnerSee(true);
	MeleeViewMesh->bCastDynamicShadow = false;
	MeleeViewMesh->CastShadow = false;
	MeleeViewMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MeleeViewMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// Overalls sleeves for the view-model arms — leader-posed to MeleeViewMesh in setup.
	MeleeViewOveralls = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeleeViewOveralls"));
	MeleeViewOveralls->SetupAttachment(MeleeViewMesh);
	MeleeViewOveralls->SetVisibility(false);
	MeleeViewOveralls->SetOnlyOwnerSee(true);
	MeleeViewOveralls->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeleeViewOveralls->bCastDynamicShadow = false;
	MeleeViewOveralls->CastShadow = false;
	MeleeViewOveralls->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Bare Marcus-skin hands for the melee view-model — leader-posed to MeleeViewMesh in
	// setup (same bare-hand mesh the ranged arms use, so hands match on every weapon).
	MeleeHands = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeleeHands"));
	MeleeHands->SetupAttachment(MeleeViewMesh);
	MeleeHands->SetVisibility(false);
	MeleeHands->SetOnlyOwnerSee(true);
	MeleeHands->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeleeHands->bCastDynamicShadow = false;
	MeleeHands->CastShadow = false;
	MeleeHands->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	PlayerMesh->SetOnlyOwnerSee(true);
	PlayerMesh->bCastDynamicShadow = true;
	PlayerMesh->CastShadow = true;
	PlayerMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Ranged arms-only view-model: leader-posed to PlayerMesh (same Operator skeleton)
	// so they ride the live Kinemation aim. Shown only while a ranged weapon is up.
	RangedArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RangedArms"));
	RangedArms->SetupAttachment(PlayerMesh);
	RangedArms->SetVisibility(false);
	RangedArms->SetOnlyOwnerSee(true);
	RangedArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangedArms->bCastDynamicShadow = false;
	RangedArms->CastShadow = false;
	RangedArms->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	RangedHands = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RangedHands"));
	RangedHands->SetupAttachment(PlayerMesh);
	RangedHands->SetVisibility(false);
	RangedHands->SetOnlyOwnerSee(true);
	RangedHands->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangedHands->bCastDynamicShadow = false;
	RangedHands->CastShadow = false;
	RangedHands->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	RangedSleeve = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RangedSleeve"));
	RangedSleeve->SetupAttachment(PlayerMesh);
	RangedSleeve->SetVisibility(false);
	RangedSleeve->SetOnlyOwnerSee(true);
	RangedSleeve->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangedSleeve->bCastDynamicShadow = false;
	RangedSleeve->CastShadow = false;
	RangedSleeve->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// --- Marcus appearance (CCMH body shell) ---
	// Meshes/AnimBP/leader-pose/source wired in SetupMarcusAppearance() at BeginPlay.
	MarcusBody = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MarcusBody"));
	MarcusBody->SetupAttachment(GetCapsuleComponent());
	MarcusBody->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	MarcusBody->SetOnlyOwnerSee(true);
	MarcusBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarcusBody->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MarcusBody->SetVisibility(false); // shown in SetupMarcusAppearance once meshes load

	MarcusOveralls = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MarcusOveralls"));
	MarcusOveralls->SetupAttachment(MarcusBody);
	MarcusOveralls->SetOnlyOwnerSee(true);
	MarcusOveralls->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarcusOveralls->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	MarcusPants = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MarcusPants"));
	MarcusPants->SetupAttachment(MarcusBody);
	MarcusPants->SetOnlyOwnerSee(true);
	MarcusPants->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarcusPants->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Head (CCMH_Head_Male — the CCMH body asset is HEADLESS; the head is a separate mesh).
	// Hidden in normal FP play (it sits at the camera); shown only during the grab 3P beat.
	MarcusHead = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MarcusHead"));
	MarcusHead->SetupAttachment(MarcusBody);
	MarcusHead->SetOnlyOwnerSee(true);
	MarcusHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarcusHead->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MarcusHead->SetVisibility(false);

	// Hair + brows — Marcus's authored look from the "Marcus" CC_SaveGame entry
	// (SidePart_02 + Eyebrows_01, dark-brown tint). Shown/hidden together with MarcusHead.
	MarcusHair = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MarcusHair"));
	MarcusHair->SetupAttachment(MarcusBody);
	MarcusHair->SetOnlyOwnerSee(true);
	MarcusHair->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarcusHair->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MarcusHair->SetVisibility(false);

	MarcusBrows = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MarcusBrows"));
	MarcusBrows->SetupAttachment(MarcusBody);
	MarcusBrows->SetOnlyOwnerSee(true);
	MarcusBrows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarcusBrows->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MarcusBrows->SetVisibility(false);

	MarcusSneakers = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MarcusSneakers"));
	MarcusSneakers->SetupAttachment(MarcusBody);
	MarcusSneakers->SetOnlyOwnerSee(true);
	MarcusSneakers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarcusSneakers->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Gameplay component — stamina, peek, head bob, interaction trace
	GameplayComp = CreateDefaultSubobject<UZP_GraceGameplayComponent>(TEXT("GameplayComp"));

	// Kinemation component — camera/anim wiring, weapon lifecycle
	KinemationComp = CreateDefaultSubobject<UZP_KinemationComponent>(TEXT("KinemationComp"));

	// Health component — player HP tracking, damage → HUD wiring
	HealthComp = CreateDefaultSubobject<UZP_HealthComponent>(TEXT("HealthComp"));

	// Map component — tracks discovered maps and current area
	MapComp = CreateDefaultSubobject<UZP_MapComponent>(TEXT("MapComp"));

	// Note component — tracks collected notes/documents
	NoteComp = CreateDefaultSubobject<UZP_NoteComponent>(TEXT("NoteComp"));

	// SignalSense — "the phone": proximity warning, layered audio + waveform + rumble
	SignalSenseComp = CreateDefaultSubobject<UZP_SignalSenseComponent>(TEXT("SignalSenseComp"));

	// Floor culling — hides actors on non-visible floors for performance
	FloorCullingComp = CreateDefaultSubobject<UZP_FloorCullingComponent>(TEXT("FloorCullingComp"));

	// Runtime ISM batching — converts repeated static meshes to instanced at play time
	ISMBatcherComp = CreateDefaultSubobject<UZP_RuntimeISMBatcher>(TEXT("ISMBatcherComp"));

	// Post-process vignette for low-health visual feedback
	DeathVignetteComp = CreateDefaultSubobject<UPostProcessComponent>(TEXT("DeathVignetteComp"));
	DeathVignetteComp->SetupAttachment(RootComponent);
	DeathVignetteComp->bUnbound = false; // Camera-only, not world volume
	DeathVignetteComp->Priority = 1.0f;
	DeathVignetteComp->Settings.bOverride_VignetteIntensity = true;
	DeathVignetteComp->Settings.VignetteIntensity = 0.f;

	// --- SH2-style darkness: constrain auto-exposure so engine can't over-brighten ---
	// Histogram mode (default) with negative bias + clamped range.
	// NOT manual mode — manual with default physical camera settings crushes everything to black.
	DeathVignetteComp->Settings.bOverride_AutoExposureBias = true;
	DeathVignetteComp->Settings.AutoExposureBias = -0.5f; // darker bias without crushing
	// Exposure window (session 64): 0.8 floor made the 2 AM exterior render
	// near-black through every window/opening (night luminance ~0.1-0.3 sits
	// far below an interior-tuned clamp). 0.2 lets eyes adapt enough to read
	// moonlit night; sealed dark rooms (~0.01) still clamp 4x below the floor
	// so they stay dark (the session-63 "warm lamps" bug needed 0.01 to return).
	DeathVignetteComp->Settings.bOverride_AutoExposureMinBrightness = true;
	DeathVignetteComp->Settings.AutoExposureMinBrightness = 0.2f;
	DeathVignetteComp->Settings.bOverride_AutoExposureMaxBrightness = true;
	DeathVignetteComp->Settings.AutoExposureMaxBrightness = 1.2f;
	DeathVignetteComp->Settings.bOverride_BloomIntensity = true;
	DeathVignetteComp->Settings.BloomIntensity = 0.2f; // kill bloom glow, keep darkness crisp

	// Enable Lumen GI for flashlight bounce — the flashlight is the player's one source
	// of truth for seeing the environment. Its light MUST bounce off surfaces naturally.
	// Overrides PPV_GlobalDarkness (GI=NONE) via player PPC priority.
	// Swimming pool was from bright environment fixtures — in horror darkness with only
	// the flashlight active, Lumen GI produces clean single-source bounce.
	DeathVignetteComp->Settings.bOverride_DynamicGlobalIlluminationMethod = true;
	DeathVignetteComp->Settings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;
	DeathVignetteComp->Settings.bOverride_IndirectLightingIntensity = true;
	DeathVignetteComp->Settings.IndirectLightingIntensity = 1.0f; // natural bounce

	// Chest flashlight — attached to camera so it pitches with the view
	// and never gets blocked by weapon models when looking down
	FlashlightComp = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightComp"));
	FlashlightComp->SetupAttachment(FirstPersonCamera);
	FlashlightComp->SetRelativeLocation(FVector(5.0f, -25.0f, 15.0f)); // above left shoulder
	// iPhone-torch profile (dev direction): a phone LED is a strong, fairly tight hotspot with a
	// wide soft spill. Concentrated inner cone + high intensity = real THROW down a corridor
	// (clear read ~20-30 m, visible reach to ~50 m under the pinned horror exposure); the wide
	// outer cone keeps close quarters covered like a phone's flood.
	FlashlightComp->SetIntensity(25000.0f);        // throw — reads surfaces far down the hallway
	FlashlightComp->SetInnerConeAngle(16.0f);      // concentrated hotspot = distance punch
	FlashlightComp->SetOuterConeAngle(40.0f);      // wide phone-LED spill for close range
	FlashlightComp->SetAttenuationRadius(5000.0f); // 50 m potential reach; inverse-square shapes it
	FlashlightComp->SetLightColor(FLinearColor(1.0f, 0.93f, 0.82f)); // slightly warmer/dingier
	FlashlightComp->SetSourceRadius(0.5f);         // sharp shadow edges
	FlashlightComp->CastShadows = true;
	FlashlightComp->SetVisibility(false); // starts OFF

	// Ambient fill light — simulates flashlight bounce off nearby surfaces.
	// SpotLight alone creates a hard circle with pitch black outside.
	// PointLight fills the surrounding area so the player can see their environment.
	FlashlightFillComp = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashlightFillComp"));
	FlashlightFillComp->SetupAttachment(FirstPersonCamera);
	FlashlightFillComp->SetRelativeLocation(FVector(20.0f, 0.0f, -15.0f)); // slightly forward and below
	FlashlightFillComp->SetIntensity(670.0f);         // dim ambient — enough to see walls, not a lantern
	FlashlightFillComp->SetAttenuationRadius(600.0f); // covers nearby room area
	FlashlightFillComp->SetLightColor(FLinearColor(1.0f, 0.93f, 0.82f)); // match flashlight warmth
	FlashlightFillComp->SetSourceRadius(20.0f);       // very soft shadows (diffuse bounce feel)
	FlashlightFillComp->CastShadows = false;           // bounce light doesn't cast sharp shadows
	FlashlightFillComp->SetVisibility(false);          // starts OFF

	// Default click sound from Character Customizer flashlight tool
	static ConstructorHelpers::FObjectFinder<USoundBase> FlashlightClickFinder(
		TEXT("/Game/CharacterCustomizer/Components/Tools/Tool_Flashlight/Click"));
	if (FlashlightClickFinder.Succeeded())
	{
		FlashlightClickSound = FlashlightClickFinder.Object;
	}

	// Footstep variations — Moonville hard-surface set (6 clips, randomized per step). The player's
	// SingleNode locomotion has no anim-notify tracks, so steps are DISTANCE-based (UpdateFootsteps).
	for (int32 StepIdx = 1; StepIdx <= 6; ++StepIdx)
	{
		ConstructorHelpers::FObjectFinder<USoundBase> StepFinder(*FString::Printf(
			TEXT("/Game/InventorySystemPro/ExampleContent/Common/Sounds/Footsteps/SW_Footstep_%d.SW_Footstep_%d"),
			StepIdx, StepIdx));
		if (StepFinder.Succeeded())
		{
			FootstepSounds.Add(StepFinder.Object);
		}
	}
	// Surface-specific sounds live in DA_Footsteps (UZP_FootstepData), lazily loaded in
	// UpdateFootsteps — the dev-authored table, not code.

	// --- Baked CDO defaults (replaces set_all_cdo.py) ---

	// Movement config DataAsset
	static ConstructorHelpers::FObjectFinder<UZP_GraceMovementConfig> MovementConfigFinder(
		TEXT("/Game/Core/Data/DA_GraceMovement_Default"));
	if (MovementConfigFinder.Succeeded()) MovementConfig = MovementConfigFinder.Object;

	// Core input actions
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/Core/Input/Actions/IA_Move"));
	if (MoveActionFinder.Succeeded()) MoveAction = MoveActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(TEXT("/Game/Core/Input/Actions/IA_Look"));
	if (LookActionFinder.Succeeded()) LookAction = LookActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> SprintActionFinder(TEXT("/Game/Core/Input/Actions/IA_Sprint"));
	if (SprintActionFinder.Succeeded()) SprintAction = SprintActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionFinder(TEXT("/Game/Core/Input/Actions/IA_Jump"));
	if (JumpActionFinder.Succeeded()) JumpAction = JumpActionFinder.Object;

	// Dodge/block anims — Kubold FPP set on the Operator skeleton. (REVERTED
	// 2026-06-20 from the Marcus_/CCMH retargets — they collapsed on the CCMH
	// view-model; back to the proven Operator-skeleton clips for SKM_Operator_Mono.)
	DodgeAnim = TSoftObjectPtr<UAnimSequenceBase>(
		FSoftObjectPath(TEXT("/Game/FPPMeleeAnimset/Animations/SwordnShield/FPP_sns_Dodge.FPP_sns_Dodge")));

	BlockLoopAnim = TSoftObjectPtr<UAnimSequenceBase>(
		FSoftObjectPath(TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockLoop.A_MeleePipe_BlockLoop")));
	BlockWalkAnim = TSoftObjectPtr<UAnimSequenceBase>(
		FSoftObjectPath(TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockWalk.A_MeleePipe_BlockWalk")));
	BlockStartAnim = TSoftObjectPtr<UAnimSequenceBase>(
		FSoftObjectPath(TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockStart.A_MeleePipe_BlockStart")));
	BlockStopAnim = TSoftObjectPtr<UAnimSequenceBase>(
		FSoftObjectPath(TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockStop.A_MeleePipe_BlockStop")));
	BlockImpact1Anim = TSoftObjectPtr<UAnimSequenceBase>(
		FSoftObjectPath(TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockImpact1.A_MeleePipe_BlockImpact1")));
	BlockImpact2Anim = TSoftObjectPtr<UAnimSequenceBase>(
		FSoftObjectPath(TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockImpact2.A_MeleePipe_BlockImpact2")));
	BlockImpact3Anim = TSoftObjectPtr<UAnimSequenceBase>(
		FSoftObjectPath(TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockImpact3.A_MeleePipe_BlockImpact3")));
	// MUST match Kinemation's MeleeIdleAnim (A_MeleePipe_Idle). Pointing this
	// at FPP_Longs_Idle (Kubold longsword) made block-release / non-forward
	// dodge return to a chest-forward longsword pose that persisted forever —
	// dev report 2026-06-19 "camera shifts behind body and never unshifts".
	MeleeIdleHoldAnim = TSoftObjectPtr<UAnimSequenceBase>(
		FSoftObjectPath(TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle.A_MeleePipe_Idle")));

	static ConstructorHelpers::FObjectFinder<UInputAction> InteractActionFinder(TEXT("/Game/Core/Input/Actions/IA_Interact"));
	if (InteractActionFinder.Succeeded()) InteractAction = InteractActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchActionFinder(TEXT("/Game/Core/Input/Actions/IA_Crouch"));
	if (CrouchActionFinder.Succeeded()) CrouchAction = CrouchActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> PeekActionFinder(TEXT("/Game/Core/Input/Actions/IA_Peek"));
	if (PeekActionFinder.Succeeded()) PeekAction = PeekActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> AimActionFinder(TEXT("/Game/Core/Input/Actions/IA_Aim"));
	if (AimActionFinder.Succeeded()) AimAction = AimActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> FireActionFinder(TEXT("/Game/Core/Input/Actions/IA_Fire"));
	if (FireActionFinder.Succeeded()) FireAction = FireActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> ReloadActionFinder(TEXT("/Game/Core/Input/Actions/IA_Reload"));
	if (ReloadActionFinder.Succeeded()) ReloadAction = ReloadActionFinder.Object;

	// Map + tab cycling
	static ConstructorHelpers::FObjectFinder<UInputAction> MapActionFinder(TEXT("/Game/Core/Input/IA_Map"));
	if (MapActionFinder.Succeeded()) MapAction = MapActionFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> TabLeftFinder(TEXT("/Game/Core/Input/Actions/IA_TabCycleLeft"));
	if (TabLeftFinder.Succeeded()) TabCycleLeftAction = TabLeftFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> TabRightFinder(TEXT("/Game/Core/Input/Actions/IA_TabCycleRight"));
	if (TabRightFinder.Succeeded()) TabCycleRightAction = TabRightFinder.Object;

	// Inventory actions (Moonville's)
	static ConstructorHelpers::FObjectFinder<UInputAction> InvMenuFinder(TEXT("/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventoryMenuOpen"));
	if (InvMenuFinder.Succeeded()) InventoryMenuAction = InvMenuFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Slot0Finder(TEXT("/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventorySlot0"));
	if (Slot0Finder.Succeeded()) InventorySlot0Action = Slot0Finder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Slot1Finder(TEXT("/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventorySlot1"));
	if (Slot1Finder.Succeeded()) InventorySlot1Action = Slot1Finder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Slot2Finder(TEXT("/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventorySlot2"));
	if (Slot2Finder.Succeeded()) InventorySlot2Action = Slot2Finder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Slot3Finder(TEXT("/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventorySlot3"));
	if (Slot3Finder.Succeeded()) InventorySlot3Action = Slot3Finder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> FlashlightFinder(TEXT("/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventoryFlashlight"));
	if (FlashlightFinder.Succeeded()) FlashlightAction = FlashlightFinder.Object;

	// Starting weapon (soft reference — doesn't force-load the asset)
	StartingWeaponItem = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/Core/Items/DA_Pipe.DA_Pipe")));

	// Bullet decal materials (soft references)
	BulletDecalMaterials.Add(TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/UniversalWallClutter/Materials/BulletHoles/MI_BulletHole_Metal_01.MI_BulletHole_Metal_01"))));
	BulletDecalMaterials.Add(TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/UniversalWallClutter/Materials/BulletHoles/MI_BulletHole_Metal_02.MI_BulletHole_Metal_02"))));
	BulletDecalMaterials.Add(TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/UniversalWallClutter/Materials/BulletHoles/MI_BulletHole_Metal_03.MI_BulletHole_Metal_03"))));

	// Movement defaults (overridden by DataAsset in GameplayComp's BeginPlay)
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->MaxWalkSpeed = 115.0f;
	MoveComp->BrakingDecelerationWalking = 1400.0f;
	MoveComp->MaxAcceleration = 1200.0f;
	MoveComp->GroundFriction = 6.0f;
	MoveComp->JumpZVelocity = 300.0f;
	MoveComp->AirControl = 0.15f;
	MoveComp->NavAgentProps.bCanCrouch = true;
	MoveComp->SetCrouchedHalfHeight(44.0f);
	MoveComp->MaxWalkSpeedCrouched = 57.0f;
}

void AZP_GraceCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Guarantee MovementConfig is never null — create transient default if none assigned.
	// All values come from UPROPERTY defaults in ZP_GraceMovementConfig.h.
	if (!MovementConfig)
	{
		MovementConfig = NewObject<UZP_GraceMovementConfig>(this);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: No DataAsset assigned — created default MovementConfig from C++ defaults."));
	}

	// Apply config to capsule, mesh, and movement component
	GetCapsuleComponent()->SetCapsuleSize(MovementConfig->CapsuleRadius, MovementConfig->CapsuleHalfHeight);
	PlayerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, MovementConfig->PlayerMeshOffsetZ));

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->MaxWalkSpeed = MovementConfig->WalkSpeed;
	MoveComp->BrakingDecelerationWalking = MovementConfig->BrakingDeceleration;
	MoveComp->MaxAcceleration = MovementConfig->MaxAcceleration;
	MoveComp->GroundFriction = MovementConfig->GroundFriction;
	MoveComp->JumpZVelocity = MovementConfig->JumpZVelocity;
	MoveComp->AirControl = MovementConfig->AirControl;
	MoveComp->SetCrouchedHalfHeight(MovementConfig->CrouchedHalfHeight);
	MoveComp->MaxWalkSpeedCrouched = MovementConfig->CrouchWalkSpeed;

	// Apply locomotion skeletal mesh to the hidden inherited Mesh
	if (LocomotionSkeletalMesh)
	{
		GetMesh()->SetSkeletalMeshAsset(LocomotionSkeletalMesh);
	}

	// Propagate config from character to components
	// This runs BEFORE BeginPlay, so components see the config when they initialize
	if (GameplayComp)
	{
		GameplayComp->MovementConfig = MovementConfig;
		GameplayComp->CameraComponent = FirstPersonCamera;
	}

	if (KinemationComp)
	{
		KinemationComp->WeaponClass = WeaponClass;
		KinemationComp->CameraComponent = FirstPersonCamera;
		KinemationComp->PlayerMeshComponent = PlayerMesh;

		// Propagate ADS config from MovementConfig
		KinemationComp->DefaultFOV = MovementConfig->DefaultFOV;
		KinemationComp->AdsFOV = MovementConfig->AdsFOV;
		KinemationComp->AdsFOVInterpSpeed = MovementConfig->AdsFOVInterpSpeed;

		// Resolve soft references and propagate decal materials
		for (const TSoftObjectPtr<UMaterialInterface>& SoftMat : BulletDecalMaterials)
		{
			if (UMaterialInterface* Mat = SoftMat.LoadSynchronous())
			{
				KinemationComp->BulletDecalMaterials.Add(Mat);
			}
		}
	}
}

void AZP_GraceCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Clamp how far the camera can pitch up/down. Reduces look-down so the FP
	// camera never reaches the visible body's interior, and limits look-up to a
	// natural ceiling. Values live as UPROPERTYs so dev can dial in Details ->
	// Camera|Pitch; CalcCamera re-clamps each frame so live edits take effect.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->ViewPitchMin = -CameraPitchDownLimitDeg;
			PC->PlayerCameraManager->ViewPitchMax =  CameraPitchUpLimitDeg;
		}
	}

	// PlayerMesh must evaluate AFTER hidden Mesh, so NativePostEvaluateAnimation
	// reads fresh source bone data when copying lower body.
	PlayerMesh->AddTickPrerequisiteComponent(GetMesh());

	// Hide the head on PlayerMesh — the capsule-anchored camera sits in front
	// of where the head is (not absorbed by the head bone like the old socketed
	// camera was), so pitching the view down brings the player's own head/face
	// into frame. Same trick MeleeViewMesh uses for its own head.
	PlayerMesh->HideBoneByName(FName("head"), EPhysBodyOp::PBO_None);

	// --- Hidden Mesh: SingleNode mode with direct anim sequences ---
	// Speed-based switching in Tick. Start with idle.
	{
		USkeletalMeshComponent* LocoMesh = GetMesh();
		LocoMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		UAnimSequenceBase* StartAnim = IdleAnimation ? IdleAnimation : WalkAnimation;
		if (StartAnim)
		{
			if (UAnimSingleNodeInstance* SNI = Cast<UAnimSingleNodeInstance>(LocoMesh->GetAnimInstance()))
			{
				SNI->SetAnimationAsset(StartAnim, true, 1.0f);
				SNI->SetPlaying(true);
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] Hidden Mesh: SingleNode with %s"), *StartAnim->GetName());
			}
		}
	}

	// Marcus CCMH body as the visible shell (retargeted from PlayerMesh)
	SetupMarcusAppearance();

	// Auto-discover Moonville components (added via SCS in editor)
	for (UActorComponent* Comp : GetComponents())
	{
		if (!Comp) continue;
		const FString ClassName = Comp->GetClass()->GetName();

		if (!MoonvilleInteractionComp && ClassName.Contains(TEXT("BP_InteractionComponent")))
		{
			MoonvilleInteractionComp = Comp;
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: Auto-detected MoonvilleInteractionComp: %s"), *Comp->GetName());
		}
		else if (!MoonvilleInventoryComp && ClassName.Contains(TEXT("BP_InventoryCharacterComponent")))
		{
			MoonvilleInventoryComp = Comp;
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: Auto-detected MoonvilleInventoryComp: %s"), *Comp->GetName());
		}
	}

	if (!MoonvilleInteractionComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ZP_GraceCharacter: MoonvilleInteractionComp NOT FOUND — interaction routing disabled."));
	}
	if (!MoonvilleInventoryComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ZP_GraceCharacter: MoonvilleInventoryComp NOT FOUND — inventory menu disabled."));
	}

	// Bind to Moonville OnInventoryUpdate for notes bridge
	BindInventoryUpdateDelegate();

	// Fresh start always gets the starting loadout. Inventory persistence is tied to the SAVE FILE: an
	// EasyGameUI LOAD restores items via the BP_EasySaveGameComponent hook (OnEguiSaveLoadVariables),
	// which overwrites this fresh inventory. A plain PIE start / new game keeps just the starting loadout.
	GrantStartingItems();
	BindEguiSaveComponent();

	// Seed the pickup-block cache so owned-weapon / capped-ammo pickups start
	// non-interactable (Tick + inventory updates keep it current thereafter).
	GatherLevelPickups();

	// Bind weapon change delegate BEFORE InitializeKinemation so the starting
	// weapon broadcast registers in the slot system.
	if (KinemationComp)
	{
		KinemationComp->OnWeaponChanged.AddDynamic(this, &AZP_GraceCharacter::OnWeaponChangedHandler);
		KinemationComp->OnThrowableConsumed.AddDynamic(this, &AZP_GraceCharacter::OnThrowableConsumedHandler);
		KinemationComp->OnReserveConsumed.AddDynamic(this, &AZP_GraceCharacter::OnReserveConsumedHandler);

		// Initialize Kinemation AFTER Super::BeginPlay() — all SCS Blueprint components
		// (AC_FirstPersonCamera, AC_TacticalShooterAnimation, etc.) have had their
		// BeginPlay by now. Wiring before that gets overwritten by their init.
		KinemationComp->InitializeKinemation();

		// Sync ActiveWeapon for BP interface compat (GetPrimaryWeapon, GetMainWeapon)
		ActiveWeapon = KinemationComp->ActiveWeapon;

		// NOTE: do NOT auto-equip a weapon here. Kinemation's BP camera/anim
		// components corrupt the camera socket if the equip transition fires
		// during their init pass (proven at +1tick AND +1.5s — DEAD END
		// 2026-06-19). Player starts unarmed; equip via inventory key.
	}

	// Head bob stays enabled even with Kinemation — AC_FirstPersonCamera only drives
	// camera ROTATION (aim sway), not position. Our built-in bob handles position offsets.

	// Bind death + vignette delegates
	if (HealthComp)
	{
		HealthComp->OnDied.AddDynamic(this, &AZP_GraceCharacter::HandleDeath);
		HealthComp->OnHealthChanged.AddDynamic(this, &AZP_GraceCharacter::UpdateHealthVignette);
	}

	// Bind HUD + Inventory Tab widget to this character's components
	if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController()))
	{
		if (PC->HUDWidget)
		{
			PC->HUDWidget->BindToCharacter(this);
		}
		if (PC->InventoryTabWidget)
		{
			PC->InventoryTabWidget->BindToCharacter(this);
		}
	}

	// --- Orchestrate ISM Batcher + Floor Culling initialization ---
	// Order matters: batch first (creates per-floor ISMCs, hides originals),
	// then floor culling (skips batched actors, toggles ISM visibility per floor).
	if (ISMBatcherComp && FloorCullingComp)
	{
		// Define always-visible zones (stairwells, vertical shafts)
		// Actors in these zones are excluded from both batching and floor culling.
		// Building 1 stairwell: X 800-1050, Y -1500 to -1150, full Z height
		FBox StairwellZone(FVector(780.0f, -1520.0f, -50.0f), FVector(1070.0f, -1130.0f, 2550.0f));
		FloorCullingComp->AlwaysVisibleZones.Add(StairwellZone);

		// Sync floor params from floor culling to ISM batcher
		ISMBatcherComp->FloorHeight = FloorCullingComp->FloorHeight;
		ISMBatcherComp->FloorBaseZ = FloorCullingComp->FloorBaseZ;
		ISMBatcherComp->NumFloors = FloorCullingComp->NumFloors;
		ISMBatcherComp->AlwaysVisibleZones = FloorCullingComp->AlwaysVisibleZones;

		// Batch all static mesh actors into per-floor ISMCs
		ISMBatcherComp->BatchStaticMeshes();

		// Wire batcher into floor culling so it can skip batched actors + toggle ISMs
		FloorCullingComp->ISMBatcher = ISMBatcherComp;

		// Now collect unbatched actors and start floor check timer
		FloorCullingComp->Initialize();
	}
	else
	{
		// Fallback: run floor culling standalone if either component is missing
		if (FloorCullingComp)
		{
			FloorCullingComp->Initialize();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter BeginPlay — Config: %s, Weapon: %s, Kinemation: %s"),
		MovementConfig ? *MovementConfig->GetName() : TEXT("NONE"),
		KinemationComp && KinemationComp->ActiveWeapon ? *KinemationComp->ActiveWeapon->GetName() : TEXT("NONE"),
		KinemationComp && KinemationComp->IsKinemationActive() ? TEXT("ACTIVE") : TEXT("OFF"));
}

FVector AZP_GraceCharacter::GetActiveMeleeHandOffset(bool bRight) const
{
	// Block uses the SAME grip connection as non-block pipe wielding (the tuned
	// MeleeHand offset) as its base, so the hands grip the weapon identically.
	// The per-hand Block offset is an OPTIONAL fine-tune added on top — leave it
	// at zero and block inherits the exact non-block connection.
	const FVector Base = bRight ? MeleeHandROffset : MeleeHandLOffset;
	if (bIsBlocking)
	{
		return Base + (bRight ? BlockHandROffset : BlockHandLOffset);
	}
	return Base;
}

void AZP_GraceCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Fall-damage apex tracking — while airborne, remember the highest point
	// reached so the drop distance is measured peak-to-landing (a jump up then
	// a fall counts from the top, not from takeoff).
	if (bTrackingFall)
	{
		FallPeakZ = FMath::Max(FallPeakZ, GetActorLocation().Z);
	}

	// Keep blocked pickups (owned weapon / ammo at cap) non-interactable. Throttled
	// so it catches ammo dropping below the cap without per-frame cost.
	PickupBlockRefreshAccum += DeltaTime;
	if (PickupBlockRefreshAccum >= 0.25f)
	{
		PickupBlockRefreshAccum = 0.f;
		RefreshPickupBlockStates();
	}

	// Grab-all sweep: after E on a pickup, keep grabbing the next-closest pickup for a
	// short window so a bunched pile (e.g. scattered ammo) comes up in one press. Each
	// tick re-invokes Moonville's own pickup path; it recomputes the closest between
	// ticks as items are consumed. Stops as soon as nothing pickup-like is closest.
	if (GrabAllTicksRemaining > 0)
	{
		GrabAllTicksRemaining--;
		AActor* Pickup = GetClosestMoonvillePickupOwner();
		if (!Pickup)
		{
			// Nothing pickup-like is closest — pile exhausted.
			GrabAllTicksRemaining = 0;
			GrabAllGrabbedActors.Reset();
		}
		else if (GrabAllGrabbedActors.Contains(Pickup))
		{
			// Same actor is still the closest because Moonville hasn't destroyed it yet.
			// Do NOT re-grab it (that duplicated non-stacking items). Wait this tick for
			// Moonville to advance ClosestInteractable to the next pile item.
		}
		else if (MoonvilleInteractionComp && !ShouldBlockPickupInteraction(Pickup))
		{
			GrabAllGrabbedActors.Add(Pickup);
			if (UFunction* F = MoonvilleInteractionComp->FindFunction(FName("Interact")))
			{
				MoonvilleInteractionComp->ProcessEvent(F, nullptr);
			}
		}
		else
		{
			// Closest is a blocked pickup (owned weapon / capped ammo) — stop sweeping.
			GrabAllTicksRemaining = 0;
			GrabAllGrabbedActors.Reset();
		}
	}

	// Live-tunable Marcus body facing (base -90 yaw + dev-dialed offset).
	if (MarcusBody)
	{
		MarcusBody->SetRelativeRotation(FRotator(MarcusBodyPitch, -90.0f + MarcusBodyYaw, 0.0f));
	}

	// Per-action CAMERA offset during weapon actions (lerped, fed to GameplayComp).
	// Priority on overlap: swing > block > reload > switch.
	{
		FVector TargetOffset = FVector::ZeroVector;
		if (KinemationComp && KinemationComp->IsSwingingState())        TargetOffset = SwingCamOffset;
		else if (bIsBlocking)                                          TargetOffset = BlockCamOffset;
		else if (DodgeClearanceRemaining > 0.f)
		{
			const bool bDodgeMelee = KinemationComp
				&& KinemationComp->CurrentWeaponType == EZP_WeaponType::Melee;
			TargetOffset = bDodgeMelee ? DodgeCamOffsetMelee : DodgeCamOffsetRanged;
		}
		else if (KinemationComp && KinemationComp->IsReloadingState()) TargetOffset = ReloadCamOffset;
		else if (KinemationComp && KinemationComp->IsSwitchingState()) TargetOffset = SwitchCamOffset;
		CurrentWeaponActionOffset = FMath::VInterpTo(CurrentWeaponActionOffset, TargetOffset, DeltaTime, WeaponActionOffsetSpeed);
		if (GameplayComp) GameplayComp->WeaponActionCamOffset = CurrentWeaponActionOffset;
	}

	// Live-tunable framing for the Marcus weapon-arm view-model. While blocking, add the
	// block-specific placement (BlockViewOffset) so the arms can sit differently in a guard.
	if (MeleeViewMesh)
	{
		const FVector ViewLoc = MeleeViewOffset + (bIsBlocking ? BlockViewOffset : FVector::ZeroVector);
		MeleeViewMesh->SetRelativeLocation(ViewLoc);
		MeleeViewMesh->SetRelativeRotation(MeleeViewRotation);
		MeleeViewMesh->SetRelativeScale3D(FVector(MeleeViewScale));
	}

	// Live-tunable offset for the RANGED arm view-model — all three move together so
	// arms, hands and sleeve stay aligned (on top of the leader-posed Kinemation aim).
	{
		const FVector  ROff = RangedArmsOffset;
		const FRotator RRot = RangedArmsRotation;
		const FVector  RScl(RangedArmsScale);
		if (RangedArms)   { RangedArms->SetRelativeLocation(ROff);   RangedArms->SetRelativeRotation(RRot);   RangedArms->SetRelativeScale3D(RScl); }
		if (RangedHands)  { RangedHands->SetRelativeLocation(ROff);  RangedHands->SetRelativeRotation(RRot);  RangedHands->SetRelativeScale3D(RScl); }
		if (RangedSleeve) { RangedSleeve->SetRelativeLocation(ROff); RangedSleeve->SetRelativeRotation(RRot); RangedSleeve->SetRelativeScale3D(RScl); }
	}

	// Weapon arms. Marcus's BODY stays visible for every state (so you always see him —
	// overalls/legs — looking down). Only his own loco ARMS hide when a weapon's arms are
	// shown, so they don't double. Melee = the CCMH MeleeViewMesh; ranged = the Kinemation
	// arms (SK_Arm/Hand + FP sleeve) leader-posed to the live aim pose, reaching forward.
	if (KinemationComp)
	{
		const bool bMeleeUp     = KinemationComp->IsMeleeViewModelActive();
		const bool bRangedArmed = KinemationComp->ActiveWeapon != nullptr && !bMeleeUp;
		const bool bWeaponArms  = bMeleeUp || bRangedArmed;

		if (bRangedArmed != bRangedArmedState)
		{
			bRangedArmedState = bRangedArmed;
			if (RangedArms)   RangedArms->SetVisibility(bRangedArmed);
			if (RangedHands)  RangedHands->SetVisibility(bRangedArmed);
			if (RangedSleeve) RangedSleeve->SetVisibility(bRangedArmed);
		}
		// Camera offset is selected EVERY frame from the current armed state — NOT
		// only on the armed-state edge. The edge-only version left bCameraOffsetActive
		// stuck at its spawn default, so the CameraRanged* knobs never took effect and
		// the gun couldn't be tuned. false => GameplayComp uses CameraRanged* (gun at
		// the pack-authored FPCamera socket); true => Marcus eye offset (CameraExtra*).
		if (GameplayComp) GameplayComp->bCameraOffsetActive = !bRangedArmed;

		// Hide Marcus's own arms whenever any weapon arms are shown (melee or ranged).
		if (MarcusBody && bWeaponArms != bMarcusArmsHidden)
		{
			bMarcusArmsHidden = bWeaponArms;
			const FName ArmRoots[] = { FName("clavicle_l"), FName("clavicle_r") };
			for (const FName& Bone : ArmRoots)
			{
				if (bWeaponArms)
				{
					MarcusBody->HideBoneByName(Bone, EPhysBodyOp::PBO_None);
					if (MarcusOveralls) MarcusOveralls->HideBoneByName(Bone, EPhysBodyOp::PBO_None);
				}
				else
				{
					MarcusBody->UnHideBoneByName(Bone);
					if (MarcusOveralls) MarcusOveralls->UnHideBoneByName(Bone);
				}
			}
		}
		// Melee CCMH sleeves show only for melee.
		if (MeleeViewOveralls) MeleeViewOveralls->SetVisibility(bMeleeUp);
	}

	// Force a fresh, deterministic retarget evaluation of Marcus AFTER PlayerMesh's
	// pose is finalized this frame. The pack's RetargetPoseFromMesh auto-eval flips
	// to ref pose every other frame against our custom dual-mesh source (confirmed:
	// source stable, Marcus alternates). Re-evaluating here — guaranteed after
	// PlayerMesh via the tick prerequisite — reads the final source pose every frame.
	if (DodgeCooldownRemaining > 0.f)
	{
		DodgeCooldownRemaining = FMath::Max(0.f, DodgeCooldownRemaining - DeltaTime);
	}

	if (DodgeLockRemaining > 0.f)
	{
		DodgeLockRemaining = FMath::Max(0.f, DodgeLockRemaining - DeltaTime);
	}

	if (DodgeClearanceRemaining > 0.f)
	{
		DodgeClearanceRemaining = FMath::Max(0.f, DodgeClearanceRemaining - DeltaTime);
	}

	if (BlockClearanceRemaining > 0.f)
	{
		BlockClearanceRemaining = FMath::Max(0.f, BlockClearanceRemaining - DeltaTime);
	}

	// Which device is the player on? (drives button glyphs on UI prompts)
	UpdateInputDeviceTracking();

	// Grab/struggle machine: camera blend weight, escape meter, damage ticks, phase timers.
	UpdateGrab(DeltaTime);

	// Transient forward nudge that covers the block/dodge stance lean-in, then
	// eases out (see UZP_GraceGameplayComponent). Driven by short windows — NOT
	// pinned to bIsBlocking — so a sustained block hold settles to neutral.
	if (GameplayComp)
	{
		GameplayComp->SetForwardClearanceActive(
			BlockClearanceRemaining > 0.f || DodgeClearanceRemaining > 0.f);
	}

	// Camera diagnostic probe burst.
	if (CameraProbeFramesLeft > 0)
	{
		LogCameraProbe(TEXT("TICK"));
		LogBoneClipProbe();
		--CameraProbeFramesLeft;
	}

	// Buffered block engage: RMB held through a swing raises the guard the instant the swing
	// allows (post-contact recovery cancel) — the press is never discarded.
	TryEngageBufferedBlock();

	// Distance-based footsteps (the SingleNode locomotion has no anim notifies to fire them).
	UpdateFootsteps(DeltaTime);

	// Block animation state machine — counts down BlockImpact lock, then
	// keeps BlockLoop/BlockWalk in sync with movement state on the view mesh.
	if (bIsBlocking)
	{
		if (BlockImpactLockRemaining > 0.f)
		{
			BlockImpactLockRemaining = FMath::Max(0.f, BlockImpactLockRemaining - DeltaTime);
		}
		if (BlockStartLockRemaining > 0.f)
		{
			BlockStartLockRemaining = FMath::Max(0.f, BlockStartLockRemaining - DeltaTime);
		}
		UpdateBlockAnimation();
	}

	// Ease the pipe's hand_r-relative grip between idle and block while the melee view
	// model is up. The pipe is parented to hand_r, so this only sets a RELATIVE offset —
	// it can never swing out of the hand on un/block. Hold the block grip through
	// bBlockResolving (until BlockStop finishes) so the grip doesn't ease back to idle
	// while the hands are still in the block pose-out.
	if (KinemationComp && KinemationComp->IsMeleeViewModelActive())
	{
		KinemationComp->UpdateMeleeGrip(DeltaTime, bIsBlocking || bBlockResolving);
	}

	// --- Interaction state watchdog (session 63 diagnostics) ---
	// Polls Moonville's interaction state 4x/sec and logs ONLY transitions,
	// catching the exact moment popups/interaction die without log spam.
	InteractStateLogAccum += DeltaTime;
	if (InteractStateLogAccum >= 0.25f)
	{
		InteractStateLogAccum = 0.f;
		const FString State = GetMoonvilleInteractionStateString();
		if (State != LastInteractStateDump)
		{
			UE_LOG(LogTemp, Warning, TEXT("[INTERACT-STATE] CHANGED: %s"), *State);
			LastInteractStateDump = State;
		}
	}

	// Switch hidden Mesh animation based on ground speed and crouch state
	if (UAnimSingleNodeInstance* SNI = Cast<UAnimSingleNodeInstance>(GetMesh()->GetAnimInstance()))
	{
		if (bOnLadder)
		{
			// While the animated top-exit plays, it owns the climb tick.
			if (bLadderTopExiting)
			{
				UpdateLadderTopExit(DeltaTime);
			}

			// --- Rung-to-rung discrete movement ---
			// Player commits to each rung step. Can't stop or reverse mid-step.
			AZP_Ladder* Ladder = ActiveLadderActor.IsValid() ? Cast<AZP_Ladder>(ActiveLadderActor.Get()) : nullptr;

			// Position-driven climb animation: anim time mapped from height on ladder.
			// One anim cycle (1.533s) = 2 rungs (47 UU). Each 23.5 UU = half cycle = hands alternate.
			if (!bLadderTopExiting && LadderClimbUpAnimation && Ladder)
			{
				if (SNI->GetCurrentAsset() != LadderClimbUpAnimation)
				{
					SNI->SetAnimationAsset(LadderClimbUpAnimation, true, 1.0f);
					SNI->SetPlaying(false); // we control position manually
				}

				const float RungSpacing = 23.5f;
				float HeightInLadder = GetActorLocation().Z - Ladder->GetBottomZ();
				float AnimDuration = LadderClimbUpAnimation->GetPlayLength();
				float HeightPerCycle = 2.f * RungSpacing; // 47 UU per full anim cycle
				float AnimFrac = FMath::Fmod(HeightInLadder / HeightPerCycle, 1.f);
				if (AnimFrac < 0.f) AnimFrac += 1.f;

				SNI->SetPosition(AnimFrac * AnimDuration, false);
			}

			if (!bLadderTopExiting && Ladder)
			{
				const float RungSpacing = 23.5f;
				const float TopClimbZ = Ladder->GetTopZ() - 80.f;
				float HeightInLadder = GetActorLocation().Z - Ladder->GetBottomZ();

				// Accept new input only when not already moving to a rung
				if (!bLadderMovingToRung && FMath::Abs(LadderClimbInput) > 0.1f)
				{
					// Snap to nearest rung, then target one rung in input direction
					float NearestRungIdx = FMath::RoundToFloat(HeightInLadder / RungSpacing);
					float NearestRungZ = Ladder->GetBottomZ() + NearestRungIdx * RungSpacing;

					if (LadderClimbInput > 0.f)
						LadderTargetRungZ = NearestRungZ + RungSpacing;
					else
						LadderTargetRungZ = NearestRungZ - RungSpacing;

					bLadderMovingToRung = true;
				}

				// Interpolate toward target rung
				if (bLadderMovingToRung)
				{
					float CurrentZ = GetActorLocation().Z;
					float Direction = (LadderTargetRungZ > CurrentZ) ? 1.f : -1.f;
					float DistToTarget = FMath::Abs(LadderTargetRungZ - CurrentZ);

					// Sine-based speed: slow at rung (start/end), fast mid-step
					float Progress = FMath::Clamp(1.f - (DistToTarget / RungSpacing), 0.f, 1.f);
					float SpeedMult = 0.3f + 1.4f * FMath::Sin(Progress * PI);

					float NewZ = CurrentZ + Direction * Ladder->ClimbSpeed * SpeedMult * DeltaTime;

					// Snap when arrived (or overshot)
					if ((Direction > 0.f && NewZ >= LadderTargetRungZ) ||
						(Direction < 0.f && NewZ <= LadderTargetRungZ))
					{
						NewZ = LadderTargetRungZ;
						bLadderMovingToRung = false;
					}

					// Bounds check
					if (NewZ <= Ladder->GetBottomZ())
					{
						ExitLadder(false);
					}
					else if (NewZ >= TopClimbZ)
					{
						BeginLadderTopExit(Ladder);
					}
					else
					{
						SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, NewZ));
					}
				}

				// --- Rung vs Hand diagnostic logging (every 30 frames) ---
				static int32 LadderDiagCounter = 0;
				if (++LadderDiagCounter >= 30)
				{
					LadderDiagCounter = 0;

					float CurrentHeight = GetActorLocation().Z - Ladder->GetBottomZ();
					int32 NearestRung = FMath::RoundToInt(CurrentHeight / RungSpacing);
					float NearestRungWorldZ = Ladder->GetBottomZ() + NearestRung * RungSpacing;

					// Hand bone world positions
					int32 HandLIdx = PlayerMesh->GetBoneIndex(FName("hand_l"));
					int32 HandRIdx = PlayerMesh->GetBoneIndex(FName("hand_r"));
					FVector HandLPos = (HandLIdx != INDEX_NONE) ? PlayerMesh->GetBoneTransform(HandLIdx).GetLocation() : FVector::ZeroVector;
					FVector HandRPos = (HandRIdx != INDEX_NONE) ? PlayerMesh->GetBoneTransform(HandRIdx).GetLocation() : FVector::ZeroVector;

					UE_LOG(LogTemp, Warning, TEXT("[LADDER-RUNG] Rung#%d RungZ=%.1f | ActorZ=%.1f HeightInLadder=%.1f | HandL_Z=%.1f HandR_Z=%.1f | Moving=%d TargetZ=%.1f"),
						NearestRung, NearestRungWorldZ,
						GetActorLocation().Z, CurrentHeight,
						HandLPos.Z, HandRPos.Z,
						bLadderMovingToRung ? 1 : 0, LadderTargetRungZ);
					UE_LOG(LogTemp, Warning, TEXT("[LADDER-RUNG] HandL=(%.1f,%.1f,%.1f) HandR=(%.1f,%.1f,%.1f)"),
						HandLPos.X, HandLPos.Y, HandLPos.Z, HandRPos.X, HandRPos.Y, HandRPos.Z);
				}
			}

			// Reset after use — only moves when W/S actively held next frame
			LadderClimbInput = 0.f;
		}
		else if (GrabPhase != EZP_GrabPhase::None)
		{
			// Grabbed: the grab machine owns BOTH meshes' playback — MarcusBody plays the victim
			// clips (SetGrabPhase), and during knockdown/get-up the hidden Mesh plays the FP clips
			// with full-body copy. The loco switcher below must not stomp either.
		}
		else
		{
			const float Speed = GetVelocity().Size2D();
			UAnimSequenceBase* DesiredAnim = nullptr;

			if (bIsCrouched)
			{
				DesiredAnim = CrouchIdleAnimation ? CrouchIdleAnimation : IdleAnimation;
				if (Speed > 10.0f && CrouchWalkAnimation)
					DesiredAnim = CrouchWalkAnimation;
			}
			else
			{
				DesiredAnim = IdleAnimation;
				if (Speed > 150.0f && RunAnimation)
					DesiredAnim = RunAnimation;
				else if (Speed > 10.0f && WalkAnimation)
					DesiredAnim = WalkAnimation;
			}

			if (DesiredAnim && SNI->GetCurrentAsset() != DesiredAnim)
			{
				SNI->SetAnimationAsset(DesiredAnim, true, 1.0f);
				SNI->SetPlaying(true);
			}

			// Mirror the same speed/crouch state onto Marcus's VISIBLE body using its
			// native CCMH clips via SingleNode. Native playback = no live retarget node
			// = no shuffle. (Weapon/aim arm poses come from MeleeViewMesh, not the body.)
			if (MarcusBody && MarcusBody->GetSkeletalMeshAsset())
			{
				UAnimSequenceBase* MDesired = MarcusIdle;
				if (bIsCrouched)
				{
					MDesired = MarcusCrouchIdle ? MarcusCrouchIdle : MarcusIdle;
					if (Speed > 10.0f && MarcusCrouchWalk) MDesired = MarcusCrouchWalk;
				}
				else
				{
					if (Speed > 150.0f && MarcusRun) MDesired = MarcusRun;
					else if (Speed > 10.0f && MarcusWalk) MDesired = MarcusWalk;
				}
				if (UAnimSingleNodeInstance* MSNI = MarcusBody->GetSingleNodeInstance())
				{
					if (MDesired && MSNI->GetCurrentAsset() != MDesired)
					{
						MSNI->SetAnimationAsset(MDesired, true, 1.0f);
						MSNI->SetPlaying(true);
					}
				}
			}
		}
	}

	// Bone copy now happens in NativePostEvaluateAnimation (inside animation pipeline).

	// Save menu open → watch for it to close (the EGUI widget removes itself via its own back/close),
	// then restore game input. UIOnly was set on open so the controller could navigate the menu; if
	// we didn't restore here the player would be stuck unable to move after closing.
	if (bSaveMenuOpen)
	{
		// Manual BACK: Face Right (B) / Escape closes the save menu. The EGUI save widget is shown via
		// raw AddToViewport (not a CommonUI activatable stack), so CommonUI's back action never routes
		// to it. Raw key poll works regardless of input mode (UIOnly) and IMC_Grace being removed.
		if (ActiveSaveMenu.IsValid())
		{
			if (APlayerController* BackPC = Cast<APlayerController>(GetController()))
			{
				if (BackPC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right)
					|| BackPC->WasInputKeyJustPressed(EKeys::Escape))
				{
					UE_LOG(LogTemp, Warning, TEXT("[UIProbe] Save menu BACK pressed — closing"));
					ActiveSaveMenu->RemoveFromParent(); // close-detection below restores game input
				}
			}
		}

		if (!ActiveSaveMenu.IsValid() || !ActiveSaveMenu->IsInViewport())
		{
			bSaveMenuOpen = false;
			ActiveSaveMenu.Reset();
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				// Re-add the gameplay context we pulled in OpenSaveMenu, then hand input back to the game.
				if (AZP_PlayerController* ZPC = Cast<AZP_PlayerController>(PC))
				{
					if (ZPC->DefaultMappingContext)
					{
						ZPC->AddMappingContext(ZPC->DefaultMappingContext, ZPC->DefaultMappingPriority);
					}
				}
				PC->SetInputMode(FInputModeGameOnly());
				PC->SetShowMouseCursor(false);
			}
		}
	}

	// First-time pickup notification (Moonville CommonUI splash): Moonville shows it but never
	// switches input mode, so the game stays in GameOnly and ONLY the mouse can reach it — keyboard/
	// gamepad do nothing (dev: "can't interact in UI, only mouse"). Same fix as the save menu: while
	// it's open, pull IMC_Grace + go GameAndUI so the controller/keyboard reach the widget's own
	// Continue button (A / Enter dismiss it; mouse still works so there's no hard-lock). Detected via
	// Moonville's bFirstTimePickupMenuOpen flag; restore on close.
	{
		// Detect by WIDGET PRESENCE (Moonville's bFirstTimePickupMenuOpen flag proved unreliable).
		static UClass* PickupCls = LoadClass<UUserWidget>(nullptr,
			TEXT("/Game/InventorySystemPro/Blueprints/UI/SubWidgets/WBP_FirstTimePickupNotificationBase.WBP_FirstTimePickupNotificationBase_C"));
		UUserWidget* PickupW = nullptr;
		if (PickupCls)
		{
			TArray<UUserWidget*> Found;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Found, PickupCls, false);
			for (UUserWidget* W : Found) { if (W && W->IsInViewport()) { PickupW = W; break; } }
		}
		const bool bPickupNow = (PickupW != nullptr);
		if (bPickupNow != bPickupMenuActive)
		{
			bPickupMenuActive = bPickupNow;
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				AZP_PlayerController* ZPC = Cast<AZP_PlayerController>(PC);
				if (bPickupNow)
				{
					UE_LOG(LogTemp, Warning, TEXT("[UIProbe] PICKUP OPEN: widget=%s"), *PickupW->GetName());
					if (ZPC && ZPC->DefaultMappingContext) { ZPC->RemoveMappingContext(ZPC->DefaultMappingContext); }
					PC->SetInputMode(FInputModeGameOnly());
					PC->SetShowMouseCursor(false);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[UIProbe] PICKUP CLOSED — restoring game input"));
					if (ZPC && ZPC->DefaultMappingContext) { ZPC->AddMappingContext(ZPC->DefaultMappingContext, ZPC->DefaultMappingPriority); }
					PC->SetInputMode(FInputModeGameOnly());
					PC->SetShowMouseCursor(false);
				}
			}
		}

		// While the pickup widget is up, CommonActivatableWidget keeps re-applying its own InputConfig
		// (UIOnly/GameAndUI) over the SetInputMode we set on open — which causes Slate to eat A and the
		// PC's WasInputKeyJustPressed to return false. Re-assert GameOnly + IMC_Grace removal every Tick
		// so the raw poll below actually receives A.
		if (bPickupMenuActive && PickupW)
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				if (AZP_PlayerController* ZPC = Cast<AZP_PlayerController>(PC))
				{
					if (ZPC->DefaultMappingContext) { ZPC->RemoveMappingContext(ZPC->DefaultMappingContext); }
				}
				PC->SetInputMode(FInputModeGameOnly());

				const bool bPressed =
					PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom) ||
					PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right)  ||
					PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left)   ||
					PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top)    ||
					PC->WasInputKeyJustPressed(EKeys::E)                          ||
					PC->WasInputKeyJustPressed(EKeys::Enter)                      ||
					PC->WasInputKeyJustPressed(EKeys::SpaceBar)                   ||
					PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);
				if (bPressed)
				{
					// Gate on Moonville's bCanBeSkipped (flips true ~0.2s after construct). If we don't
					// gate, the SAME A press that picked the item up also fires this poll on the tick
					// the widget appears, dismissing it instantly — widget never visible.
					bool bCanSkip = false;
					if (FBoolProperty* SkipP = CastField<FBoolProperty>(PickupW->GetClass()->FindPropertyByName(FName("bCanBeSkipped"))))
					{
						bCanSkip = SkipP->GetPropertyValue(SkipP->ContainerPtrToValuePtr<void>(PickupW));
					}
					if (bCanSkip)
					{
						bool bClosed = false;
						if (UFunction* CloseFn = PickupW->FindFunction(FName("CloseFirstTimePickupNotification")))
						{
							void* Parms = FMemory_Alloca(FMath::Max<int32>(CloseFn->ParmsSize, 1));
							FMemory::Memzero(Parms, CloseFn->ParmsSize);
							PickupW->ProcessEvent(CloseFn, Parms);
							bClosed = true;
							UE_LOG(LogTemp, Warning, TEXT("[UIProbe] Pickup dismissed via key -> CloseFirstTimePickupNotification"));
						}
						if (!bClosed)
						{
							PickupW->RemoveFromParent();
							UE_LOG(LogTemp, Warning, TEXT("[UIProbe] Pickup dismissed via RemoveFromParent fallback"));
						}
					}
				}
			}
		}
	}

	// --- Weapon slot keys (raw polling) ---
	// Bypasses Enhanced Input entirely to avoid IMC conflicts between
	// IMC_Grace and Moonville's IMC_InventoryCharacter double-firing.
	// Gated while the save menu / first-time pickup is open so the D-pad drives the UI, not weapons
	// (the raw poll reads hardware state and ignores input mode, so it needs an explicit guard).
	if (!bInventoryMenuOpen && !bMapOpen && !bOnLadder && !bSaveMenuOpen && !bPickupMenuActive
		&& GrabPhase == EZP_GrabPhase::None)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (PC->WasInputKeyJustPressed(EKeys::One))        Input_InventorySlot(0);
			else if (PC->WasInputKeyJustPressed(EKeys::Two))   Input_InventorySlot(1);
			else if (PC->WasInputKeyJustPressed(EKeys::Three)) Input_InventorySlot(2);
			else if (PC->WasInputKeyJustPressed(EKeys::Four))  Input_InventorySlot(3);
			// Controller D-pad → quick slots. Raw-polled here for the same reason as the number
			// keys: slots bypass Enhanced Input to dodge the IMC_Grace / IMC_InventoryCharacter
			// double-fire conflict, which also stranded the D-pad mapping. Order matches the
			// original IMC_Grace D-pad bindings (Up=0, Right=1, Left=2, Down=3).
			else if (PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Up))    { UE_LOG(LogTemp, Warning, TEXT("[DPadProbe] raw EKey DPad_Up -> slot 3 (Key4)")); Input_InventorySlot(3); }
			else if (PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Right)) { UE_LOG(LogTemp, Warning, TEXT("[DPadProbe] raw EKey DPad_Right -> slot 0")); Input_InventorySlot(0); }
			else if (PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Left))  { UE_LOG(LogTemp, Warning, TEXT("[DPadProbe] raw EKey DPad_Left -> slot 2 (Key3)")); Input_InventorySlot(2); }
			else if (PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Down))  { UE_LOG(LogTemp, Warning, TEXT("[DPadProbe] raw EKey DPad_Down -> slot 1")); Input_InventorySlot(1); }
		}
	}

	// --- Container close detection (briefcase sync + weapon unequip) ---
	CheckContainerClosed();

	// Flashlight toggle is now an Enhanced Input action (IA_InventoryFlashlight, bound to
	// Input_Flashlight). Mapped in IMC_Grace (F + Right Shoulder); IMC_Grace stays active during
	// menus so it still toggles with a menu open, same as the old raw F poll.

	// Flashlight follows camera with lag (chest-mounted feel)
	if (FlashlightComp && bFlashlightOn && FirstPersonCamera)
	{
		FRotator TargetRot = FirstPersonCamera->GetComponentRotation();
		TargetRot.Pitch += FlashlightPitchOffset;
		FRotator NewRot = FMath::RInterpTo(FlashlightComp->GetComponentRotation(), TargetRot, DeltaTime, FlashlightInterpSpeed);
		FlashlightComp->SetWorldRotation(NewRot);
	}

	// --- Gun-muzzle wall standoff ---
	// The body capsule (radius ~55) stops short of walls, but the weapon
	// extends past the camera and clips through. Account for the gun's reach
	// in the player's collision: hold the whole capsule a gun-length off any
	// wall the player faces so the muzzle can't penetrate. We move the CAPSULE
	// (camera rides it  normal); we never touch PlayerMesh/weapon (dead-end).
	if (GunCollisionReach > 0.f && FirstPersonCamera && !bOnLadder)
	{
		const FVector CamLoc = FirstPersonCamera->GetComponentLocation();
		const FVector ProbeEnd = CamLoc + FirstPersonCamera->GetForwardVector() * GunCollisionReach;

		FHitResult GunHit;
		FCollisionQueryParams GunParams;
		GunParams.AddIgnoredActor(this);
		FCollisionObjectQueryParams GunObj;
		GunObj.AddObjectTypesToQuery(ECC_WorldStatic);
		if (GetWorld()->SweepSingleByObjectType(GunHit, CamLoc, ProbeEnd, FQuat::Identity,
			GunObj, FCollisionShape::MakeSphere(GunCollisionRadius), GunParams))
		{
			// Walls only  ignore floor/ceiling so looking down doesn't shove you.
			// Skip start-penetrating (camera already buried) to avoid a yank.
			if (!GunHit.bStartPenetrating && FMath::Abs(GunHit.ImpactNormal.Z) < 0.7f)
			{
				const float Overshoot = GunCollisionReach - GunHit.Distance;
				if (Overshoot > 0.f)
				{
					FVector Push = GunHit.ImpactNormal * Overshoot;
					Push.Z = 0.f; // horizontal only  never lift or drop the player
					AddActorWorldOffset(Push, true); // swept  won't tunnel through geometry
				}
			}
		}
	}

}

// --- CalcCamera override ---
// During ladder climbing: simple fixed-offset position + standard controller rotation.
// CalcCamera runs last — nothing can override it.
void AZP_GraceCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	// Grab/struggle: over-the-right-shoulder 3P frame, blended with the live FP view by
	// GrabCamWeight (lerped in UpdateGrab) so entry and return are smooth on every exit path.
	// Same mechanism as the ladder — CalcCamera has final say, nothing detaches.
	if (GrabCamWeight > KINDA_SMALL_NUMBER)
	{
		FMinimalViewInfo FPView;
		Super::CalcCamera(DeltaTime, FPView);

		const FVector Anchor = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
		const FRotator YawRot(0.f, GetActorRotation().Yaw, 0.f);
		const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		FVector CamPos = Anchor - Fwd * GrabCamBack + Right * GrabCamRight + FVector(0.f, 0.f, GrabCamUp);

		// Hand-rolled spring-arm probe (no spring arm exists on this rig): clamp to geometry so
		// the shoulder camera never ends up inside a wall in tight corridors.
		FCollisionQueryParams Q(FName(TEXT("ZPGrabCam")), false);
		Q.AddIgnoredActor(this);
		if (AActor* G = GrabberActor.Get()) { Q.AddIgnoredActor(G); }
		FHitResult CamHit;
		if (GetWorld()->SweepSingleByChannel(CamHit, Anchor, CamPos, FQuat::Identity, ECC_Camera,
			FCollisionShape::MakeSphere(12.f), Q))
		{
			CamPos = CamHit.Location;
		}

		// Aim at the midpoint between Marcus's head and the grabber's head — frames the wrestle.
		FVector LookAt = Anchor + Fwd * 60.f;
		if (AActor* G = GrabberActor.Get())
		{
			LookAt = (Anchor + G->GetActorLocation() + FVector(0.f, 0.f, 60.f)) * 0.5f;
		}
		const FRotator CamRot = (LookAt - CamPos).Rotation();

		const float W = FMath::SmoothStep(0.f, 1.f, GrabCamWeight);
		OutResult.Location = FMath::Lerp(FPView.Location, CamPos, W);
		OutResult.Rotation = FQuat::Slerp(FPView.Rotation.Quaternion(), CamRot.Quaternion(), W).Rotator();
		OutResult.FOV = FPView.FOV;
		return;
	}

	if (bLadderTopExiting)
	{
		// Camera rises from the ladder eye to the standing-on-floor eye, tracking
		// the body as its root motion carries it over the edge.
		const float Frac = (LadderTopExitDuration > 0.f)
			? FMath::Clamp(LadderTopExitElapsed / LadderTopExitDuration, 0.f, 1.f) : 1.f;
		const float Smooth = FMath::SmoothStep(0.f, 1.f, Frac);
		OutResult.Location = FMath::Lerp(LadderTopExitCamStart, LadderTopExitCamEnd, Smooth);
		OutResult.Rotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
		OutResult.FOV = FirstPersonCamera ? FirstPersonCamera->FieldOfView : 90.f;
		return;
	}

	if (bOnLadder)
	{
		// Position: actor origin + eye height. Head mesh is hidden during climbing
		// so no forward push needed — camera sits at true eye position.
		OutResult.Location = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
		OutResult.Rotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
		OutResult.FOV = FirstPersonCamera ? FirstPersonCamera->FieldOfView : 90.f;
		return;
	}

	Super::CalcCamera(DeltaTime, OutResult);

	// Re-apply input clamps each frame so live edits to CameraPitchDownLimitDeg /
	// CameraPitchUpLimitDeg take effect without restarting PIE.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->ViewPitchMin = -CameraPitchDownLimitDeg;
			PC->PlayerCameraManager->ViewPitchMax =  CameraPitchUpLimitDeg;
		}
	}

	// Final-camera-pitch clamp catches BOTH player look input AND animation-driven
	// socket dives (Kinemation reload/equip pitches FPCamera socket below horizon)
	// so the camera never reaches the body's interior cavity. Normalize first to
	// handle wrapped values from socket transforms.
	FRotator R = OutResult.Rotation;
	R.Pitch = FMath::Clamp(FRotator::NormalizeAxis(R.Pitch), -CameraPitchDownLimitDeg, CameraPitchUpLimitDeg);
	OutResult.Rotation = R;
}

// --- Crouch ---

void AZP_GraceCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	// Snapshot lower body bones BEFORE Super adjusts anything
	if (UZP_GracePlayerAnimInstance* AnimInst = Cast<UZP_GracePlayerAnimInstance>(PlayerMesh->GetAnimInstance()))
	{
		const float InterpSpeed = MovementConfig ? MovementConfig->CrouchCameraInterpSpeed : 8.0f;
		AnimInst->StartBoneBlend(InterpSpeed);
	}

	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Smooth camera transition — compensate for the instant capsule drop
	if (GameplayComp)
	{
		GameplayComp->StopSprint(); // can't sprint while crouched — cancel any active sprint
		GameplayComp->OnCrouchHeightChanged(ScaledHalfHeightAdjust);
	}
}

void AZP_GraceCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	// Snapshot lower body bones BEFORE Super adjusts anything
	if (UZP_GracePlayerAnimInstance* AnimInst = Cast<UZP_GracePlayerAnimInstance>(PlayerMesh->GetAnimInstance()))
	{
		const float InterpSpeed = MovementConfig ? MovementConfig->CrouchCameraInterpSpeed : 8.0f;
		AnimInst->StartBoneBlend(InterpSpeed);
	}

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// Smooth camera transition — compensate for the instant capsule rise
	if (GameplayComp)
	{
		GameplayComp->OnCrouchHeightChanged(-ScaledHalfHeightAdjust);
	}
}

// --- Input Setup ---

void AZP_GraceCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] ZP_GraceCharacter: Failed to cast to EnhancedInputComponent!"));
		return;
	}

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AZP_GraceCharacter::Input_Move);
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AZP_GraceCharacter::Input_MoveCompleted);
	}
	if (LookAction)
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AZP_GraceCharacter::Input_Look);
	}
	if (SprintAction)
	{
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_SprintStarted);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AZP_GraceCharacter::Input_SprintCompleted);
	}
	if (JumpAction)
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_Jump);
	}
	if (InteractAction)
	{
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_Interact);
	}
	if (CrouchAction)
	{
		// Toggle crouch: press once to crouch, press again to stand. (No Completed/release bind.)
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_CrouchStarted);
	}
	if (PeekAction)
	{
		EIC->BindAction(PeekAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_PeekStarted);
		EIC->BindAction(PeekAction, ETriggerEvent::Completed, this, &AZP_GraceCharacter::Input_PeekCompleted);
	}
	if (AimAction)
	{
		EIC->BindAction(AimAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_AimStarted);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &AZP_GraceCharacter::Input_AimCompleted);
	}
	if (FireAction)
	{
		EIC->BindAction(FireAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_FireStarted);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AZP_GraceCharacter::Input_FireCompleted);
	}
	if (ReloadAction)
	{
		EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_ReloadStarted);
	}
	if (InventoryMenuAction)
	{
		EIC->BindAction(InventoryMenuAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_InventoryMenu);
	}
	if (FlashlightAction)
	{
		EIC->BindAction(FlashlightAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_Flashlight);
	}
	if (MapAction)
	{
		EIC->BindAction(MapAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_Map);
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP DEBUG: MapAction BOUND (%s)"), *MapAction->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] MAP DEBUG: MapAction is NULL — M key will not work!"));
	}
	if (TabCycleLeftAction)
	{
		EIC->BindAction(TabCycleLeftAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_TabCycleLeft);
	}
	if (TabCycleRightAction)
	{
		EIC->BindAction(TabCycleRightAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_TabCycleRight);
	}
	// Weapon slot keys are handled via raw key polling in Tick() to bypass
	// Enhanced Input IMC conflicts between IMC_Grace and IMC_InventoryCharacter.

	UE_LOG(LogTemp, Warning, TEXT("[TheSignal] Slot keys: raw polling in Tick. InvMenu=%s"),
		InventoryMenuAction ? TEXT("BOUND") : TEXT("NULL"));

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: Input bound — Move:%s Look:%s Sprint:%s Jump:%s Interact:%s Crouch:%s Peek:%s Aim:%s Fire:%s Reload:%s Inventory:%s"),
		MoveAction ? TEXT("OK") : TEXT("MISSING"),
		LookAction ? TEXT("OK") : TEXT("MISSING"),
		SprintAction ? TEXT("OK") : TEXT("MISSING"),
		JumpAction ? TEXT("OK") : TEXT("MISSING"),
		InteractAction ? TEXT("OK") : TEXT("MISSING"),
		CrouchAction ? TEXT("OK") : TEXT("MISSING"),
		PeekAction ? TEXT("OK") : TEXT("MISSING"),
		AimAction ? TEXT("OK") : TEXT("MISSING"),
		FireAction ? TEXT("OK") : TEXT("MISSING"),
		ReloadAction ? TEXT("OK") : TEXT("MISSING"),
		InventoryMenuAction ? TEXT("OK") : TEXT("MISSING"));
}

// --- Input Handlers ---

void AZP_GraceCharacter::Input_Move(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle

	const FVector2D MoveInput = Value.Get<FVector2D>();
	CurrentMoveInput = MoveInput;

	// On ladder: forward/back = climb up/down, no lateral movement
	if (bOnLadder)
	{
		LadderClimbInput = MoveInput.Y; // W = +1 (up), S = -1 (down)
		return;
	}

	if (GameplayComp)
	{
		GameplayComp->SetForwardInput(MoveInput.Y);
	}

	if (Controller)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Backward penalty: scale backward input to 55%.
		// CMC's AnalogInputModifier reads input magnitude → reduces MaxSpeed proportionally.
		float ForwardScale = MoveInput.Y;
		if (ForwardScale < 0.f)
		{
			ForwardScale *= 0.55f;
		}
		AddMovementInput(ForwardDir, ForwardScale);
		AddMovementInput(RightDir, MoveInput.X);
	}
}

void AZP_GraceCharacter::Input_MoveCompleted(const FInputActionValue& Value)
{
	if (GameplayComp)
	{
		GameplayComp->SetForwardInput(0.0f);
	}
	LadderClimbInput = 0.f;
	CurrentMoveInput = FVector2D::ZeroVector;
}

void AZP_GraceCharacter::Input_Look(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle

	const FVector2D LookInput = Value.Get<FVector2D>();

	if (bOnLadder) return; // Camera fully locked during climbing

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(-LookInput.Y);
}

void AZP_GraceCharacter::Input_SprintStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen || bOnLadder) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	if (DodgeLockRemaining > 0.f) return; // locked out mid-dodge
	// Toggle sprint: press to start, press again to stop
	if (GameplayComp)
	{
		if (GameplayComp->bIsSprinting)
		{
			GameplayComp->StopSprint();
		}
		else
		{
			GameplayComp->StartSprint();
		}
	}
}

void AZP_GraceCharacter::Input_SprintCompleted(const FInputActionValue& Value)
{
	// Toggle mode: release does nothing. Sprint stops on next press or stamina depletion.
}

void AZP_GraceCharacter::CancelSprintFromAction()
{
	// Committing to an action breaks a sprint. Movement, look, and door/item interact are the
	// only inputs that DON'T call this (sprint needs movement; interacting shouldn't break stride).
	if (GameplayComp && GameplayComp->bIsSprinting)
	{
		GameplayComp->StopSprint();
	}
}

void AZP_GraceCharacter::Input_Jump(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	if (bOnLadder)
	{
		// Don't interrupt the animated climb-over once it has started.
		if (bLadderTopExiting) return;

		// If in upper half of ladder, exit at top; otherwise drop from current position
		AZP_Ladder* Ladder = Cast<AZP_Ladder>(ActiveLadderActor.Get());
		bool bNearTop = Ladder && GetActorLocation().Z >= (Ladder->GetBottomZ() + Ladder->GetTopZ()) * 0.5f;
		ExitLadder(bNearTop);
		return;
	}
	PerformDodge();
}

void AZP_GraceCharacter::Input_Interact(const FInputActionValue& Value)
{
	// NOTE (session 63): do NOT hook transfer-all here. Driving Moonville's
	// menu state machine from the Interact key (via reflection + its toggle)
	// desynced its menu state and killed ALL interaction popups/interacts.
	// Loot-all belongs inside WBP_ItemContainerMenu_Horror, where the open
	// container and the close path are unambiguous.

	UE_LOG(LogTemp, Warning, TEXT("[INTERACT-STATE] E pressed: %s | ZP: InvMenu=%d Map=%d Ladder=%d CurrentInteractable=%s"),
		*GetMoonvilleInteractionStateString(), bInventoryMenuOpen, bMapOpen, bOnLadder,
		CurrentInteractable.IsValid() ? *CurrentInteractable->GetName() : TEXT("None"));

	if (bInventoryMenuOpen || bMapOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Blocked — bInventoryMenuOpen=%d, bMapOpen=%d"), bInventoryMenuOpen, bMapOpen);
		return;
	}
	if (bOnLadder) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — no interacting mid-struggle

	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] === Input_Interact fired ==="));

	// --- Line-of-sight priority: if looking at a Moonville pickup, it takes
	//     precedence over overlap-based door/interactable detection. ---
	if (MoonvilleInteractionComp)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);

			FVector TraceEnd = CamLoc + CamRot.Vector() * 300.0f;
			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			// Use object type query — BP_ItemPickup is WorldDynamic, ECC_Visibility may not hit it
			FCollisionObjectQueryParams ObjParams;
			ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
			ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);

			// Multi-trace to skip through volumes (player is inside MapVolume which hits at dist 0)
			TArray<FHitResult> AllHits;
			GetWorld()->LineTraceMultiByObjectType(AllHits, CamLoc, TraceEnd, ObjParams, Params);

			// Find first non-volume hit on VISIBLE geometry. Invisible
			// interaction spheres (Moonville lockers) bulge over doorways and
			// through walls — hitting them here hijacked the press from doors
			// and looted lockers through walls (session 63, log-proven:
			// every E at the door hit BP_LootLocker's hidden sphere).
			FHitResult* BestHit = nullptr;
			for (FHitResult& H : AllHits)
			{
				AActor* A = H.GetActor();
				if (!A || A->GetClass()->GetName().Contains(TEXT("Volume")))
				{
					continue;
				}
				UPrimitiveComponent* HC = H.GetComponent();
				if (HC && (HC->IsA<UShapeComponent>() || !HC->IsVisible()))
				{
					continue;
				}
				BestHit = &H;
				break;
			}

			bool bTraceHit = (BestHit != nullptr);
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Line trace (300 UU). TotalHits=%d, ValidHit=%d"),
				AllHits.Num(), bTraceHit);

			if (bTraceHit)
			{
				AActor* HitActor = BestHit->GetActor();
				if (HitActor)
				{
					FString HitActorName = HitActor->GetName();

					// Walk the class hierarchy to match containers (BP_LootLocker → BP_ItemContainer_Horror → BP_ItemContainer)
					bool bMatchPickup = false;
					bool bMatchContainer = false;
					bool bIsBriefcase = false;
					for (UClass* C = HitActor->GetClass(); C; C = C->GetSuperClass())
					{
						FString CName = C->GetName();
						if (CName.Contains(TEXT("ItemPickup"))) bMatchPickup = true;
						if (CName.Contains(TEXT("ItemContainer")) || CName.Contains(TEXT("Chest")) || CName.Contains(TEXT("LootLocker"))) bMatchContainer = true;
						if (CName.Contains(TEXT("Briefcase"))) { bMatchContainer = true; bIsBriefcase = true; }
					}

					UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Hit: %s (class: %s) — Pickup=%d, Container=%d, Briefcase=%d"),
						*HitActorName, *HitActor->GetClass()->GetName(), bMatchPickup, bMatchContainer, bIsBriefcase);

					// Block pickup of a weapon already owned (2d) or ammo at cap (2b) —
					// looking right at it and pressing E does nothing.
					if (bMatchPickup && !bMatchContainer && ShouldBlockPickupInteraction(HitActor))
					{
						UE_LOG(LogTemp, Log, TEXT("[Pickup] %s blocked (already owned / ammo at cap)"), *HitActorName);
						return;
					}

					if (bMatchPickup || bMatchContainer)
					{
						// --- Loot Locker filtering ---
						// Ammo for weapons the player doesn't own is stripped, but
						// EMPTY lockers stay fully interactable (dev call, session
						// 63): the old DisableLockerInteraction/LootedEmptyLockers
						// path permanently killed filtered-empty lockers — popup
						// gone, never openable again — which read as "half the
						// lockers are broken".
						if (bMatchContainer && !bIsBriefcase)
						{
							FilterLockerAmmo(HitActor);
						}

						{
						// Pre-load ALL overlapping briefcases before Interact().
						// Moonville opens the CLOSEST overlap, which may differ from our trace target.
						// Loading all ensures whichever briefcase Moonville picks has correct data.
						if (bMatchContainer)
						{
							if (UZP_BriefcaseSubsystem* BriefcaseSub = GetGameInstance()->GetSubsystem<UZP_BriefcaseSubsystem>())
							{
								if (BriefcaseSub->HasStoredData())
								{
									TArray<AActor*> OverlappingActors;
									GetOverlappingActors(OverlappingActors);
									for (AActor* OA : OverlappingActors)
									{
										if (!OA) continue;
										bool bOAIsBriefcase = false;
										for (UClass* C = OA->GetClass(); C; C = C->GetSuperClass())
										{
											if (C->GetName().Contains(TEXT("Briefcase")))
											{
												bOAIsBriefcase = true;
												break;
											}
										}
										if (bOAIsBriefcase)
										{
											UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Pre-loading briefcase inventory into overlapping %s"), *OA->GetName());
											BriefcaseSub->LoadIntoBriefcase(OA);
										}
									}
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] No stored briefcase data — first briefcase this session"));
								}
							}
						}

						if (bIsBriefcase)
						{
							ActiveBriefcaseActor = HitActor;
						}

						// Track that a container is being opened — wait for Moonville to set bPlayerIsUsingActor=true
						bWaitingForContainerOpen = true;
						bContainerWasOpen = false;
						ContainerOpenWaitFrames = 0;
						ActiveContainerActor = HitActor;
						UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Container opened — bContainerWasOpen=true, ActiveContainer=%s, ActiveBriefcase=%s"),
							*HitActorName,
							ActiveBriefcaseActor.IsValid() ? *ActiveBriefcaseActor->GetName() : TEXT("None"));

						// Pure pickup (not a container) → sweep the rest of the pile this press.
						if (bMatchPickup && !bMatchContainer)
						{
							// Seed the sweep with the actor this press will grab so the first
							// sweep tick doesn't re-grab it while it lingers (the x5 bug).
							GrabAllGrabbedActors.Reset();
							AActor* SeedFirst = GetClosestMoonvillePickupOwner();
							if (SeedFirst)
							{
								GrabAllGrabbedActors.Add(SeedFirst);
							}
							GrabAllTicksRemaining = 20;
						}

						UFunction* InteractFunc = MoonvilleInteractionComp->FindFunction(FName("Interact"));
						if (InteractFunc)
						{
							UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Calling Moonville Interact()"));
							MoonvilleInteractionComp->ProcessEvent(InteractFunc, nullptr);
							return;
						}
						else
						{
							UE_LOG(LogTemp, Error, TEXT("[ZP-BUG] Moonville Interact() function NOT FOUND!"));
						}
						}
					}
					else if (HitActor->GetClass()->ImplementsInterface(UZP_Interactable::StaticClass()))
					{
						// Line trace hit an IZP_Interactable (ladder, door, etc.) — interact directly
						UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Line trace hit IZP_Interactable: %s — calling OnInteract"), *HitActorName);
						IZP_Interactable::Execute_OnInteract(HitActor, this);
						return;
					}
					else if (AZP_InteractDoor* DoorTrigger = AZP_InteractDoor::FindDoorForActor(HitActor))
					{
						// Trace hit a door mesh — route to its InteractDoor trigger
						UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Line trace hit door mesh %s — routing to trigger %s"), *HitActorName, *DoorTrigger->GetName());
						IZP_Interactable::Execute_OnInteract(DoorTrigger, this);
						return;
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Hit actor class '%s' does NOT match any filter — falling through"), *HitActor->GetClass()->GetName());
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Trace hit but GetActor() returned null"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Line trace missed — no hit within 300 UU"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] MoonvilleInteractionComp is NULL — skipping line trace"));
	}

	// Check our IZP_Interactable system (save points, doors, ladders, etc.)
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Layer 2 check: CurrentInteractable=%s, ImplementsInterface=%d"),
		CurrentInteractable.IsValid() ? *CurrentInteractable->GetName() : TEXT("NULL"),
		CurrentInteractable.IsValid() ? CurrentInteractable->GetClass()->ImplementsInterface(UZP_Interactable::StaticClass()) : 0);
	if (CurrentInteractable.IsValid() && CurrentInteractable->GetClass()->ImplementsInterface(UZP_Interactable::StaticClass()))
	{
		// For doors: require line-of-sight (player must be looking toward the door).
		// Prevents door overlap from consuming E when player is looking at nearby lockers/containers.
		bool bShouldInteract = true;
		if (AZP_InteractDoor* LOSDoor = Cast<AZP_InteractDoor>(CurrentInteractable.Get()))
		{
			APlayerController* LOSCheck_PC = Cast<APlayerController>(GetController());
			if (LOSCheck_PC)
			{
				FVector LOSCamLoc;
				FRotator LOSCamRot;
				LOSCheck_PC->GetPlayerViewPoint(LOSCamLoc, LOSCamRot);

				// Aim the cone at the DOOR PANEL, not the trigger actor — the
				// trigger pivot often sits beside the doorway, failing the cone
				// while the player looks straight at the door. The failed check
				// fell through to Moonville's fallback, which grabbed whatever
				// locker was closest ("doors blocked by other interactions",
				// session 63).
				FVector DoorTargetLoc = CurrentInteractable->GetActorLocation();
				if (LOSDoor->DoorActor)
				{
					DoorTargetLoc = LOSDoor->DoorActor->GetActorLocation();
				}
				else if (LOSDoor->DoorMesh && LOSDoor->DoorMesh->GetStaticMesh())
				{
					DoorTargetLoc = LOSDoor->DoorMesh->GetComponentLocation();
				}

				FVector ToDoor = (DoorTargetLoc - LOSCamLoc).GetSafeNormal();
				float Dot = FVector::DotProduct(LOSCamRot.Vector(), ToDoor);
				bShouldInteract = (Dot > 0.5f); // ~60° cone
				UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Layer 2 door LOS check: Dot=%.2f (target %s), Pass=%d"),
					Dot,
					LOSDoor->DoorActor ? *LOSDoor->DoorActor->GetName() : TEXT("self DoorMesh"),
					bShouldInteract);
			}
		}

		if (bShouldInteract)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Layer 2: IZP_Interactable — %s (class: %s)"), *CurrentInteractable->GetName(), *CurrentInteractable->GetClass()->GetName());
			IZP_Interactable::Execute_OnInteract(CurrentInteractable.Get(), this);
			return;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Layer 2: Door LOS check FAILED — skipping to let Moonville handle"));
		}
	}

	// Check for NPC Interaction Component (CC-based NPCs that don't inherit from AZP_NPC)
	if (CurrentInteractable.IsValid())
	{
		if (UZP_NPCInteractionComponent* NPCComp = CurrentInteractable->FindComponentByClass<UZP_NPCInteractionComponent>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] NPC interaction — %s"), *CurrentInteractable->GetName());
			NPCComp->HandleInteract(this);
			return;
		}
	}

	// Route through Moonville interaction if available (inventory pickups, etc.)
	if (MoonvilleInteractionComp)
	{
		// Lockers now open through THIS proximity path (the crosshair trace
		// skips their invisible spheres) — run the same container prep the
		// trace path does, aimed at Moonville's actual target.
		if (FObjectProperty* ClosestProp = CastField<FObjectProperty>(
			MoonvilleInteractionComp->GetClass()->FindPropertyByName(TEXT("ClosestInteractable"))))
		{
			UActorComponent* ClosestComp = Cast<UActorComponent>(ClosestProp->GetObjectPropertyValue(
				ClosestProp->ContainerPtrToValuePtr<void>(MoonvilleInteractionComp)));
			AActor* ClosestOwner = ClosestComp ? ClosestComp->GetOwner() : nullptr;

			// Same pickup gate as the crosshair path — Moonville's proximity Interact
			// would otherwise grab an owned weapon / capped ammo here. Safe on any
			// actor: ShouldBlockPickupInteraction returns false for non-pickups.
			if (ClosestOwner && ShouldBlockPickupInteraction(ClosestOwner))
			{
				UE_LOG(LogTemp, Log, TEXT("[Pickup] %s blocked at proximity (owned / ammo at cap)"), *ClosestOwner->GetName());
				return;
			}

			if (ClosestOwner)
			{
				bool bClosestContainer = false;
				bool bClosestBriefcase = false;
				for (UClass* C = ClosestOwner->GetClass(); C; C = C->GetSuperClass())
				{
					const FString CName = C->GetName();
					if (CName.Contains(TEXT("ItemContainer")) || CName.Contains(TEXT("Chest")) || CName.Contains(TEXT("LootLocker"))) bClosestContainer = true;
					if (CName.Contains(TEXT("Briefcase"))) { bClosestContainer = true; bClosestBriefcase = true; }
				}
				if (bClosestContainer)
				{
					if (!bClosestBriefcase)
					{
						FilterLockerAmmo(ClosestOwner);
					}
					else
					{
						ActiveBriefcaseActor = ClosestOwner;
					}
					bWaitingForContainerOpen = true;
					bContainerWasOpen = false;
					ContainerOpenWaitFrames = 0;
					ActiveContainerActor = ClosestOwner;
					UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Layer 3 container prep: %s"), *ClosestOwner->GetName());
				}
			}
		}

		// If the closest thing is a pickup, sweep the rest of the pile this press.
		if (AActor* FirstPickup = GetClosestMoonvillePickupOwner())
		{
			// Seed the sweep with the actor this press will grab so the first sweep tick
			// doesn't re-grab it while it lingers (the x5 bug).
			GrabAllGrabbedActors.Reset();
			GrabAllGrabbedActors.Add(FirstPickup);
			GrabAllTicksRemaining = 20;
		}

		UFunction* InteractFunc = MoonvilleInteractionComp->FindFunction(FName("Interact"));
		if (InteractFunc)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Layer 3: Moonville fallback Interact()"));
			MoonvilleInteractionComp->ProcessEvent(InteractFunc, nullptr);
			return;
		}
	}

	// Fallback to original interaction
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Layer 4: Fallback OnInteract"));
	AActor* Target = GameplayComp ? GameplayComp->CurrentInteractionTarget : nullptr;
	OnInteract(Target);
}

void AZP_GraceCharacter::SetCurrentInteractable(AActor* Interactable)
{
	CurrentInteractable = Interactable;
}

void AZP_GraceCharacter::ClearCurrentInteractable(AActor* Interactable)
{
	// Only clear if it's the same actor (prevents race conditions with overlapping volumes)
	if (CurrentInteractable.Get() == Interactable)
	{
		CurrentInteractable = nullptr;
	}
}

void AZP_GraceCharacter::Input_CrouchStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen || bOnLadder) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	// Toggle: stand if crouched, crouch if standing.
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AZP_GraceCharacter::Input_CrouchCompleted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	UnCrouch();
}

void AZP_GraceCharacter::Input_PeekStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen || bOnLadder) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	CancelSprintFromAction();
	if (GameplayComp) GameplayComp->bWantsPeek = true;
}

void AZP_GraceCharacter::Input_PeekCompleted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	if (GameplayComp) GameplayComp->bWantsPeek = false;
}

void AZP_GraceCharacter::Input_AimStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	if (!KinemationComp || !KinemationComp->ActiveWeapon) return;

	CancelSprintFromAction(); // aiming / blocking breaks a sprint

	// Pipe equipped → RMB blocks instead of ADS-ing. The press is remembered (bBlockWanted) and
	// the guard raises the INSTANT context allows — mid-swing presses buffer and engage the frame
	// the swing's contact passes (recovery block-cancel), no release-and-re-press needed.
	if (KinemationComp->CurrentWeaponType == EZP_WeaponType::Melee)
	{
		bBlockWanted = true;
		TryEngageBufferedBlock();
		return;
	}

	if (KinemationComp->bMeleeSwingActive ||
		KinemationComp->CurrentWeaponType == EZP_WeaponType::Throwable)
	{
		return;
	}
	if (GameplayComp) GameplayComp->bWantsAim = true;
	KinemationComp->SetAiming(true);
}

void AZP_GraceCharacter::Input_AimCompleted(const FInputActionValue& Value)
{
	bBlockWanted = false; // RMB physically released — kill any buffered block intent

	if (bInventoryMenuOpen || bMapOpen) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle

	if (bIsBlocking)
	{
		ReleaseBlock();
		return;
	}

	// Don't release ADS if melee swing is controlling it
	if (KinemationComp && KinemationComp->bMeleeSwingActive)
	{
		return;
	}
	if (GameplayComp) GameplayComp->bWantsAim = false;
	if (KinemationComp) KinemationComp->SetAiming(false);
}

void AZP_GraceCharacter::UpdateFootsteps(float DeltaTime)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move || !Move->IsMovingOnGround() || bOnLadder)
	{
		FootstepDistanceAccum = 0.f;
		return;
	}
	const float Speed = GetVelocity().Size2D();
	if (Speed < 60.f) // standing / drifting — no steps
	{
		FootstepDistanceAccum = 0.f;
		return;
	}

	const bool bCrouched = Move->IsCrouching();
	const bool bSprint = GameplayComp && GameplayComp->bIsSprinting;
	const float Stride = bSprint ? FootstepSprintStride
	                             : (bCrouched ? FootstepWalkStride * 0.85f : FootstepWalkStride);

	FootstepDistanceAccum += Speed * DeltaTime;
	if (FootstepDistanceAccum >= Stride)
	{
		FootstepDistanceAccum = 0.f;

		if (!FootstepData)
		{
			FootstepData = LoadObject<UZP_FootstepData>(nullptr, TEXT("/Game/Core/Data/DA_Footsteps.DA_Footsteps"));
		}

		// Resolve the FLOOR under our feet against the surface table: authored PhysMat SurfaceType
		// first (exact), else material-name keywords (works on unauthored pack floors).
		const FZP_FootstepSurface* Surface = nullptr;
		FHitResult Floor;
		FCollisionQueryParams Q(FName(TEXT("ZPFootstepSurface")), false);
		Q.AddIgnoredActor(this);
		Q.bReturnPhysicalMaterial = true;
		if (FootstepData && GetWorld()->LineTraceSingleByChannel(Floor, GetActorLocation(),
			GetActorLocation() - FVector(0.f, 0.f, 160.f), ECC_WorldStatic, Q))
		{
			EPhysicalSurface FloorSurface = SurfaceType_Default;
			if (const UPhysicalMaterial* PhysMat = Floor.PhysMaterial.Get())
			{
				FloorSurface = PhysMat->SurfaceType;
			}
			FString MatName;
			if (UPrimitiveComponent* FloorComp = Floor.GetComponent())
			{
				if (UMaterialInterface* FloorMat = FloorComp->GetMaterial(0))
				{
					MatName = FloorMat->GetName().ToLower();
				}
			}
			Surface = FootstepData->Resolve(FloorSurface, MatName);
		}

		// Sound pool priority: matched surface row -> table default set -> hard C++ fallback.
		const TArray<TObjectPtr<USoundBase>>* Set = &FootstepSounds;
		float SurfaceVolMul = 1.f, PitchMin = 0.92f, PitchMax = 1.08f;
		if (Surface)
		{
			Set = &Surface->Sounds;
			SurfaceVolMul = Surface->VolumeMul;
			PitchMin = Surface->PitchMin;
			PitchMax = Surface->PitchMax;
		}
		else if (FootstepData && FootstepData->DefaultSounds.Num() > 0)
		{
			Set = &FootstepData->DefaultSounds;
		}

		if (Set->Num() > 0)
		{
			if (USoundBase* Step = (*Set)[FMath::RandRange(0, Set->Num() - 1)])
			{
				// Own-body foley: 2D, quiet, slight pitch jitter so repeats don't machine-gun.
				// Sprint steps land heavier.
				const float Vol = FootstepVolume * SurfaceVolMul
					* (bCrouched ? 0.5f : 1.f) * (bSprint ? SprintFootstepVolumeMul : 1.f);
				UGameplayStatics::PlaySound2D(this, Step, Vol, FMath::FRandRange(PitchMin, PitchMax));
			}
		}
	}
}

void AZP_GraceCharacter::TryEngageBufferedBlock()
{
	if (!bBlockWanted || bIsBlocking) { return; }
	if (bInventoryMenuOpen || bMapOpen || bOnLadder || GrabPhase != EZP_GrabPhase::None) { return; }
	if (!KinemationComp || KinemationComp->CurrentWeaponType != EZP_WeaponType::Melee) { return; }
	// The strike + follow-through always play in full; once the swing passes
	// MeleeBlockCancelFraction only return-to-idle dead frames remain, and the held guard cancels
	// into them — no perceived clipping AND no perceived pause between swing and block.
	if (KinemationComp->bMeleeSwingActive && !KinemationComp->bMeleeSwingTailCancelable) { return; }
	// You can't raise a guard you can't pay for — needs one blocked hit's worth of stamina.
	if (GameplayComp && GameplayComp->GetStaminaFraction() < BlockMinStaminaFractionToStart) { return; }

	if (KinemationComp->bMeleeSwingActive)
	{
		KinemationComp->CancelMeleeSwingRecovery(); // tail only — StartBlockNow owns the hands now
	}
	StartBlockNow();
}

void AZP_GraceCharacter::InterruptBlockForSwing()
{
	if (!bIsBlocking) { return; }
	// Attack outranks block: drop the guard with NO BlockStop pose-out — the swing clip takes the
	// view mesh this same frame. bBlockWanted is untouched, so the buffered engage re-raises the
	// guard automatically the moment the swing animation ends.
	bIsBlocking = false;
	bBlockResolving = false;
	bBlockWalkActive = false;
	BlockImpactLockRemaining = 0.f;
	BlockStartLockRemaining = 0.f;
}

void AZP_GraceCharacter::StartBlockNow()
{
	CancelSprintFromAction(); // blocking breaks a sprint

	bIsBlocking = true;
	bBlockResolving = false;
	bBlockWalkActive = false;
	BlockImpactLockRemaining = 0.f;
	BlockStartLockRemaining = 0.f;
	BlockClearanceRemaining = BlockClearanceWindow; // transient camera nudge over the lean-in

	// Probe: dump camera state for 30 frames so we see when block shifts the view.
	CameraProbeTag = TEXT("BLOCK");
	CameraProbeFramesLeft = 180;
	LogCameraProbe(TEXT("BLOCK-ENGAGE"));

	// Play BlockStart (non-loop) first — the in-motion clip. UpdateBlockAnimation
	// is gated by BlockStartLockRemaining so it can't yank Start out and snap
	// to BlockLoop early. When the lock decays, UpdateBlockAnimation takes
	// over and swaps to BlockLoop / BlockWalk based on motion.
	UAnimSequenceBase* StartAnim = BlockStartAnim.LoadSynchronous();
	if (MeleeViewMesh && StartAnim)
	{
		if (UAnimSingleNodeInstance* SNI = MeleeViewMesh->GetSingleNodeInstance())
		{
			SNI->SetAnimationAsset(StartAnim, /*bLoop*/ false, 1.0f);
			SNI->SetPlaying(true);
			BlockStartLockRemaining = StartAnim->GetPlayLength();
		}
	}
	else if (MeleeViewMesh)
	{
		// Fallback if Start asset is missing — go straight to loop.
		if (UAnimSequenceBase* LoopAnim = BlockLoopAnim.LoadSynchronous())
		{
			if (UAnimSingleNodeInstance* SNI = MeleeViewMesh->GetSingleNodeInstance())
			{
				SNI->SetAnimationAsset(LoopAnim, /*bLoop*/ true, 1.0f);
				SNI->SetPlaying(true);
			}
		}
	}
}

void AZP_GraceCharacter::ReleaseBlock()
{
	if (!bIsBlocking) { return; }
	bIsBlocking = false;
	bBlockResolving = true; // keep block offsets alive until BlockStop finishes
	bBlockWalkActive = false;
	BlockImpactLockRemaining = 0.f;
	BlockStartLockRemaining = 0.f;
	// Mirror the block-START forward nudge so the BlockStop lean-OUT doesn't
	// swing the body through the lens. Without this the unblock pose-out clips
	// the camera (dev report: "body swings into my camera, looks messy").
	BlockClearanceRemaining = BlockClearanceWindow;

	// Play BlockStop (non-loop) first — the pose-out motion. Schedule a
	// timer to settle into MeleeIdle when Stop finishes so the pose doesn't
	// snap back to neutral. Same pattern as the dodge return at line ~2654.
	UAnimSequenceBase* StopAnim = BlockStopAnim.LoadSynchronous();
	if (MeleeViewMesh && StopAnim)
	{
		if (UAnimSingleNodeInstance* SNI = MeleeViewMesh->GetSingleNodeInstance())
		{
			SNI->SetAnimationAsset(StopAnim, /*bLoop*/ false, 1.0f);
			SNI->SetPlaying(true);
		}

		const float StopLen = StopAnim->GetPlayLength();
		FTimerHandle StopReturnHandle;
		GetWorldTimerManager().SetTimer(StopReturnHandle, [this]()
		{
			bBlockResolving = false; // BlockStop finished — offsets may ease out now
			if (bIsBlocking) return; // re-raised before stop finished
			if (!KinemationComp
				|| KinemationComp->CurrentWeaponType != EZP_WeaponType::Melee)
			{
				return;
			}
			UAnimSequenceBase* IdleAnim = MeleeIdleHoldAnim.LoadSynchronous();
			if (!IdleAnim || !MeleeViewMesh) return;
			if (UAnimSingleNodeInstance* IdleSNI = MeleeViewMesh->GetSingleNodeInstance())
			{
				IdleSNI->SetAnimationAsset(IdleAnim, /*bLoop*/ true, 1.0f);
				IdleSNI->SetPlaying(true);
			}
		}, StopLen, false);
	}
	else if (MeleeViewMesh && MeleeIdleHoldAnim.IsValid())
	{
		bBlockResolving = false; // no stop clip — nothing to cover
		// Fallback if Stop asset is missing — go straight to idle.
		if (UAnimSequenceBase* IdleAnim = MeleeIdleHoldAnim.LoadSynchronous())
		{
			if (UAnimSingleNodeInstance* SNI = MeleeViewMesh->GetSingleNodeInstance())
			{
				SNI->SetAnimationAsset(IdleAnim, /*bLoop*/ true, 1.0f);
				SNI->SetPlaying(true);
			}
		}
	}
}

void AZP_GraceCharacter::Input_FireStarted(const FInputActionValue& Value)
{
	// GRABBED: LMB is the struggle input — intercepted BEFORE the menu/weapon early-outs
	// (the grab stows the weapon, so the ActiveWeapon check below would eat every mash press).
	if (GrabPhase != EZP_GrabPhase::None)
	{
		bGrabEscapeHeld = true;
		GrabMashPressed();
		return;
	}
	if (bInventoryMenuOpen || bMapOpen || !KinemationComp || !KinemationComp->ActiveWeapon) return;
	CancelSprintFromAction(); // firing / melee swinging breaks a sprint

	// Attack > Block > Idle: swinging while the guard is up drops the guard for the swing;
	// the held RMB re-raises it the instant the swing animation ends (buffered engage).
	if (bIsBlocking && KinemationComp->CurrentWeaponType == EZP_WeaponType::Melee)
	{
		InterruptBlockForSwing();
	}

	KinemationComp->FirePressed();
}

void AZP_GraceCharacter::Input_FireCompleted(const FInputActionValue& Value)
{
	if (GrabPhase != EZP_GrabPhase::None) { bGrabEscapeHeld = false; return; }
	if (bInventoryMenuOpen || bMapOpen) { return; }
	if (KinemationComp) KinemationComp->FireReleased();
}

void AZP_GraceCharacter::Input_ReloadStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen || bOnLadder) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	if (DodgeLockRemaining > 0.f) return; // no reload mid-dodge
	CancelSprintFromAction(); // reloading breaks a sprint
	if (KinemationComp) KinemationComp->Reload();
}

bool AZP_GraceCharacter::IsModalMenuOpen() const
{
	if (bSaveMenuOpen || bPickupMenuActive) return true;
	if (const AZP_PlayerController* ZPC = Cast<AZP_PlayerController>(GetController()))
	{
		return ZPC->IsPauseMenuOpen();
	}
	return false;
}

void AZP_GraceCharacter::Input_InventoryMenu(const FInputActionValue& Value)
{
	// Don't let the inventory/notes/map tabs open UNDER the save point or pause menu (Moonville's
	// IMC_InventoryCharacter stays active under those menus, so this action would otherwise still fire).
	if (IsModalMenuOpen()) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — no menus under the struggle

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController());
	if (!PC || !MoonvilleInventoryComp) return;

	CancelSprintFromAction(); // opening the inventory breaks a sprint

	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-INPUT] Tab pressed. bInventoryMenuOpen=%d, TabWidget IsMenuOpen=%d"),
		bInventoryMenuOpen,
		PC->InventoryTabWidget ? PC->InventoryTabWidget->IsMenuOpen() : -1);

	// Always toggle Moonville — it handles its own open/close lifecycle
	UFunction* ToggleFunc = MoonvilleInventoryComp->FindFunction(FName("ToggleInventoryMenu"));
	if (ToggleFunc)
	{
		struct { APlayerController* PlayerController; } Params;
		Params.PlayerController = PC;
		MoonvilleInventoryComp->ProcessEvent(ToggleFunc, &Params);
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-INPUT] Called Moonville ToggleInventoryMenu"));
	}

	// Tell tab controller Moonville was toggled — NativeTick detects open/close reactively
	if (PC->InventoryTabWidget)
	{
		PC->InventoryTabWidget->NotifyMoonvilleToggled(EZP_InventoryTab::Inventory);
	}

	// Rough state toggle — NativeTick will correct if wrong
	bInventoryMenuOpen = !bInventoryMenuOpen;
	if (!bInventoryMenuOpen) bMapOpen = false;

	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-INPUT] After toggle: bInventoryMenuOpen=%d"), bInventoryMenuOpen);
}

void AZP_GraceCharacter::Input_Map(const FInputActionValue& Value)
{
	// Don't let the map tab open UNDER the save point or pause menu.
	if (IsModalMenuOpen()) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — no menus under the struggle

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController());
	if (!PC) return;

	CancelSprintFromAction(); // opening the map breaks a sprint

	// Use tab widget's reactive state — it tracks Moonville's actual viewport presence
	const bool bMenuActuallyOpen = PC->InventoryTabWidget && PC->InventoryTabWidget->IsMenuOpen();

	if (bMenuActuallyOpen)
	{
		if (PC->InventoryTabWidget->GetCurrentTab() == EZP_InventoryTab::Map)
		{
			// Already on map tab — close everything
			if (MoonvilleInventoryComp)
			{
				UFunction* ToggleFunc = MoonvilleInventoryComp->FindFunction(FName("ToggleInventoryMenu"));
				if (ToggleFunc)
				{
					struct { APlayerController* PlayerController; } Params;
					Params.PlayerController = PC;
					MoonvilleInventoryComp->ProcessEvent(ToggleFunc, &Params);
				}
			}
			// NativeTick will detect close and sync state
			bInventoryMenuOpen = false;
			bMapOpen = false;
		}
		else
		{
			// On another tab — just switch to map
			PC->InventoryTabWidget->SwitchToTab(EZP_InventoryTab::Map);
			bMapOpen = true;
		}
	}
	else
	{
		// Menu closed — open Moonville then notify tab controller
		if (MoonvilleInventoryComp)
		{
			UFunction* ToggleFunc = MoonvilleInventoryComp->FindFunction(FName("ToggleInventoryMenu"));
			if (ToggleFunc)
			{
				struct { APlayerController* PlayerController; } Params;
				Params.PlayerController = PC;
				MoonvilleInventoryComp->ProcessEvent(ToggleFunc, &Params);
			}
		}

		if (PC->InventoryTabWidget)
		{
			PC->InventoryTabWidget->NotifyMoonvilleToggled(EZP_InventoryTab::Map);
		}

		bInventoryMenuOpen = true;
		bMapOpen = true;
	}
}

void AZP_GraceCharacter::Input_TabCycleLeft(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-CYCLE] Q pressed (TabCycleLeft)"));
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController());
	if (!PC || !PC->InventoryTabWidget || !PC->InventoryTabWidget->IsMenuOpen()) return;
	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-CYCLE] Cycling LEFT"));
	PC->InventoryTabWidget->CycleTab(-1);
}

void AZP_GraceCharacter::Input_TabCycleRight(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-CYCLE] E pressed (TabCycleRight)"));
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController());
	if (!PC || !PC->InventoryTabWidget || !PC->InventoryTabWidget->IsMenuOpen()) return;
	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-CYCLE] Cycling RIGHT"));
	PC->InventoryTabWidget->CycleTab(1);
}

void AZP_GraceCharacter::OnWeaponChangedHandler(AActor* NewWeapon)
{
	// Sync for BP interface compat (GetPrimaryWeapon, GetMainWeapon)
	ActiveWeapon = NewWeapon;

	// New weapon up → the reserve count must reflect the inventory ammo for THIS gun.
	SyncReserveFromInventory();

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: Weapon changed → %s"),
		NewWeapon ? *NewWeapon->GetName() : TEXT("NONE"));
}

void AZP_GraceCharacter::OnThrowableConsumedHandler()
{
	if (!MoonvilleInventoryComp) return;

	// Find the grenade's item DA by scanning ItemSlots for matching weapon class
	UObject* FoundItemDA = nullptr;
	TSubclassOf<AActor> ConsumedClass = KinemationComp ? KinemationComp->WeaponClass : nullptr;
	if (ConsumedClass)
	{
		FProperty* SlotsProp = MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ItemSlots"));
		FArrayProperty* ArrayProp = SlotsProp ? CastField<FArrayProperty>(SlotsProp) : nullptr;
		if (ArrayProp)
		{
			FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
			FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
			if (StructInner)
			{
				// Find the Item_ property in the struct
				FProperty* ItemProp = nullptr;
				for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
				{
					if (It->GetName().Contains(TEXT("Item_")))
					{
						ItemProp = *It;
						break;
					}
				}
				FObjectProperty* ObjProp = ItemProp ? CastField<FObjectProperty>(ItemProp) : nullptr;

				if (ObjProp)
				{
					for (int32 i = 0; i < ArrayHelper.Num(); ++i)
					{
						void* ElementData = ArrayHelper.GetRawPtr(i);
						UObject* ItemDA = ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<void>(ElementData));
						if (ItemDA)
						{
							TSubclassOf<AActor> ItemWeaponClass = GetWeaponClassFromItem(ItemDA);
							if (ItemWeaponClass == ConsumedClass)
							{
								FoundItemDA = ItemDA;
								break;
							}
						}
					}
				}
			}
		}
	}

	// Remove from Moonville inventory using the found DA
	if (FoundItemDA)
	{
		UFunction* RemoveFunc = MoonvilleInventoryComp->FindFunction(FName("RemoveItemByDataAsset"));
		if (RemoveFunc)
		{
			struct { UObject* ItemDataAsset; int32 AmountToRemove; } Params;
			Params.ItemDataAsset = FoundItemDA;
			Params.AmountToRemove = 1;
			MoonvilleInventoryComp->ProcessEvent(RemoveFunc, &Params);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] Throwable consumed — removed %s from inventory"),
				*FoundItemDA->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] Throwable consumed — could not find item DA in ItemSlots"));
	}

	// Stack-aware (session 63): if more grenades remain in inventory, rearm
	// the held one so the player stays on grenades. ThrowProjectile only
	// auto-switches back when CurrentAmmo stays at 0 (supply exhausted).
	if (ConsumedClass && KinemationComp && CountWeaponClassInInventory(ConsumedClass) > 0)
	{
		KinemationComp->RestockThrowable();
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Throwable consumed — supply remains, rearmed"));
	}

	LastThrowableItemDA = nullptr;
	LastThrowableSlotIndex = -1;
}

// The ONLY reliable way to find a container's item storage: the component
// that actually has an ItemSlots property. Name matching grabbed
// BP_InventoryActorComponent (the interaction comp, NO items) first —
// every locker read as "empty" and got permanently disabled on close
// (session 63, log-proven: "contents: no ItemSlots" on item-bearing lockers).
static UActorComponent* FindItemSlotsComponent(AActor* Actor)
{
	if (!Actor) return nullptr;
	for (UActorComponent* Comp : Actor->GetComponents())
	{
		if (Comp && Comp->GetClass()->FindPropertyByName(FName("ItemSlots")))
		{
			return Comp;
		}
	}
	return nullptr;
}

FString AZP_GraceCharacter::DescribeLockerContents(AActor* LockerActor)
{
	if (!LockerActor) return TEXT("null");

	UActorComponent* LockerInvComp = FindItemSlotsComponent(LockerActor);
	if (!LockerInvComp) return TEXT("no inv comp");

	FProperty* SlotsProp = LockerInvComp->GetClass()->FindPropertyByName(FName("ItemSlots"));
	FArrayProperty* ArrayProp = SlotsProp ? CastField<FArrayProperty>(SlotsProp) : nullptr;
	FStructProperty* StructInner = ArrayProp ? CastField<FStructProperty>(ArrayProp->Inner) : nullptr;
	if (!StructInner) return TEXT("no ItemSlots");

	FObjectProperty* ItemObjProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("Item_")))
		{
			ItemObjProp = CastField<FObjectProperty>(*It);
			break;
		}
	}
	if (!ItemObjProp) return TEXT("no Item field");

	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(LockerInvComp));
	TArray<FString> Names;
	for (int32 i = 0; i < ArrayHelper.Num(); ++i)
	{
		UObject* SlotItem = ItemObjProp->GetObjectPropertyValue(
			ItemObjProp->ContainerPtrToValuePtr<void>(ArrayHelper.GetRawPtr(i)));
		if (SlotItem)
		{
			Names.Add(SlotItem->GetName());
		}
	}
	return FString::Printf(TEXT("%d items [%s]"), Names.Num(), *FString::Join(Names, TEXT(", ")));
}

FString AZP_GraceCharacter::GetMoonvilleInteractionStateString() const
{
	if (!MoonvilleInteractionComp)
	{
		return TEXT("MoonvilleInteractionComp=NULL");
	}

	auto GetBool = [this](const TCHAR* Name) -> int32
	{
		FBoolProperty* P = CastField<FBoolProperty>(
			MoonvilleInteractionComp->GetClass()->FindPropertyByName(Name));
		if (!P) return -1;
		return P->GetPropertyValue(P->ContainerPtrToValuePtr<void>(MoonvilleInteractionComp)) ? 1 : 0;
	};

	FString Closest = TEXT("None");
	if (FObjectProperty* OP = CastField<FObjectProperty>(
		MoonvilleInteractionComp->GetClass()->FindPropertyByName(TEXT("ClosestInteractable"))))
	{
		UObject* O = OP->GetObjectPropertyValue(OP->ContainerPtrToValuePtr<void>(MoonvilleInteractionComp));
		if (UActorComponent* AC = Cast<UActorComponent>(O))
		{
			Closest = AC->GetOwner() ? AC->GetOwner()->GetName() : AC->GetName();
		}
		else if (O)
		{
			Closest = O->GetName();
		}
	}

	int32 NumInteractables = -1;
	FProperty* IntsProp = MoonvilleInteractionComp->GetClass()->FindPropertyByName(TEXT("Interactables"));
	if (FArrayProperty* AP = IntsProp ? CastField<FArrayProperty>(IntsProp) : nullptr)
	{
		FScriptArrayHelper H(AP, IntsProp->ContainerPtrToValuePtr<void>(MoonvilleInteractionComp));
		NumInteractables = H.Num();
	}

	return FString::Printf(TEXT("MenuOpen=%d CooledDown=%d FirstPickup=%d Closest=%s NearbyInteractables=%d"),
		GetBool(TEXT("bInventoryMenuOpen")), GetBool(TEXT("bIsInteractionCooledDown")),
		GetBool(TEXT("bFirstTimePickupMenuOpen")), *Closest, NumInteractables);
}

bool AZP_GraceCharacter::TransferAllFromOpenContainer()
{
	if (!MoonvilleInventoryComp) return false;

	// While a container menu is up, Moonville points the player's inventory
	// component at the container's via OtherInventory.
	// GATE 1 — Moonville's CLIENT-side menu flag must be up. The container's
	// bPlayerIsUsingActor is set by a server event that sticks in standalone
	// (TICKET-016: Moonville server events fail silently) — acting on it alone
	// ate every E press once any locker had been opened (session 63
	// regression: doors and other lockers went dead).
	if (!MoonvilleInteractionComp)
	{
		return false;
	}
	FBoolProperty* MenuProp = CastField<FBoolProperty>(
		MoonvilleInteractionComp->GetClass()->FindPropertyByName(FName("bInventoryMenuOpen")));
	if (!MenuProp || !MenuProp->GetPropertyValue(
		MenuProp->ContainerPtrToValuePtr<void>(MoonvilleInteractionComp)))
	{
		return false;
	}

	// Find the container flagged in-use (the link runs container→player;
	// the player-side OtherInventory is never set — proven session 63).
	AActor* ContainerActor = nullptr;
	FBoolProperty* ContainerUsingProp = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		FBoolProperty* UsingProp = CastField<FBoolProperty>(
			It->GetClass()->FindPropertyByName(FName("bPlayerIsUsingActor")));
		if (UsingProp && UsingProp->GetPropertyValue(
			UsingProp->ContainerPtrToValuePtr<void>(*It)))
		{
			ContainerActor = *It;
			ContainerUsingProp = UsingProp;
			break;
		}
	}
	if (!ContainerActor)
	{
		return false;
	}

	// GATE 2 — the in-use container must be at arm's reach. A stale flag on a
	// locker across the map must never hijack the Interact key.
	if (FVector::DistSquared(ContainerActor->GetActorLocation(), GetActorLocation())
		> FMath::Square(500.f))
	{
		UE_LOG(LogTemp, Log, TEXT("[TransferAll] ignoring stale in-use flag on distant %s"),
			*ContainerActor->GetName());
		return false;
	}

	UActorComponent* ContainerComp = FindItemSlotsComponent(ContainerActor);
	if (!ContainerComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TransferAll] %s is in use but has no ItemSlots component"),
			*ContainerActor->GetName());
		return false;
	}

	FProperty* SlotsProp = ContainerComp->GetClass()->FindPropertyByName(FName("ItemSlots"));
	FArrayProperty* ArrayProp = SlotsProp ? CastField<FArrayProperty>(SlotsProp) : nullptr;
	FStructProperty* StructInner = ArrayProp ? CastField<FStructProperty>(ArrayProp->Inner) : nullptr;
	if (!StructInner)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TransferAll] ItemSlots array not found on %s"),
			*ContainerComp->GetClass()->GetName());
		return false;
	}

	UFunction* TransferFunc = ContainerComp->FindFunction(FName("TransferItem"));
	if (!TransferFunc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TransferAll] TransferItem function not found on %s"),
			*ContainerComp->GetClass()->GetName());
		return false;
	}

	// Snapshot the slots first — TransferItem mutates the array as it moves items
	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(ContainerComp));
	const int32 Num = ArrayHelper.Num();
	if (Num == 0)
	{
		// Empty container: E just closes the menu (and must NOT fall through
		// to world interaction underneath the open menu)
		if (UFunction* ToggleFunc = MoonvilleInventoryComp->FindFunction(FName("ToggleInventoryMenu")))
		{
			struct { APlayerController* PlayerController; } CloseParams;
			CloseParams.PlayerController = Cast<APlayerController>(GetController());
			MoonvilleInventoryComp->ProcessEvent(ToggleFunc, &CloseParams);
		}
		if (ContainerUsingProp)
		{
			ContainerUsingProp->SetPropertyValue(
				ContainerUsingProp->ContainerPtrToValuePtr<void>(ContainerActor), false);
		}
		UE_LOG(LogTemp, Log, TEXT("[TransferAll] %s empty — closed menu"),
			ContainerActor ? *ContainerActor->GetName() : TEXT("?"));
		return true;
	}

	TArray<TArray<uint8>> SlotCopies;
	SlotCopies.SetNum(Num);
	for (int32 i = 0; i < Num; ++i)
	{
		SlotCopies[i].SetNumZeroed(StructInner->Struct->GetStructureSize());
		StructInner->Struct->InitializeStruct(SlotCopies[i].GetData());
		StructInner->Struct->CopyScriptStruct(SlotCopies[i].GetData(), ArrayHelper.GetRawPtr(i));
	}

	// TransferItem(ItemSlot, TargetInventory = player's inventory) per slot
	int32 Transferred = 0;
	for (TArray<uint8>& SlotData : SlotCopies)
	{
		TArray<uint8> Parms;
		Parms.SetNumZeroed(TransferFunc->ParmsSize);
		for (TFieldIterator<FProperty> It(TransferFunc); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			It->InitializeValue_InContainer(Parms.GetData());
			if (FStructProperty* SP = CastField<FStructProperty>(*It))
			{
				SP->Struct->CopyScriptStruct(It->ContainerPtrToValuePtr<void>(Parms.GetData()), SlotData.GetData());
			}
			else if (FObjectProperty* OP = CastField<FObjectProperty>(*It))
			{
				OP->SetObjectPropertyValue(It->ContainerPtrToValuePtr<void>(Parms.GetData()), MoonvilleInventoryComp);
			}
		}
		ContainerComp->ProcessEvent(TransferFunc, Parms.GetData());
		++Transferred;
		for (TFieldIterator<FProperty> It(TransferFunc); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			It->DestroyValue_InContainer(Parms.GetData());
		}
	}
	for (TArray<uint8>& SlotData : SlotCopies)
	{
		StructInner->Struct->DestroyStruct(SlotData.GetData());
	}

	// Close the menu after looting (dev design: E opens, E loots-and-closes).
	// Same client-side ToggleInventoryMenu call the Tab key uses — the only
	// close path proven reliable in standalone.
	if (UFunction* ToggleFunc = MoonvilleInventoryComp->FindFunction(FName("ToggleInventoryMenu")))
	{
		struct { APlayerController* PlayerController; } CloseParams;
		CloseParams.PlayerController = Cast<APlayerController>(GetController());
		MoonvilleInventoryComp->ProcessEvent(ToggleFunc, &CloseParams);
	}

	// Clear the container's in-use flag ourselves — the Moonville server
	// event that should clear it never runs in standalone (TICKET-016).
	if (ContainerUsingProp)
	{
		ContainerUsingProp->SetPropertyValue(
			ContainerUsingProp->ContainerPtrToValuePtr<void>(ContainerActor), false);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Transfer All — moved %d slots from %s, menu closed"),
		Transferred, ContainerActor ? *ContainerActor->GetName() : TEXT("container"));
	return true;
}

int32 AZP_GraceCharacter::CountWeaponClassInInventory(TSubclassOf<AActor> InWeaponClass)
{
	if (!MoonvilleInventoryComp || !InWeaponClass) return 0;

	FProperty* SlotsProp = MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ItemSlots"));
	FArrayProperty* ArrayProp = SlotsProp ? CastField<FArrayProperty>(SlotsProp) : nullptr;
	if (!ArrayProp) return 0;

	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return 0;

	// Locate the item DA and stack amount fields in Moonville's FItemSlot
	FProperty* ItemProp = nullptr;
	FProperty* AmountProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (!ItemProp && It->GetName().Contains(TEXT("Item_"))) ItemProp = *It;
		if (!AmountProp && It->GetName().Contains(TEXT("Amount"))) AmountProp = *It;
	}
	FObjectProperty* ObjProp = ItemProp ? CastField<FObjectProperty>(ItemProp) : nullptr;
	FIntProperty* IntProp = AmountProp ? CastField<FIntProperty>(AmountProp) : nullptr;
	if (!ObjProp) return 0;

	int32 Total = 0;
	for (int32 i = 0; i < ArrayHelper.Num(); ++i)
	{
		void* ElementData = ArrayHelper.GetRawPtr(i);
		UObject* ItemDA = ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<void>(ElementData));
		if (ItemDA && GetWeaponClassFromItem(ItemDA) == InWeaponClass)
		{
			// Sum stack amounts; if the Amount field wasn't found, count slots
			Total += IntProp ? IntProp->GetPropertyValue(IntProp->ContainerPtrToValuePtr<void>(ElementData)) : 1;
		}
	}
	return Total;
}

// ---------------------------------------------------------------------------
// Notes Bridge — Moonville OnInventoryUpdate → NoteComponent
// ---------------------------------------------------------------------------

void AZP_GraceCharacter::BindInventoryUpdateDelegate()
{
	if (!MoonvilleInventoryComp) return;

	FMulticastDelegateProperty* DispatcherProp = CastField<FMulticastDelegateProperty>(
		MoonvilleInventoryComp->GetClass()->FindPropertyByName(TEXT("OnInventoryUpdate")));

	if (DispatcherProp)
	{
		FScriptDelegate NewDelegate;
		NewDelegate.BindUFunction(this, FName("HandleInventoryUpdate"));

		// Get the delegate instance from the component and add our binding
		void* DelegateAddr = DispatcherProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp);
		DispatcherProp->AddDelegate(NewDelegate, MoonvilleInventoryComp);

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Bound to Moonville OnInventoryUpdate dispatcher"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] OnInventoryUpdate dispatcher NOT FOUND on MoonvilleInventoryComp"));
	}
}

void AZP_GraceCharacter::ScanInventoryForNotes()
{
	HandleInventoryUpdate();
}

// Ammo lives in the Moonville inventory grid (the survival challenge). The HUD
// "reserve" number is just a MIRROR of how much matching ammo the player carries
// for the equipped weapon — these helpers read/spend that, never moving ammo out
// of the grid except when a reload actually consumes rounds.

int32 AZP_GraceCharacter::GetInventoryAmmoCount(EZP_WeaponIcon Icon)
{
	if (!MoonvilleInventoryComp || Icon == EZP_WeaponIcon::None) return 0;

	FProperty* SlotsProp = MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ItemSlots"));
	FArrayProperty* ArrayProp = SlotsProp ? CastField<FArrayProperty>(SlotsProp) : nullptr;
	if (!ArrayProp) return 0;
	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return 0;

	FProperty* ItemProp = nullptr;
	FProperty* AmountProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (!ItemProp && It->GetName().Contains(TEXT("Item_")))   ItemProp = *It;
		if (!AmountProp && It->GetName().Contains(TEXT("Amount"))) AmountProp = *It;
	}
	FObjectProperty* ObjProp = ItemProp ? CastField<FObjectProperty>(ItemProp) : nullptr;
	FIntProperty* IntProp = AmountProp ? CastField<FIntProperty>(AmountProp) : nullptr;
	if (!ObjProp) return 0;

	int32 Total = 0;
	for (int32 i = 0; i < ArrayHelper.Num(); ++i)
	{
		void* Elem = ArrayHelper.GetRawPtr(i);
		UObject* ItemDA = ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<void>(Elem));
		if (!ItemDA) continue;
		const FString Name = ItemDA->GetName();
		if (!Name.Contains(TEXT("Ammo"))) continue;
		if (UZP_KinemationComponent::AmmoNameToIcon(Name) != Icon) continue;
		Total += IntProp ? IntProp->GetPropertyValue(IntProp->ContainerPtrToValuePtr<void>(Elem)) : 1;
	}
	return Total;
}

void AZP_GraceCharacter::EnforceAmmoCaps()
{
	if (!MoonvilleInventoryComp) return;

	// A pickup adds a WHOLE stack, so picking up a 5-stack at 20/24 lands you at 25.
	// Trim any ammo type back down to its cap (overflow is discarded — the cap is a
	// hard ceiling). The block gate handles the already-at-cap case; this handles the
	// would-exceed case.
	static const EZP_WeaponIcon AmmoIcons[] = {
		EZP_WeaponIcon::Pistol, EZP_WeaponIcon::Shotgun, EZP_WeaponIcon::Rifle };

	for (EZP_WeaponIcon Icon : AmmoIcons)
	{
		const int32 Cap  = UZP_KinemationComponent::GetReserveCapForIcon(Icon);
		const int32 Have = GetInventoryAmmoCount(Icon);
		if (Cap > 0 && Have > Cap)
		{
			UE_LOG(LogTemp, Log, TEXT("[Ammo] Trimming icon %d %d -> cap %d"), (int32)Icon, Have, Cap);
			RemoveInventoryAmmo(Icon, Have - Cap);
		}
	}
}

void AZP_GraceCharacter::SyncReserveFromInventory()
{
	if (!KinemationComp) return;
	KinemationComp->ReserveAmmo = GetInventoryAmmoCount(KinemationComp->CurrentWeaponIcon);
	KinemationComp->OnAmmoChanged.Broadcast(KinemationComp->CurrentAmmo, KinemationComp->ReserveAmmo);
}

void AZP_GraceCharacter::RemoveInventoryAmmo(EZP_WeaponIcon Icon, int32 Rounds)
{
	if (!MoonvilleInventoryComp || Rounds <= 0 || Icon == EZP_WeaponIcon::None) return;
	UFunction* RemoveFunc = MoonvilleInventoryComp->FindFunction(FName("RemoveItemByDataAsset"));
	if (!RemoveFunc) return;

	FProperty* SlotsProp = MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ItemSlots"));
	FArrayProperty* ArrayProp = SlotsProp ? CastField<FArrayProperty>(SlotsProp) : nullptr;
	if (!ArrayProp) return;
	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return;

	FProperty* ItemProp = nullptr;
	FProperty* AmountProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (!ItemProp && It->GetName().Contains(TEXT("Item_")))   ItemProp = *It;
		if (!AmountProp && It->GetName().Contains(TEXT("Amount"))) AmountProp = *It;
	}
	FObjectProperty* ObjProp = ItemProp ? CastField<FObjectProperty>(ItemProp) : nullptr;
	FIntProperty* IntProp = AmountProp ? CastField<FIntProperty>(AmountProp) : nullptr;
	if (!ObjProp) return;

	// Aggregate matching ammo by DA, then remove the rounds across stacks.
	TMap<UObject*, int32> ByDA;
	for (int32 i = 0; i < ArrayHelper.Num(); ++i)
	{
		void* Elem = ArrayHelper.GetRawPtr(i);
		UObject* ItemDA = ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<void>(Elem));
		if (!ItemDA) continue;
		const FString Name = ItemDA->GetName();
		if (!Name.Contains(TEXT("Ammo"))) continue;
		if (UZP_KinemationComponent::AmmoNameToIcon(Name) != Icon) continue;
		ByDA.FindOrAdd(ItemDA) += IntProp ? IntProp->GetPropertyValue(IntProp->ContainerPtrToValuePtr<void>(Elem)) : 1;
	}

	int32 Remaining = Rounds;
	for (const TPair<UObject*, int32>& P : ByDA)
	{
		if (Remaining <= 0) break;
		const int32 Take = FMath::Min(Remaining, P.Value);
		struct { UObject* ItemDataAsset; int32 AmountToRemove; } Params;
		Params.ItemDataAsset = P.Key;
		Params.AmountToRemove = Take;
		MoonvilleInventoryComp->ProcessEvent(RemoveFunc, &Params);
		Remaining -= Take;
	}
}

void AZP_GraceCharacter::OnReserveConsumedHandler(int32 Rounds, EZP_WeaponIcon Icon)
{
	// A reload pulled Rounds into the magazine — spend that many ammo items from the
	// inventory, then re-sync the reserve mirror from what's left.
	RemoveInventoryAmmo(Icon, Rounds);
	SyncReserveFromInventory();
}

void AZP_GraceCharacter::HandleInventoryUpdate()
{
	// Trim any ammo type that a whole-stack pickup pushed over its cap, THEN mirror the
	// (now-capped) inventory into the HUD reserve count.
	EnforceAmmoCaps();
	SyncReserveFromInventory();

	// Inventory changed → re-evaluate objective HasItem requirements (key card / mysterious note
	// pickups advance their steps; this is the single notify point for the whole Moonville grid).
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UZP_ObjectiveSubsystem* Obj = GI->GetSubsystem<UZP_ObjectiveSubsystem>())
		{
			Obj->NotifyInventoryChanged(NAME_None);
		}
	}


	// Inventory changed (acquired/dropped a weapon) → re-evaluate which pickups are
	// now blocked (e.g. picked up shotgun → the other shotgun goes non-interactable).
	GatherLevelPickups();
	RefreshPickupBlockStates();

	UE_LOG(LogTemp, Warning, TEXT("[NoteBridge] === SCAN START === MoonvilleInv=%s NoteComp=%s"),
		MoonvilleInventoryComp ? TEXT("valid") : TEXT("null"),
		NoteComp ? TEXT("valid") : TEXT("null"));

	if (!MoonvilleInventoryComp || !NoteComp) return;

	// Scan all items in Moonville inventory for note items (tagged with "Item.Note")
	// NOT static — re-request each call so tag registration order doesn't matter
	const FGameplayTag NoteTag = FGameplayTag::RequestGameplayTag(FName("Item.Note"), false);
	if (!NoteTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NoteBridge] Item.Note GameplayTag NOT REGISTERED — cannot scan for notes"));
		return;
	}

	FProperty* SlotsProp = MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ItemSlots"));
	FArrayProperty* ArrayProp = SlotsProp ? CastField<FArrayProperty>(SlotsProp) : nullptr;
	if (!ArrayProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[NoteBridge] ItemSlots property not found on inventory component"));
		return;
	}

	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return;

	// Find the Item_ property in FFItemSlot
	FObjectProperty* ItemObjProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("Item_")))
		{
			ItemObjProp = CastField<FObjectProperty>(*It);
			break;
		}
	}
	if (!ItemObjProp) return;

	// Property accessors for PDA_Item (looked up per-call to avoid stale pointers across PIE)
	FTextProperty* NameProp = nullptr;
	FTextProperty* DescProp = nullptr;
	FStructProperty* TagsProp = nullptr;
	bool bPropsResolved = false;

	int32 TotalItems = 0;
	int32 NoteTaggedItems = 0;
	int32 NewNotesAdded = 0;
	int32 AlreadyCollected = 0;

	UE_LOG(LogTemp, Warning, TEXT("[NoteBridge] Scanning %d inventory slots..."), ArrayHelper.Num());

	for (int32 i = 0; i < ArrayHelper.Num(); ++i)
	{
		void* ElementData = ArrayHelper.GetRawPtr(i);
		UObject* ItemDA = ItemObjProp->GetObjectPropertyValue(ItemObjProp->ContainerPtrToValuePtr<void>(ElementData));
		if (!ItemDA) continue;
		TotalItems++;

		// Resolve property pointers from first valid item's class
		if (!bPropsResolved)
		{
			UClass* ItemClass = ItemDA->GetClass();
			NameProp = CastField<FTextProperty>(ItemClass->FindPropertyByName(FName("Name")));
			DescProp = CastField<FTextProperty>(ItemClass->FindPropertyByName(FName("Description")));
			TagsProp = CastField<FStructProperty>(ItemClass->FindPropertyByName(FName("GameplayTags")));
			bPropsResolved = true;
			UE_LOG(LogTemp, Warning, TEXT("[NoteBridge] ItemClass=%s Name=%s Desc=%s Tags=%s"),
				*ItemClass->GetName(),
				NameProp ? TEXT("found") : TEXT("NULL"),
				DescProp ? TEXT("found") : TEXT("NULL"),
				TagsProp ? TEXT("found") : TEXT("NULL"));
		}

		if (!TagsProp) continue;

		// Check GameplayTags for Item.Note
		const FGameplayTagContainer* ItemTags = TagsProp->ContainerPtrToValuePtr<FGameplayTagContainer>(ItemDA);

		// Log ALL tags on this item for diagnosis
		FString TagsStr = ItemTags ? ItemTags->ToStringSimple() : TEXT("(null)");
		bool bHasNoteTag = ItemTags && ItemTags->HasTag(NoteTag);

		UE_LOG(LogTemp, Warning, TEXT("[NoteBridge] Slot[%d] Item=%s Class=%s Tags=[%s] HasNoteTag=%s"),
			i, *ItemDA->GetName(), *ItemDA->GetClass()->GetName(),
			*TagsStr, bHasNoteTag ? TEXT("YES") : TEXT("no"));

		if (!bHasNoteTag) continue;
		NoteTaggedItems++;

		// Build NoteID from DataAsset path name
		FName NoteID = FName(*ItemDA->GetPathName());
		if (NoteComp->HasNote(NoteID))
		{
			UE_LOG(LogTemp, Warning, TEXT("[NoteBridge]   -> Already collected (ID=%s)"), *NoteID.ToString());
			AlreadyCollected++;
			continue;
		}

		// Extract Name and Description
		FText Title = NameProp ? NameProp->GetPropertyValue(NameProp->ContainerPtrToValuePtr<void>(ItemDA)) : FText::FromString(TEXT("Unknown Note"));
		FText Content = DescProp ? DescProp->GetPropertyValue(DescProp->ContainerPtrToValuePtr<void>(ItemDA)) : FText::GetEmpty();

		FZP_NoteEntry NewNote;
		NewNote.NoteID = NoteID;
		NewNote.Title = Title;
		NewNote.Content = Content;
		NewNote.bIsCode = false;

		NoteComp->AddNote(NewNote);
		NewNotesAdded++;

		UE_LOG(LogTemp, Warning, TEXT("[NoteBridge]   -> ADDED note '%s' (ID=%s)"),
			*Title.ToString(), *NoteID.ToString());
	}

	UE_LOG(LogTemp, Warning, TEXT("[NoteBridge] === SCAN DONE === Slots=%d Items=%d NoteTagged=%d NewAdded=%d AlreadyHad=%d TotalInComp=%d"),
		ArrayHelper.Num(), TotalItems, NoteTaggedItems, NewNotesAdded, AlreadyCollected,
		NoteComp->GetNotes().Num());
}

UObject* AZP_GraceCharacter::GetItemDAFromShortcutSlot(int32 SlotIndex)
{
	if (!MoonvilleInventoryComp) return nullptr;

	FProperty* SlotsProp = MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ShortcutSlots"));
	if (!SlotsProp) return nullptr;

	FArrayProperty* ArrayProp = CastField<FArrayProperty>(SlotsProp);
	if (!ArrayProp) return nullptr;

	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
	if (SlotIndex >= ArrayHelper.Num()) return nullptr;

	void* ElementData = ArrayHelper.GetRawPtr(SlotIndex);

	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return nullptr;

	FProperty* ItemProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("Item_")))
		{
			ItemProp = *It;
			break;
		}
	}
	if (!ItemProp) return nullptr;

	FObjectProperty* ObjProp = CastField<FObjectProperty>(ItemProp);
	if (!ObjProp) return nullptr;

	return ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<void>(ElementData));
}

// --- Quick Slot Input ---
// Reads directly from Moonville's ShortcutSlots[] array.
// Keys 1-4 map to ShortcutSlots[0-3]. Weapons get equipped via Kinemation.
// Consumables trigger Moonville's ExecuteItemActionByShortcut (handles use + inventory removal).

void AZP_GraceCharacter::Input_InventorySlot0(const FInputActionValue& Value) { Input_InventorySlot(0); }
void AZP_GraceCharacter::Input_InventorySlot1(const FInputActionValue& Value) { Input_InventorySlot(1); }
void AZP_GraceCharacter::Input_InventorySlot2(const FInputActionValue& Value) { Input_InventorySlot(2); }
void AZP_GraceCharacter::Input_InventorySlot3(const FInputActionValue& Value) { Input_InventorySlot(3); }

void AZP_GraceCharacter::Input_InventorySlot(int32 SlotIndex)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	if (SlotIndex < 0 || SlotIndex > 3) return;
	if (DodgeLockRemaining > 0.f) return; // no weapon swap mid-dodge

	CancelSprintFromAction(); // swapping / using a slot item breaks a sprint

	// Try weapon equip first
	TSubclassOf<AActor> SlotWeaponClass = GetWeaponFromShortcutSlot(SlotIndex);
	if (SlotWeaponClass)
	{
		if (!KinemationComp) return;

		// A quick-slot still referencing a weapon that's no longer in the GRID (e.g. moved to a briefcase)
		// is stale — don't equip a phantom weapon the player doesn't actually have.
		if (!IsWeaponClassInGrid(SlotWeaponClass))
		{
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] Quick-slot %d: %s not in inventory (stale shortcut) — ignoring"),
				SlotIndex, *SlotWeaponClass->GetName());
			return;
		}

		// Throwables: only equippable while supply remains (stack-aware —
		// replaces the old permanent consumed-class blocklist that killed
		// the hotkey after one throw from a stack)
		if (SlotWeaponClass->GetName().Contains(TEXT("Grenade"))
			&& CountWeaponClassInInventory(SlotWeaponClass) <= 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] No %s left in inventory — ignoring hotkey"), *SlotWeaponClass->GetName());
			return;
		}

		// Don't re-equip same weapon class
		if (KinemationComp->ActiveWeapon && KinemationComp->WeaponClass == SlotWeaponClass)
		{
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] Weapon in slot %d already equipped"), SlotIndex);
			return;
		}

		// Capture the item DA BEFORE equip — Moonville may clear the slot during EquipWeaponClass
		UObject* PreEquipItemDA = GetItemDAFromShortcutSlot(SlotIndex);

		// Probe: capture camera state through the equip transition (dev reports
		// equip "breaks camera" same as dodge/block).
		CameraProbeTag = TEXT("EQUIP");
		CameraProbeFramesLeft = 180;
		LogCameraProbe(TEXT("PRE-EQUIP"));

		bool bSuccess = KinemationComp->EquipWeaponClass(SlotWeaponClass);

		LogCameraProbe(TEXT("POST-EQUIP"));

		// Track slot index + item DA for throwable inventory removal
		if (bSuccess && KinemationComp->CurrentWeaponType == EZP_WeaponType::Throwable)
		{
			LastThrowableSlotIndex = SlotIndex;
			LastThrowableItemDA = PreEquipItemDA;
		}

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Key %d → shortcut slot %d → %s → %s"),
			SlotIndex + 1, SlotIndex, *SlotWeaponClass->GetName(),
			bSuccess ? TEXT("EQUIPPED") : TEXT("FAILED"));
		return;
	}

	// Not a weapon — try using item action (consumables, etc.)
	UseItemFromShortcutSlot(SlotIndex);
}

void AZP_GraceCharacter::UseItemFromShortcutSlot(int32 SlotIndex)
{
	if (!MoonvilleInventoryComp) return;

	UFunction* UseFunc = MoonvilleInventoryComp->FindFunction(FName("ExecuteItemActionByShortcut"));
	if (!UseFunc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ExecuteItemActionByShortcut not found on inventory component"));
		return;
	}

	struct { int32 ItemIndex; } Params;
	Params.ItemIndex = SlotIndex;
	MoonvilleInventoryComp->ProcessEvent(UseFunc, &Params);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Key %d → executed item action from shortcut slot %d"),
		SlotIndex + 1, SlotIndex);
}

TSubclassOf<AActor> AZP_GraceCharacter::GetWeaponFromShortcutSlot(int32 SlotIndex)
{
	if (!MoonvilleInventoryComp) return nullptr;

	// Read ShortcutSlots array from BP_InventoryCharacterComponent via reflection
	FProperty* SlotsProp = MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ShortcutSlots"));
	if (!SlotsProp) return nullptr;

	FArrayProperty* ArrayProp = CastField<FArrayProperty>(SlotsProp);
	if (!ArrayProp) return nullptr;

	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
	if (SlotIndex >= ArrayHelper.Num()) return nullptr;

	void* ElementData = ArrayHelper.GetRawPtr(SlotIndex);

	// Find Item (PDA_Item) in FItemSlot struct
	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return nullptr;

	FProperty* ItemProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("Item_")))
		{
			ItemProp = *It;
			break;
		}
	}
	if (!ItemProp) return nullptr;

	FObjectProperty* ObjProp = CastField<FObjectProperty>(ItemProp);
	if (!ObjProp) return nullptr;

	UObject* ItemDA = ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<void>(ElementData));
	if (!ItemDA) return nullptr;

	return GetWeaponClassFromItem(ItemDA);
}

bool AZP_GraceCharacter::IsWeaponClassInGrid(TSubclassOf<AActor> InWeaponClass)
{
	if (!MoonvilleInventoryComp || !InWeaponClass) return false;

	FArrayProperty* GridProp = CastField<FArrayProperty>(MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ItemSlots")));
	if (!GridProp) return false;
	FStructProperty* StructInner = CastField<FStructProperty>(GridProp->Inner);
	if (!StructInner) return false;

	FObjectProperty* ItemProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("Item_"))) { ItemProp = CastField<FObjectProperty>(*It); break; }
	}
	if (!ItemProp) return false;

	FScriptArrayHelper Helper(GridProp, GridProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
	for (int32 i = 0; i < Helper.Num(); ++i)
	{
		UObject* ItemDA = ItemProp->GetObjectPropertyValue(ItemProp->ContainerPtrToValuePtr<void>(Helper.GetRawPtr(i)));
		if (ItemDA && GetWeaponClassFromItem(ItemDA) == InWeaponClass) return true;
	}
	return false;
}

TSubclassOf<AActor> AZP_GraceCharacter::GetWeaponClassFromItem(UObject* ItemDA)
{
	if (!ItemDA) return nullptr;

	// Read ItemActionPayload.ObjectClass from PDA_Item via reflection
	FProperty* PayloadProp = ItemDA->GetClass()->FindPropertyByName(FName("ItemActionPayload"));
	if (!PayloadProp) return nullptr;

	FStructProperty* PayloadStructProp = CastField<FStructProperty>(PayloadProp);
	if (!PayloadStructProp) return nullptr;

	void* PayloadData = PayloadProp->ContainerPtrToValuePtr<void>(ItemDA);

	// Find ObjectClass within the payload struct
	FProperty* ObjClassProp = nullptr;
	for (TFieldIterator<FProperty> It(PayloadStructProp->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("ObjectClass")))
		{
			ObjClassProp = *It;
			break;
		}
	}
	if (!ObjClassProp) return nullptr;

	void* ObjClassData = ObjClassProp->ContainerPtrToValuePtr<void>(PayloadData);

	// Resolve the class reference (handles soft class, hard class, or object ref)
	UClass* ResolvedClass = nullptr;

	if (FSoftClassProperty* SoftProp = CastField<FSoftClassProperty>(ObjClassProp))
	{
		FSoftObjectPtr& SoftRef = *static_cast<FSoftObjectPtr*>(ObjClassData);
		UObject* Resolved = SoftRef.Get();
		if (!Resolved) Resolved = SoftRef.LoadSynchronous();
		if (Resolved)
		{
			if (UBlueprint* BP = Cast<UBlueprint>(Resolved))
				ResolvedClass = BP->GeneratedClass;
			else
				ResolvedClass = Cast<UClass>(Resolved);
		}
	}
	else if (FClassProperty* ClassProp = CastField<FClassProperty>(ObjClassProp))
	{
		ResolvedClass = *static_cast<UClass**>(ObjClassData);
	}
	else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(ObjClassProp))
	{
		UObject* Obj = *static_cast<UObject**>(ObjClassData);
		if (UBlueprint* BP = Cast<UBlueprint>(Obj))
			ResolvedClass = BP->GeneratedClass;
		else
			ResolvedClass = Cast<UClass>(Obj);
	}

	// Validate it's an Actor subclass (weapon BPs are Actor-based)
	if (ResolvedClass && ResolvedClass->IsChildOf(AActor::StaticClass()))
	{
		return TSubclassOf<AActor>(ResolvedClass);
	}

	return nullptr;
}

UObject* AZP_GraceCharacter::GetPickupItemDA(AActor* PickupActor)
{
	if (!PickupActor) return nullptr;

	// BP_ItemPickup designers set "Item"; "ItemDataAsset" is the runtime mirror.
	for (const TCHAR* PropName : { TEXT("Item"), TEXT("ItemDataAsset") })
	{
		if (FObjectProperty* ItemProp = CastField<FObjectProperty>(
				PickupActor->GetClass()->FindPropertyByName(FName(PropName))))
		{
			if (UObject* DA = ItemProp->GetObjectPropertyValue(
					ItemProp->ContainerPtrToValuePtr<void>(PickupActor)))
			{
				return DA;
			}
		}
	}
	return nullptr;
}

AActor* AZP_GraceCharacter::GetClosestMoonvillePickupOwner()
{
	if (!MoonvilleInteractionComp) return nullptr;

	FObjectProperty* ClosestProp = CastField<FObjectProperty>(
		MoonvilleInteractionComp->GetClass()->FindPropertyByName(TEXT("ClosestInteractable")));
	if (!ClosestProp) return nullptr;

	UActorComponent* Comp = Cast<UActorComponent>(ClosestProp->GetObjectPropertyValue(
		ClosestProp->ContainerPtrToValuePtr<void>(MoonvilleInteractionComp)));
	AActor* PickupOwner = Comp ? Comp->GetOwner() : nullptr;
	if (!PickupOwner) return nullptr;

	for (UClass* C = PickupOwner->GetClass(); C; C = C->GetSuperClass())
	{
		if (C->GetName().Contains(TEXT("ItemPickup"))) return PickupOwner;
	}
	return nullptr;
}

bool AZP_GraceCharacter::ShouldBlockPickupInteraction(AActor* PickupActor)
{
	if (!PickupActor || !KinemationComp) return false;

	UObject* ItemDA = GetPickupItemDA(PickupActor);
	if (!ItemDA) return false;

	const FString ItemName = ItemDA->GetName();

	// 2d — already own this weapon. Throwables (grenade/rock) are EXCEPTED: they
	// stack, so picking up more must stay allowed.
	TSubclassOf<AActor> WClass = GetWeaponClassFromItem(ItemDA);
	if (WClass)
	{
		const FString WName = WClass->GetName();
		const bool bThrowable = WName.Contains(TEXT("Grenade")) || WName.Contains(TEXT("Rock"));
		if (!bThrowable && CountWeaponClassInInventory(WClass) > 0)
		{
			return true;
		}
	}

	// 2b — ammo whose reserve is already at the cap for that weapon type.
	if (ItemName.Contains(TEXT("Ammo")))
	{
		const EZP_WeaponIcon Icon = UZP_KinemationComponent::AmmoNameToIcon(ItemName);
		if (Icon != EZP_WeaponIcon::None
			&& GetInventoryAmmoCount(Icon) >= UZP_KinemationComponent::GetReserveCapForIcon(Icon))
		{
			return true;
		}
	}

	return false;
}

void AZP_GraceCharacter::GatherLevelPickups()
{
	CachedPickups.Reset();
	if (!GetWorld()) return;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		if (!A) continue;
		for (UClass* C = A->GetClass(); C; C = C->GetSuperClass())
		{
			if (C->GetName().Contains(TEXT("ItemPickup")))
			{
				CachedPickups.Add(A);
				break;
			}
		}
	}
}

void AZP_GraceCharacter::RefreshPickupBlockStates()
{
	for (int32 i = CachedPickups.Num() - 1; i >= 0; --i)
	{
		AActor* P = CachedPickups[i].Get();
		if (!P)
		{
			CachedPickups.RemoveAtSwap(i);
			continue;
		}

		const ECollisionEnabled::Type Desired = ShouldBlockPickupInteraction(P)
			? ECollisionEnabled::NoCollision   // off → Moonville can't see it (no popup, no pickup)
			: ECollisionEnabled::QueryOnly;    // on → normal overlap detection

		for (const TCHAR* CompName : { TEXT("InteractionArea"), TEXT("InteractionCollision") })
		{
			if (FObjectProperty* CompProp = CastField<FObjectProperty>(P->GetClass()->FindPropertyByName(FName(CompName))))
			{
				if (UPrimitiveComponent* Sphere = Cast<UPrimitiveComponent>(
						CompProp->GetObjectPropertyValue(CompProp->ContainerPtrToValuePtr<void>(P))))
				{
					if (Sphere->GetCollisionEnabled() != Desired)
					{
						Sphere->SetCollisionEnabled(Desired);
					}
				}
			}
		}
	}
}

// --- Death / Vignette ---

void AZP_GraceCharacter::HandleDeath()
{
	// Died mid-grab (bite ticks / fail chunk): tear the grab down FIRST — releases the grabber,
	// restores collision ignores/movement/meshes/HUD — then the normal fade-to-black respawn.
	if (GrabPhase != EZP_GrabPhase::None)
	{
		EndGrab(/*bAborted*/true);
	}

	if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController()))
	{
		PC->OnPawnDied();
	}
}

void AZP_GraceCharacter::UpdateHealthVignette(float NewHealth, float MaxHealth, float DamageAmount)
{
	if (!DeathVignetteComp || MaxHealth <= 0.f) return;

	const float HealthPct = NewHealth / MaxHealth;

	if (HealthPct >= 0.5f)
	{
		DeathVignetteComp->Settings.VignetteIntensity = 0.f;
	}
	else
	{
		// Lerp from 0 (at 50% HP) to 1.5 (at 0% HP)
		DeathVignetteComp->Settings.VignetteIntensity =
			FMath::GetMappedRangeValueClamped(FVector2D(0.5f, 0.f), FVector2D(0.f, 1.5f), HealthPct);
	}
}

// --- Flashlight ---

void AZP_GraceCharacter::Input_Flashlight(const FInputActionValue& Value)
{
	if (GrabPhase != EZP_GrabPhase::None) return; // grabbed — input is owned by the struggle
	ToggleFlashlight();
}

void AZP_GraceCharacter::ToggleFlashlight()
{
	CancelSprintFromAction(); // toggling the flashlight breaks a sprint
	bFlashlightOn = !bFlashlightOn;

	if (FlashlightComp)
	{
		FlashlightComp->SetVisibility(bFlashlightOn);

		// Snap to camera direction when turning on (avoid lerp from stale rotation)
		if (bFlashlightOn && FirstPersonCamera)
		{
			FRotator SnapRot = FirstPersonCamera->GetComponentRotation();
			SnapRot.Pitch += FlashlightPitchOffset;
			FlashlightComp->SetWorldRotation(SnapRot);
		}
	}

	// Fill light toggles with the flashlight
	if (FlashlightFillComp)
	{
		FlashlightFillComp->SetVisibility(bFlashlightOn);
	}

	if (FlashlightClickSound)
	{
		UGameplayStatics::PlaySound2D(this, FlashlightClickSound);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Flashlight %s"), bFlashlightOn ? TEXT("ON") : TEXT("OFF"));
}

// --- Starting Items ---

void AZP_GraceCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// Begin tracking a fall the moment we leave the ground. Seed the apex with
	// the current Z; Tick raises it if we keep rising (jump arc).
	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		bTrackingFall = true;
		FallPeakZ = GetActorLocation().Z;
	}
}

void AZP_GraceCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!bTrackingFall)
	{
		return;
	}
	bTrackingFall = false;

	const float DropDistance = FallPeakZ - GetActorLocation().Z;

	// Below the safe threshold: no damage (normal jumps/short hops).
	if (DropDistance <= FallDamageMinDistance)
	{
		return;
	}

	// Lerp min→max damage across the [min,max] distance band; clamp above max.
	float FallDamage = FallDamageMaxAmount;
	if (FallDamageMaxDistance > FallDamageMinDistance)
	{
		const float Alpha = FMath::Clamp(
			(DropDistance - FallDamageMinDistance) / (FallDamageMaxDistance - FallDamageMinDistance),
			0.f, 1.f);
		FallDamage = FMath::Lerp(FallDamageMinAmount, FallDamageMaxAmount, Alpha);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Fall: drop %.0fcm -> %.0f damage"), DropDistance, FallDamage);

	// Apply straight through HealthComp — a fall isn't blockable and shouldn't
	// route through the block-reduction / attacker-stagger path in TakeDamage.
	if (HealthComp && FallDamage > 0.f)
	{
		HealthComp->ApplyDamage(FallDamage);
	}
}

float AZP_GraceCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (HealthComp && HealthComp->bIsDead)
	{
		return 0.f;
	}

	// Downed i-frames: while knocked down / getting up after a failed grab struggle, the player
	// cannot respond — hits are free shots on a helpless target. The fail already cost
	// FailDamageChunk; nothing else lands until Marcus is back on his feet.
	if (GrabPhase == EZP_GrabPhase::FailKnockdown || GrabPhase == EZP_GrabPhase::GetUp)
	{
		return 0.f;
	}

	float IncomingDamage = DamageAmount;
	if (bIsBlocking && DamageAmount > 0.f)
	{
		// A blocked hit COSTS stamina (~1/3 of max). Not enough left = GUARD BREAK: the guard
		// drops, this hit lands at FULL damage, no counter-stagger — blocking is a resource,
		// not a free wall. (With RMB still held, the guard re-raises once stamina recovers past
		// BlockMinStaminaFractionToStart.)
		if (GameplayComp && !GameplayComp->TryConsumeStaminaPercent(BlockHitStaminaPercent))
		{
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] GUARD BREAK — blocked hit with insufficient stamina, full damage taken"));
			ReleaseBlock();
		}
		else
		{
			IncomingDamage *= BlockDamageReductionMul;

			// Probe: capture camera state through the impact reaction.
			CameraProbeTag = TEXT("BLOCK-HIT");
			CameraProbeFramesLeft = 180;
			LogCameraProbe(TEXT("HIT-RECEIVED"));

			// Random FPP_Longs_BlockImpact 1/2/3 plays on the view mesh.
			PlayBlockImpactAnim();

			// Stagger the attacker on a successful block — rate-limited by BlockStaggerCooldown so
			// blocking can't permanently stun-lock an enemy. Routed through IZP_Staggerable so it
			// works for any enemy (Shambler, Scytheer, and future types) with no per-enemy code.
			const double NowBlock = GetWorld()->GetTimeSeconds();
			if ((NowBlock - LastBlockStaggerTime) >= BlockStaggerCooldown)
			{
				LastBlockStaggerTime = NowBlock;
				if (EventInstigator)
				{
					if (APawn* AttackerPawn = EventInstigator->GetPawn())
					{
						StaggerEnemy(AttackerPawn, BlockStaggerDuration);
					}
				}
			}
		}
	}

	const float ActualDamage = Super::TakeDamage(IncomingDamage, DamageEvent, EventInstigator, DamageCauser);

	if (HealthComp)
	{
		HealthComp->ApplyDamage(ActualDamage);
	}

	// Camera flinch — immediate visceral feedback that something hit you.
	// Skipped while blocking: the block already gives visceral feedback
	// (BlockImpact anim + stagger), and a camera flinch on top reads as "broken".
	// Skipped while grabbed: the flinch writes ControlRotation, which would corrupt the
	// scripted 3P camera AND the FP view direction restored after the grab.
	if (ActualDamage > 0.f && !bIsBlocking && GrabPhase == EZP_GrabPhase::None)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			// Scale flinch with damage — heavier hits = bigger jolt
			const float FlinchScale = FMath::Clamp(ActualDamage / 25.f, 0.5f, 3.0f);
			PC->AddPitchInput(-2.0f * FlinchScale);
			PC->AddYawInput(FMath::RandRange(-1.5f, 1.5f) * FlinchScale);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Grace took %.0f damage from %s"),
		ActualDamage, DamageCauser ? *DamageCauser->GetName() : TEXT("unknown"));

	return ActualDamage;
}

void AZP_GraceCharacter::StaggerEnemy(AActor* Enemy, float Duration)
{
	if (!Enemy || Duration <= 0.f) { return; }

	// The enemy itself may implement IZP_Staggerable (actor-based AI like the Scytheer)...
	if (Enemy->GetClass()->ImplementsInterface(UZP_Staggerable::StaticClass()))
	{
		IZP_Staggerable::Execute_ReceiveStagger(Enemy, Duration);
		return;
	}
	// ...or one of its components does (component-based AI like the Shambler).
	for (UActorComponent* Comp : Enemy->GetComponents())
	{
		if (Comp && Comp->GetClass()->ImplementsInterface(UZP_Staggerable::StaticClass()))
		{
			IZP_Staggerable::Execute_ReceiveStagger(Comp, Duration);
			return;
		}
	}
}

void AZP_GraceCharacter::MeleeStaggerEnemy(AActor* Enemy)
{
	// Rate-limit melee-hit staggers (0 = every hit staggers; the swing cooldown still paces it).
	const double Now = GetWorld()->GetTimeSeconds();
	if ((Now - LastHitStaggerTime) < HitStaggerCooldown)
	{
		return;
	}
	LastHitStaggerTime = Now;
	StaggerEnemy(Enemy, HitStaggerDuration);
}

void AZP_GraceCharacter::OpenSaveMenu(UUserWidget* Menu)
{
	if (!Menu) return;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	ActiveSaveMenu = Menu;
	bSaveMenuOpen = true;

	// REMOVE the gameplay mapping context (IMC_Grace) while the save menu is open. The EGUI save
	// widget is a CommonActivatableWidget — when it activates it re-applies its own input config on
	// top of any SetInputMode we set, which let gameplay actions keep firing (dev heard the player
	// dodging/moving "under" the menu, stealing the pad's nav inputs). With IMC_Grace pulled, NO
	// gameplay action can fire regardless of the active input mode; the UI keeps its own nav input.
	AZP_PlayerController* ZPC = Cast<AZP_PlayerController>(PC);
	UE_LOG(LogTemp, Warning, TEXT("[UIProbe] OpenSaveMenu: widget=%s  ZPC=%s  DefaultMC=%s"),
		*Menu->GetClass()->GetName(),
		ZPC ? TEXT("OK") : TEXT("NULL(cast failed!)"),
		(ZPC && ZPC->DefaultMappingContext) ? *ZPC->DefaultMappingContext->GetName() : TEXT("NULL"));
	if (ZPC && ZPC->DefaultMappingContext)
	{
		ZPC->RemoveMappingContext(ZPC->DefaultMappingContext);
		UE_LOG(LogTemp, Warning, TEXT("[UIProbe] OpenSaveMenu: removed gameplay context %s"),
			*ZPC->DefaultMappingContext->GetName());
	}

	// NOTE: Back is handled manually in Tick (Face Right / Esc) — this widget is shown via raw
	// AddToViewport, not a CommonUI activatable stack, so CommonUI's back action never routes to it.
	// Adding nav contexts here doesn't help; polling the key + closing it ourselves is reliable.

	// GameAndUI (NOT UIOnly): gameplay is already dead because we removed IMC_Grace above, so we don't
	// need UIOnly — and UIOnly routes ALL input to Slate, so the PlayerController never sees keys and
	// the manual Back poll (Tick) reads nothing. GameAndUI lets the widget keep UI navigation while
	// letting unhandled keys (Face Right / Esc) reach the PlayerController for the Back poll.
	// Do NOT force focus to the root — the CommonActivatableWidget focuses its own desired target
	// (a save card) inside a proper nav scope; clobbering it gave only ONE Slate move then locked.
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);

	// Auto-probe now AND again after the CommonActivatableWidget has had a chance to re-apply its own
	// input config — if GAMEPLAY:Move/Jump still show live keys in the deferred dump, something
	// re-added IMC_Grace after us.
	if (ZPC)
	{
		ZPC->DumpUIInput();
		FTimerHandle Tmp;
		GetWorld()->GetTimerManager().SetTimer(Tmp, FTimerDelegate::CreateWeakLambda(ZPC, [ZPC]()
		{
			UE_LOG(LogTemp, Warning, TEXT("[UIProbe] --- deferred (post-activation) dump ---"));
			ZPC->DumpUIInput();
		}), 0.5f, false);
	}
}

void AZP_GraceCharacter::GrantStartingItems()
{
	// Add StartingWeaponItem to Moonville inventory.
	// Moonville's AutoAssignShortcut handles shortcut bar placement.
	// Quick-slot keys read from Moonville's ShortcutSlots[] at press time.
	UObject* ItemDA = StartingWeaponItem.LoadSynchronous();
	if (ItemDA && MoonvilleInventoryComp)
	{
		UFunction* AddFunc = MoonvilleInventoryComp->FindFunction(FName("AddItemSimple"));
		if (AddFunc)
		{
			struct { UObject* ItemToAdd; int32 Amount; } Params;
			Params.ItemToAdd = ItemDA;
			Params.Amount = 1;
			MoonvilleInventoryComp->ProcessEvent(AddFunc, &Params);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] GrantStartingItems: Added %s to Moonville inventory"), *ItemDA->GetName());
		}
	}
	else if (!ItemDA)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] StartingWeaponItem not set — no starting item granted"));
	}
}

void AZP_GraceCharacter::BindEguiSaveComponent()
{
	// Find the EasyGameUI save component (BP_EasySaveGameComponent) on this actor and bind our handler to
	// its LoadingOrSavingVariables dispatcher. It fires on every per-slot SAVE and LOAD. The player is a
	// Persistent EGUI actor restored IN PLACE, so this hook (not BeginPlay) is how a load reaches us.
	UActorComponent* SaveComp = nullptr;
	for (UActorComponent* C : GetComponents())
	{
		if (C && C->GetClass()->GetName().Contains(TEXT("EasySaveGameComponent")))
		{
			SaveComp = C;
			break;
		}
	}
	if (!SaveComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] Inventory persist: NO BP_EasySaveGameComponent on the player — add it to BP_GraceCharacter (ActorSaveType=Persistent, HasVariablesToSave=true). Inventory will NOT persist via the save file."));
		return;
	}

	FMulticastDelegateProperty* DProp = CastField<FMulticastDelegateProperty>(
		SaveComp->GetClass()->FindPropertyByName(FName("LoadingOrSavingVariables")));
	if (!DProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] Inventory persist: LoadingOrSavingVariables dispatcher not found on %s"), *SaveComp->GetClass()->GetName());
		return;
	}

	FScriptDelegate Del;
	Del.BindUFunction(this, FName("OnEguiSaveLoadVariables"));
	DProp->AddDelegate(MoveTemp(Del), SaveComp, DProp->ContainerPtrToValuePtr<void>(SaveComp));
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Inventory persist: bound to EGUI LoadingOrSavingVariables on %s"), *SaveComp->GetName());
}

void AZP_GraceCharacter::OnEguiSaveLoadVariables(uint8 OperationType, FJsonObjectWrapper JsonObject)
{
	// OperationType (E_SaveGameOperationType): 0 = Save, 1 = Load.
	if (!JsonObject.JsonObject.IsValid()) return;

	static const FString InvKey(TEXT("SignalInventory"));
	static const FString SizeXKey(TEXT("SignalInvSizeX"));
	static const FString SizeYKey(TEXT("SignalInvSizeY"));

	static const FString EquipKey(TEXT("SignalEquippedWeapon"));

	if (OperationType == 0) // SAVE — write the current inventory into the slot's JSON
	{
		FVector2D Size(0.f, 0.f);
		const FString InvText = ExportInventoryText(Size);
		JsonObject.JsonObject->SetStringField(InvKey, InvText);
		JsonObject.JsonObject->SetNumberField(SizeXKey, Size.X);
		JsonObject.JsonObject->SetNumberField(SizeYKey, Size.Y);
		// Remember which weapon was in-hand so we re-equip it on load (the grid restore alone leaves you
		// holding the default weapon).
		if (KinemationComp && KinemationComp->WeaponClass.Get())
		{
			JsonObject.JsonObject->SetStringField(EquipKey, KinemationComp->WeaponClass.Get()->GetPathName());
		}
		// Also persist the quick-slot bar (ShortcutSlots). LoadInventoryFromSavegame only restores the GRID
		// (ItemSlots), so without this the D-pad/number quick-slots come back empty (weapons not assigned) and
		// slot 0 keeps a stale starting-weapon ref you can still equip after moving it out.
		if (FArrayProperty* ShortcutProp = MoonvilleInventoryComp
			? CastField<FArrayProperty>(MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ShortcutSlots"))) : nullptr)
		{
			FString ShortcutText;
			ShortcutProp->ExportText_Direct(ShortcutText, ShortcutProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp), nullptr, MoonvilleInventoryComp, PPF_None);
			JsonObject.JsonObject->SetStringField(TEXT("SignalShortcuts"), ShortcutText);
		}
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Inventory SAVE -> EGUI json (%d chars)"), InvText.Len());

		// Objectives are save-file-tied too (bAutoPersist=false → fresh PIE starts clean). Persist on save.
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UZP_ObjectiveSubsystem* Obj = GI->GetSubsystem<UZP_ObjectiveSubsystem>()) { Obj->SaveObjectiveState(); }
		}
	}
	else // LOAD — read it back from the slot's JSON and apply
	{
		FString InvText;
		if (JsonObject.JsonObject->TryGetStringField(InvKey, InvText) && !InvText.IsEmpty())
		{
			double SX = 0.0, SY = 0.0;
			JsonObject.JsonObject->TryGetNumberField(SizeXKey, SX);
			JsonObject.JsonObject->TryGetNumberField(SizeYKey, SY);
			ApplyInventoryFromText(InvText, FVector2D((float)SX, (float)SY));
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] Inventory LOAD <- EGUI json (%d chars)"), InvText.Len());
		}

		// Restore the quick-slot bar AFTER the grid so the D-pad/number slots map to the saved weapons (and
		// the stale starting-weapon ref in slot 0 is overwritten).
		FString ShortcutText;
		if (MoonvilleInventoryComp
			&& JsonObject.JsonObject->TryGetStringField(TEXT("SignalShortcuts"), ShortcutText) && !ShortcutText.IsEmpty())
		{
			if (FArrayProperty* ShortcutProp = CastField<FArrayProperty>(MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ShortcutSlots"))))
			{
				ShortcutProp->ImportText_Direct(*ShortcutText, ShortcutProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp), MoonvilleInventoryComp, PPF_None);
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] Inventory LOAD: restored quick-slot bar (%d chars)"), ShortcutText.Len());
			}
		}

		// Re-equip the saved in-hand weapon, deferred a tick so the just-restored grid/pawn are settled.
		FString WeaponPath;
		if (JsonObject.JsonObject->TryGetStringField(EquipKey, WeaponPath) && !WeaponPath.IsEmpty())
		{
			if (UWorld* W = GetWorld())
			{
				TWeakObjectPtr<AZP_GraceCharacter> WeakThis(this);
				const FString PathCopy = WeaponPath;
				W->GetTimerManager().SetTimerForNextTick([WeakThis, PathCopy]()
				{
					if (AZP_GraceCharacter* Self = WeakThis.Get()) { Self->ReEquipWeaponByPath(PathCopy); }
				});
			}
		}

		// Objectives: restore from the save file (bAutoPersist=false, so this hook is the only restore path).
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UZP_ObjectiveSubsystem* Obj = GI->GetSubsystem<UZP_ObjectiveSubsystem>()) { Obj->LoadObjectiveState(); }
		}
	}
}

void AZP_GraceCharacter::ReEquipWeaponByPath(const FString& WeaponClassPath)
{
	if (!KinemationComp || WeaponClassPath.IsEmpty()) return;
	if (UClass* WClass = LoadClass<AActor>(nullptr, *WeaponClassPath))
	{
		KinemationComp->EquipWeaponClass(WClass);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Re-equipped saved weapon: %s"), *WClass->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ReEquipWeaponByPath: could not load class '%s'"), *WeaponClassPath);
	}
}

FString AZP_GraceCharacter::ExportInventoryText(FVector2D& OutSizeExpansion) const
{
	OutSizeExpansion = FVector2D::ZeroVector;
	if (!MoonvilleInventoryComp) return FString();

	FArrayProperty* SlotsProp = CastField<FArrayProperty>(
		MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ItemSlots")));
	if (!SlotsProp) return FString();

	// UE text export of the whole ItemSlots array — preserves EVERY FFItemSlot field (Amount/stack count,
	// Durability, InventoryPosition, rotation, GUID...). Item refs export as asset paths. (Briefcase idiom.)
	FString Out;
	SlotsProp->ExportText_Direct(Out, SlotsProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp), nullptr, MoonvilleInventoryComp, PPF_None);

	if (FStructProperty* SizeProp = CastField<FStructProperty>(
		MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("InventorySizeExpansion"))))
	{
		OutSizeExpansion = *SizeProp->ContainerPtrToValuePtr<FVector2D>(MoonvilleInventoryComp);
	}
	return Out;
}

void AZP_GraceCharacter::ApplyInventoryFromText(const FString& Text, FVector2D SizeExpansion)
{
	if (!MoonvilleInventoryComp || Text.IsEmpty()) return;

	// Restore via Moonville's own LoadInventoryFromSavegame(ItemSlots, InventorySizeExpansion): it sets the
	// size, wholesale-replaces ItemSlots with-notify (rebuilds the grid UI), and refreshes the allocation
	// cache. We import the exported text directly into the function's ItemSlots param, then ProcessEvent.
	UFunction* Fn = MoonvilleInventoryComp->FindFunction(FName("LoadInventoryFromSavegame"));
	if (!Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ApplyInventoryFromText: LoadInventoryFromSavegame not found"));
		return;
	}

	void* Params = FMemory::Malloc(FMath::Max<int32>(Fn->ParmsSize, 1), Fn->GetMinAlignment());
	FMemory::Memzero(Params, Fn->ParmsSize);
	for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->InitializeValue_InContainer(Params);
	}

	if (FArrayProperty* SlotsParam = CastField<FArrayProperty>(Fn->FindPropertyByName(FName("ItemSlots"))))
	{
		SlotsParam->ImportText_Direct(*Text, SlotsParam->ContainerPtrToValuePtr<void>(Params), MoonvilleInventoryComp, PPF_None);
	}
	if (FStructProperty* SizeParam = CastField<FStructProperty>(Fn->FindPropertyByName(FName("InventorySizeExpansion"))))
	{
		*SizeParam->ContainerPtrToValuePtr<FVector2D>(Params) = SizeExpansion;
	}

	MoonvilleInventoryComp->ProcessEvent(Fn, Params);

	for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->DestroyValue_InContainer(Params);
	}
	FMemory::Free(Params);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ApplyInventoryFromText: applied %d chars via LoadInventoryFromSavegame"), Text.Len());
}

// --- Diagnostics ---

void AZP_GraceCharacter::LogCameraProbe(const TCHAR* Phase)
{
	const FVector CamWorld = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : FVector::ZeroVector;
	const FVector CamRel   = FirstPersonCamera ? FirstPersonCamera->GetRelativeLocation()   : FVector::ZeroVector;
	const FRotator CamWorldRot = FirstPersonCamera ? FirstPersonCamera->GetComponentRotation() : FRotator::ZeroRotator;
	const FRotator CamRelRot   = FirstPersonCamera ? FirstPersonCamera->GetRelativeRotation()  : FRotator::ZeroRotator;
	const FVector SocketWorld = (PlayerMesh && PlayerMesh->DoesSocketExist(FName("FPCamera")))
		? PlayerMesh->GetSocketLocation(FName("FPCamera")) : FVector::ZeroVector;
	const FRotator SocketWorldRot = (PlayerMesh && PlayerMesh->DoesSocketExist(FName("FPCamera")))
		? PlayerMesh->GetSocketRotation(FName("FPCamera")) : FRotator::ZeroRotator;
	const FVector MeshWorld = PlayerMesh ? PlayerMesh->GetComponentLocation() : FVector::ZeroVector;
	const FVector ActorLoc  = GetActorLocation();
	const FVector Velocity  = GetVelocity();
	const FRotator ControlRot = GetControlRotation();
	const FRotator ActorRot   = GetActorRotation();

	UE_LOG(LogTemp, Warning,
		TEXT("[CAM-PROBE %s %s] cam_w=(%.1f,%.1f,%.1f) cam_rel=(%.1f,%.1f,%.1f) "
			 "camRot_w=(P%.1f,Y%.1f,R%.1f) camRot_rel=(P%.1f,Y%.1f,R%.1f) "
			 "sock_w=(%.1f,%.1f,%.1f) sockRot=(P%.1f,Y%.1f,R%.1f) "
			 "mesh_w=(%.1f,%.1f,%.1f) actor=(%.1f,%.1f,%.1f) "
			 "vel=(%.1f,%.1f,%.1f) ctrlRot=(P%.1f,Y%.1f,R%.1f) actorRot=(P%.1f,Y%.1f,R%.1f)"),
		*CameraProbeTag, Phase,
		CamWorld.X, CamWorld.Y, CamWorld.Z,
		CamRel.X, CamRel.Y, CamRel.Z,
		CamWorldRot.Pitch, CamWorldRot.Yaw, CamWorldRot.Roll,
		CamRelRot.Pitch, CamRelRot.Yaw, CamRelRot.Roll,
		SocketWorld.X, SocketWorld.Y, SocketWorld.Z,
		SocketWorldRot.Pitch, SocketWorldRot.Yaw, SocketWorldRot.Roll,
		MeshWorld.X, MeshWorld.Y, MeshWorld.Z,
		ActorLoc.X, ActorLoc.Y, ActorLoc.Z,
		Velocity.X, Velocity.Y, Velocity.Z,
		ControlRot.Pitch, ControlRot.Yaw, ControlRot.Roll,
		ActorRot.Pitch, ActorRot.Yaw, ActorRot.Roll);
}

void AZP_GraceCharacter::LogBoneClipProbe()
{
	if (!FirstPersonCamera) return;
	const FVector CamPos = FirstPersonCamera->GetComponentLocation();
	const FVector CamFwd = FirstPersonCamera->GetForwardVector();

	// Static bone lists — names are the standard UE5 mannequin / Operator skeleton.
	static const TArray<FName> PlayerBones = {
		FName("head"), FName("neck_01"),
		FName("spine_05"), FName("spine_04"), FName("spine_03"),
		FName("clavicle_l"), FName("clavicle_r"),
		FName("upperarm_l"), FName("upperarm_r")
	};
	static const TArray<FName> ViewBones = {
		FName("neck_01"),
		FName("spine_05"), FName("spine_04"), FName("spine_03"),
		FName("clavicle_l"), FName("clavicle_r"),
		FName("upperarm_l"), FName("upperarm_r"),
		FName("lowerarm_l"), FName("lowerarm_r"),
		FName("hand_l"), FName("hand_r")
	};

	auto DumpMesh = [&](USkeletalMeshComponent* Skel, const TCHAR* Tag, const TArray<FName>& Bones)
	{
		if (!Skel) return;
		FString Hits;
		for (const FName& BoneName : Bones)
		{
			const int32 Idx = Skel->GetBoneIndex(BoneName);
			if (Idx == INDEX_NONE) continue;
			const FVector BonePos = Skel->GetBoneLocation(BoneName, EBoneSpaces::WorldSpace);
			const FVector Delta = BonePos - CamPos;
			const float Dist = Delta.Size();
			// Only log bones within 50cm of the camera — those are the ones
			// that could be eating POV. Anything further is irrelevant.
			if (Dist > 50.f) continue;
			const float Fwd = FVector::DotProduct(Delta, CamFwd);
			const float Lat = (Delta - CamFwd * Fwd).Size();
			Hits += FString::Printf(TEXT(" %s(d%.1f f%.1f l%.1f)"),
				*BoneName.ToString(), Dist, Fwd, Lat);
		}
		if (Hits.IsEmpty()) return; // nothing close — quieter logs
		UE_LOG(LogTemp, Warning, TEXT("[CLIP-PROBE %s %s] cam=(%.1f,%.1f,%.1f)%s"),
			*CameraProbeTag, Tag, CamPos.X, CamPos.Y, CamPos.Z, *Hits);
	};

	DumpMesh(PlayerMesh,     TEXT("PLR"),  PlayerBones);
	DumpMesh(MeleeViewMesh,  TEXT("VIEW"), ViewBones);
}

// --- Marcus appearance (CCMH body shell) ---

void AZP_GraceCharacter::SetupMarcusAppearance()
{
	if (!MarcusBody || !PlayerMesh) return;

	// Body + apparel per "Marcus" entry in CC_SaveGame.sav (CCMH_Preset_Male_01):
	// Apparel: ["Overalls_01", "", "Sneaker_02" (color variant 1), "Cap_01"].
	USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Body_Male.CCMH_Body_Male"));
	if (!Body)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] SetupMarcusAppearance — CCMH body failed to load; keeping Operator body."));
		return;
	}

	MarcusBody->SetSkeletalMesh(Body);
	MarcusBody->SetVisibility(true);

	// Marcus plays NATIVE CCMH locomotion clips (retargeted OFFLINE from the player's
	// Manny loco clips via MH_Retargeter; assets in /Game/Marcus/Anims) directly via
	// SingleNode — the SAME approach the Character Customizer demo uses, with NO live
	// RetargetPoseFromMesh node. That node flickered to ref pose every other frame
	// against our custom dual-mesh source ("shuffle"); native SingleNode playback is
	// deterministic = no shuffle. Speed-driven in the locomotion update (UpdateLocoAnim).
	MarcusBody->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MarcusIdle       = LoadObject<UAnimSequenceBase>(nullptr, TEXT("/Game/Marcus/Anims/Marcus_M_Neutral_Stand_Idle_Loop.Marcus_M_Neutral_Stand_Idle_Loop"));
	MarcusWalk       = LoadObject<UAnimSequenceBase>(nullptr, TEXT("/Game/Marcus/Anims/Marcus_M_Neutral_Walk_Loop_F.Marcus_M_Neutral_Walk_Loop_F"));
	MarcusRun        = LoadObject<UAnimSequenceBase>(nullptr, TEXT("/Game/Marcus/Anims/Marcus_M_Neutral_Run_Loop_F.Marcus_M_Neutral_Run_Loop_F"));
	MarcusCrouchIdle = LoadObject<UAnimSequenceBase>(nullptr, TEXT("/Game/Marcus/Anims/Marcus_M_Neutral_Crouch_Idle_Loop.Marcus_M_Neutral_Crouch_Idle_Loop"));
	MarcusCrouchWalk = LoadObject<UAnimSequenceBase>(nullptr, TEXT("/Game/Marcus/Anims/Marcus_M_Neutral_Crouch_Loop_F.Marcus_M_Neutral_Crouch_Loop_F"));
	if (MarcusIdle)
	{
		if (UAnimSingleNodeInstance* SNI = MarcusBody->GetSingleNodeInstance())
		{
			SNI->SetAnimationAsset(MarcusIdle, true, 1.0f);
			SNI->SetPlaying(true);
		}
	}

	// NO head-bone hide here. CCMH_Body_Male is HEADLESS (the head is the separate
	// CCMH_Head_Male on MarcusHead below), so hiding "head" on the body does nothing for the
	// body — but bone visibility PROPAGATES to leader-posed followers (the hand_l/r lesson,
	// 2026-06-20), and every vertex of the head mesh is skinned under that bone: the hide
	// collapsed the entire grab-time head invisible (2026-07-02). FP never sees the head
	// because MarcusHead itself stays SetVisibility(false) outside the grab beat.

	// Attach Marcus to the CAPSULE (clean, always-upright frame — identical to the
	// hidden locomotion CharacterMesh0). The pack AnimBP's RetargetPoseFromMesh
	// sources from its public 'Attach' var (set below to PlayerMesh) — NOT from the
	// parent component (verified in the AnimGraph: SourceMeshComponent <- Get Attach).
	// So Marcus does NOT need to be a child of PlayerMesh. Nwiro parented it there,
	// which put it in a non-upright, aim-tracking frame → the body tilted (side-plank)
	// or faced 45deg off depending on look direction. CCMH visual forward is local +Y,
	// so a -90 yaw points the body at pawn-forward. Verified live: matches the
	// known-good CharacterMesh0 frame exactly (forward + up.z=1).
	MarcusBody->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	MarcusBody->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	MarcusBody->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// Marcus (CCMH) is taller than the Operator the FP camera was tuned for, so his
	// eyes sat ~19cm above the camera → the camera was buried in his chest and looking
	// down showed only floor + feet. Scaling around the foot-level component origin
	// drops his eyes to the camera while keeping feet on the floor. Measured live:
	// camera 124cm above floor, eyes 143cm → 124/143 = 0.869.
	MarcusBody->SetRelativeScale3D(FVector(0.869f));

	// Apparel: same CCMH skeleton -> leader-pose to MarcusBody. Per Marcus save:
	//   Overalls_01 (Male upper+lower body), Sneaker_02 (Male, color variant 1), Cap_01.
	// MarcusPants stays unassigned — Overalls_01 is a full-body suit, no lower-only slot.
	if (USkeletalMesh* Overalls = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/CharacterCustomizer/Characters/CCMH/Apparel/ApparelPack_01/Male/UpperLowerBody/Overalls_01/Overalls_01.Overalls_01")))
	{
		MarcusOveralls->SetSkeletalMesh(Overalls);
		MarcusOveralls->SetLeaderPoseComponent(MarcusBody);
	}
	// Cap_01 (headwear) intentionally NOT shown on the self/FP body — it rides the
	// head bone, which sits at the FP camera, so it would fill the lens (the head is
	// hidden above for the same reason). Add it back only for a 3P / shadow body.

	// Head: CCMH_Head_Male is a SEPARATE mesh (the body asset is headless — verified 2026-07-02
	// when the grab's 3P camera showed a headless Marcus; the old HideBoneByName("head") on the
	// body was a no-op). Leader-posed like the apparel; stays hidden except during the grab beat.
	if (USkeletalMesh* Head = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Head_Male.CCMH_Head_Male")))
	{
		MarcusHead->SetSkeletalMesh(Head);
		MarcusHead->SetLeaderPoseComponent(MarcusBody);
		MarcusHead->SetVisibility(false);
	}
	// Hair + brows per the "Marcus" CC_SaveGame entry: SidePart_02 / Eyebrows_01 / Beard_Default
	// (= no beard). Both are CCMH_Skeleton skeletal meshes (verified) -> leader-pose like the head.
	if (USkeletalMesh* Hair = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/CharacterCustomizer/Characters/CCMH/Hair/Hairstyles/Side_Part_02/Side_Part_02_M.Side_Part_02_M")))
	{
		MarcusHair->SetSkeletalMesh(Hair);
		MarcusHair->SetLeaderPoseComponent(MarcusBody);
		MarcusHair->SetVisibility(false);
		// The preset's authored "Hair Tint" — Dark Brown (0.251, 0.145, 0.098).
		if (UMaterialInstanceDynamic* HairMID = MarcusHair->CreateAndSetMaterialInstanceDynamic(0))
		{
			HairMID->SetVectorParameterValue(FName(TEXT("Hair Tint")),
				FLinearColor(0.250980f, 0.145098f, 0.098039f, 1.f));
		}
	}
	if (USkeletalMesh* Brows = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/CharacterCustomizer/Characters/CCMH/Hair/Eyebrows/Eyebrows_01_Male.Eyebrows_01_Male")))
	{
		MarcusBrows->SetSkeletalMesh(Brows);
		MarcusBrows->SetLeaderPoseComponent(MarcusBody);
		MarcusBrows->SetVisibility(false);
	}
	if (USkeletalMesh* Sneakers = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/CharacterCustomizer/Characters/CCMH/Apparel/Male/Footwear/Sneaker_02/Sneaker_02.Sneaker_02")))
	{
		MarcusSneakers->SetSkeletalMesh(Sneakers);
		MarcusSneakers->SetLeaderPoseComponent(MarcusBody);
	}

	// Weapon-arm view-model SLEEVES: DISABLED 2026-06-20. This leader-posed a CCMH
	// Overalls_01 mesh to MeleeViewMesh — but MeleeViewMesh is back to the Operator
	// skeleton (SKM_Operator_Mono), so a CCMH mesh can't leader-pose it (skeleton
	// mismatch → frozen ref pose). The Operator mono view-model is already fully
	// clothed, so no separate sleeve is needed. Leave MeleeViewOveralls empty.

	// MeleeHands overlay DISABLED 2026-06-20: a separate SK_Hand_01a mesh leader-posed
	// over the gloved hand (with the glove hidden) z-fought / rendered checkered. The
	// hand is now skinned directly on the Operator mesh's own hand geometry with the
	// flat MI_HandSkin material (Kinemation setup) — one mesh, no overlay, no z-fight.
	// MeleeHands stays empty/hidden.

	// Hide the Operator visible body from the OWNER only (FP body hide). NOT
	// SetVisibility(false): that stops Kinemation's AC_FirstPersonCamera driving the
	// camera rotation, leaving the raw ~90deg-rolled FPCamera socket orientation.
	// SetOwnerNoSee keeps the mesh visible/ticking (camera drive + retarget source
	// intact, shadow preserved) while hiding it from this player's own view.
	PlayerMesh->SetOwnerNoSee(true);

	// RANGED arms-only view-model: SK_Arm + SK_Hand (bare-skin, Operator skeleton)
	// leader-posed to PlayerMesh so they ride the LIVE Kinemation aim pose — just arms
	// + gun, no body, so there's NO clothing/boots mismatch. Reskinned to Marcus's CCMH
	// skin so the hands/forearms match his body. Shown only while ranged (Tick toggle).
	{
		UMaterialInterface* MarcusSkin = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/CharacterCustomizer/Characters/CCMH/Materials/Skin/MI_Skin_Body_CCMH.MI_Skin_Body_CCMH"));
		if (USkeletalMesh* ArmMesh = LoadObject<USkeletalMesh>(nullptr,
			TEXT("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SK_Arm_01a.SK_Arm_01a")))
		{
			RangedArms->SetSkeletalMesh(ArmMesh);
			RangedArms->SetLeaderPoseComponent(PlayerMesh);
			if (MarcusSkin) RangedArms->SetMaterial(0, MarcusSkin);
		}
		if (USkeletalMesh* HandMesh = LoadObject<USkeletalMesh>(nullptr,
			TEXT("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SK_Hand_01a.SK_Hand_01a")))
		{
			RangedHands->SetSkeletalMesh(HandMesh);
			RangedHands->SetLeaderPoseComponent(PlayerMesh);
			if (MarcusSkin) RangedHands->SetMaterial(0, MarcusSkin);
		}
		// FP shirt sleeve over the bare arms so they're clothed, reskinned toward Marcus's
		// shirt (closest Operator-skeleton sleeve we can leader-pose; CCMH overalls can't).
		if (USkeletalMesh* SleeveMesh = LoadObject<USkeletalMesh>(nullptr,
			TEXT("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SK_Shirt_01a_FPP.SK_Shirt_01a_FPP")))
		{
			RangedSleeve->SetSkeletalMesh(SleeveMesh);
			RangedSleeve->SetLeaderPoseComponent(PlayerMesh);
			if (UMaterialInterface* MarcusShirt = LoadObject<UMaterialInterface>(nullptr,
				TEXT("/Game/CharacterCustomizer/Characters/CCMH/Apparel/Male/UpperBody/TShirt_01/MI_TShirt_Male.MI_TShirt_Male")))
			{
				RangedSleeve->SetMaterial(0, MarcusShirt);
			}
		}
	}

	// (Per-hand melee grip layer removed — an empty post-process AnimGraph outputs ref
	// pose and flattened the arms. Needs a pass-through graph; revisiting separately.)

	// FULL first-person body — chest, arms, legs, feet all visible. ONLY the head is
	// hidden (above), since it sits at the FP camera. The dev wants to see the whole
	// body looking down, and the arms must show for weapon/melee poses. (Reverts
	// Nwiro's spine_03 hide, which deleted the entire upper body + arms.)

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] SetupMarcusAppearance — Marcus CCMH body active (full body), retargeting from PlayerMesh."));
}

// --- Dodge ---

void AZP_GraceCharacter::PerformDodge()
{
	if (DodgeCooldownRemaining > 0.f) return;
	if (GrabPhase != EZP_GrabPhase::None) return; // can't dodge while grabbed
	if (GetCharacterMovement()->IsFalling()) return;
	if (bIsBlocking) return; // can't dodge while blocking

	FVector2D Dir2D = CurrentMoveInput;
	if (Dir2D.IsNearlyZero())
	{
		Dir2D = FVector2D(0.f, -1.f); // stationary → backstep
	}
	Dir2D.Normalize();

	// No forward dash — lunging forward on your feet is a dash, not a dodge. Block
	// ANY forward component: straight-forward AND the two forward-diagonals. Allowed
	// dodges are everything with no forward lean — pure side (Y≈0), straight back,
	// and the back-diagonals (Y<0). Checked BEFORE spending stamina so a blocked
	// forward press costs nothing.
	if (Dir2D.Y > 0.05f)
	{
		return;
	}

	// Costs stamina — and is blocked entirely if you don't have enough.
	if (GameplayComp && !GameplayComp->TryConsumeStaminaPercent(DodgeStaminaCostPercent))
	{
		return;
	}

	CancelSprintFromAction(); // the dodge has committed — break any active sprint

	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Fwd   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	FVector LaunchDir = (Fwd * Dir2D.Y) + (Right * Dir2D.X);
	LaunchDir.Normalize();

	// Probe: capture the camera state for the next ~30 frames so we can see
	// exactly where the shift happens. Tagged DODGE so grep is easy.
	CameraProbeTag = TEXT("DODGE");
	CameraProbeFramesLeft = 180;
	LogCameraProbe(TEXT("PRE-LAUNCH"));

	// LaunchCharacter sets PendingLaunchVelocity, which overrides the CMC input
	// controller's per-tick velocity-write — actually dashes. AddImpulse was
	// getting eaten by input damping (probe shows ~10 cm/s net velocity change
	// at DodgeImpulse=400). bXYOverride=true is the original Launch behavior;
	// the camera spike at the previous 700 came from velocity-spiking the
	// locomotion blend, not from the override mechanic itself.
	LaunchCharacter(LaunchDir * DodgeImpulse, /*bXYOverride*/ true, /*bZOverride*/ false);
	DodgeCooldownRemaining = DodgeCooldown;
	DodgeClearanceRemaining = DodgeClearanceWindow;
	DodgeLockRemaining = DodgeLockWindow; // locks sprint + weapon swap for the dash
	LogCameraProbe(TEXT("POST-LAUNCH"));

	// Keep the pipe GLUED to the hand through the dash. There is no pipe-fitted
	// dodge clip: the A_MeleePipe_* set has none, and FPP_sns_Dodge (used before)
	// is a sword&shield clip whose fingers don't match the pipe's fitted hand_r
	// grip, so it floated the pipe off the hand (dev report). Instead, hold the
	// seated idle grip (A_MeleePipe_Idle) so hand_r stays at the fitted pose and
	// the pipe never leaves the hand — the dash reads from body movement + the
	// dodge camera offset. (DodgeAnim left wired for a future fitted clip.)
	const bool bMeleeUp = KinemationComp
		&& KinemationComp->CurrentWeaponType == EZP_WeaponType::Melee;
	if (bMeleeUp && MeleeViewMesh && !bIsBlocking)
	{
		if (UAnimSequenceBase* IdleAnim = MeleeIdleHoldAnim.LoadSynchronous())
		{
			if (UAnimSingleNodeInstance* SNI = MeleeViewMesh->GetSingleNodeInstance())
			{
				if (SNI->GetCurrentAsset() != IdleAnim)
				{
					SNI->SetAnimationAsset(IdleAnim, true, 1.0f);
					SNI->SetPlaying(true);
				}
			}
		}
	}
}

// --- Block ---

void AZP_GraceCharacter::UpdateBlockAnimation()
{
	if (!MeleeViewMesh) return;
	if (BlockImpactLockRemaining > 0.f) return; // impact reaction owns playback
	if (BlockStartLockRemaining > 0.f) return;  // start transition owns playback

	const float Speed2D = GetVelocity().Size2D();
	const bool bShouldWalk = Speed2D > 50.f;

	UAnimSequenceBase* Target = bShouldWalk
		? BlockWalkAnim.LoadSynchronous()
		: BlockLoopAnim.LoadSynchronous();
	if (!Target) return;

	UAnimSingleNodeInstance* SNI = MeleeViewMesh->GetSingleNodeInstance();
	if (!SNI) return;

	if (SNI->GetCurrentAsset() != Target)
	{
		SNI->SetAnimationAsset(Target, /*bLoop*/ true, 1.0f);
		SNI->SetPlaying(true);
		bBlockWalkActive = bShouldWalk;
	}
}

void AZP_GraceCharacter::PlayBlockImpactAnim()
{
	if (!MeleeViewMesh) return;

	TSoftObjectPtr<UAnimSequenceBase>* Choices[3] = {
		&BlockImpact1Anim, &BlockImpact2Anim, &BlockImpact3Anim
	};
	const int32 Idx = FMath::RandRange(0, 2);
	UAnimSequenceBase* Anim = Choices[Idx]->LoadSynchronous();
	if (!Anim) return;

	if (UAnimSingleNodeInstance* SNI = MeleeViewMesh->GetSingleNodeInstance())
	{
		SNI->SetAnimationAsset(Anim, /*bLoop*/ false, 1.0f);
		SNI->SetPlaying(true);
		BlockImpactLockRemaining = Anim->GetPlayLength();
	}
}


// --- Container Close Detection ---

void AZP_GraceCharacter::CheckContainerClosed()
{
	if (!bWaitingForContainerOpen && !bContainerWasOpen) return;

	AActor* ContainerToCheck = ActiveContainerActor.IsValid() ? ActiveContainerActor.Get() : nullptr;
	if (!ContainerToCheck)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] ActiveContainerActor is null — resetting"));
		bWaitingForContainerOpen = false;
		bContainerWasOpen = false;
		return;
	}

	// Find bPlayerIsUsingActor via reflection
	FBoolProperty* UsingProp = nullptr;
	for (TFieldIterator<FBoolProperty> It(ContainerToCheck->GetClass()); It; ++It)
	{
		if (It->GetName().Contains(TEXT("bPlayerIsUsingActor")))
		{
			UsingProp = *It;
			break;
		}
	}

	if (!UsingProp)
	{
		UE_LOG(LogTemp, Error, TEXT("[ZP-BUG] bPlayerIsUsingActor NOT FOUND on %s (class: %s)"),
			*ContainerToCheck->GetName(), *ContainerToCheck->GetClass()->GetName());
		bWaitingForContainerOpen = false;
		bContainerWasOpen = false;
		ActiveContainerActor.Reset();
		ActiveBriefcaseActor.Reset();
		return;
	}

	bool bStillUsing = UsingProp->GetPropertyValue_InContainer(ContainerToCheck);

	// Phase 1: Waiting for container to OPEN (bPlayerIsUsingActor becomes true)
	if (bWaitingForContainerOpen)
	{
		ContainerOpenWaitFrames++;

		if (bStillUsing)
		{
			// Our traced container opened — use it
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Container OPENED (bPlayerIsUsingActor=true) on %s after %d frames — contents: %s"),
				*ContainerToCheck->GetName(), ContainerOpenWaitFrames, *DescribeLockerContents(ContainerToCheck));
			bWaitingForContainerOpen = false;
			bContainerWasOpen = true;
			ContainerOpenWaitFrames = 0;
		}
		else
		{
			// Moonville might have opened a DIFFERENT container (closest overlap, not our trace target).
			// Scan all overlapping actors to find which one actually opened.
			TArray<AActor*> OverlappingActors;
			GetOverlappingActors(OverlappingActors);
			for (AActor* OA : OverlappingActors)
			{
				if (!OA || OA == ContainerToCheck) continue;
				for (TFieldIterator<FBoolProperty> It(OA->GetClass()); It; ++It)
				{
					if (It->GetName().Contains(TEXT("bPlayerIsUsingActor")))
					{
						if (It->GetPropertyValue_InContainer(OA))
						{
							UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Container OPENED on DIFFERENT actor: %s (class: %s) after %d frames — switching tracking"),
								*OA->GetName(), *OA->GetClass()->GetName(), ContainerOpenWaitFrames);
							ActiveContainerActor = OA;

							// Check if the actual opened container is a briefcase
							bool bActualIsBriefcase = false;
							for (UClass* C = OA->GetClass(); C; C = C->GetSuperClass())
							{
								if (C->GetName().Contains(TEXT("Briefcase")))
								{
									bActualIsBriefcase = true;
									break;
								}
							}
							if (bActualIsBriefcase)
							{
								ActiveBriefcaseActor = OA;
								// Moonville opened a briefcase we didn't trace — load stored inventory into it
								if (UZP_BriefcaseSubsystem* BriefcaseSub = GetGameInstance()->GetSubsystem<UZP_BriefcaseSubsystem>())
								{
									UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Late-loading briefcase inventory into %s (HasData=%d)"),
										*OA->GetName(), BriefcaseSub->HasStoredData());
									BriefcaseSub->LoadIntoBriefcase(OA);
								}
							}

							bWaitingForContainerOpen = false;
							bContainerWasOpen = true;
							ContainerOpenWaitFrames = 0;
						}
						break;
					}
				}
				if (bContainerWasOpen) break;
			}

			// Timeout: if no container opened after 30 frames (~0.5s), the container was instant-loot
			// (no lingering UI, bPlayerIsUsingActor never set true). Run unequip check and reset.
			if (bWaitingForContainerOpen && ContainerOpenWaitFrames >= 30 && ActiveContainerActor.IsValid())
		{
			// Enriched (session 63): the timeout means E targeted this container
			// but Moonville never opened it — the refusal case. Dump everything.
			UE_LOG(LogTemp, Error, TEXT("[ZP-BUG] CONTAINER REFUSED TO OPEN: %s — contents: %s | Moonville: %s"),
				*ActiveContainerActor->GetName(),
				*DescribeLockerContents(ActiveContainerActor.Get()),
				*GetMoonvilleInteractionStateString());
		}
		if (bWaitingForContainerOpen && ContainerOpenWaitFrames >= 30)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Phase 1 TIMEOUT after %d frames — container has no lingering UI. Running UnequipMissingWeapon."), ContainerOpenWaitFrames);
				bWaitingForContainerOpen = false;
				bContainerWasOpen = false;
				ContainerOpenWaitFrames = 0;
				ActiveContainerActor.Reset();
				ActiveBriefcaseActor.Reset();
				UnequipMissingWeapon();
			}
		}
		return;
	}

	// Phase 2: Container is open, waiting for it to CLOSE (bPlayerIsUsingActor becomes false)
	// Re-read the property on the current ActiveContainerActor (may have been switched in Phase 1)
	ContainerToCheck = ActiveContainerActor.IsValid() ? ActiveContainerActor.Get() : nullptr;
	if (!ContainerToCheck)
	{
		bContainerWasOpen = false;
		return;
	}
	UsingProp = nullptr;
	for (TFieldIterator<FBoolProperty> It(ContainerToCheck->GetClass()); It; ++It)
	{
		if (It->GetName().Contains(TEXT("bPlayerIsUsingActor")))
		{
			UsingProp = *It;
			break;
		}
	}
	if (UsingProp)
	{
		bStillUsing = UsingProp->GetPropertyValue_InContainer(ContainerToCheck);
	}

	// bPlayerIsUsingActor is cleared by a Moonville close event that NEVER
	// fires in standalone (TICKET-016) — waiting on it left lockers flagged
	// in-use forever, refusing to reopen ("can't loot the item again").
	// Close signal = the menu WIDGET actually on screen. (The interaction
	// component's bInventoryMenuOpen flag tracks the Tab menu, NOT container
	// menus — keying on it cleared the flag 13ms after opening and wrecked
	// the open state. Log-proven, session 63.)
	bool bMenuStillOpen = false;
	{
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), Widgets, UUserWidget::StaticClass(), false);
		for (UUserWidget* W : Widgets)
		{
			if (!W || !W->IsInViewport() || !W->IsVisible())
			{
				continue;
			}
			const FString WClassName = W->GetClass()->GetName();
			if (WClassName.Contains(TEXT("ContainerMenu")) || WClassName.Contains(TEXT("InventoryMenu")))
			{
				bMenuStillOpen = true;
				break;
			}
		}
	}
	if (bStillUsing && bMenuStillOpen)
	{
		// Instant objective-deposit autocomplete: the moment an OPEN objective/deposit container holds
		// every required item, complete it (ProcessObjectiveDeposit sets the flag, kills the beacon
		// light, and permanently locks interaction) and auto-close the menu — so the player sees it go
		// dark + un-interactable = "done, no longer relevant". Otherwise it's still in use: leave open.
		bool bOpenObjectiveContainer = false;
		for (UClass* C = ContainerToCheck->GetClass(); C; C = C->GetSuperClass())
		{
			if (C->GetName().Contains(TEXT("ObjectiveContainer")))
			{
				bOpenObjectiveContainer = true;
				break;
			}
		}
		if (!(bOpenObjectiveContainer && UZP_ObjectiveDepositLibrary::TryAutoCompleteObjectiveContainer(ContainerToCheck)))
		{
			return; // still open, not complete yet
		}

		// Just auto-completed → close the Moonville menu (same client-side ToggleInventoryMenu the Tab
		// key / loot-all uses — the only close path proven reliable in standalone), then fall through
		// to the closed-handling below to reset watchdog state.
		if (MoonvilleInventoryComp)
		{
			if (UFunction* ToggleFunc = MoonvilleInventoryComp->FindFunction(FName("ToggleInventoryMenu")))
			{
				struct { APlayerController* PlayerController; } CloseParams;
				CloseParams.PlayerController = Cast<APlayerController>(GetController());
				MoonvilleInventoryComp->ProcessEvent(ToggleFunc, &CloseParams);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveContainer %s auto-completed on deposit — menu closed"),
			*ContainerToCheck->GetName());
	}

	// Container is now closed. Clear a stuck in-use flag so it can reopen.
	if (bStillUsing && UsingProp)
	{
		UsingProp->SetPropertyValue_InContainer(ContainerToCheck, false);
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Cleared stuck bPlayerIsUsingActor on %s (menu closed, flag was still true)"),
			*ContainerToCheck->GetName());
	}
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Container CLOSED on %s — contents: %s"),
		*ContainerToCheck->GetName(), *DescribeLockerContents(ContainerToCheck));
	bContainerWasOpen = false;

	// Check if loot locker is now empty — disable interaction if so.
	// NEVER the briefcase: it's the player's persistent storage chest and
	// must stay interactable for the whole game (session 63: closing it
	// empty permanently disabled the player's own storage).
	bool bClosedBriefcase = false;
	for (UClass* C = ContainerToCheck->GetClass(); C; C = C->GetSuperClass())
	{
		if (C->GetName().Contains(TEXT("Briefcase")))
		{
			bClosedBriefcase = true;
			break;
		}
	}
	// NEVER an objective / deposit container (BP_ObjectiveContainer, child of
	// BP_ItemContainer_Horror): it is empty BY DESIGN before the player deposits
	// anything (the FIRST close, before any items go in, is the trap). The empty
	// check below would call DisableLockerInteraction on that first close,
	// permanently NoCollision-ing its interaction spheres (LootedEmptyLockers is
	// never read back) — so it opens once and can never reopen. Deposited items
	// are intentionally LEFT in the container (no consume), and the objective
	// container does its OWN intentional lock on completion via
	// UZP_ObjectiveDepositLibrary::ProcessObjectiveDeposit — so this watchdog must
	// stay hands-off. Matched by class name like the briefcase.
	bool bClosedObjectiveContainer = false;
	for (UClass* C = ContainerToCheck->GetClass(); C; C = C->GetSuperClass())
	{
		if (C->GetName().Contains(TEXT("ObjectiveContainer")))
		{
			bClosedObjectiveContainer = true;
			break;
		}
	}
	if (!bClosedBriefcase && !bClosedObjectiveContainer && IsLockerInventoryEmpty(ContainerToCheck))
	{
		DisableLockerInteraction(ContainerToCheck);
	}

	// Sync briefcase data back to subsystem
	if (ActiveBriefcaseActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Syncing briefcase inventory to subsystem from %s"),
			*ActiveBriefcaseActor->GetName());
		if (UZP_BriefcaseSubsystem* BriefcaseSub = GetGameInstance()->GetSubsystem<UZP_BriefcaseSubsystem>())
		{
			BriefcaseSub->SaveFromBriefcase(ActiveBriefcaseActor.Get());
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Briefcase data saved. HasData=%d"), BriefcaseSub->HasStoredData());
		}
		ActiveBriefcaseActor.Reset();
	}

	ActiveContainerActor.Reset();

	// Check if equipped weapon was transferred out of inventory (BUG-001)
	UnequipMissingWeapon();
}

void AZP_GraceCharacter::UnequipMissingWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] UnequipMissingWeapon called. KinemationComp=%d, MoonvilleInventoryComp=%d, ActiveWeapon=%s"),
		KinemationComp != nullptr, MoonvilleInventoryComp != nullptr,
		ActiveWeapon ? *ActiveWeapon->GetName() : TEXT("NULL"));

	if (!KinemationComp || !MoonvilleInventoryComp || !ActiveWeapon) return;

	// Check if the currently equipped weapon's class still matches any shortcut slot.
	const TSubclassOf<AActor> CurrentWeaponClass = ActiveWeapon->GetClass();
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Current weapon class: %s"), *CurrentWeaponClass->GetName());

	bool bWeaponStillInSlots = false;

	// Check the GRID (ItemSlots) — the source of truth — NOT the shortcut bar. The shortcut bar can keep a
	// STALE ref to the weapon after it's moved to a briefcase (Moonville doesn't always clear it), which let
	// the old shortcut-based check think the weapon was still owned and leave it equipped (the regressed bug).
	if (FArrayProperty* GridProp = CastField<FArrayProperty>(MoonvilleInventoryComp->GetClass()->FindPropertyByName(FName("ItemSlots"))))
	{
		if (FStructProperty* StructInner = CastField<FStructProperty>(GridProp->Inner))
		{
			FObjectProperty* ItemProp = nullptr;
			for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
			{
				if (It->GetName().Contains(TEXT("Item_"))) { ItemProp = CastField<FObjectProperty>(*It); break; }
			}
			if (ItemProp)
			{
				FScriptArrayHelper Helper(GridProp, GridProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
				for (int32 i = 0; i < Helper.Num() && !bWeaponStillInSlots; ++i)
				{
					UObject* ItemDA = ItemProp->GetObjectPropertyValue(ItemProp->ContainerPtrToValuePtr<void>(Helper.GetRawPtr(i)));
					if (ItemDA && GetWeaponClassFromItem(ItemDA) == CurrentWeaponClass)
					{
						bWeaponStillInSlots = true;
						UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Equipped weapon still in the GRID (ItemSlots[%d]) — keep equipped"), i);
					}
				}
			}
		}
	}

	if (!bWeaponStillInSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Weapon NOT found in any slot — UNEQUIPPING"));

		// Unequip all via Moonville
		UFunction* UnequipAllFunc = MoonvilleInventoryComp->FindFunction(FName("UnequipAllItems"));
		if (UnequipAllFunc)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Calling UnequipAllItems"));
			MoonvilleInventoryComp->ProcessEvent(UnequipAllFunc, nullptr);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ZP-BUG] UnequipAllItems function NOT FOUND on inventory comp!"));
		}

		// Clear weapon from KinemationComp (direct C++ call — UnequipWeapon handles full cleanup)
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Calling KinemationComp->UnequipWeapon()"));
		KinemationComp->UnequipWeapon();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Weapon still in inventory — no action needed"));
	}
}

// ===========================================================================
// Loot Locker Filtering
// ===========================================================================

void AZP_GraceCharacter::FilterLockerAmmo(AActor* LockerActor)
{
	if (!LockerActor || !MoonvilleInventoryComp) return;

	// Find the component that actually holds the items (has ItemSlots)
	UActorComponent* LockerInvComp = FindItemSlotsComponent(LockerActor);
	if (!LockerInvComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootLocker] No ItemSlots component found on %s"), *LockerActor->GetName());
		return;
	}

	// Find ItemSlots array on locker
	FProperty* SlotsProp = LockerInvComp->GetClass()->FindPropertyByName(FName("ItemSlots"));
	if (!SlotsProp) return;
	FArrayProperty* ArrayProp = CastField<FArrayProperty>(SlotsProp);
	if (!ArrayProp) return;
	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(LockerInvComp));
	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return;

	// Find the Item_ and Amount fields in the slot struct.
	FObjectProperty* ItemObjProp = nullptr;
	FIntProperty* AmountIntProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (!ItemObjProp && It->GetName().Contains(TEXT("Item_")))    ItemObjProp = CastField<FObjectProperty>(*It);
		if (!AmountIntProp && It->GetName().Contains(TEXT("Amount")))  AmountIntProp = CastField<FIntProperty>(*It);
	}
	if (!ItemObjProp) return;

	// Aggregate the EXACT amount of each unowned-ammo DA across all stacks. The old code passed
	// AmountToRemove = 999, but Moonville REJECTS a removal when the amount exceeds what's actually
	// there (the same trap as the ammo-pickup runaway) — so the ammo was never removed and kept
	// showing. Removing the exact total is what RemoveInventoryAmmo already does correctly.
	TMap<UObject*, int32> AmmoToRemove;
	for (int32 i = 0; i < ArrayHelper.Num(); i++)
	{
		void* ElementData = ArrayHelper.GetRawPtr(i);
		UObject* SlotItem = ItemObjProp->GetObjectPropertyValue(ItemObjProp->ContainerPtrToValuePtr<void>(ElementData));
		if (!SlotItem) continue;

		const FString ItemName = SlotItem->GetName();
		if (!ItemName.Contains(TEXT("Ammo"))) continue;   // only filter ammo items
		if (PlayerHasWeaponForAmmo(ItemName)) continue;   // keep ammo the player can actually use

		const int32 SlotAmount = AmountIntProp
			? AmountIntProp->GetPropertyValue(AmountIntProp->ContainerPtrToValuePtr<void>(ElementData))
			: 1;
		AmmoToRemove.FindOrAdd(SlotItem) += FMath::Max(SlotAmount, 1);
		UE_LOG(LogTemp, Log, TEXT("[LootLocker] Filtering out %s x%d — player has no matching weapon"), *ItemName, FMath::Max(SlotAmount, 1));
	}

	if (AmmoToRemove.Num() == 0) return;

	// Remove filtered ammo via Moonville API
	UFunction* RemoveFunc = LockerInvComp->FindFunction(FName("RemoveItemByDataAsset"));
	if (!RemoveFunc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootLocker] RemoveItemByDataAsset not found on locker inv comp"));
		return;
	}

	for (const TPair<UObject*, int32>& P : AmmoToRemove)
	{
		struct { UObject* ItemDataAsset; int32 AmountToRemove; } Params;
		Params.ItemDataAsset = P.Key;
		Params.AmountToRemove = P.Value; // EXACT total — over-removal is silently rejected by Moonville
		LockerInvComp->ProcessEvent(RemoveFunc, &Params);
		UE_LOG(LogTemp, Log, TEXT("[LootLocker] Removed %s x%d from locker %s"), *P.Key->GetName(), P.Value, *LockerActor->GetName());
	}
}

bool AZP_GraceCharacter::PlayerHasWeaponForAmmo(const FString& AmmoItemName)
{
	if (!MoonvilleInventoryComp) return false;

	// Ammo→Weapon mapping: ammo pattern → weapon patterns that consume it
	struct FAmmoWeaponMapping
	{
		const TCHAR* AmmoPattern;
		TArray<FString> WeaponPatterns;
	};

	static const FAmmoWeaponMapping Mappings[] = {
		{ TEXT("9mm"),      { TEXT("Pistol"), TEXT("Viper") } },
		{ TEXT("Buckshot"), { TEXT("Shotgun"), TEXT("Herrington"), TEXT("SRM") } },
		{ TEXT("556"),      { TEXT("Rifle"), TEXT("AK"), TEXT("TR15") } },
	};

	// Determine which weapon patterns to look for
	TArray<FString> RequiredWeaponPatterns;
	for (const auto& M : Mappings)
	{
		if (AmmoItemName.Contains(M.AmmoPattern))
		{
			RequiredWeaponPatterns = M.WeaponPatterns;
			break;
		}
	}
	if (RequiredWeaponPatterns.Num() == 0)
	{
		// Unknown ammo type — don't filter it out
		return true;
	}

	// Check player's ItemSlots and ShortcutSlots for any matching weapon
	auto CheckSlotArray = [&](const FName& ArrayName) -> bool
	{
		FProperty* SlotsProp = MoonvilleInventoryComp->GetClass()->FindPropertyByName(ArrayName);
		if (!SlotsProp) return false;
		FArrayProperty* ArrProp = CastField<FArrayProperty>(SlotsProp);
		if (!ArrProp) return false;
		FScriptArrayHelper ArrHelper(ArrProp, SlotsProp->ContainerPtrToValuePtr<void>(MoonvilleInventoryComp));
		FStructProperty* StructInner = CastField<FStructProperty>(ArrProp->Inner);
		if (!StructInner) return false;

		FObjectProperty* ItemObjProp = nullptr;
		for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
		{
			if (It->GetName().Contains(TEXT("Item_")))
			{
				ItemObjProp = CastField<FObjectProperty>(*It);
				break;
			}
		}
		if (!ItemObjProp) return false;

		for (int32 i = 0; i < ArrHelper.Num(); i++)
		{
			void* ElementData = ArrHelper.GetRawPtr(i);
			UObject* SlotItem = ItemObjProp->GetObjectPropertyValue(ItemObjProp->ContainerPtrToValuePtr<void>(ElementData));
			if (!SlotItem) continue;

			FString SlotItemName = SlotItem->GetName();
			for (const FString& Pattern : RequiredWeaponPatterns)
			{
				if (SlotItemName.Contains(Pattern))
				{
					return true;
				}
			}
		}
		return false;
	};

	return CheckSlotArray(FName("ItemSlots")) || CheckSlotArray(FName("ShortcutSlots"));
}

bool AZP_GraceCharacter::IsLockerInventoryEmpty(AActor* LockerActor)
{
	if (!LockerActor) return true;

	// Find the component that actually holds the items. If none exists we
	// must NOT call it empty — "can't read it" used to disable item-bearing
	// lockers forever (session 63).
	UActorComponent* LockerInvComp = FindItemSlotsComponent(LockerActor);
	if (!LockerInvComp) return false;

	// Check ItemSlots for any non-null items
	FProperty* SlotsProp = LockerInvComp->GetClass()->FindPropertyByName(FName("ItemSlots"));
	if (!SlotsProp) return true;
	FArrayProperty* ArrayProp = CastField<FArrayProperty>(SlotsProp);
	if (!ArrayProp) return true;
	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(LockerInvComp));
	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return true;

	FObjectProperty* ItemObjProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("Item_")))
		{
			ItemObjProp = CastField<FObjectProperty>(*It);
			break;
		}
	}
	if (!ItemObjProp) return true;

	for (int32 i = 0; i < ArrayHelper.Num(); i++)
	{
		void* ElementData = ArrayHelper.GetRawPtr(i);
		UObject* SlotItem = ItemObjProp->GetObjectPropertyValue(ItemObjProp->ContainerPtrToValuePtr<void>(ElementData));
		if (SlotItem)
		{
			return false; // Found an item — not empty
		}
	}

	return true;
}

void AZP_GraceCharacter::DisableLockerInteraction(AActor* LockerActor)
{
	if (!LockerActor) return;

	// Disable all sphere colliders on the locker (Moonville's InteractionArea)
	TArray<USphereComponent*> Spheres;
	LockerActor->GetComponents(Spheres);
	for (USphereComponent* S : Spheres)
	{
		S->SetGenerateOverlapEvents(false);
		S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogTemp, Log, TEXT("[LootLocker] Disabled interaction sphere %s on %s"), *S->GetName(), *LockerActor->GetName());
	}

	LootedEmptyLockers.Add(FName(*LockerActor->GetName()));
	UE_LOG(LogTemp, Log, TEXT("[LootLocker] Locker %s marked as looted and empty"), *LockerActor->GetName());
}

// ===========================================================================
// Grab / Struggle (zombie grapple — Docs/Plan_GrabStruggle.md)
// ===========================================================================

void AZP_GraceCharacter::UpdateInputDeviceTracking()
{
	// Lightweight last-device poll — no CommonInput module dependency (Build.cs rules). A small
	// fixed set of pad buttons/axes flips to gamepad; mouse activity or movement keys flip back.
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) { return; }

	static const FKey PadButtons[] = {
		EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_FaceButton_Right,
		EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top,
		EKeys::Gamepad_LeftShoulder, EKeys::Gamepad_RightShoulder,
		EKeys::Gamepad_LeftTrigger, EKeys::Gamepad_RightTrigger,
		EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Down,
		EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right,
		EKeys::Gamepad_LeftThumbstick, EKeys::Gamepad_RightThumbstick };
	for (const FKey& K : PadButtons)
	{
		if (PC->WasInputKeyJustPressed(K)) { bLastInputGamepad = true; return; }
	}
	static const FKey PadAxes[] = {
		EKeys::Gamepad_LeftX, EKeys::Gamepad_LeftY, EKeys::Gamepad_RightX, EKeys::Gamepad_RightY };
	for (const FKey& K : PadAxes)
	{
		if (FMath::Abs(PC->GetInputAnalogKeyState(K)) > 0.25f) { bLastInputGamepad = true; return; }
	}

	float MouseDX = 0.f, MouseDY = 0.f;
	PC->GetInputMouseDelta(MouseDX, MouseDY);
	static const FKey KBMKeys[] = {
		EKeys::LeftMouseButton, EKeys::RightMouseButton,
		EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::E, EKeys::SpaceBar };
	bool bKBM = (FMath::Abs(MouseDX) > 0.1f || FMath::Abs(MouseDY) > 0.1f);
	for (const FKey& K : KBMKeys)
	{
		if (bKBM) { break; }
		bKBM = PC->WasInputKeyJustPressed(K);
	}
	if (bKBM) { bLastInputGamepad = false; }
}

void AZP_GraceCharacter::LoadGrabAnims()
{
	if (bGrabAnimsLoaded) { return; }
	bGrabAnimsLoaded = true;
	// Lazy-load (LoadAnimDefaults pattern — never hard-reference retargeted clips from a ctor).
	auto L = [](const TCHAR* P) { return LoadObject<UAnimSequenceBase>(nullptr, P); };
	GrabAnimEntry       = L(TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabEntry.A_Marcus_GrabEntry"));
	GrabAnimMunch       = L(TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabMunch.A_Marcus_GrabMunch"));
	GrabAnimWrestle     = L(TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabWrestle.A_Marcus_GrabWrestle"));
	GrabAnimKick        = L(TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabKick.A_Marcus_GrabKick"));
	GrabAnimPush        = L(TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabPush.A_Marcus_GrabPush"));
	GrabAnimKnockdownFP = L(TEXT("/Game/Marcus/GrabAnims/A_Marcus_KnockdownFP.A_Marcus_KnockdownFP"));
	GrabAnimGetUpBack   = L(TEXT("/Game/Marcus/GrabAnims/A_Marcus_GetUpBack.A_Marcus_GetUpBack"));
	UE_LOG(LogTemp, Log, TEXT("[Grab] anims loaded: entry=%d munch=%d wrestle=%d kick=%d push=%d knockdown=%d getup=%d"),
		GrabAnimEntry != nullptr, GrabAnimMunch != nullptr, GrabAnimWrestle != nullptr,
		GrabAnimKick != nullptr, GrabAnimPush != nullptr, GrabAnimKnockdownFP != nullptr, GrabAnimGetUpBack != nullptr);
}

EZP_GrabAttemptResult AZP_GraceCharacter::TryBeginGrab(AActor* Grabber)
{
	if (!Grabber || GrabPhase != EZP_GrabPhase::None) { return EZP_GrabAttemptResult::Unavailable; }
	if (HealthComp && HealthComp->bIsDead) { return EZP_GrabAttemptResult::Unavailable; }
	if (bOnLadder || bInventoryMenuOpen || bMapOpen || IsModalMenuOpen()) { return EZP_GrabAttemptResult::Unavailable; }
	if (GrabImmunityRemaining > 0.f) { return EZP_GrabAttemptResult::Unavailable; }
	if (DodgeLockRemaining > 0.f) { return EZP_GrabAttemptResult::Unavailable; } // mid-dodge = the dodge evaded the latch
	if (bIsBlocking) { return EZP_GrabAttemptResult::Deflected; } // the guard turns the grab away

	LoadGrabAnims();

	GrabberActor = Grabber;
	CancelSprintFromAction();
	bBlockWanted = false; // a held RMB must not re-raise the guard under the grab
	if (bIsCrouched) { UnCrouch(); }

	// Stow the weapon (the ladder recipe) — re-equipped in EndGrab.
	if (KinemationComp && KinemationComp->ActiveWeapon)
	{
		PreGrabWeaponClass = KinemationComp->ActiveWeapon->GetClass();
		KinemationComp->UnequipWeapon();
	}
	else
	{
		PreGrabWeaponClass = nullptr;
	}

	// Freeze + snap to face the grabber. Control rotation is set ONCE and never moves during the
	// grab (look input is gated), so the FP view direction on return has no snap.
	FVector ToGrabber = Grabber->GetActorLocation() - GetActorLocation();
	ToGrabber.Z = 0.f;
	if (!ToGrabber.IsNearlyZero())
	{
		const float Yaw = ToGrabber.Rotation().Yaw;
		if (Controller) { Controller->SetControlRotation(FRotator(0.f, Yaw, 0.f)); }
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->StopMovementImmediately();
		CMC->SetMovementMode(MOVE_None);
	}
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->IgnoreActorWhenMoving(Grabber, true); // grabber ignores us from its own side
	}

	// The 3P camera is about to see the body — show the head + Marcus's authored hair/brows
	// (all separate leader-posed meshes, hidden in FP because they sit at the camera).
	if (MarcusHead) { MarcusHead->SetVisibility(true); }
	if (MarcusHair) { MarcusHair->SetVisibility(true); }
	if (MarcusBrows) { MarcusBrows->SetVisibility(true); }

	// DIM the flashlight for the duration (full-off was an overcorrection — a pitch-dark room
	// went black for the whole grapple). GrabFlashlightDimMul drops beam+fill to a silhouette
	// read; original intensities restored in EndGrab. Toggling is input-gated mid-grab.
	if (bFlashlightOn)
	{
		if (FlashlightComp)
		{
			PreGrabFlashlightIntensity = FlashlightComp->Intensity;
			FlashlightComp->SetIntensity(PreGrabFlashlightIntensity * GrabFlashlightDimMul);
		}
		if (FlashlightFillComp)
		{
			PreGrabFlashlightFillIntensity = FlashlightFillComp->Intensity;
			FlashlightFillComp->SetIntensity(PreGrabFlashlightFillIntensity * GrabFlashlightDimMul);
		}
	}

	// Mash prompt (device-matched glyph) + held damage vignette.
	bGrabPromptGamepad = bLastInputGamepad;
	if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController()))
	{
		if (PC->HUDWidget)
		{
			PC->HUDWidget->ShowGrabPrompt(GrabPromptText, bGrabPromptGamepad);
			PC->HUDWidget->SetDamageVignetteHold(GrabVignetteHold);
		}
	}

	EscapeProgress = 0.f;
	StruggleTimeRemaining = StruggleTimeLimit;
	GrabHeldTime = 0.f;
	GrabNextTickIn = 1.f;
	bGrabEscapeHeld = false;
	SetGrabPhase(EZP_GrabPhase::Entry);
	UE_LOG(LogTemp, Warning, TEXT("[Grab] LATCHED by %s"), *Grabber->GetName());
	return EZP_GrabAttemptResult::Grabbed;
}

void AZP_GraceCharacter::AbortGrab()
{
	EndGrab(/*bAborted*/true);
}

void AZP_GraceCharacter::SetGrabPhase(EZP_GrabPhase NewPhase)
{
	GrabPhase = NewPhase;
	switch (NewPhase)
	{
	case EZP_GrabPhase::Entry:
		PlayGrabClipOnBody(GrabAnimEntry, /*bLoop*/false);
		GrabPhaseTimeRemaining = GrabAnimEntry ? GrabAnimEntry->GetPlayLength() : 0.6f;
		break;

	case EZP_GrabPhase::Munch:
		PlayGrabClipOnBody(GrabAnimMunch, /*bLoop*/true);
		NotifyGrabberPhase(EZP_GrabPhase::Munch);
		break;

	case EZP_GrabPhase::Wrestle:
		PlayGrabClipOnBody(GrabAnimWrestle, /*bLoop*/true);
		NotifyGrabberPhase(EZP_GrabPhase::Wrestle);
		break;

	case EZP_GrabPhase::EscapeKick:
	case EZP_GrabPhase::EscapePush:
	{
		UAnimSequenceBase* Clip = (NewPhase == EZP_GrabPhase::EscapeKick) ? GrabAnimKick : GrabAnimPush;
		PlayGrabClipOnBody(Clip, /*bLoop*/false);
		GrabPhaseTimeRemaining = Clip ? Clip->GetPlayLength() : 2.3f;
		GrabImmunityRemaining = PostEscapeGrabImmunity; // anti-chain-grab window
		NotifyGrabberPhase(NewPhase); // grabber: paired reaction + knockback + stun
		if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController()))
		{
			if (PC->HUDWidget)
			{
				PC->HUDWidget->HideGrabPrompt();
				PC->HUDWidget->SetDamageVignetteHold(0.f);
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[Grab] ESCAPED (%s)"),
			NewPhase == EZP_GrabPhase::EscapeKick ? TEXT("kick") : TEXT("push"));
		break;
	}

	case EZP_GrabPhase::FailKnockdown:
	{
		NotifyGrabberPhase(EZP_GrabPhase::FailKnockdown); // grabber: authored shove-down, then backs off
		if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController()))
		{
			if (PC->HUDWidget)
			{
				PC->HUDWidget->HideGrabPrompt();
				PC->HUDWidget->SetDamageVignetteHold(0.f);
			}
		}
		// Camera returns to 1P (GrabCamWeight target flips to 0) and RIDES THE FALL: the FPP
		// knockdown plays on the hidden Mesh, the full-body copy drives PlayerMesh, and the
		// camera is socketed to PlayerMesh's FPCamera — the exact ladder mechanism.
		if (MarcusBody)
		{
			MarcusBody->SetVisibility(false, /*bPropagateToChildren*/true); // hides head+apparel too — no double body on the floor
		}
		if (PlayerMesh)
		{
			if (UZP_GracePlayerAnimInstance* AnimInst = Cast<UZP_GracePlayerAnimInstance>(PlayerMesh->GetAnimInstance()))
			{
				AnimInst->bCopyAllBones = true;
			}
		}
		PlayGrabClipOnFPRig(GrabAnimKnockdownFP);
		GrabPhaseTimeRemaining = GrabAnimKnockdownFP ? GrabAnimKnockdownFP->GetPlayLength() : 3.87f;
		GrabImmunityRemaining = FMath::Max(GrabImmunityRemaining, PostEscapeGrabImmunity); // no grabs off the floor
		UE_LOG(LogTemp, Warning, TEXT("[Grab] STRUGGLE FAILED — knockdown"));
		break;
	}

	case EZP_GrabPhase::GetUp:
		PlayGrabClipOnFPRig(GrabAnimGetUpBack);
		GrabPhaseTimeRemaining = GrabAnimGetUpBack ? GrabAnimGetUpBack->GetPlayLength() : 1.5f;
		break;

	case EZP_GrabPhase::None:
	default:
		break;
	}
}

void AZP_GraceCharacter::UpdateGrab(float DeltaTime)
{
	if (GrabImmunityRemaining > 0.f)
	{
		GrabImmunityRemaining = FMath::Max(0.f, GrabImmunityRemaining - DeltaTime);
	}

	// Camera blend weight: 1 = full over-the-shoulder frame. Blends out over the tail of the
	// escape reaction, or immediately at knockdown/abort — CalcCamera consumes it every frame,
	// so every exit path returns to FP smoothly.
	const bool bWants3P =
		GrabPhase == EZP_GrabPhase::Entry ||
		GrabPhase == EZP_GrabPhase::Munch ||
		GrabPhase == EZP_GrabPhase::Wrestle ||
		((GrabPhase == EZP_GrabPhase::EscapeKick || GrabPhase == EZP_GrabPhase::EscapePush)
			&& GrabPhaseTimeRemaining > GrabCamBlendOut);
	const float TargetW = bWants3P ? 1.f : 0.f;
	const float BlendTime = (TargetW > GrabCamWeight) ? GrabCamBlendIn : GrabCamBlendOut;
	GrabCamWeight = FMath::FInterpConstantTo(GrabCamWeight, TargetW, DeltaTime,
		(BlendTime > KINDA_SMALL_NUMBER) ? (1.f / BlendTime) : 1000.f);

	if (GrabPhase == EZP_GrabPhase::None) { return; }

	switch (GrabPhase)
	{
	case EZP_GrabPhase::Entry:
		GrabPhaseTimeRemaining -= DeltaTime;
		if (GrabPhaseTimeRemaining <= 0.f)
		{
			SetGrabPhase(EZP_GrabPhase::Munch);
		}
		break;

	case EZP_GrabPhase::Munch:
	case EZP_GrabPhase::Wrestle:
	{
		StruggleTimeRemaining -= DeltaTime;
		GrabHeldTime += DeltaTime;

		// The prompt glyph follows the device the player is actually using (KBM <-> pad).
		if (bGrabPromptGamepad != bLastInputGamepad)
		{
			bGrabPromptGamepad = bLastInputGamepad;
			if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController()))
			{
				if (PC->HUDWidget) { PC->HUDWidget->ShowGrabPrompt(GrabPromptText, bGrabPromptGamepad); }
			}
		}

		// Meter: Hold accessibility mode fills while attack is held; Tap mode decays between presses.
		if (bEscapeHoldMode)
		{
			if (bGrabEscapeHeld) { EscapeProgress += HoldFillPerSecond * DeltaTime; }
		}
		else
		{
			EscapeProgress = FMath::Max(0.f, EscapeProgress - MashDecayPerSecond * DeltaTime);
		}

		// Ticking damage — GrabTickDamagePerSecond lands once per second HELD (munch AND wrestle),
		// direct through HealthComp (fall-damage precedent: no block logic, no camera flinch; the
		// HUD vignette pulses via OnHealthChanged). By design: fastest escape (GrabMinTrappedTime)
		// = half a normal attack; riding out the full struggle = one full attack, then the fall.
		GrabNextTickIn -= DeltaTime;
		if (GrabNextTickIn <= 0.f)
		{
			GrabNextTickIn += 1.f;
			if (HealthComp)
			{
				HealthComp->ApplyDamage(GrabTickDamagePerSecond);
				if (HealthComp->bIsDead) { return; } // HandleDeath already tore the grab down
			}
		}

		// Escape is gated by the minimum trapped time — the meter can be full earlier; the
		// break fires the moment the gate opens.
		if (EscapeProgress >= 1.f && GrabHeldTime >= GrabMinTrappedTime)
		{
			SetGrabPhase(FMath::RandBool() ? EZP_GrabPhase::EscapeKick : EZP_GrabPhase::EscapePush);
		}
		else if (StruggleTimeRemaining <= 0.f)
		{
			// FAIL: ticks already totaled a full attack; FailDamageChunk is optional extra on top.
			if (FailDamageChunk > 0.f && HealthComp)
			{
				HealthComp->ApplyDamage(FailDamageChunk);
				if (HealthComp->bIsDead) { return; }
			}
			SetGrabPhase(EZP_GrabPhase::FailKnockdown);
		}
		break;
	}

	case EZP_GrabPhase::EscapeKick:
	case EZP_GrabPhase::EscapePush:
		GrabPhaseTimeRemaining -= DeltaTime;
		if (GrabPhaseTimeRemaining <= 0.f)
		{
			EndGrab(/*bAborted*/false);
		}
		break;

	case EZP_GrabPhase::FailKnockdown:
		GrabPhaseTimeRemaining -= DeltaTime;
		if (GrabPhaseTimeRemaining <= 0.f)
		{
			SetGrabPhase(EZP_GrabPhase::GetUp);
		}
		break;

	case EZP_GrabPhase::GetUp:
		GrabPhaseTimeRemaining -= DeltaTime;
		if (GrabPhaseTimeRemaining <= 0.f)
		{
			EndGrab(/*bAborted*/false);
		}
		break;

	default:
		break;
	}
}

void AZP_GraceCharacter::GrabMashPressed()
{
	if (GrabPhase == EZP_GrabPhase::Munch)
	{
		// First press: the struggle engages — both bodies switch to the wrestle pair.
		if (!bEscapeHoldMode) { EscapeProgress += MashGainPerPress; }
		SetGrabPhase(EZP_GrabPhase::Wrestle);
	}
	else if (GrabPhase == EZP_GrabPhase::Wrestle && !bEscapeHoldMode)
	{
		EscapeProgress = FMath::Min(EscapeProgress + MashGainPerPress, 1.5f);
	}
}

void AZP_GraceCharacter::EndGrab(bool bAborted)
{
	if (GrabPhase == EZP_GrabPhase::None) { return; }
	GrabPhase = EZP_GrabPhase::None;
	bGrabEscapeHeld = false;

	// Aborts (grabber died / grab broken by damage / player died) must tell the grabber —
	// clean exits already notified it via their outcome phase.
	if (bAborted)
	{
		NotifyGrabberPhase(EZP_GrabPhase::None);
	}

	// Movement + collision + rotation back (the ladder-exit recipe).
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		if (CMC->MovementMode == MOVE_None) { CMC->SetMovementMode(MOVE_Walking); }
	}
	bUseControllerRotationYaw = true;
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		if (AActor* G = GrabberActor.Get()) { Cap->IgnoreActorWhenMoving(G, false); }
	}

	// Meshes back to FP configuration: body visible, head re-hidden (it sits at the FP
	// camera — the propagate-true restore below would otherwise show it), full-body copy off.
	// The loco switcher in Tick re-seats both meshes' clips next frame (GrabPhase == None).
	if (MarcusBody)
	{
		MarcusBody->SetVisibility(true, /*bPropagateToChildren*/true);
	}
	if (MarcusHead) { MarcusHead->SetVisibility(false); }
	if (MarcusHair) { MarcusHair->SetVisibility(false); }
	if (MarcusBrows) { MarcusBrows->SetVisibility(false); }

	// Flashlight back to full brightness if it was dimmed for the grapple.
	if (FlashlightComp && PreGrabFlashlightIntensity >= 0.f)
	{
		FlashlightComp->SetIntensity(PreGrabFlashlightIntensity);
	}
	if (FlashlightFillComp && PreGrabFlashlightFillIntensity >= 0.f)
	{
		FlashlightFillComp->SetIntensity(PreGrabFlashlightFillIntensity);
	}
	PreGrabFlashlightIntensity = -1.f;
	PreGrabFlashlightFillIntensity = -1.f;
	if (PlayerMesh)
	{
		if (UZP_GracePlayerAnimInstance* AnimInst = Cast<UZP_GracePlayerAnimInstance>(PlayerMesh->GetAnimInstance()))
		{
			AnimInst->bCopyAllBones = false;
		}
	}

	// HUD released.
	if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController()))
	{
		if (PC->HUDWidget)
		{
			PC->HUDWidget->HideGrabPrompt();
			PC->HUDWidget->SetDamageVignetteHold(0.f);
		}
	}

	// Weapon back in hand.
	if (PreGrabWeaponClass && KinemationComp)
	{
		KinemationComp->EquipWeaponClass(PreGrabWeaponClass);
	}
	PreGrabWeaponClass = nullptr;
	GrabberActor = nullptr;

	UE_LOG(LogTemp, Warning, TEXT("[Grab] ended (aborted=%d)"), bAborted ? 1 : 0);
}

void AZP_GraceCharacter::PlayGrabClipOnBody(UAnimSequenceBase* Clip, bool bLoop)
{
	if (!Clip || !MarcusBody) { return; }
	if (UAnimSingleNodeInstance* SNI = MarcusBody->GetSingleNodeInstance())
	{
		SNI->SetAnimationAsset(Clip, bLoop, 1.0f);
		SNI->SetPlaying(true);
	}
}

void AZP_GraceCharacter::PlayGrabClipOnFPRig(UAnimSequenceBase* Clip)
{
	if (!Clip) { return; }
	if (UAnimSingleNodeInstance* SNI = Cast<UAnimSingleNodeInstance>(GetMesh()->GetAnimInstance()))
	{
		SNI->SetAnimationAsset(Clip, /*bLoop*/false, 1.0f);
		SNI->SetPlaying(true);
	}
}

void AZP_GraceCharacter::NotifyGrabberPhase(EZP_GrabPhase Phase)
{
	AActor* G = GrabberActor.Get();
	if (!G) { return; }
	// The grabber interface may live on the actor OR one of its components (the Shambler's
	// behavior component) — same resolution pattern as StaggerEnemy.
	if (IZP_Grabber* GI = Cast<IZP_Grabber>(G))
	{
		GI->OnVictimGrabPhase(Phase);
		return;
	}
	for (UActorComponent* Comp : G->GetComponents())
	{
		if (IZP_Grabber* CI = Cast<IZP_Grabber>(Comp))
		{
			CI->OnVictimGrabPhase(Phase);
			return;
		}
	}
}

// ===========================================================================
// Ladder Climbing
// ===========================================================================

void AZP_GraceCharacter::EnterLadder(AActor* LadderActor)
{
	if (bOnLadder || !LadderActor) return;

	AZP_Ladder* Ladder = Cast<AZP_Ladder>(LadderActor);
	if (!Ladder) return;

	bOnLadder = true;
	ActiveLadderActor = LadderActor;
	LadderClimbInput = 0.f;
	bLadderMovingToRung = false;
	LadderTargetRungZ = 0.f;

	// Hide interaction prompt while climbing
	if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController()))
	{
		if (PC->HUDWidget) PC->HUDWidget->HideInteractionPrompt();
	}

	// Save current weapon class and unequip — will re-equip on exit
	if (KinemationComp && KinemationComp->ActiveWeapon)
	{
		PreLadderWeaponClass = KinemationComp->ActiveWeapon->GetClass();
		KinemationComp->UnequipWeapon();
	}
	else
	{
		PreLadderWeaponClass = nullptr;
	}

	// TopClimbZ: highest point the player can BE on the ladder.
	const float TopClimbZ = Ladder->GetTopZ() - 80.f;
	// Snap Z to nearest rung so player always starts at a clean rung position
	const float RungSpacing = 23.5f;
	float RawHeight = GetActorLocation().Z - Ladder->GetBottomZ();
	float SnappedHeight = FMath::RoundToFloat(RawHeight / RungSpacing) * RungSpacing;
	float ClampedZ = FMath::Clamp(Ladder->GetBottomZ() + SnappedHeight, Ladder->GetBottomZ(), TopClimbZ);
	FVector LadderCenter = Ladder->GetLadderCenter();
	const float StandoffDist = 75.f;
	float MidZ = (Ladder->GetBottomZ() + Ladder->GetTopZ()) * 0.5f;
	bool bClimbingDown = GetActorLocation().Z >= MidZ;

	FVector SnapXY;
	if (!bClimbingDown)
	{
		// CLIMBING UP: snap to fixed perpendicular (always centered on ladder face)
		FVector SurfaceNormal = Ladder->GetLadderSurfaceNormal();
		FVector ToPlayer = GetActorLocation() - LadderCenter;
		ToPlayer.Z = 0.f;
		FVector SnapDir = (FVector::DotProduct(ToPlayer, SurfaceNormal) >= 0.f) ? SurfaceNormal : -SurfaceNormal;
		SnapXY = LadderCenter + SnapDir * StandoffDist;
	}
	else
	{
		// CLIMBING DOWN: find the open side of the ladder via line traces.
		// Player may be on the wrong side (wall side) on the upper floor.
		// Surface normal = perpendicular to ladder face. Candidates are +/- normal direction.
		FVector SurfaceNormal = Ladder->GetLadderSurfaceNormal();
		FVector CandA = SurfaceNormal;
		FVector CandB = -SurfaceNormal;

		FVector TraceStart(LadderCenter.X, LadderCenter.Y, GetActorLocation().Z);
		const float TraceLen = 200.f;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(Ladder);

		FHitResult HitA, HitB;
		GetWorld()->LineTraceSingleByChannel(HitA, TraceStart, TraceStart + CandA * TraceLen, ECC_Visibility, Params);
		GetWorld()->LineTraceSingleByChannel(HitB, TraceStart, TraceStart + CandB * TraceLen, ECC_Visibility, Params);

		float DistA = HitA.bBlockingHit ? HitA.Distance : TraceLen;
		float DistB = HitB.bBlockingHit ? HitB.Distance : TraceLen;

		FVector FrontDir = (DistA >= DistB) ? CandA : CandB;
		SnapXY = LadderCenter + FrontDir * StandoffDist;

		UE_LOG(LogTemp, Warning, TEXT("[LADDER-DOWN] TraceA dist=%.1f TraceB dist=%.1f FrontDir=(%.0f,%.0f) SnapXY=(%.1f,%.1f)"),
			DistA, DistB, FrontDir.X, FrontDir.Y, SnapXY.X, SnapXY.Y);
	}

	SetActorLocation(FVector(SnapXY.X, SnapXY.Y, ClampedZ));

	if (Controller)
	{
		FRotator LadderFacing = Ladder->GetClimbFacingRotation();

		// Camera: face perpendicular to the ladder surface (fixed direction every time).
		FVector WorldNormal = Ladder->GetLadderSurfaceNormal();
		// Pick the direction toward the ladder from the player's side
		FVector ToLadder = LadderCenter - FVector(SnapXY.X, SnapXY.Y, 0.f);
		ToLadder.Z = 0.f;
		FVector CameraDir = (FVector::DotProduct(ToLadder.GetSafeNormal(), WorldNormal) >= 0.f) ? WorldNormal : -WorldNormal;
		Controller->SetControlRotation(FRotator(0.f, CameraDir.Rotation().Yaw, 0.f));

		// Body: face the ladder's fixed climbing direction (for animation)
		SetActorRotation(FRotator(0.f, LadderFacing.Yaw + 180.f, 0.f));
		bUseControllerRotationYaw = false;
	}

	// Switch to flying movement — no gravity, no walking physics
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->SetMovementMode(MOVE_Flying);
		CMC->StopMovementImmediately();
	}

	// No camera detach needed — CalcCamera override has final say on position/rotation.
	// Kinemation components can keep ticking; their output is ignored during climbing.

	// Enable full-body bone copy so 1P climbing animations from hidden Mesh
	// override Kinemation's upper body on the visible PlayerMesh.
	if (UZP_GracePlayerAnimInstance* AnimInst = Cast<UZP_GracePlayerAnimInstance>(PlayerMesh->GetAnimInstance()))
	{
		AnimInst->bCopyAllBones = true;
	}

	// Hide head bone on visible mesh so camera doesn't see inside the skull.
	// PlayerMesh is the visible 1P mesh — hide head and neck.
	PlayerMesh->HideBoneByName(FName("head"), PBO_None);
	PlayerMesh->HideBoneByName(FName("neck_02"), PBO_None);
	PlayerMesh->HideBoneByName(FName("neck_01"), PBO_None);

	// Set ladder idle animation on hidden mesh
	if (UAnimSingleNodeInstance* SNI = Cast<UAnimSingleNodeInstance>(GetMesh()->GetAnimInstance()))
	{
		if (LadderIdleAnimation)
		{
			SNI->SetAnimationAsset(LadderIdleAnimation, true, 1.0f);
			SNI->SetPlaying(true);
		}
	}

	// Cancel any active gameplay states
	if (GameplayComp)
	{
		GameplayComp->StopSprint();
		GameplayComp->bWantsAim = false;
		GameplayComp->bWantsPeek = false;
	}

	// Uncrouch if crouched
	if (bIsCrouched)
	{
		UnCrouch();
	}

	// --- DIAGNOSTIC: Ladder positioning ---
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] === ENTER LADDER ==="));
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] Ladder actor: %s"), *Ladder->GetName());
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] LadderRoot world: (%.1f, %.1f, %.1f)"),
		Ladder->GetActorLocation().X, Ladder->GetActorLocation().Y, Ladder->GetActorLocation().Z);
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] LadderRot: (P=%.1f, Y=%.1f, R=%.1f)"),
		Ladder->GetActorRotation().Pitch, Ladder->GetActorRotation().Yaw, Ladder->GetActorRotation().Roll);
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] BottomAttachPoint relative: (%.1f, %.1f, %.1f)"),
		Ladder->BottomAttachPoint->GetRelativeLocation().X,
		Ladder->BottomAttachPoint->GetRelativeLocation().Y,
		Ladder->BottomAttachPoint->GetRelativeLocation().Z);
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] BottomAttachPoint world: (%.1f, %.1f, %.1f)"),
		Ladder->GetBottomAttachLocation().X, Ladder->GetBottomAttachLocation().Y, Ladder->GetBottomAttachLocation().Z);
	{
		FVector Center = Ladder->GetLadderCenter();
		FVector Normal = Ladder->GetLadderSurfaceNormal();
		UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] LadderCenter: (%.1f, %.1f, %.1f) SurfaceNormal: (%.2f, %.2f, %.2f)"),
			Center.X, Center.Y, Center.Z, Normal.X, Normal.Y, Normal.Z);
	}
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] Player position (no teleport): (%.1f, %.1f, %.1f)"),
		GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] Player AFTER snap: (%.1f, %.1f, %.1f)"),
		GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] Player capsule radius=%.0f halfHeight=%.0f"),
		GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] ClimbDirection arrow rot: (P=%.1f, Y=%.1f, R=%.1f)"),
		Ladder->ClimbDirection->GetComponentRotation().Pitch,
		Ladder->ClimbDirection->GetComponentRotation().Yaw,
		Ladder->ClimbDirection->GetComponentRotation().Roll);
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] ActorRot after snap: (P=%.1f, Y=%.1f, R=%.1f) ControlRot: (P=%.1f, Y=%.1f, R=%.1f)"),
		GetActorRotation().Pitch, GetActorRotation().Yaw, GetActorRotation().Roll,
		Controller ? Controller->GetControlRotation().Pitch : 0.f,
		Controller ? Controller->GetControlRotation().Yaw : 0.f,
		Controller ? Controller->GetControlRotation().Roll : 0.f);
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] BaseEyeHeight=%.0f, CalcCamera eye pos: (%.1f, %.1f, %.1f)"),
		BaseEyeHeight,
		GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + BaseEyeHeight);
	UE_LOG(LogTemp, Warning, TEXT("[LADDER-DIAG] === END ==="));
}

void AZP_GraceCharacter::ExitLadder(bool bExitTop)
{
	if (!bOnLadder) return;

	AZP_Ladder* Ladder = Cast<AZP_Ladder>(ActiveLadderActor.Get());

	// Teleport to exit point
	if (Ladder)
	{
		if (bExitTop && !bLadderTopExiting)
		{
			// Reached top: raise Z and push toward/past the ladder onto the upper floor.
			FVector ExitLoc = GetActorLocation();
			ExitLoc.Z = Ladder->GetTopExitLocation().Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			// Push 120 UU toward the ladder center (and past it onto the upper floor).
			// Player is ~75 UU from center, so 120 UU push = ~45 UU past center.
			FVector ToLadder = Ladder->GetLadderCenter() - GetActorLocation();
			ToLadder.Z = 0.f;
			if (ToLadder.SizeSquared() > 1.f)
			{
				ExitLoc += ToLadder.GetSafeNormal() * 120.f;
			}

			SetActorLocation(ExitLoc);
		}
		// Otherwise (Space dismount): keep current position — don't snap to bottom.
		// Player drops from wherever they are on the ladder.
	}

	// Restore walking movement and actor rotation behavior
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->SetMovementMode(MOVE_Walking);
	}

	// Auto-crouch if we climbed out at the top into a space too short to stand
	// (crawlspace / low ceiling). Test whether a full standing capsule fits at
	// the landing spot; if WorldStatic blocks it, crouch.
	if (bExitTop)
	{
		UCapsuleComponent* Cap = GetCapsuleComponent();
		const float StandHalf = Cap->GetScaledCapsuleHalfHeight();
		const float Radius = Cap->GetScaledCapsuleRadius();
		const FVector FloorPt = GetActorLocation() - FVector(0.f, 0.f, StandHalf);
		const FVector StandCenter = FloorPt + FVector(0.f, 0.f, StandHalf);
		// Shrink slightly so the floor/walls aren't a false positive.
		const FCollisionShape StandCap = FCollisionShape::MakeCapsule(Radius - 2.f, StandHalf - 2.f);
		FCollisionQueryParams CrouchParams;
		CrouchParams.AddIgnoredActor(this);
		const bool bCannotStand = GetWorld()->OverlapBlockingTestByChannel(
			StandCenter, FQuat::Identity, ECC_WorldStatic, StandCap, CrouchParams);
		if (bCannotStand)
		{
			Crouch();
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] Ladder top-exit: low ceiling -> auto-crouch"));
		}
	}
	bUseControllerRotationYaw = true; // Restore FPS mouse-look → actor rotation

	// No camera reattach needed — never detached. CalcCamera stops overriding
	// when bOnLadder goes false, and Super::CalcCamera resumes using the camera component.

	// Unhide head bones — restore normal 1P rendering
	PlayerMesh->UnHideBoneByName(FName("head"));
	PlayerMesh->UnHideBoneByName(FName("neck_02"));
	PlayerMesh->UnHideBoneByName(FName("neck_01"));

	// Disable full-body bone copy — back to Kinemation upper body + lower body bone copy
	if (UZP_GracePlayerAnimInstance* AnimInst = Cast<UZP_GracePlayerAnimInstance>(PlayerMesh->GetAnimInstance()))
	{
		AnimInst->bCopyAllBones = false;
	}

	// Re-equip weapon if we had one before climbing
	if (PreLadderWeaponClass && KinemationComp)
	{
		KinemationComp->EquipWeaponClass(PreLadderWeaponClass);
	}
	PreLadderWeaponClass = nullptr;

	bOnLadder = false;
	ActiveLadderActor = nullptr;
	LadderClimbInput = 0.f;
	bLadderMovingToRung = false;
	LadderTargetRungZ = 0.f;
	bLadderTopExiting = false;
	LadderTopExitElapsed = 0.f;

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ExitLadder: Dismounted (top=%d)"), bExitTop);
}

void AZP_GraceCharacter::BeginLadderTopExit(AZP_Ladder* Ladder)
{
	// No ladder or no animation configured -> fall back to the instant exit.
	if (!Ladder || !LadderTopExitAnimation)
	{
		ExitLadder(true);
		return;
	}

	bLadderTopExiting = true;
	LadderTopExitElapsed = 0.f;
	LadderTopExitDuration = LadderTopExitAnimation->GetPlayLength();
	bLadderMovingToRung = false;

	// Resting spot = the same place the legacy instant exit landed: floor height,
	// pushed past the ladder onto the upper floor.
	FVector EndLoc = GetActorLocation();
	EndLoc.Z = Ladder->GetTopExitLocation().Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector ToLadder = Ladder->GetLadderCenter() - GetActorLocation();
	ToLadder.Z = 0.f;
	if (ToLadder.SizeSquared() > 1.f)
	{
		EndLoc += ToLadder.GetSafeNormal() * 120.f;
	}
	LadderTopExitEndLoc = EndLoc;

	// Camera interpolates from the current ladder eye to the final standing eye.
	LadderTopExitCamStart = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
	LadderTopExitCamEnd = EndLoc + FVector(0.f, 0.f, BaseEyeHeight);

	// Play the climb-over animation on the hidden Mesh; PlayerMesh copies all
	// bones (bCopyAllBones stays true until ExitLadder), so the mantle and its
	// baked root motion show on the 1P body. Scrub position manually so the
	// camera lerp stays in sync. The capsule stays put; the body's root motion
	// does the travelling, then we snap the capsule to EndLoc at the end.
	if (UAnimSingleNodeInstance* SNI = Cast<UAnimSingleNodeInstance>(GetMesh()->GetAnimInstance()))
	{
		SNI->SetAnimationAsset(LadderTopExitAnimation, false, 1.0f);
		SNI->SetPlaying(false);
		SNI->SetPosition(0.f, false);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] BeginLadderTopExit: anim=%s dur=%.2f end=(%.0f,%.0f,%.0f)"),
		*LadderTopExitAnimation->GetName(), LadderTopExitDuration, EndLoc.X, EndLoc.Y, EndLoc.Z);
}

void AZP_GraceCharacter::UpdateLadderTopExit(float DeltaTime)
{
	LadderTopExitElapsed += DeltaTime * LadderTopExitPlayRate;
	const float Frac = (LadderTopExitDuration > 0.f)
		? FMath::Clamp(LadderTopExitElapsed / LadderTopExitDuration, 0.f, 1.f) : 1.f;

	// Scrub the climb-over animation in step with the camera lerp.
	if (UAnimSingleNodeInstance* SNI = Cast<UAnimSingleNodeInstance>(GetMesh()->GetAnimInstance()))
	{
		SNI->SetPosition(Frac * LadderTopExitDuration, false);
	}

	if (Frac >= 1.f)
	{
		// Body has reached the floor (visually, via root motion). Snap the capsule
		// to the resting spot, then run the normal dismount cleanup. The teleport in
		// ExitLadder is skipped while bLadderTopExiting is still true.
		SetActorLocation(LadderTopExitEndLoc);
		ExitLadder(true);
	}
}
