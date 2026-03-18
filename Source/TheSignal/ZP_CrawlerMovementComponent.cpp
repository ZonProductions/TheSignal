// Copyright The Signal. All Rights Reserved.

#include "ZP_CrawlerMovementComponent.h"
#include "GameFramework/Character.h"

static constexpr float GroundGravity = 980.f;
static constexpr float TerminalVelocity = 2000.f;
static constexpr float ClimbSpeed = 300.f;
static constexpr float WallContactDuration = 1.5f;

UZP_CrawlerMovementComponent::UZP_CrawlerMovementComponent()
{
	MaxStepHeight = 200.f;
}

FVector UZP_CrawlerMovementComponent::GetEffectiveTarget() const
{
	if (MoveTargetActor.IsValid())
	{
		return MoveTargetActor->GetActorLocation();
	}
	return MoveTarget;
}

void UZP_CrawlerMovementComponent::BeginSlam(float RiseHeight, float HoldDuration, float FallSpeed)
{
	if (bSlamming) return;

	bSlamming = true;
	bSlamImpacted = false;
	SlamTimer = 0.f;
	SlamHoldTime = HoldDuration;
	WallContactTimer = 0.f;
	ContactWallNormal = FVector::ZeroVector;
}

void UZP_CrawlerMovementComponent::PhysFlying(float deltaTime, int32 Iterations)
{
	if (deltaTime < UE_SMALL_NUMBER) return;
	deltaTime = FMath::Min(deltaTime, 0.05f);

	RestorePreAdditiveRootMotionVelocity();

	// Tick down wall contact
	if (WallContactTimer > 0.f)
	{
		WallContactTimer -= deltaTime;
		if (WallContactTimer <= 0.f)
		{
			WallContactTimer = 0.f;
			ContactWallNormal = FVector::ZeroVector;
			bJustCrested = true;
		}
	}

	// ========================================================================
	// VELOCITY (3 branches: slam / climb / ground pursuit)
	// ========================================================================

	// Tick slam timer — creature keeps moving during slam (no freeze)
	if (bSlamming)
	{
		SlamTimer += deltaTime;
		if (SlamTimer >= SlamHoldTime)
		{
			bSlamming = false;
			bSlamImpacted = true;
		}
	}

	if (WallContactTimer > 0.f && !ContactWallNormal.IsNearlyZero())
	{
		// CLIMBING: move up along wall surface toward target
		const FVector CurrentPos = UpdatedComponent->GetComponentLocation();
		const FVector Target = GetEffectiveTarget();
		const FVector ToTarget = (Target - CurrentPos).GetSafeNormal();

		// Project target direction onto wall plane, bias upward
		FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, ContactWallNormal).GetSafeNormal();
		FVector ToTargetOnWall = FVector::VectorPlaneProject(ToTarget, ContactWallNormal).GetSafeNormal();

		// Blend: mostly toward target on wall, some upward bias
		FVector ClimbDir;
		if (!ToTargetOnWall.IsNearlyZero())
		{
			ClimbDir = (ToTargetOnWall * 0.6f + WallUp * 0.4f).GetSafeNormal();
		}
		else
		{
			ClimbDir = WallUp;
		}

		// Press slightly into wall to maintain contact
		Velocity = (ClimbDir * 0.92f - ContactWallNormal * 0.08f).GetSafeNormal() * ClimbSpeed;

		UE_LOG(LogTemp, Verbose, TEXT("[ZP_CMC] %s: CLIMBING — vel=(%s) wallTimer=%.2f"),
			*GetOwner()->GetName(), *Velocity.ToCompactString(), WallContactTimer);
	}
	else if (bOnCeiling)
	{
		// ON CEILING: stay put, no gravity. Behavior will handle when to come down.
		Velocity = FVector::ZeroVector;
	}
	else
	{
		// GROUND PURSUIT: move toward target horizontally, apply gravity
		if (Velocity.Z > 0.f) Velocity.Z = 0.f;

		const FVector Target = GetEffectiveTarget();
		const FVector CurrentPos = UpdatedComponent->GetComponentLocation();
		const FVector ToTarget = Target - CurrentPos;
		const FVector HorizDir = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();
		const float Speed = GetMaxSpeed();

		if (!HorizDir.IsNearlyZero() && Speed > UE_SMALL_NUMBER)
		{
			const FVector CurrentHoriz = FVector(Velocity.X, Velocity.Y, 0.f);
			const FVector DesiredHoriz = HorizDir * Speed;
			const FVector Smoothed = FMath::VInterpTo(CurrentHoriz, DesiredHoriz, deltaTime, 8.0f);
			Velocity.X = Smoothed.X;
			Velocity.Y = Smoothed.Y;
		}
		else
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}

		// Gravity
		Velocity.Z -= GroundGravity * deltaTime;
		Velocity.Z = FMath::Max(Velocity.Z, -TerminalVelocity);
	}

	// ========================================================================
	// MOVE
	// ========================================================================
	Iterations++;
	bJustTeleported = false;

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;

	if (Adjusted.IsNearlyZero()) return;

	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.bBlockingHit && !Hit.bStartPenetrating)
	{
		const FVector HitNormal = Hit.ImpactNormal;
		const bool bIsFloor = HitNormal.Z > 0.7f;
		const bool bIsWall = FMath::Abs(HitNormal.Z) < 0.5f;
		const bool bIsCeiling = HitNormal.Z < -0.7f;

		if (bIsCeiling && bClimbEnabled)
		{
			// Hit ceiling while climbing — perch here
			bOnCeiling = true;
			WallContactTimer = 0.f;
			ContactWallNormal = FVector::ZeroVector;
			Velocity = FVector::ZeroVector;
			bJustCrested = true;

			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: HIT CEILING — perching"),
				*GetOwner()->GetName());
		}
		else if (bIsWall && bClimbEnabled)
		{
			// Verify this is a structural wall (blocks CrawlerSurface channel), not furniture.
			// Capsule collision hits everything, but we only want to climb room walls.
			FHitResult VerifyHit;
			FCollisionQueryParams VerifyParams;
			VerifyParams.AddIgnoredActor(GetOwner());
			const FVector VerifyStart = Hit.ImpactPoint + HitNormal * 20.f;
			const FVector VerifyEnd = Hit.ImpactPoint - HitNormal * 20.f;
			const bool bStructuralWall = GetWorld()->LineTraceSingleByChannel(
				VerifyHit, VerifyStart, VerifyEnd, ECC_GameTraceChannel1, VerifyParams);

			if (!bStructuralWall)
			{
				// Furniture — slide along it, don't climb
				UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: WALL HIT — FURNITURE (not structural), sliding. actor=%s"),
					*GetOwner()->GetName(), Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("null"));

				const FVector Remainder = Adjusted * (1.f - Hit.Time);
				FVector WallSlide = FVector::VectorPlaneProject(Remainder, HitNormal);
				if (!WallSlide.IsNearlyZero())
				{
					FHitResult SlideHit(1.f);
					SafeMoveUpdatedComponent(WallSlide, UpdatedComponent->GetComponentQuat(), true, SlideHit);
				}
			}
			else
			{
			// Structural wall — engage/refresh climb
			WallContactTimer = WallContactDuration;
			ContactWallNormal = HitNormal;

			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: WALL HIT — climb engaged, N=(%s), actor=%s"),
				*GetOwner()->GetName(), *HitNormal.ToCompactString(),
				Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("null"));

			// Slide up along wall
			const FVector Remainder = Adjusted * (1.f - Hit.Time);
			FVector UpSlide = FVector(0.f, 0.f, FMath::Max(Remainder.Z, 5.f));
			if (!UpSlide.IsNearlyZero())
			{
				FHitResult SlideHit(1.f);
				SafeMoveUpdatedComponent(UpSlide, UpdatedComponent->GetComponentQuat(), true, SlideHit);

				if (SlideHit.bBlockingHit && SlideHit.ImpactNormal.Z < -0.7f)
				{
					// Slid into ceiling — perch
					bOnCeiling = true;
					WallContactTimer = 0.f;
					ContactWallNormal = FVector::ZeroVector;
					Velocity = FVector::ZeroVector;
					bJustCrested = true;

					UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: CLIMBED TO CEILING — perching"),
						*GetOwner()->GetName());
				}
			}
			} // end structural wall
		}
		else if (bIsFloor)
		{
			// Floor hit
			Velocity.Z = 0.f;

			if (WallContactTimer > 0.f)
			{
				// Was climbing, hit floor — done climbing
				WallContactTimer = 0.f;
				ContactWallNormal = FVector::ZeroVector;
				bJustCrested = true;
			}

			const FVector GravDir(0.f, 0.f, -1.f);
			if (!StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit))
			{
				SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
			}
		}
		else if (bIsWall)
		{
			// Wall hit but climbing disabled
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: WALL HIT — climb NOT enabled (bClimbEnabled=%d) actor=%s"),
				*GetOwner()->GetName(), bClimbEnabled,
				Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("null"));
			const FVector Remainder = Adjusted * (1.f - Hit.Time);
			FVector WallSlide = FVector::VectorPlaneProject(Remainder, HitNormal);
			if (!WallSlide.IsNearlyZero())
			{
				FHitResult SlideHit(1.f);
				SafeMoveUpdatedComponent(WallSlide, UpdatedComponent->GetComponentQuat(), true, SlideHit);
			}

			if (FMath::Abs(HitNormal.Z) < 0.3f)
			{
				StepUp(FVector(0.f, 0.f, -1.f), Adjusted * (1.f - Hit.Time), Hit);
			}
		}
	}
	else if (Hit.bStartPenetrating)
	{
		MoveUpdatedComponent(Hit.ImpactNormal * 10.f, UpdatedComponent->GetComponentQuat(), false);
		bJustTeleported = true;
	}

	// Post-move
	const float Displacement = FVector::Dist(OldLocation, UpdatedComponent->GetComponentLocation());
	if (Displacement < 1.f)
	{
		bJustTeleported = true;
		if (!bSlamming && !bOnCeiling && WallContactTimer <= 0.f
			&& Velocity.Z < -GroundGravity * deltaTime * 2.f)
		{
			Velocity.Z = -GroundGravity * deltaTime;
		}
	}

	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}

	// Body rotation
	{
		FRotator DesiredRot = UpdatedComponent->GetComponentRotation();
		float InterpRate = 12.f;

		if (WallContactTimer > 0.f && !ContactWallNormal.IsNearlyZero())
		{
			// On wall: align body to wall surface
			const FVector CreatureUp = ContactWallNormal;
			FVector ClimbForward = FVector::VectorPlaneProject(FVector::UpVector, ContactWallNormal).GetSafeNormal();
			if (!ClimbForward.IsNearlyZero())
			{
				DesiredRot = FRotationMatrix::MakeFromXZ(ClimbForward, CreatureUp).Rotator();
			}
			InterpRate = 8.f;
		}
		else if (bOnCeiling)
		{
			// On ceiling: upside down
			FVector FaceDir = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
			if (FaceDir.IsNearlyZero())
			{
				FaceDir = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
			}
			DesiredRot = FRotationMatrix::MakeFromXZ(FaceDir, FVector(0.f, 0.f, -1.f)).Rotator();
			InterpRate = 6.f;
		}
		else
		{
			// Ground: face movement direction
			FVector FaceDir = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
			if (!FaceDir.IsNearlyZero())
			{
				DesiredRot = FRotationMatrix::MakeFromXZ(FaceDir, FVector::UpVector).Rotator();
			}
		}

		const FRotator CurrentRot = UpdatedComponent->GetComponentRotation();
		const FRotator NewRot = FMath::RInterpTo(CurrentRot, DesiredRot, deltaTime, InterpRate);
		UpdatedComponent->MoveComponent(FVector::ZeroVector, NewRot, false);
	}

	// Void kill
	if (UpdatedComponent->GetComponentLocation().Z < -5000.f)
	{
		GetOwner()->Destroy();
	}
}
