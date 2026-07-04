// Copyright The Signal. All Rights Reserved.

#include "ZP_CrawlerMovementComponent.h"

UZP_CrawlerMovementComponent::UZP_CrawlerMovementComponent()
{
	// PhysFlying owns rotation — make sure the controller and the base CMC don't fight it.
	bOrientRotationToMovement = false;
	bUseControllerDesiredRotation = false;
	RotationRate = FRotator::ZeroRotator;
	MaxStepHeight = 60.f;
}

FVector UZP_CrawlerMovementComponent::GetEffectiveTarget() const
{
	return MoveTargetActor.IsValid() ? MoveTargetActor->GetActorLocation() : MoveTarget;
}

void UZP_CrawlerMovementComponent::SetWallCling(const FVector& InWallNormal)
{
	WallNormal = InWallNormal.GetSafeNormal();
	bClinging = !WallNormal.IsNearlyZero();
	bLaunching = false;
	bSlamming = false;
	Velocity = FVector::ZeroVector;
}

void UZP_CrawlerMovementComponent::ReleaseWall()
{
	bClinging = false;
	WallNormal = FVector::ZeroVector;
}

void UZP_CrawlerMovementComponent::BeginLaunch(const FVector& TargetLocation)
{
	ReleaseWall();
	bSlamming = false;
	bLaunching = true;
	bImpacted = false;
	LaunchTimer = 0.f;

	// Solve a short, punchy ballistic arc onto the target (L4D Hunter feel: fast, low).
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector ToTarget = TargetLocation - Start;
	const FVector Horiz(ToTarget.X, ToTarget.Y, 0.f);
	const float HorizDist = Horiz.Size();
	const FVector HorizDir = Horiz.GetSafeNormal();

	const float HorizSpeed = AZP_LaunchHorizSpeed;
	const float t = FMath::Clamp(HorizDist / HorizSpeed, AZP_LaunchTimeMin, AZP_LaunchTimeMax);
	const float Vz = (ToTarget.Z / t) + 0.5f * AZP_CrawlerGravity * t;
	Velocity = HorizDir * (HorizDist / t) + FVector(0.f, 0.f, Vz);
}

void UZP_CrawlerMovementComponent::BeginSlam(float HoldDuration)
{
	if (bSlamming) return;
	ReleaseWall();
	bLaunching = false;
	bSlamming = true;
	bImpacted = false;
	SlamTimer = 0.f;
	SlamHold = HoldDuration;
	Velocity = FVector::ZeroVector;
}

void UZP_CrawlerMovementComponent::PhysFlying(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < UE_SMALL_NUMBER) return;
	DeltaTime = FMath::Min(DeltaTime, 0.05f);

	RestorePreAdditiveRootMotionVelocity();

	// --- VELOCITY (one mode) ---
	if (bLaunching)
	{
		LaunchTimer += DeltaTime;
		Velocity.Z = FMath::Max(Velocity.Z - AZP_CrawlerGravity * DeltaTime, -AZP_TerminalVelocity);
		if (LaunchTimer > AZP_LaunchMaxDuration) { bLaunching = false; bImpacted = true; } // safety: never fly forever
	}
	else if (bSlamming)
	{
		Velocity = FVector::ZeroVector;
		SlamTimer += DeltaTime;
		if (SlamTimer >= SlamHold) { bSlamming = false; bImpacted = true; }
	}
	else if (bClinging)
	{
		Velocity = FVector::ZeroVector; // frozen on the wall
	}
	else
	{
		// Ground pursuit: ease toward the target horizontally, apply gravity.
		if (Velocity.Z > 0.f) Velocity.Z = 0.f;
		const FVector Pos = UpdatedComponent->GetComponentLocation();
		const FVector ToTarget = GetEffectiveTarget() - Pos;
		const FVector Dir = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();
		const float Speed = GetMaxSpeed();
		if (!Dir.IsNearlyZero() && Speed > UE_SMALL_NUMBER)
		{
			const FVector Smoothed = FMath::VInterpTo(FVector(Velocity.X, Velocity.Y, 0.f), Dir * Speed, DeltaTime, AZP_GroundPursuitInterpSpeed);
			Velocity.X = Smoothed.X;
			Velocity.Y = Smoothed.Y;
		}
		else
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}
		Velocity.Z = FMath::Max(Velocity.Z - AZP_CrawlerGravity * DeltaTime, -AZP_TerminalVelocity);
	}

	// --- ROTATION (always — even frozen, so a clung crawler stays aligned to its wall) ---
	ApplyBodyRotation(DeltaTime);

	// --- MOVE ---
	Iterations++;
	const FVector Delta = Velocity * DeltaTime;
	if (Delta.IsNearlyZero()) return;

	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.bBlockingHit)
	{
		if (bLaunching)
		{
			// Pounce connected — end it and flag impact for the behavior's damage check.
			bLaunching = false;
			bImpacted = true;
			Velocity = FVector::ZeroVector;
			return;
		}
		// Ground: slide along walls/obstacles.
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}

	// Void kill — never leave a runaway crawler falling forever.
	if (UpdatedComponent->GetComponentLocation().Z < AZP_VoidKillZ)
	{
		GetOwner()->Destroy();
	}
}

void UZP_CrawlerMovementComponent::ApplyBodyRotation(float DeltaTime)
{
	FRotator Desired = UpdatedComponent->GetComponentRotation();
	float Rate = AZP_BodyRotationRate;

	if (bClinging && !WallNormal.IsNearlyZero())
	{
		// Wall = floor: up = wall normal, forward = up the wall.
		const FVector Forward = FVector::VectorPlaneProject(FVector::UpVector, WallNormal).GetSafeNormal();
		if (!Forward.IsNearlyZero())
		{
			Desired = FRotationMatrix::MakeFromXZ(Forward, WallNormal).Rotator();
		}
		Rate = AZP_ClingRotationRate;
	}
	else
	{
		// Ground/air: stay upright, face travel direction.
		const FVector Face = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
		if (!Face.IsNearlyZero())
		{
			Desired = FRotationMatrix::MakeFromXZ(Face, FVector::UpVector).Rotator();
		}
	}

	const FRotator New = FMath::RInterpTo(UpdatedComponent->GetComponentRotation(), Desired, DeltaTime, Rate);
	UpdatedComponent->MoveComponent(FVector::ZeroVector, New, false);
}
