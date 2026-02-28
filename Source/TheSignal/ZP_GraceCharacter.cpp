// Copyright The Signal. All Rights Reserved.

#include "ZP_GraceCharacter.h"
#include "ZP_GraceMovementConfig.h"
#include "ZP_EventBroadcaster.h"
#include "KinemationBridge.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"

AZP_GraceCharacter::AZP_GraceCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule — average male, slightly hunched from anxiety
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

	// First-person camera attached to capsule root (NOT skeleton)
	// Kinemation's AC_FirstPersonCamera will override position when integrated
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// Visible first-person arms — child of camera so arms follow the view
	// Skeletal mesh set by Blueprint/Python (SKM_Operator_Mono or similar)
	PlayerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerMesh"));
	PlayerMesh->SetupAttachment(FirstPersonCamera);
	PlayerMesh->SetOnlyOwnerSee(true);
	PlayerMesh->bCastDynamicShadow = false;
	PlayerMesh->CastShadow = false;

	// Movement defaults (overridden by DataAsset in BeginPlay)
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

void AZP_GraceCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Apply DataAsset config (if set)
	ApplyMovementConfig();

	// Capture base camera Z for head bob
	BaseCameraZ = FirstPersonCamera->GetRelativeLocation().Z;

	// Initialize stamina
	if (MovementConfig)
	{
		CurrentStamina = MovementConfig->MaxStamina;
	}

	// Cache EventBroadcaster
	if (UGameInstance* GI = GetGameInstance())
	{
		EventBroadcaster = GI->GetSubsystem<UZP_EventBroadcaster>();
	}

	// Wire Kinemation camera (gracefully skips if TacticalCameraComp not set)
	InitKinemationCamera();

	// Wire Kinemation animation components (gracefully skips if not present)
	InitKinemationAnimation();

	// Spawn and equip default weapon (gracefully skips if WeaponClass not set)
	SpawnAndEquipWeapon();

	// Disable built-in head bob when Kinemation drives the camera
	if (TacticalAnimComp)
	{
		bUseBuiltInHeadBob = false;
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Kinemation active — built-in head bob DISABLED."));
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter BeginPlay — Config: %s, Stamina: %.0f, Kinemation: Camera=%s Anim=%s Weapon=%s"),
		MovementConfig ? *MovementConfig->GetName() : TEXT("NONE (using defaults)"),
		CurrentStamina,
		TacticalCameraComp ? TEXT("OK") : TEXT("OFF"),
		TacticalAnimComp ? TEXT("OK") : TEXT("OFF"),
		ActiveWeapon ? *ActiveWeapon->GetName() : TEXT("NONE"));
}

void AZP_GraceCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bUseBuiltInHeadBob)
	{
		UpdateHeadBob(DeltaTime);
	}

	UpdatePeek(DeltaTime);
	UpdateStamina(DeltaTime);
	UpdateInteractionTrace();
}

// --- Config Application ---

void AZP_GraceCharacter::ApplyMovementConfig()
{
	if (!MovementConfig)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ZP_GraceCharacter: No MovementConfig DataAsset set. Using constructor defaults."));
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->MaxWalkSpeed = MovementConfig->WalkSpeed;
	MoveComp->BrakingDecelerationWalking = MovementConfig->BrakingDeceleration;
	MoveComp->MaxAcceleration = MovementConfig->MaxAcceleration;
	MoveComp->GroundFriction = MovementConfig->GroundFriction;
	MoveComp->JumpZVelocity = MovementConfig->JumpZVelocity;
	MoveComp->AirControl = MovementConfig->AirControl;

	// Apply camera height
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, MovementConfig->CameraHeightOffset));
	FirstPersonCamera->SetFieldOfView(MovementConfig->DefaultFOV);
}

// --- Kinemation Camera ---

void AZP_GraceCharacter::InitKinemationCamera()
{
	// Auto-detect AC_FirstPersonCamera if TacticalCameraComp not explicitly set
	if (!TacticalCameraComp)
	{
		for (UActorComponent* Comp : GetComponents())
		{
			if (Comp && Comp->GetClass()->GetName().Contains(TEXT("AC_FirstPersonCamera")))
			{
				TacticalCameraComp = Comp;
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] InitKinemationCamera: Auto-detected %s"), *Comp->GetName());
				break;
			}
		}
	}

	if (!TacticalCameraComp)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] InitKinemationCamera: No AC_FirstPersonCamera found — skipping Kinemation wiring."));
		return;
	}

	// Wire AC_FirstPersonCamera to our camera component
	FKinemationBridge::UpdateTargetCamera(TacticalCameraComp, FirstPersonCamera);

	// Wire AC_FirstPersonCamera to our PlayerMesh
	if (PlayerMesh)
	{
		FKinemationBridge::UpdatePlayerMesh(TacticalCameraComp, PlayerMesh);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InitKinemationCamera: Wired AC_FirstPersonCamera — Camera: %s, Mesh: %s"),
		*FirstPersonCamera->GetName(),
		PlayerMesh ? *PlayerMesh->GetName() : TEXT("NONE"));
}

void AZP_GraceCharacter::TriggerCameraShake(UObject* ShakeData)
{
	if (!TacticalCameraComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TriggerCameraShake: TacticalCameraComp not set."));
		return;
	}
	FKinemationBridge::PlayCameraShake(TacticalCameraComp, ShakeData);
}

void AZP_GraceCharacter::SetTargetFOV(float NewFOV, float InterpSpeed)
{
	if (!TacticalCameraComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] SetTargetFOV: TacticalCameraComp not set."));
		return;
	}
	FKinemationBridge::UpdateTargetFOV(TacticalCameraComp, NewFOV, InterpSpeed);
}

// --- Kinemation Animation Components ---

void AZP_GraceCharacter::InitKinemationAnimation()
{
	for (UActorComponent* Comp : GetComponents())
	{
		if (!Comp) continue;

		const FString ClassName = Comp->GetClass()->GetName();

		if (!TacticalAnimComp && ClassName.Contains(TEXT("AC_TacticalShooterAnimation")))
		{
			TacticalAnimComp = Comp;
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] InitKinemationAnimation: Auto-detected TacticalAnimComp: %s"), *Comp->GetName());
		}
		else if (!RecoilAnimComp && ClassName.Contains(TEXT("AC_RecoilAnimation")))
		{
			RecoilAnimComp = Comp;
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] InitKinemationAnimation: Auto-detected RecoilAnimComp: %s"), *Comp->GetName());
		}
		else if (!IKMotionComp && ClassName.Contains(TEXT("AC_IKMotionPlayer")))
		{
			IKMotionComp = Comp;
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] InitKinemationAnimation: Auto-detected IKMotionComp: %s"), *Comp->GetName());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InitKinemationAnimation: TacticalAnim=%s, Recoil=%s, IK=%s"),
		TacticalAnimComp ? TEXT("OK") : TEXT("NONE"),
		RecoilAnimComp ? TEXT("OK") : TEXT("NONE"),
		IKMotionComp ? TEXT("OK") : TEXT("NONE"));
}

// --- Kinemation Weapon ---

void AZP_GraceCharacter::SpawnAndEquipWeapon()
{
	if (!WeaponClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] SpawnAndEquipWeapon: No WeaponClass set — skipping."));
		return;
	}

	if (!PlayerMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] SpawnAndEquipWeapon: PlayerMesh is null!"));
		return;
	}

	// Spawn weapon with Owner = this
	// Weapon's BeginPlay uses GetOwner() → GetComponentByClass() to find our components
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ActiveWeapon = GetWorld()->SpawnActor<AActor>(WeaponClass, FTransform::Identity, SpawnParams);
	if (!ActiveWeapon)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SpawnAndEquipWeapon: Failed to spawn weapon!"));
		return;
	}

	// Attach weapon to PlayerMesh at the gun socket
	const FName WeaponSocket(TEXT("VB ik_hand_gun"));
	ActiveWeapon->AttachToComponent(PlayerMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] SpawnAndEquipWeapon: Spawned %s, attached to %s at socket %s"),
		*ActiveWeapon->GetName(), *PlayerMesh->GetName(), *WeaponSocket.ToString());

	// Feed weapon's view settings to AC_TacticalShooterAnimation
	if (TacticalAnimComp)
	{
		UObject* Settings = FKinemationBridge::WeaponGetSettings(ActiveWeapon);
		if (Settings)
		{
			FKinemationBridge::AnimSetActiveSettings(TacticalAnimComp, Settings);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] SpawnAndEquipWeapon: Set ActiveSettings to %s"), *Settings->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] SpawnAndEquipWeapon: WeaponGetSettings returned null"));
		}
	}

	// Defer weapon draw to next tick — weapon internals + AnimInstance
	// need one frame to fully initialize after SpawnActor
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		if (ActiveWeapon)
		{
			FKinemationBridge::WeaponDraw(ActiveWeapon);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] SpawnAndEquipWeapon: WeaponDraw called (deferred)."));
		}
	});
}

// --- Sprint ---

void AZP_GraceCharacter::StartSprint()
{
	if (CurrentStamina <= 0.0f) return;

	bIsSprinting = true;

	const float Speed = MovementConfig ? MovementConfig->SprintSpeed : 350.0f;
	GetCharacterMovement()->MaxWalkSpeed = Speed;

	if (EventBroadcaster)
	{
		EventBroadcaster->BroadcastSprintChanged(true);
	}
}

void AZP_GraceCharacter::StopSprint()
{
	bIsSprinting = false;

	const float Speed = MovementConfig ? MovementConfig->WalkSpeed : 200.0f;
	GetCharacterMovement()->MaxWalkSpeed = Speed;

	// Start regen delay timer
	StaminaRegenTimer = MovementConfig ? MovementConfig->StaminaRegenDelay : 1.5f;

	if (EventBroadcaster)
	{
		EventBroadcaster->BroadcastSprintChanged(false);
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

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ZP_GraceCharacter: Input bound — Move:%s Look:%s Sprint:%s Jump:%s Interact:%s Crouch:%s Aim:%s Fire:%s Reload:%s"),
		MoveAction ? TEXT("OK") : TEXT("MISSING"),
		LookAction ? TEXT("OK") : TEXT("MISSING"),
		SprintAction ? TEXT("OK") : TEXT("MISSING"),
		JumpAction ? TEXT("OK") : TEXT("MISSING"),
		InteractAction ? TEXT("OK") : TEXT("MISSING"),
		CrouchAction ? TEXT("OK") : TEXT("MISSING"),
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
	StartSprint();
}

void AZP_GraceCharacter::Input_SprintCompleted(const FInputActionValue& Value)
{
	StopSprint();
}

void AZP_GraceCharacter::Input_Jump(const FInputActionValue& Value)
{
	Jump();
}

void AZP_GraceCharacter::Input_Interact(const FInputActionValue& Value)
{
	OnInteract(CurrentInteractionTarget);
}

void AZP_GraceCharacter::Input_CrouchStarted(const FInputActionValue& Value)
{
	Crouch();
}

void AZP_GraceCharacter::Input_CrouchCompleted(const FInputActionValue& Value)
{
	UnCrouch();
}

void AZP_GraceCharacter::Input_AimStarted(const FInputActionValue& Value)
{
	bWantsPeek = true;
}

void AZP_GraceCharacter::Input_AimCompleted(const FInputActionValue& Value)
{
	bWantsPeek = false;
}

void AZP_GraceCharacter::Input_FireStarted(const FInputActionValue& Value)
{
	if (ActiveWeapon)
	{
		FKinemationBridge::WeaponOnFirePressed(ActiveWeapon);
	}
}

void AZP_GraceCharacter::Input_FireCompleted(const FInputActionValue& Value)
{
	if (ActiveWeapon)
	{
		FKinemationBridge::WeaponOnFireReleased(ActiveWeapon);
	}
}

void AZP_GraceCharacter::Input_ReloadStarted(const FInputActionValue& Value)
{
	if (ActiveWeapon)
	{
		FKinemationBridge::WeaponOnReload(ActiveWeapon);
	}
}

// --- Head Bob (ported from TheSignalCharacter) ---

void AZP_GraceCharacter::UpdateHeadBob(float DeltaTime)
{
	const float Speed = GetVelocity().Size2D();
	const float MinSpeedForBob = 10.0f;

	// Config values (with fallback defaults)
	const float BobFreq = MovementConfig ? MovementConfig->HeadBobFrequency : 1.6f;
	const float BobVertAmp = MovementConfig ? MovementConfig->HeadBobVerticalAmplitude : 1.2f;
	const float BobHorizAmp = MovementConfig ? MovementConfig->HeadBobHorizontalAmplitude : 0.6f;
	const float SprintFreqMult = MovementConfig ? MovementConfig->SprintBobFrequencyMultiplier : 1.4f;
	const float SprintAmpMult = MovementConfig ? MovementConfig->SprintBobAmplitudeMultiplier : 1.3f;
	const float ReturnSpeed = MovementConfig ? MovementConfig->HeadBobReturnSpeed : 6.0f;

	// Peek damping: reduce bob amplitude when peeking
	const float PeekHorizDamp = MovementConfig ? FMath::Lerp(1.0f, MovementConfig->HeadBobPeekDamping, PeekAlpha) : 1.0f;
	const float PeekVertDamp = MovementConfig ? FMath::Lerp(1.0f, MovementConfig->HeadBobPeekVerticalDamping, PeekAlpha) : 1.0f;

	if (Speed > MinSpeedForBob && GetCharacterMovement()->IsMovingOnGround())
	{
		const float FreqMultiplier = bIsSprinting ? SprintFreqMult : 1.0f;
		const float AmpMultiplier = bIsSprinting ? SprintAmpMult : 1.0f;

		HeadBobTimer += DeltaTime * BobFreq * FreqMultiplier * PI * 2.0f;

		HeadBobOffsetZ = FMath::Sin(HeadBobTimer) * BobVertAmp * AmpMultiplier * PeekVertDamp;
		HeadBobOffsetY = FMath::Cos(HeadBobTimer * 0.5f) * BobHorizAmp * AmpMultiplier * PeekHorizDamp;
	}
	else
	{
		// Smooth return to rest
		HeadBobTimer = 0.0f;
		HeadBobOffsetZ = FMath::FInterpTo(HeadBobOffsetZ, 0.0f, DeltaTime, ReturnSpeed);
		HeadBobOffsetY = FMath::FInterpTo(HeadBobOffsetY, 0.0f, DeltaTime, ReturnSpeed);
	}
	// Camera position is composed in UpdatePeek (bob + peek offsets together)
}

// --- Stamina ---

void AZP_GraceCharacter::UpdateStamina(float DeltaTime)
{
	const float MaxStam = MovementConfig ? MovementConfig->MaxStamina : 100.0f;
	const float DrainRate = MovementConfig ? MovementConfig->StaminaDrainRate : 20.0f;
	const float RegenRate = MovementConfig ? MovementConfig->StaminaRegenRate : 15.0f;

	if (bIsSprinting)
	{
		CurrentStamina = FMath::Max(0.0f, CurrentStamina - DrainRate * DeltaTime);

		if (CurrentStamina <= 0.0f)
		{
			StopSprint();
		}
	}
	else
	{
		// Regen delay countdown
		if (StaminaRegenTimer > 0.0f)
		{
			StaminaRegenTimer -= DeltaTime;
		}
		else
		{
			CurrentStamina = FMath::Min(MaxStam, CurrentStamina + RegenRate * DeltaTime);
		}
	}

	// Broadcast normalized stamina
	if (EventBroadcaster && MaxStam > 0.0f)
	{
		EventBroadcaster->BroadcastStaminaChanged(CurrentStamina / MaxStam);
	}
}

// --- Interaction Trace ---

void AZP_GraceCharacter::UpdateInteractionTrace()
{
	const float TraceRange = MovementConfig ? MovementConfig->InteractionTraceRange : 250.0f;

	FVector Start = FirstPersonCamera->GetComponentLocation();
	FVector End = Start + FirstPersonCamera->GetForwardVector() * TraceRange;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	AActor* PreviousTarget = CurrentInteractionTarget;

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		CurrentInteractionTarget = Hit.GetActor();
	}
	else
	{
		CurrentInteractionTarget = nullptr;
	}

	// Broadcast only on change
	if (CurrentInteractionTarget != PreviousTarget && EventBroadcaster)
	{
		EventBroadcaster->BroadcastInteractionTargetChanged(CurrentInteractionTarget);
	}
}

// --- Peek System ---

int32 AZP_GraceCharacter::TracePeekSide(const FVector& Origin, const FVector& Forward, const FVector& Right, float DirectionSign) const
{
	const float TraceRange = MovementConfig ? MovementConfig->PeekWallDetectionRange : 120.0f;
	const float TraceRadius = MovementConfig ? MovementConfig->PeekTraceRadius : 10.0f;
	const float FanHalfAngle = MovementConfig ? MovementConfig->PeekTraceFanHalfAngle : 60.0f;
	const float MaxWallAngle = MovementConfig ? MovementConfig->PeekMaxWallAngleFromVertical : 20.0f;

	const FVector SideDir = Right * DirectionSign;

	// 5-ray fan: perpendicular + 2 pairs spread across the half-angle
	const float Angles[] = { 0.0f, -FanHalfAngle * 0.5f, FanHalfAngle * 0.5f, -FanHalfAngle, FanHalfAngle };

	int32 HitCount = 0;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	for (int32 i = 0; i < 5; ++i)
	{
		const FVector TraceDir = SideDir.RotateAngleAxis(Angles[i], FVector::UpVector).GetSafeNormal();
		const FVector TraceEnd = Origin + TraceDir * TraceRange;

		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, Origin, TraceEnd, FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeSphere(TraceRadius), Params))
		{
			const float AngleFromVertical = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(FVector::DotProduct(Hit.ImpactNormal, FVector::UpVector))));
			if (AngleFromVertical > (90.0f - MaxWallAngle))
			{
				++HitCount;
			}
		}
	}

	return HitCount;
}

EZP_PeekDirection AZP_GraceCharacter::DetectPeekDirection() const
{
	if (!Controller) return EZP_PeekDirection::None;

	const int32 Threshold = MovementConfig ? MovementConfig->PeekWallHitThreshold : 2;

	const FVector Origin = FirstPersonCamera->GetComponentLocation();
	const FRotator ControlRot = Controller->GetControlRotation();
	const FVector Forward = FRotationMatrix(FRotator(0.0f, ControlRot.Yaw, 0.0f)).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(FRotator(0.0f, ControlRot.Yaw, 0.0f)).GetUnitAxis(EAxis::Y);

	const int32 LeftHits = TracePeekSide(Origin, Forward, Right, -1.0f);
	const int32 RightHits = TracePeekSide(Origin, Forward, Right, 1.0f);

	const bool bWallLeft = (LeftHits >= Threshold);
	const bool bWallRight = (RightHits >= Threshold);

	if (bWallLeft && !bWallRight)
	{
		return EZP_PeekDirection::Right;
	}
	if (bWallRight && !bWallLeft)
	{
		return EZP_PeekDirection::Left;
	}

	return EZP_PeekDirection::None;
}

void AZP_GraceCharacter::UpdatePeek(float DeltaTime)
{
	const float InterpSpeed = MovementConfig ? MovementConfig->PeekInterpSpeed : 8.0f;
	const float ReturnSpeed = MovementConfig ? MovementConfig->PeekReturnInterpSpeed : 6.0f;
	const float LateralOffset = MovementConfig ? MovementConfig->PeekLateralOffset : 25.0f;
	const float ForwardOffset = MovementConfig ? MovementConfig->PeekForwardOffset : 8.0f;
	const float RollAngle = MovementConfig ? MovementConfig->PeekRollAngle : 3.0f;

	// Determine desired peek direction — detect once on RMB press, then lock
	EZP_PeekDirection DesiredDirection = EZP_PeekDirection::None;

	const bool bOnGround = GetCharacterMovement()->IsMovingOnGround();
	const bool bCanPeek = bWantsPeek
		&& !bIsSprinting
		&& bOnGround;

	if (bCanPeek)
	{
		if (!bPeekLocked)
		{
			LockedPeekDirection = DetectPeekDirection();
			bPeekLocked = true;
		}
		DesiredDirection = LockedPeekDirection;
	}
	else
	{
		// RMB released or can't peek — unlock
		if (bPeekLocked)
		{
			bPeekLocked = false;
			LockedPeekDirection = EZP_PeekDirection::None;
		}
	}

	CurrentPeekDirection = DesiredDirection;

	// Static target: full lean when wall detected
	const float TargetAlpha = (CurrentPeekDirection != EZP_PeekDirection::None) ? 1.0f : 0.0f;
	const float Speed = (TargetAlpha > PeekAlpha) ? InterpSpeed : ReturnSpeed;
	PeekAlpha = FMath::FInterpTo(PeekAlpha, TargetAlpha, DeltaTime, Speed);

	// Snap to zero when very close (avoid floating-point drift)
	if (PeekAlpha < 0.001f)
	{
		PeekAlpha = 0.0f;
	}

	// Compute direction sign: Left = -1, Right = +1
	// Use PreviousPeekDirection during exit interpolation so camera returns from correct side
	const EZP_PeekDirection ActiveDirection = (CurrentPeekDirection != EZP_PeekDirection::None)
		? CurrentPeekDirection
		: PreviousPeekDirection;

	const float DirSign = (ActiveDirection == EZP_PeekDirection::Left) ? -1.0f
		: (ActiveDirection == EZP_PeekDirection::Right) ? 1.0f
		: 0.0f;

	// Compose final camera position: base + head bob + peek (no fighting)
	const float PeekY = LateralOffset * DirSign * PeekAlpha;
	const float PeekX = ForwardOffset * PeekAlpha;

	FVector FinalCameraLoc;
	FinalCameraLoc.X = PeekX;
	FinalCameraLoc.Y = HeadBobOffsetY + PeekY;
	FinalCameraLoc.Z = BaseCameraZ + HeadBobOffsetZ;
	FirstPersonCamera->SetRelativeLocation(FinalCameraLoc);

	// Apply roll via RelativeRotation (safe — bUsePawnControlRotation only drives Pitch/Yaw)
	const float TargetRoll = RollAngle * DirSign * PeekAlpha;
	CurrentPeekRoll = FMath::FInterpTo(CurrentPeekRoll, TargetRoll, DeltaTime, Speed);
	FRotator CameraRot = FirstPersonCamera->GetRelativeRotation();
	CameraRot.Roll = CurrentPeekRoll;
	FirstPersonCamera->SetRelativeRotation(CameraRot);

	// Broadcast on direction change
	if (CurrentPeekDirection != PreviousPeekDirection && EventBroadcaster)
	{
		const int32 BroadcastDir = (CurrentPeekDirection == EZP_PeekDirection::Left) ? -1
			: (CurrentPeekDirection == EZP_PeekDirection::Right) ? 1
			: 0;
		EventBroadcaster->BroadcastPeekChanged(BroadcastDir, PeekAlpha);
	}

	// Track previous direction for exit interpolation
	if (CurrentPeekDirection != EZP_PeekDirection::None)
	{
		PreviousPeekDirection = CurrentPeekDirection;
	}
	// Clear previous once fully returned
	if (PeekAlpha == 0.0f)
	{
		PreviousPeekDirection = EZP_PeekDirection::None;
	}
}
