// Copyright The Signal. All Rights Reserved.

#include "ZP_GraceGameplayComponent.h"
#include "ZP_GraceMovementConfig.h"
#include "ZP_EventBroadcaster.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UZP_GraceGameplayComponent::UZP_GraceGameplayComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UZP_GraceGameplayComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Guarantee MovementConfig is never null — create transient default if none assigned.
	// All values come from UPROPERTY defaults in ZP_GraceMovementConfig.h.
	if (!MovementConfig)
	{
		MovementConfig = NewObject<UZP_GraceMovementConfig>(this);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] GraceGameplayComponent: No DataAsset assigned — created default MovementConfig from C++ defaults."));
	}

	// Cache CharacterMovementComponent
	if (ACharacter* Char = Cast<ACharacter>(Owner))
	{
		CachedMovement = Char->GetCharacterMovement();
	}

	// Auto-discover camera if not explicitly set
	if (!CameraComponent)
	{
		for (UActorComponent* Comp : Owner->GetComponents())
		{
			if (UCameraComponent* Cam = Cast<UCameraComponent>(Comp))
			{
				if (Cam->GetName() == TEXT("FirstPersonCamera"))
				{
					CameraComponent = Cam;
					break;
				}
			}
		}
	}

	// Cache EventBroadcaster
	if (UGameInstance* GI = Owner->GetGameInstance())
	{
		EventBroadcaster = GI->GetSubsystem<UZP_EventBroadcaster>();
	}

	// Apply movement config
	ApplyMovementConfig();

	// Camera is attached to FPCamera socket — its position is driven by the skeleton.
	// BaseCameraZ = 0 so head bob operates as offset from the socket origin.
	BaseCameraZ = 0.0f;

	// Cache the PlayerMesh (camera's attach parent) for mesh-level peek.
	// Peek offsets go to the mesh so the gun follows the lean.
	if (CameraComponent)
	{
		CachedMeshComponent = CameraComponent->GetAttachParent();
		if (CachedMeshComponent)
		{
			CachedMeshBaseLocation = CachedMeshComponent->GetRelativeLocation();
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] Cached mesh base location: %s"), *CachedMeshBaseLocation.ToString());
		}
	}

	// Initialize stamina
	CurrentStamina = MovementConfig->MaxStamina;

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] GraceGameplayComponent BeginPlay — Config: %s, Stamina: %.0f, Camera: %s"),
		*MovementConfig->GetName(),
		CurrentStamina,
		CameraComponent ? *CameraComponent->GetName() : TEXT("NONE"));
}

void UZP_GraceGameplayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUseBuiltInHeadBob)
	{
		UpdateHeadBob(DeltaTime);
	}

	UpdatePeek(DeltaTime);
	UpdateStamina(DeltaTime);
	UpdateInteractionTrace();
	UpdateGASPState();
}

// --- Config Application ---

void UZP_GraceGameplayComponent::ApplyMovementConfig()
{
	if (!MovementConfig) return;

	if (CachedMovement)
	{
		CachedMovement->MaxWalkSpeed = MovementConfig->WalkSpeed;
		CachedMovement->BrakingDecelerationWalking = MovementConfig->BrakingDeceleration;
		CachedMovement->MaxAcceleration = MovementConfig->MaxAcceleration;
		CachedMovement->GroundFriction = MovementConfig->GroundFriction;
		CachedMovement->JumpZVelocity = MovementConfig->JumpZVelocity;
		CachedMovement->AirControl = MovementConfig->AirControl;
	}

	if (CameraComponent)
	{
		// Camera position is driven by FPCamera socket attachment — don't override with height offset.
		// Only set FOV from config.
		CameraComponent->SetFieldOfView(MovementConfig->DefaultFOV);
	}
}

// --- Sprint ---

void UZP_GraceGameplayComponent::StartSprint()
{
	if (CurrentStamina <= 0.0f) return;

	bIsSprinting = true;

	if (CachedMovement)
	{
		CachedMovement->MaxWalkSpeed = MovementConfig->SprintSpeed;
	}

	if (EventBroadcaster)
	{
		EventBroadcaster->BroadcastSprintChanged(true);
	}
}

void UZP_GraceGameplayComponent::StopSprint()
{
	bIsSprinting = false;

	if (CachedMovement)
	{
		CachedMovement->MaxWalkSpeed = MovementConfig->WalkSpeed;
	}

	// Start regen delay timer
	StaminaRegenTimer = MovementConfig->StaminaRegenDelay;

	if (EventBroadcaster)
	{
		EventBroadcaster->BroadcastSprintChanged(false);
	}
}

// --- Head Bob ---

void UZP_GraceGameplayComponent::UpdateHeadBob(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner || !CachedMovement) return;

	const float Speed = Owner->GetVelocity().Size2D();
	const float MinSpeedForBob = 10.0f;

	// Config values
	const float BobFreq = MovementConfig->HeadBobFrequency;
	const float BobVertAmp = MovementConfig->HeadBobVerticalAmplitude;
	const float BobHorizAmp = MovementConfig->HeadBobHorizontalAmplitude;
	const float SprintFreqMult = MovementConfig->SprintBobFrequencyMultiplier;
	const float SprintAmpMult = MovementConfig->SprintBobAmplitudeMultiplier;
	const float ReturnSpeed = MovementConfig->HeadBobReturnSpeed;

	// Peek damping: reduce bob when peeking
	const float PeekDamp = FMath::Lerp(1.0f, MovementConfig->HeadBobPeekDamping, PeekAlpha);

	if (Speed > MinSpeedForBob && CachedMovement->IsMovingOnGround())
	{
		const float FreqMultiplier = bIsSprinting ? SprintFreqMult : 1.0f;
		const float AmpMultiplier = bIsSprinting ? SprintAmpMult : 1.0f;

		HeadBobTimer += DeltaTime * BobFreq * FreqMultiplier * PI * 2.0f;

		// Positional bob — vertical step + horizontal weight shift
		HeadBobOffsetZ = FMath::Sin(HeadBobTimer) * BobVertAmp * AmpMultiplier * PeekDamp;
		HeadBobOffsetY = FMath::Cos(HeadBobTimer * 0.5f) * BobHorizAmp * AmpMultiplier * PeekDamp;
	}
	else
	{
		// Smooth return to rest
		HeadBobTimer = 0.0f;
		HeadBobOffsetZ = FMath::FInterpTo(HeadBobOffsetZ, 0.0f, DeltaTime, ReturnSpeed);
		HeadBobOffsetY = FMath::FInterpTo(HeadBobOffsetY, 0.0f, DeltaTime, ReturnSpeed);
	}
	// Position offsets are composed in UpdatePeek
}

// --- Stamina ---

void UZP_GraceGameplayComponent::UpdateStamina(float DeltaTime)
{
	const float MaxStam = MovementConfig->MaxStamina;
	const float DrainRate = MovementConfig->StaminaDrainRate;
	const float RegenRate = MovementConfig->StaminaRegenRate;

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

// --- GASP State ---

void UZP_GraceGameplayComponent::UpdateGASPState()
{
	// Compute gait for GASP AnimBP: 0=Walk, 1=Run, 2=Sprint (matches E_Gait enum order)
	if (bIsSprinting)
	{
		GASPGait = 2;
	}
	else
	{
		// Use walk speed as threshold — below it we're walking, above we're running.
		// CharacterMovementComponent controls actual speed; we just classify it.
		const float Speed = GetOwner() ? GetOwner()->GetVelocity().Size2D() : 0.0f;
		const float WalkThreshold = MovementConfig ? MovementConfig->WalkSpeed : 200.0f;
		GASPGait = (Speed > WalkThreshold * 0.5f) ? 1 : 0;
	}
}

// --- Interaction Trace ---

void UZP_GraceGameplayComponent::UpdateInteractionTrace()
{
	if (!CameraComponent) return;

	const float TraceRange = MovementConfig->InteractionTraceRange;

	FVector Start = CameraComponent->GetComponentLocation();
	FVector End = Start + CameraComponent->GetForwardVector() * TraceRange;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

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

int32 UZP_GraceGameplayComponent::TracePeekSide(const FVector& Origin, const FVector& Forward, const FVector& Right, float DirectionSign) const
{
	const float TraceRange = MovementConfig->PeekWallDetectionRange;
	const float TraceRadius = MovementConfig->PeekTraceRadius;
	const float FanHalfAngle = MovementConfig->PeekTraceFanHalfAngle;
	const float MaxWallAngle = MovementConfig->PeekMaxWallAngleFromVertical;

	const FVector SideDir = Right * DirectionSign;

	// 5-ray fan: perpendicular + 2 pairs spread across the half-angle
	const float Angles[] = { 0.0f, -FanHalfAngle * 0.5f, FanHalfAngle * 0.5f, -FanHalfAngle, FanHalfAngle };

	int32 HitCount = 0;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

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

EZP_PeekDirection UZP_GraceGameplayComponent::DetectPeekDirection() const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* Controller = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (!Controller || !CameraComponent) return EZP_PeekDirection::None;

	const int32 Threshold = MovementConfig->PeekWallHitThreshold;

	const FVector Origin = CameraComponent->GetComponentLocation();
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

void UZP_GraceGameplayComponent::UpdatePeek(float DeltaTime)
{
	if (!CameraComponent || !CachedMovement) return;

	const float InterpSpeed = MovementConfig->PeekInterpSpeed;
	const float ReturnSpeed = MovementConfig->PeekReturnInterpSpeed;
	const float LateralOffset = MovementConfig->PeekLateralOffset;
	const float ForwardOffset = MovementConfig->PeekForwardOffset;
	const float RollAngle = MovementConfig->PeekRollAngle;

	// Determine desired peek direction
	// Q key (bWantsPeek): camera-only lean, direction locked on press
	// RMB (bWantsAim): gun + camera lean when wall detected, direction locked on press
	EZP_PeekDirection DesiredDirection = EZP_PeekDirection::None;

	const bool bOnGround = CachedMovement->IsMovingOnGround();
	const bool bCanAct = !bIsSprinting && bOnGround;

	if (bWantsPeek && bCanAct)
	{
		if (!bPeekLocked)
		{
			LockedPeekDirection = DetectPeekDirection();
			bPeekFromAim = false;
			bPeekLocked = true;
		}
		DesiredDirection = LockedPeekDirection;
	}
	else if (bWantsAim && bCanAct)
	{
		if (!bPeekLocked)
		{
			EZP_PeekDirection WallDir = DetectPeekDirection();
			if (WallDir != EZP_PeekDirection::None)
			{
				LockedPeekDirection = WallDir;
				bPeekFromAim = true;
				bPeekLocked = true;
			}
		}
		DesiredDirection = LockedPeekDirection;
	}
	else
	{
		if (bPeekLocked)
		{
			bPeekLocked = false;
			LockedPeekDirection = EZP_PeekDirection::None;
		}
		// bPeekFromAim persists during exit interpolation
	}

	CurrentPeekDirection = DesiredDirection;

	// Alpha interpolation
	const float TargetAlpha = (CurrentPeekDirection != EZP_PeekDirection::None) ? 1.0f : 0.0f;
	const float Speed = (TargetAlpha > PeekAlpha) ? InterpSpeed : ReturnSpeed;
	PeekAlpha = FMath::FInterpTo(PeekAlpha, TargetAlpha, DeltaTime, Speed);

	if (PeekAlpha < 0.001f)
	{
		PeekAlpha = 0.0f;
	}

	// Direction sign: Left = -1, Right = +1
	const EZP_PeekDirection ActiveDirection = (CurrentPeekDirection != EZP_PeekDirection::None)
		? CurrentPeekDirection
		: PreviousPeekDirection;

	const float DirSign = (ActiveDirection == EZP_PeekDirection::Left) ? -1.0f
		: (ActiveDirection == EZP_PeekDirection::Right) ? 1.0f
		: 0.0f;

	// Peek offset in capsule space
	const float PeekY = LateralOffset * DirSign * PeekAlpha;
	const float PeekX = ForwardOffset * PeekAlpha;

	// --- Socket-space transform helper ---
	auto ToSocketSpace = [&](const FVector& CapsuleOffset) -> FVector
	{
		USceneComponent* AttachParent = CameraComponent->GetAttachParent();
		FName AttachSocket = CameraComponent->GetAttachSocketName();
		if (AttachParent && AttachSocket != NAME_None)
		{
			const FQuat SocketWorldRot = AttachParent->GetSocketQuaternion(AttachSocket);
			const FQuat CapsuleWorldRot = GetOwner()->GetActorQuat();
			const FQuat DeltaRot = SocketWorldRot.Inverse() * CapsuleWorldRot;
			return DeltaRot.RotateVector(CapsuleOffset);
		}
		return CapsuleOffset;
	};

	// --- Apply peek position based on source ---
	if (bPeekFromAim && CachedMeshComponent)
	{
		// ADS peek: move PlayerMesh — gun + camera + arms lean together
		FVector MeshLoc = CachedMeshBaseLocation;
		MeshLoc.X += PeekX;
		MeshLoc.Y += PeekY;
		CachedMeshComponent->SetRelativeLocation(MeshLoc);

		// Camera gets head bob only (socket-space transformed)
		FVector BobCapsule(0.0f, HeadBobOffsetY, BaseCameraZ + HeadBobOffsetZ);
		CameraComponent->SetRelativeLocation(ToSocketSpace(BobCapsule));
	}
	else
	{
		// Q peek (or no peek): camera-only peek + bob, mesh at base
		if (CachedMeshComponent)
		{
			CachedMeshComponent->SetRelativeLocation(CachedMeshBaseLocation);
		}

		FVector CapsuleOffset(PeekX, HeadBobOffsetY + PeekY, BaseCameraZ + HeadBobOffsetZ);
		CameraComponent->SetRelativeLocation(ToSocketSpace(CapsuleOffset));
	}

	// --- Peek roll (rotation) ---
	const float TargetRoll = RollAngle * DirSign * PeekAlpha;
	CurrentPeekRoll = FMath::FInterpTo(CurrentPeekRoll, TargetRoll, DeltaTime, Speed);

	FRotator CameraRot = CameraComponent->GetRelativeRotation();
	CameraRot.Roll = CurrentPeekRoll;
	CameraComponent->SetRelativeRotation(CameraRot);

	// Clear bPeekFromAim once fully returned
	if (PeekAlpha == 0.0f)
	{
		bPeekFromAim = false;
	}

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
	if (PeekAlpha == 0.0f)
	{
		PreviousPeekDirection = EZP_PeekDirection::None;
	}
}
