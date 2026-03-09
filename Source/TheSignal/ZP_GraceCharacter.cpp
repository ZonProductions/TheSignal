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
#include "ZP_MapComponent.h"
#include "ZP_FloorCullingComponent.h"
#include "ZP_RuntimeISMBatcher.h"
#include "ZP_MapWidget.h"
#include "ZP_NPCInteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/PostProcessComponent.h"
#include "Components/SpotLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AZP_GraceCharacter::AZP_GraceCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule — average male, slightly hunched from anxiety
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

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

	// Kill Lumen's indirect GI bounce — eliminates "swimming pool" caustic effect.
	// Only direct light (flashlight, moonlight) illuminates. No bounced fill light.
	DeathVignetteComp->Settings.bOverride_IndirectLightingIntensity = true;
	DeathVignetteComp->Settings.IndirectLightingIntensity = 0.0f;

	// Chest/shoulder flashlight — TLOU/SH2 style
	// Raised above weapon line + offset left so rifle doesn't block the cone
	FlashlightComp = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightComp"));
	FlashlightComp->SetupAttachment(GetCapsuleComponent());
	FlashlightComp->SetRelativeLocation(FVector(25.0f, -12.0f, 55.0f));
	FlashlightComp->SetIntensity(8000.0f);        // bright enough to read surfaces down hallways
	FlashlightComp->SetInnerConeAngle(12.0f);      // wider hotspot for hallway coverage
	FlashlightComp->SetOuterConeAngle(28.0f);      // broader falloff — see walls and floor
	FlashlightComp->SetAttenuationRadius(2800.0f); // longer reach for corridors
	FlashlightComp->SetLightColor(FLinearColor(1.0f, 0.93f, 0.82f)); // slightly warmer/dingier
	FlashlightComp->SetSourceRadius(0.5f);         // sharp shadow edges
	FlashlightComp->CastShadows = true;
	FlashlightComp->SetVisibility(false); // starts OFF

	// Default click sound from Character Customizer flashlight tool
	static ConstructorHelpers::FObjectFinder<USoundBase> FlashlightClickFinder(
		TEXT("/Game/CharacterCustomizer/Components/Tools/Tool_Flashlight/Click"));
	if (FlashlightClickFinder.Succeeded())
	{
		FlashlightClickSound = FlashlightClickFinder.Object;
	}

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

	// Bind HUD to this character's components
	if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController()))
	{
		if (PC->HUDWidget)
		{
			PC->HUDWidget->BindToCharacter(this);
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

	// Bone copy now happens in NativePostEvaluateAnimation (inside animation pipeline).

	// --- Weapon slot keys (raw polling) ---
	// Bypasses Enhanced Input entirely to avoid IMC conflicts between
	// IMC_Grace and Moonville's IMC_InventoryCharacter double-firing.
	if (!bInventoryMenuOpen && !bMapOpen)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (PC->WasInputKeyJustPressed(EKeys::One))        Input_InventorySlot(0);
			else if (PC->WasInputKeyJustPressed(EKeys::Two))   Input_InventorySlot(1);
			else if (PC->WasInputKeyJustPressed(EKeys::Three)) Input_InventorySlot(2);
			else if (PC->WasInputKeyJustPressed(EKeys::Four))  Input_InventorySlot(3);
		}
	}

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
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(-LookInput.Y);
}

void AZP_GraceCharacter::Input_SprintStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
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
	Jump();
}

void AZP_GraceCharacter::Input_Interact(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;

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
			if (GetWorld()->LineTraceSingleByObjectType(Hit, CamLoc, TraceEnd, ObjParams, Params))
			{
				AActor* HitActor = Hit.GetActor();
				if (HitActor && HitActor != CurrentInteractable.Get())
				{
					FString HitClassName = HitActor->GetClass()->GetName();
					if (HitClassName.Contains(TEXT("ItemPickup")) || HitClassName.Contains(TEXT("Chest")))
					{
						UFunction* InteractFunc = MoonvilleInteractionComp->FindFunction(FName("Interact"));
						if (InteractFunc)
						{
							MoonvilleInteractionComp->ProcessEvent(InteractFunc, nullptr);
							return;
						}
					}
				}
			}
		}
	}

	// Check our IZP_Interactable system (save points, doors, pickups, etc.)
	if (CurrentInteractable.IsValid() && CurrentInteractable->GetClass()->ImplementsInterface(UZP_Interactable::StaticClass()))
	{
		IZP_Interactable::Execute_OnInteract(CurrentInteractable.Get(), this);
		return;
	}

	// Check for NPC Interaction Component (CC-based NPCs that don't inherit from AZP_NPC)
	if (CurrentInteractable.IsValid())
	{
		if (UZP_NPCInteractionComponent* NPCComp = CurrentInteractable->FindComponentByClass<UZP_NPCInteractionComponent>())
		{
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
			MoonvilleInteractionComp->ProcessEvent(InteractFunc, nullptr);
			return;
		}
	}

	// Fallback to original interaction
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
	if (bInventoryMenuOpen || bMapOpen) return;
	Crouch();
}

void AZP_GraceCharacter::Input_CrouchCompleted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	UnCrouch();
}

void AZP_GraceCharacter::Input_PeekStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
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
	// Block manual ADS during melee swing animation or when melee/throwable is equipped
	if (KinemationComp && (KinemationComp->bMeleeSwingActive ||
		KinemationComp->CurrentWeaponType == EZP_WeaponType::Melee ||
		KinemationComp->CurrentWeaponType == EZP_WeaponType::Throwable))
	{
		return;
	}
	if (GameplayComp) GameplayComp->bWantsAim = true;
	if (KinemationComp) KinemationComp->SetAiming(true);
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
	if (KinemationComp) KinemationComp->FirePressed();
}

void AZP_GraceCharacter::Input_FireCompleted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (KinemationComp) KinemationComp->FireReleased();
}

void AZP_GraceCharacter::Input_ReloadStarted(const FInputActionValue& Value)
{
	if (bInventoryMenuOpen || bMapOpen) return;
	if (KinemationComp) KinemationComp->Reload();
}

void AZP_GraceCharacter::Input_InventoryMenu(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: Input_InventoryMenu FIRED"));

	// Call Moonville's ToggleInventoryMenu — it handles UI open/close,
	// then calls BPI_Inventory.OnOpenInventory/OnCloseInventory on us,
	// which sets bInventoryMenuOpen via our BP interface implementation.
	if (MoonvilleInventoryComp)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			UFunction* ToggleFunc = MoonvilleInventoryComp->FindFunction(FName("ToggleInventoryMenu"));
			if (ToggleFunc)
			{
				struct { APlayerController* PlayerController; } Params;
				Params.PlayerController = PC;
				MoonvilleInventoryComp->ProcessEvent(ToggleFunc, &Params);
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: Called ToggleInventoryMenu on %s"),
					*MoonvilleInventoryComp->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[TheSignal] ZP_GraceCharacter: ToggleInventoryMenu function NOT FOUND on %s"),
					*MoonvilleInventoryComp->GetClass()->GetName());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ZP_GraceCharacter: No MoonvilleInventoryComp — cannot toggle inventory."));
	}
}

void AZP_GraceCharacter::Input_Map(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP DEBUG: Input_Map FIRED"));

	if (bInventoryMenuOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP DEBUG: Blocked — inventory menu open"));
		return;
	}

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] MAP DEBUG: PlayerController is NULL"));
		return;
	}
	if (!PC->MapWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] MAP DEBUG: PC->MapWidget is NULL (MapWidgetClass not set on PC_Grace?)"));
		return;
	}

	if (PC->MapWidget->IsMapVisible())
	{
		PC->MapWidget->HideMap();
		bMapOpen = false;
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP DEBUG: Map HIDDEN"));
	}
	else
	{
		PC->MapWidget->ShowMap(MapComp);
		bMapOpen = true;
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP DEBUG: Map SHOWN (MapComp=%s)"),
			MapComp ? TEXT("valid") : TEXT("NULL"));
	}
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
