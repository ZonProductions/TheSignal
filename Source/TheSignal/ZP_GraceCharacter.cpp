// Copyright The Signal. All Rights Reserved.

#include "ZP_GraceCharacter.h"
#include "ZP_GraceGameplayComponent.h"
#include "ZP_KinemationComponent.h"
#include "ZP_WeaponTypes.h"
#include "ZP_HealthComponent.h"
#include "ZP_GraceMovementConfig.h"
#include "ZP_GracePlayerAnimInstance.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "ZP_Interactable.h"
#include "ZP_InteractDoor.h"
#include "ZP_MapComponent.h"
#include "ZP_NoteComponent.h"
#include "ZP_InventoryTabTypes.h"
#include "ZP_InventoryTabWidget.h"
#include "GameplayTagContainer.h"
#include "UObject/UnrealType.h"
#include "ZP_FloorCullingComponent.h"
#include "ZP_RuntimeISMBatcher.h"
#include "ZP_MapWidget.h"
#include "ZP_NPCInteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
	// Without socket attachment, looking up/down causes parallax (gun leaves view).
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(PlayerMesh, FName(TEXT("FPCamera")));
	FirstPersonCamera->SetRelativeLocation(FVector::ZeroVector);
	FirstPersonCamera->SetRelativeRotation(FRotator::ZeroRotator);
	FirstPersonCamera->bUsePawnControlRotation = false;
	PlayerMesh->SetOnlyOwnerSee(true);
	PlayerMesh->bCastDynamicShadow = true;
	PlayerMesh->CastShadow = true;
	PlayerMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

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
	DeathVignetteComp->Settings.bOverride_AutoExposureMinBrightness = true;
	DeathVignetteComp->Settings.AutoExposureMinBrightness = 0.01f; // allow very dark scenes
	DeathVignetteComp->Settings.bOverride_AutoExposureMaxBrightness = true;
	DeathVignetteComp->Settings.AutoExposureMaxBrightness = 1.5f; // cap brightening
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
	FlashlightComp->SetIntensity(8000.0f);        // bright enough to read surfaces down hallways
	FlashlightComp->SetInnerConeAngle(20.0f);      // wide hotspot so walls are illuminated up close
	FlashlightComp->SetOuterConeAngle(45.0f);      // broad flood — walnut-size at close range fixed
	FlashlightComp->SetAttenuationRadius(2800.0f); // longer reach for corridors
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

	// Starting weapon (soft reference — doesn't force-load the asset)
	StartingWeaponItem = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/Core/Items/DA_Grace_Pistol.DA_Grace_Pistol")));

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

	// PlayerMesh must evaluate AFTER hidden Mesh, so NativePostEvaluateAnimation
	// reads fresh source bone data when copying lower body.
	PlayerMesh->AddTickPrerequisiteComponent(GetMesh());

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

	// Grant starting items to inventory + set weapon slot 0
	GrantStartingItems();

	// Bind weapon change delegate BEFORE InitializeKinemation so the starting
	// weapon broadcast registers in the slot system.
	if (KinemationComp)
	{
		KinemationComp->OnWeaponChanged.AddDynamic(this, &AZP_GraceCharacter::OnWeaponChangedHandler);
		KinemationComp->OnThrowableConsumed.AddDynamic(this, &AZP_GraceCharacter::OnThrowableConsumedHandler);

		// Initialize Kinemation AFTER Super::BeginPlay() — all SCS Blueprint components
		// (AC_FirstPersonCamera, AC_TacticalShooterAnimation, etc.) have had their
		// BeginPlay by now. Wiring before that gets overwritten by their init.
		KinemationComp->InitializeKinemation();

		// Sync ActiveWeapon for BP interface compat (GetPrimaryWeapon, GetMainWeapon)
		ActiveWeapon = KinemationComp->ActiveWeapon;
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

void AZP_GraceCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Switch hidden Mesh animation based on ground speed and crouch state
	if (UAnimSingleNodeInstance* SNI = Cast<UAnimSingleNodeInstance>(GetMesh()->GetAnimInstance()))
	{
		if (bOnLadder)
		{
			// --- Rung-to-rung discrete movement ---
			// Player commits to each rung step. Can't stop or reverse mid-step.
			AZP_Ladder* Ladder = ActiveLadderActor.IsValid() ? Cast<AZP_Ladder>(ActiveLadderActor.Get()) : nullptr;

			// Position-driven climb animation: anim time mapped from height on ladder.
			// One anim cycle (1.533s) = 2 rungs (47 UU). Each 23.5 UU = half cycle = hands alternate.
			if (LadderClimbUpAnimation && Ladder)
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

			if (Ladder)
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
						ExitLadder(true);
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
		}
	}

	// Bone copy now happens in NativePostEvaluateAnimation (inside animation pipeline).

	// --- Weapon slot keys (raw polling) ---
	// Bypasses Enhanced Input entirely to avoid IMC conflicts between
	// IMC_Grace and Moonville's IMC_InventoryCharacter double-firing.
	if (!bInventoryMenuOpen && !bMapOpen && !bOnLadder)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (PC->WasInputKeyJustPressed(EKeys::One))        Input_InventorySlot(0);
			else if (PC->WasInputKeyJustPressed(EKeys::Two))   Input_InventorySlot(1);
			else if (PC->WasInputKeyJustPressed(EKeys::Three)) Input_InventorySlot(2);
			else if (PC->WasInputKeyJustPressed(EKeys::Four))  Input_InventorySlot(3);
		}
	}

	// --- Container close detection (briefcase sync + weapon unequip) ---
	CheckContainerClosed();

	// Flashlight toggle (F key — works even with menus open)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC && PC->WasInputKeyJustPressed(EKeys::F))
		{
			ToggleFlashlight();
		}
	}

	// Flashlight follows camera with lag (chest-mounted feel)
	if (FlashlightComp && bFlashlightOn && FirstPersonCamera)
	{
		FRotator TargetRot = FirstPersonCamera->GetComponentRotation();
		TargetRot.Pitch += FlashlightPitchOffset;
		FRotator NewRot = FMath::RInterpTo(FlashlightComp->GetComponentRotation(), TargetRot, DeltaTime, FlashlightInterpSpeed);
		FlashlightComp->SetWorldRotation(NewRot);
	}

}

// --- CalcCamera override ---
// During ladder climbing: simple fixed-offset position + standard controller rotation.
// CalcCamera runs last — nothing can override it.
void AZP_GraceCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
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
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AZP_GraceCharacter::Input_CrouchStarted);
		EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AZP_GraceCharacter::Input_CrouchCompleted);
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

	const FVector2D MoveInput = Value.Get<FVector2D>();

	// On ladder: forward/back = climb up/down, no lateral movement
	if (bOnLadder)
	{
		LadderClimbInput = MoveInput.Y; // W = +1 (up), S = -1 (down)
		return;
	}

	if (Controller)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDir, MoveInput.Y);
		AddMovementInput(RightDir, MoveInput.X);
	}
}

void AZP_GraceCharacter::Input_Look(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;

	const FVector2D LookInput = Value.Get<FVector2D>();

	if (bOnLadder) return; // Camera fully locked during climbing

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(-LookInput.Y);
}

void AZP_GraceCharacter::Input_SprintStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen || bOnLadder) return;
	if (GameplayComp) GameplayComp->StartSprint();
}

void AZP_GraceCharacter::Input_SprintCompleted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (GameplayComp) GameplayComp->StopSprint();
}

void AZP_GraceCharacter::Input_Jump(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (bOnLadder)
	{
		// If in upper half of ladder, exit at top; otherwise drop from current position
		AZP_Ladder* Ladder = Cast<AZP_Ladder>(ActiveLadderActor.Get());
		bool bNearTop = Ladder && GetActorLocation().Z >= (Ladder->GetBottomZ() + Ladder->GetTopZ()) * 0.5f;
		ExitLadder(bNearTop);
		return;
	}
	Jump();
}

void AZP_GraceCharacter::Input_Interact(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Blocked — bInventoryMenuOpen=%d, bMapOpen=%d"), bInventoryMenuOpen, bMapOpen);
		return;
	}
	if (bOnLadder) return;

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

			// Find first non-volume hit
			FHitResult* BestHit = nullptr;
			for (FHitResult& H : AllHits)
			{
				AActor* A = H.GetActor();
				if (A && !A->GetClass()->GetName().Contains(TEXT("Volume")))
				{
					BestHit = &H;
					break;
				}
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

					if (bMatchPickup || bMatchContainer)
					{
						// --- Loot Locker filtering ---
						bool bSkipLoot = false;
						if (bMatchContainer)
						{
							// Skip if already looted and empty
							if (LootedEmptyLockers.Contains(FName(*HitActorName)))
							{
								UE_LOG(LogTemp, Log, TEXT("[LootLocker] %s is looted and empty — skipping"), *HitActorName);
								bSkipLoot = true;
							}
							// Filter ammo for weapons player doesn't have (not briefcases)
							else if (!bIsBriefcase)
							{
								FilterLockerAmmo(HitActor);
								if (IsLockerInventoryEmpty(HitActor))
								{
									DisableLockerInteraction(HitActor);
									bSkipLoot = true;
								}
							}
						}

						if (!bSkipLoot)
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
						} // end if (!bSkipLoot)
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
		if (Cast<AZP_InteractDoor>(CurrentInteractable.Get()))
		{
			APlayerController* LOSCheck_PC = Cast<APlayerController>(GetController());
			if (LOSCheck_PC)
			{
				FVector LOSCamLoc;
				FRotator LOSCamRot;
				LOSCheck_PC->GetPlayerViewPoint(LOSCamLoc, LOSCamRot);
				FVector ToDoor = (CurrentInteractable->GetActorLocation() - LOSCamLoc).GetSafeNormal();
				float Dot = FVector::DotProduct(LOSCamRot.Vector(), ToDoor);
				bShouldInteract = (Dot > 0.5f); // ~60° cone
				UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Layer 2 door LOS check: Dot=%.2f, Pass=%d"), Dot, bShouldInteract);
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
	Crouch();
}

void AZP_GraceCharacter::Input_CrouchCompleted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	UnCrouch();
}

void AZP_GraceCharacter::Input_PeekStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen || bOnLadder) return;
	if (GameplayComp) GameplayComp->bWantsPeek = true;
}

void AZP_GraceCharacter::Input_PeekCompleted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (GameplayComp) GameplayComp->bWantsPeek = false;
}

void AZP_GraceCharacter::Input_AimStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	// Block ADS when no weapon equipped or during melee swing / melee / throwable
	if (!KinemationComp || !KinemationComp->ActiveWeapon) return;
	if (KinemationComp->bMeleeSwingActive ||
		KinemationComp->CurrentWeaponType == EZP_WeaponType::Melee ||
		KinemationComp->CurrentWeaponType == EZP_WeaponType::Throwable)
	{
		return;
	}
	if (GameplayComp) GameplayComp->bWantsAim = true;
	KinemationComp->SetAiming(true);
}

void AZP_GraceCharacter::Input_AimCompleted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	// Don't release ADS if melee swing is controlling it
	if (KinemationComp && KinemationComp->bMeleeSwingActive)
	{
		return;
	}
	if (GameplayComp) GameplayComp->bWantsAim = false;
	if (KinemationComp) KinemationComp->SetAiming(false);
}

void AZP_GraceCharacter::Input_FireStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (!KinemationComp || !KinemationComp->ActiveWeapon) return;
	KinemationComp->FirePressed();
}

void AZP_GraceCharacter::Input_FireCompleted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (KinemationComp) KinemationComp->FireReleased();
}

void AZP_GraceCharacter::Input_ReloadStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen || bOnLadder) return;
	if (KinemationComp) KinemationComp->Reload();
}

void AZP_GraceCharacter::Input_InventoryMenu(const FInputActionValue& Value)
{
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController());
	if (!PC || !MoonvilleInventoryComp) return;

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
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController());
	if (!PC) return;

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

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: Weapon changed → %s"),
		NewWeapon ? *NewWeapon->GetName() : TEXT("NONE"));
}

void AZP_GraceCharacter::OnThrowableConsumedHandler()
{
	if (!MoonvilleInventoryComp) return;

	// Block this weapon class from ever being re-equipped
	if (KinemationComp && KinemationComp->WeaponClass)
	{
		ConsumedWeaponClasses.Add(KinemationComp->WeaponClass);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Throwable consumed — weapon class %s blocked from re-equip"),
			*KinemationComp->WeaponClass->GetName());
	}

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

	LastThrowableItemDA = nullptr;
	LastThrowableSlotIndex = -1;
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

void AZP_GraceCharacter::HandleInventoryUpdate()
{
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
	if (SlotIndex < 0 || SlotIndex > 3) return;

	// Try weapon equip first
	TSubclassOf<AActor> SlotWeaponClass = GetWeaponFromShortcutSlot(SlotIndex);
	if (SlotWeaponClass)
	{
		if (!KinemationComp) return;

		// Block consumed throwables (grenade already used)
		if (ConsumedWeaponClasses.Contains(SlotWeaponClass))
		{
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] Weapon class %s already consumed — ignoring"), *SlotWeaponClass->GetName());
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

		bool bSuccess = KinemationComp->EquipWeaponClass(SlotWeaponClass);

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

// --- Death / Vignette ---

void AZP_GraceCharacter::HandleDeath()
{
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

void AZP_GraceCharacter::ToggleFlashlight()
{
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

float AZP_GraceCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (HealthComp && HealthComp->bIsDead)
	{
		return 0.f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HealthComp)
	{
		HealthComp->ApplyDamage(ActualDamage);
	}

	// Camera flinch — immediate visceral feedback that something hit you
	if (ActualDamage > 0.f)
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
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Container OPENED (bPlayerIsUsingActor=true) on %s after %d frames"), *ContainerToCheck->GetName(), ContainerOpenWaitFrames);
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
	if (bStillUsing) return; // Still open

	// Container is now closed
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Container CLOSED (bPlayerIsUsingActor=false) on %s"), *ContainerToCheck->GetName());
	bContainerWasOpen = false;

	// Check if loot locker is now empty — disable interaction if so
	if (IsLockerInventoryEmpty(ContainerToCheck))
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

	for (int32 i = 0; i < 4; ++i)
	{
		TSubclassOf<AActor> SlotWeaponClass = GetWeaponFromShortcutSlot(i);
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Slot %d: %s"), i,
			SlotWeaponClass ? *SlotWeaponClass->GetName() : TEXT("EMPTY"));

		if (SlotWeaponClass && SlotWeaponClass == CurrentWeaponClass)
		{
			bWeaponStillInSlots = true;
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] MATCH found in slot %d — weapon still in inventory"), i);
			break;
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

	// Find the locker's inventory component (BP_InventoryActorComponent)
	UActorComponent* LockerInvComp = nullptr;
	for (UActorComponent* Comp : LockerActor->GetComponents())
	{
		if (Comp->GetClass()->GetName().Contains(TEXT("InventoryActorComponent")) ||
			Comp->GetClass()->GetName().Contains(TEXT("InventoryComponent")))
		{
			LockerInvComp = Comp;
			break;
		}
	}
	if (!LockerInvComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootLocker] No inventory component found on %s"), *LockerActor->GetName());
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

	// Find Item_ field in slot struct
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

	// Collect ammo items to remove
	TArray<UObject*> AmmoToRemove;
	for (int32 i = 0; i < ArrayHelper.Num(); i++)
	{
		void* ElementData = ArrayHelper.GetRawPtr(i);
		UObject* SlotItem = ItemObjProp->GetObjectPropertyValue(ItemObjProp->ContainerPtrToValuePtr<void>(ElementData));
		if (!SlotItem) continue;

		FString ItemName = SlotItem->GetName();
		// Only filter ammo items
		if (!ItemName.Contains(TEXT("Ammo"))) continue;

		if (!PlayerHasWeaponForAmmo(ItemName))
		{
			AmmoToRemove.Add(SlotItem);
			UE_LOG(LogTemp, Log, TEXT("[LootLocker] Filtering out %s — player has no matching weapon"), *ItemName);
		}
	}

	// Remove filtered ammo via Moonville API
	UFunction* RemoveFunc = LockerInvComp->FindFunction(FName("RemoveItemByDataAsset"));
	if (!RemoveFunc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootLocker] RemoveItemByDataAsset not found on locker inv comp"));
		return;
	}

	for (UObject* AmmoDA : AmmoToRemove)
	{
		struct { UObject* ItemDataAsset; int32 AmountToRemove; } Params;
		Params.ItemDataAsset = AmmoDA;
		Params.AmountToRemove = 999; // Remove all stacks
		LockerInvComp->ProcessEvent(RemoveFunc, &Params);
		UE_LOG(LogTemp, Log, TEXT("[LootLocker] Removed %s from locker %s"), *AmmoDA->GetName(), *LockerActor->GetName());
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

	// Find locker's inventory component
	UActorComponent* LockerInvComp = nullptr;
	for (UActorComponent* Comp : LockerActor->GetComponents())
	{
		if (Comp->GetClass()->GetName().Contains(TEXT("InventoryActorComponent")) ||
			Comp->GetClass()->GetName().Contains(TEXT("InventoryComponent")))
		{
			LockerInvComp = Comp;
			break;
		}
	}
	if (!LockerInvComp) return true;

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
		if (bExitTop)
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

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ExitLadder: Dismounted (top=%d)"), bExitTop);
}
