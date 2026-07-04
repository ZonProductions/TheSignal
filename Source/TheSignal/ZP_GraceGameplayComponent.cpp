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

	// Tick AFTER owner so crouch offset is set before we consume it
	AddTickPrerequisiteActor(Owner);

	// Guarantee AZP_MovementConfig is never null — create transient default if none assigned.
	// All values come from UPROPERTY defaults in ZP_GraceMovementConfig.h.
	if (!AZP_MovementConfig)
	{
		AZP_MovementConfig = NewObject<UZP_GraceMovementConfig>(this);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] GraceGameplayComponent: No DataAsset assigned — created default AZP_MovementConfig from C++ defaults."));
	}

	// Cache CharacterMovementComponent
	if (ACharacter* Char = Cast<ACharacter>(Owner))
	{
		CachedMovement = Char->GetCharacterMovement();
	}

	// Auto-discover camera if not explicitly set
	if (!AZP_CameraComponent)
	{
		for (UActorComponent* Comp : Owner->GetComponents())
		{
			if (UCameraComponent* Cam = Cast<UCameraComponent>(Comp))
			{
				if (Cam->GetName() == AZP_CameraAutoDiscoverName.ToString())
				{
					AZP_CameraComponent = Cam;
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

	// Camera is attached to FPCamera socket but offset 20cm forward (set in
	// the character constructor) to keep it ahead of the body geometry through
	// spine lean. Capture both X and Z from the camera's initial relative pos
	// so peek/bob math preserves the offset.
	if (AZP_CameraComponent)
	{
		const FVector InitRel = AZP_CameraComponent->GetRelativeLocation();
		BaseCameraX = InitRel.X;
		BaseCameraZ = InitRel.Z;
	}
	else
	{
		BaseCameraX = BaseCameraZ = 0.0f;
	}

	// Cache the PlayerMesh (camera's attach parent) for mesh-level peek.
	// Peek offsets go to the mesh so the gun follows the lean.
	if (AZP_CameraComponent)
	{
		CachedMeshComponent = AZP_CameraComponent->GetAttachParent();
		if (CachedMeshComponent)
		{
			CachedMeshBaseLocation = CachedMeshComponent->GetRelativeLocation();
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] Cached mesh base location: %s"), *CachedMeshBaseLocation.ToString());
		}
	}

	// Initialize stamina
	CurrentStamina = AZP_MovementConfig->AZP_MaxStamina;

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] GraceGameplayComponent BeginPlay — Config: %s, Stamina: %.0f, Camera: %s"),
		*AZP_MovementConfig->GetName(),
		CurrentStamina,
		AZP_CameraComponent ? *AZP_CameraComponent->GetName() : TEXT("NONE"));
}

void UZP_GraceGameplayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Smooth crouch camera transition — lerp compensation offset toward 0
	if (CrouchMeshOffsetZ != 0.0f)
	{
		const float AZP_InterpSpeed = AZP_MovementConfig ? AZP_MovementConfig->AZP_CrouchCameraInterpSpeed : 10.0f;
		CrouchMeshOffsetZ = FMath::FInterpTo(CrouchMeshOffsetZ, 0.0f, DeltaTime, AZP_InterpSpeed);
		if (FMath::Abs(CrouchMeshOffsetZ) < 0.01f)
		{
			CrouchMeshOffsetZ = 0.0f;
		}
	}

	if (bAZP_UseBuiltInHeadBob)
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
	if (!AZP_MovementConfig) return;

	if (CachedMovement)
	{
		CachedMovement->MaxWalkSpeed = AZP_MovementConfig->AZP_WalkSpeed;
		CachedMovement->BrakingDecelerationWalking = AZP_MovementConfig->AZP_BrakingDeceleration;
		CachedMovement->MaxAcceleration = AZP_MovementConfig->AZP_MaxAcceleration;
		CachedMovement->GroundFriction = AZP_MovementConfig->AZP_GroundFriction;
		CachedMovement->JumpZVelocity = AZP_MovementConfig->AZP_JumpZVelocity;
		CachedMovement->AirControl = AZP_MovementConfig->AZP_AirControl;
	}

	if (AZP_CameraComponent)
	{
		// Camera position is driven by FPCamera socket attachment — don't override with height offset.
		// Only set FOV from config.
		AZP_CameraComponent->SetFieldOfView(AZP_MovementConfig->AZP_DefaultFOV);
	}
}

// --- Crouch Camera ---

void UZP_GraceGameplayComponent::OnCrouchHeightChanged(float HeightAdjust)
{
	CrouchMeshOffsetZ += HeightAdjust;
}

// --- Sprint ---

void UZP_GraceGameplayComponent::StartSprint()
{
	if (CurrentStamina <= 0.0f) return;
	if (CachedMovement && CachedMovement->IsCrouching()) return; // no sprint while crouched

	bIsSprinting = true;

	if (CachedMovement)
	{
		CachedMovement->MaxWalkSpeed = AZP_MovementConfig->AZP_SprintSpeed;
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
		CachedMovement->MaxWalkSpeed = AZP_MovementConfig->AZP_WalkSpeed;
	}

	// Start regen delay timer
	StaminaRegenTimer = AZP_MovementConfig->AZP_StaminaRegenDelay;

	if (EventBroadcaster)
	{
		EventBroadcaster->BroadcastSprintChanged(false);
	}
}

bool UZP_GraceGameplayComponent::TryConsumeStaminaPercent(float Percent)
{
	if (!AZP_MovementConfig) return false;

	const float MaxStam = AZP_MovementConfig->AZP_MaxStamina;
	const float Cost = MaxStam * (Percent / 100.0f);
	if (Cost <= 0.0f) return true; // free action

	if (CurrentStamina < Cost) return false; // not enough — spend nothing

	CurrentStamina = FMath::Max(0.0f, CurrentStamina - Cost);

	// Hold off auto-regen so a dodge actually costs something.
	StaminaRegenTimer = AZP_MovementConfig->AZP_StaminaRegenDelay;

	if (EventBroadcaster && MaxStam > 0.0f)
	{
		EventBroadcaster->BroadcastStaminaChanged(CurrentStamina / MaxStam);
	}
	return true;
}

bool UZP_GraceGameplayComponent::DrainStaminaPercent(float Percent)
{
	if (!AZP_MovementConfig || Percent <= 0.0f) { return CurrentStamina > 0.0f; }

	const float MaxStam = AZP_MovementConfig->AZP_MaxStamina;
	CurrentStamina = FMath::Max(0.0f, CurrentStamina - MaxStam * (Percent / 100.0f));

	// Continuous costs (holding block) also hold off auto-regen — no regen while the pose is held.
	StaminaRegenTimer = AZP_MovementConfig->AZP_StaminaRegenDelay;

	if (EventBroadcaster && MaxStam > 0.0f)
	{
		EventBroadcaster->BroadcastStaminaChanged(CurrentStamina / MaxStam);
	}
	return CurrentStamina > 0.0f;
}

float UZP_GraceGameplayComponent::GetStaminaFraction() const
{
	if (!AZP_MovementConfig || AZP_MovementConfig->AZP_MaxStamina <= 0.0f) { return 0.0f; }
	return CurrentStamina / AZP_MovementConfig->AZP_MaxStamina;
}

// --- Head Bob ---

void UZP_GraceGameplayComponent::UpdateHeadBob(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner || !CachedMovement) return;

	const float Speed = Owner->GetVelocity().Size2D();
	const float MinSpeedForBob = AZP_MinSpeedForBob;

	// Config values
	const float BobFreq = AZP_MovementConfig->AZP_HeadBobFrequency;
	const float BobVertAmp = AZP_MovementConfig->AZP_HeadBobVerticalAmplitude;
	const float BobHorizAmp = AZP_MovementConfig->AZP_HeadBobHorizontalAmplitude;
	const float SprintFreqMult = AZP_MovementConfig->AZP_SprintBobFrequencyMultiplier;
	const float SprintAmpMult = AZP_MovementConfig->AZP_SprintBobAmplitudeMultiplier;
	const float ReturnSpeed = AZP_MovementConfig->AZP_HeadBobReturnSpeed;

	// Peek damping: reduce bob when peeking
	const float PeekDamp = FMath::Lerp(1.0f, AZP_MovementConfig->AZP_HeadBobPeekDamping, PeekAlpha);

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
	const float MaxStam = AZP_MovementConfig->AZP_MaxStamina;
	const float DrainRate = AZP_MovementConfig->AZP_StaminaDrainRate;
	const float RegenRate = AZP_MovementConfig->AZP_StaminaRegenRate;

	const bool bOnGround = CachedMovement && CachedMovement->IsMovingOnGround();
	const FVector Velocity = CachedMovement ? CachedMovement->Velocity : FVector::ZeroVector;
	const FVector Forward = GetOwner() ? GetOwner()->GetActorForwardVector() : FVector::ForwardVector;
	const float ForwardSpeed = FVector::DotProduct(Velocity, Forward);
	const float PlanarSpeed = Velocity.Size2D();
	const bool bWantsForward = CurrentForwardInput > 0.1f;
	const bool bCrouched = CachedMovement && CachedMovement->IsCrouching();

	// Actively sprint-moving (and therefore draining). Regen is gated on THIS, not on whether
	// the sprint key is still held — holding sprint while standing still must still regen. Crouch
	// never drains (you can't sprint crouched), so crouching always regens.
	const bool bDraining = bIsSprinting && bOnGround && bWantsForward && ForwardSpeed > AZP_SprintDrainForwardSpeedThreshold && !bCrouched;

	// Wall stall: pressing forward on ground but not actually moving → killed by wall.
	if (bIsSprinting && bOnGround && bWantsForward && PlanarSpeed < AZP_WallStuckSpeedThreshold)
	{
		WallStuckTimer += DeltaTime;
		if (WallStuckTimer > AZP_WallStuckCancelTime)
		{
			StopSprint();
			WallStuckTimer = 0.0f;
		}
	}
	else
	{
		WallStuckTimer = 0.0f;
	}

	if (bDraining)
	{
		// Hold the regen delay full while draining, so the countdown begins the instant you stop.
		StaminaRegenTimer = AZP_MovementConfig->AZP_StaminaRegenDelay;
		CurrentStamina = FMath::Max(0.0f, CurrentStamina - DrainRate * DeltaTime);
		if (CurrentStamina <= 0.0f)
		{
			StopSprint();
		}
	}
	else
	{
		// Not draining (stopped, released, airborne, or sprint held while standing still):
		// count down the delay, then auto-regen — regardless of the sprint key.
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
		const float WalkThreshold = AZP_MovementConfig ? AZP_MovementConfig->AZP_WalkSpeed : 115.0f;
		GASPGait = (Speed > WalkThreshold * AZP_GaitRunSpeedRatio) ? 1 : 0;
	}
}

// --- Interaction Trace ---

void UZP_GraceGameplayComponent::UpdateInteractionTrace()
{
	if (!AZP_CameraComponent) return;

	const float TraceRange = AZP_MovementConfig->AZP_InteractionTraceRange;

	FVector Start = AZP_CameraComponent->GetComponentLocation();
	FVector End = Start + AZP_CameraComponent->GetForwardVector() * TraceRange;

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
	const float TraceRange = AZP_MovementConfig->AZP_PeekWallDetectionRange;
	const float TraceRadius = AZP_MovementConfig->AZP_PeekTraceRadius;
	const float FanHalfAngle = AZP_MovementConfig->AZP_PeekTraceFanHalfAngle;
	const float MaxWallAngle = AZP_MovementConfig->AZP_PeekMaxWallAngleFromVertical;

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
	if (!Controller || !AZP_CameraComponent) return EZP_PeekDirection::None;

	const int32 Threshold = AZP_MovementConfig->AZP_PeekWallHitThreshold;

	const FVector Origin = AZP_CameraComponent->GetComponentLocation();
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
	if (!AZP_CameraComponent || !CachedMovement) return;

	// Block/dodge forward camera clearance — a transient nudge that covers the
	// stance lean-IN, then settles. Snap to full INSTANTLY when active so there's
	// no ease-in frame where the body can flash into the lens ("cannot break at
	// all"); ease smoothly back to neutral when inactive so a sustained block hold
	// settles to the normal eye position instead of floating forward forever.
	if (bForwardClearanceActive)
	{
		CurrentForwardClearance = AZP_BlockDodgeForwardClearance;
	}
	else
	{
		CurrentForwardClearance = FMath::FInterpTo(
			CurrentForwardClearance, 0.0f, DeltaTime, AZP_ForwardClearanceInterpSpeed);
	}

	const float AZP_InterpSpeed = AZP_MovementConfig->AZP_PeekInterpSpeed;
	const float ReturnSpeed = AZP_MovementConfig->AZP_PeekReturnInterpSpeed;
	const float LateralOffset = AZP_MovementConfig->AZP_PeekLateralOffset;
	const float ForwardOffset = AZP_MovementConfig->AZP_PeekForwardOffset;
	const float RollAngle = AZP_MovementConfig->AZP_PeekRollAngle;

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
	const float Speed = (TargetAlpha > PeekAlpha) ? AZP_InterpSpeed : ReturnSpeed;
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
		USceneComponent* AttachParent = AZP_CameraComponent->GetAttachParent();
		FName AttachSocket = AZP_CameraComponent->GetAttachSocketName();
		if (AttachParent && AttachSocket != NAME_None)
		{
			const FQuat SocketWorldRot = AttachParent->GetSocketQuaternion(AttachSocket);
			const FQuat CapsuleWorldRot = GetOwner()->GetActorQuat();
			const FQuat DeltaRot = SocketWorldRot.Inverse() * CapsuleWorldRot;
			return DeltaRot.RotateVector(CapsuleOffset);
		}
		return CapsuleOffset;
	};

	// Marcus eye-offset while unarmed/melee; the separate ranged offsets while a ranged
	// weapon is up (Operator arms show — a different body needing its own framing).
	const float EffCamForward = bCameraOffsetActive ? AZP_CameraExtraForward : AZP_CameraRangedForward;
	const float EffCamHeight  = bCameraOffsetActive ? AZP_CameraExtraHeight  : AZP_CameraRangedHeight;

	// --- Apply peek position based on source ---
	if (bPeekFromAim && CachedMeshComponent)
	{
		// ADS peek: move PlayerMesh — gun + camera + arms lean together
		FVector MeshLoc = CachedMeshBaseLocation;
		MeshLoc.X += PeekX;
		MeshLoc.Y += PeekY;
		MeshLoc.Z += CrouchMeshOffsetZ;
		CachedMeshComponent->SetRelativeLocation(MeshLoc);

		// Camera gets head bob only (socket-space transformed); BaseCameraX
		// preserves the forward offset from the constructor. WeaponActionCamOffset
		// pushes the lens off the body during reload/switch/swing/block.
		FVector BobCapsule(BaseCameraX + CurrentForwardClearance + EffCamForward + WeaponActionCamOffset.X, HeadBobOffsetY + WeaponActionCamOffset.Y, BaseCameraZ + HeadBobOffsetZ + EffCamHeight + WeaponActionCamOffset.Z);
		AZP_CameraComponent->SetRelativeLocation(ToSocketSpace(BobCapsule));
	}
	else
	{
		// Q peek (or no peek): camera-only peek + bob, mesh at base + crouch offset
		if (CachedMeshComponent)
		{
			FVector MeshLoc = CachedMeshBaseLocation;
			MeshLoc.Z += CrouchMeshOffsetZ;
			CachedMeshComponent->SetRelativeLocation(MeshLoc);
		}

		FVector CapsuleOffset(BaseCameraX + PeekX + CurrentForwardClearance + EffCamForward + WeaponActionCamOffset.X, HeadBobOffsetY + PeekY + WeaponActionCamOffset.Y, BaseCameraZ + HeadBobOffsetZ + EffCamHeight + WeaponActionCamOffset.Z);
		AZP_CameraComponent->SetRelativeLocation(ToSocketSpace(CapsuleOffset));
	}

	// --- Peek roll (rotation) ---
	const float TargetRoll = RollAngle * DirSign * PeekAlpha;
	CurrentPeekRoll = FMath::FInterpTo(CurrentPeekRoll, TargetRoll, DeltaTime, Speed);

	FRotator CameraRot = AZP_CameraComponent->GetRelativeRotation();
	CameraRot.Roll = CurrentPeekRoll;
	AZP_CameraComponent->SetRelativeRotation(CameraRot);

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
