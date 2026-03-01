// Copyright The Signal. All Rights Reserved.

#include "ZP_GraceCharacter.h"
#include "ZP_GraceGameplayComponent.h"
#include "ZP_KinemationComponent.h"
#include "ZP_GraceMovementConfig.h"
#include "ZP_GracePlayerAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "EnhancedInputComponent.h"

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
	PlayerMesh->bCastDynamicShadow = false;
	PlayerMesh->CastShadow = false;
	PlayerMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Gameplay component — stamina, peek, head bob, interaction trace
	GameplayComp = CreateDefaultSubobject<UZP_GraceGameplayComponent>(TEXT("GameplayComp"));

	// Kinemation component — camera/anim wiring, weapon lifecycle
	KinemationComp = CreateDefaultSubobject<UZP_KinemationComponent>(TEXT("KinemationComp"));

	// Movement defaults (overridden by DataAsset in GameplayComp's BeginPlay)
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->MaxWalkSpeed = 200.0f;
	MoveComp->BrakingDecelerationWalking = 1400.0f;
	MoveComp->MaxAcceleration = 1200.0f;
	MoveComp->GroundFriction = 6.0f;
	MoveComp->JumpZVelocity = 300.0f;
	MoveComp->AirControl = 0.15f;
	MoveComp->NavAgentProps.bCanCrouch = true;
	MoveComp->SetCrouchedHalfHeight(58.0f);
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

	// Initialize Kinemation AFTER Super::BeginPlay() — all SCS Blueprint components
	// (AC_FirstPersonCamera, AC_TacticalShooterAnimation, etc.) have had their
	// BeginPlay by now. Wiring before that gets overwritten by their init.
	if (KinemationComp)
	{
		KinemationComp->InitializeKinemation();

		// Sync ActiveWeapon for BP interface compat (GetPrimaryWeapon, GetMainWeapon)
		ActiveWeapon = KinemationComp->ActiveWeapon;
	}

	// Head bob stays enabled even with Kinemation — AC_FirstPersonCamera only drives
	// camera ROTATION (aim sway), not position. Our built-in bob handles position offsets.

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter BeginPlay — Config: %s, Weapon: %s, Kinemation: %s"),
		MovementConfig ? *MovementConfig->GetName() : TEXT("NONE"),
		KinemationComp && KinemationComp->ActiveWeapon ? *KinemationComp->ActiveWeapon->GetName() : TEXT("NONE"),
		KinemationComp && KinemationComp->IsKinemationActive() ? TEXT("ACTIVE") : TEXT("OFF"));
}

void AZP_GraceCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Switch hidden Mesh animation based on ground speed
	if (UAnimSingleNodeInstance* SNI = Cast<UAnimSingleNodeInstance>(GetMesh()->GetAnimInstance()))
	{
		const float Speed = GetVelocity().Size2D();

		UAnimSequenceBase* DesiredAnim = IdleAnimation;
		if (Speed > 150.0f && RunAnimation)
			DesiredAnim = RunAnimation;
		else if (Speed > 10.0f && WalkAnimation)
			DesiredAnim = WalkAnimation;

		if (DesiredAnim && SNI->GetCurrentAsset() != DesiredAnim)
		{
			SNI->SetAnimationAsset(DesiredAnim, true, 1.0f);
			SNI->SetPlaying(true);
		}
	}

	// Bone copy now happens in NativePostEvaluateAnimation (inside animation pipeline).
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

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: Input bound — Move:%s Look:%s Sprint:%s Jump:%s Interact:%s Crouch:%s Peek:%s Aim:%s Fire:%s Reload:%s"),
		MoveAction ? TEXT("OK") : TEXT("MISSING"),
		LookAction ? TEXT("OK") : TEXT("MISSING"),
		SprintAction ? TEXT("OK") : TEXT("MISSING"),
		JumpAction ? TEXT("OK") : TEXT("MISSING"),
		InteractAction ? TEXT("OK") : TEXT("MISSING"),
		CrouchAction ? TEXT("OK") : TEXT("MISSING"),
		PeekAction ? TEXT("OK") : TEXT("MISSING"),
		AimAction ? TEXT("OK") : TEXT("MISSING"),
		FireAction ? TEXT("OK") : TEXT("MISSING"),
		ReloadAction ? TEXT("OK") : TEXT("MISSING"));
}

// --- Input Handlers ---

void AZP_GraceCharacter::Input_Move(const FInputActionValue& Value)
{
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
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(-LookInput.Y);
}

void AZP_GraceCharacter::Input_SprintStarted(const FInputActionValue& Value)
{
	if (GameplayComp) GameplayComp->StartSprint();
}

void AZP_GraceCharacter::Input_SprintCompleted(const FInputActionValue& Value)
{
	if (GameplayComp) GameplayComp->StopSprint();
}

void AZP_GraceCharacter::Input_Jump(const FInputActionValue& Value)
{
	Jump();
}

void AZP_GraceCharacter::Input_Interact(const FInputActionValue& Value)
{
	AActor* Target = GameplayComp ? GameplayComp->CurrentInteractionTarget : nullptr;
	OnInteract(Target);
}

void AZP_GraceCharacter::Input_CrouchStarted(const FInputActionValue& Value)
{
	Crouch();
}

void AZP_GraceCharacter::Input_CrouchCompleted(const FInputActionValue& Value)
{
	UnCrouch();
}

void AZP_GraceCharacter::Input_PeekStarted(const FInputActionValue& Value)
{
	if (GameplayComp) GameplayComp->bWantsPeek = true;
}

void AZP_GraceCharacter::Input_PeekCompleted(const FInputActionValue& Value)
{
	if (GameplayComp) GameplayComp->bWantsPeek = false;
}

void AZP_GraceCharacter::Input_AimStarted(const FInputActionValue& Value)
{
	if (GameplayComp) GameplayComp->bWantsAim = true;
}

void AZP_GraceCharacter::Input_AimCompleted(const FInputActionValue& Value)
{
	if (GameplayComp) GameplayComp->bWantsAim = false;
}

void AZP_GraceCharacter::Input_FireStarted(const FInputActionValue& Value)
{
	if (KinemationComp) KinemationComp->FirePressed();
}

void AZP_GraceCharacter::Input_FireCompleted(const FInputActionValue& Value)
{
	if (KinemationComp) KinemationComp->FireReleased();
}

void AZP_GraceCharacter::Input_ReloadStarted(const FInputActionValue& Value)
{
	if (KinemationComp) KinemationComp->Reload();
}
