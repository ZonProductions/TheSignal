// Copyright The Signal. All Rights Reserved.

#include "ZP_CrawlerMovementComponent.h"
#include "ZP_WallMap.h"
#include "GameFramework/Character.h"

#define ZP_CMC_DEBUG_LOGS 0

// Minimum displacement (UU) to count as "actually moved"
static constexpr float MinMoveDistance = 1.0f;

// Minimum wall height (UU) worth climbing. Shorter walls are stepped over or ignored.
// Prevents wasting time climbing curbs, pipes, and trim pieces.
static constexpr float MinClimbWallHeight = 150.f;

// Climb speed range (UU/s) — clamped to prevent crawling or teleporting on walls
static constexpr float MinClimbSpeed = 350.f;
static constexpr float MaxClimbSpeed = 600.f;

// How long wall contact persists after the last wall hit (seconds).
// 1.5s prevents hop-glitch: creature briefly loses wall contact on lips/gaps,
// this keeps climb mode active through those gaps instead of dropping to ground pursuit.
static constexpr float WallContactDuration = 1.5f;

// Ground pursuit gravity (UU/s²) — keeps creature grounded in FLYING mode
static constexpr float GroundGravity = 980.f;

// Terminal velocity cap (UU/s) — prevents infinite gravity accumulation when grounded
static constexpr float TerminalVelocity = 2000.f;

// Launch parameters — L4D Hunter style: visible arc, NOT a teleport.
// Slow enough to read, high enough to see the parabola.
static constexpr float LaunchHorizontalSpeed = 400.f;
static constexpr float LaunchUpSpeed = 600.f;
static constexpr float LaunchGravity = 980.f;
static constexpr float LaunchMaxDuration = 2.0f;
static constexpr float PostLandingCooldown = 10.0;

// Floor probe parameters — prevents walking off cliffs/into water
static constexpr float FloorProbeAhead = 120.f;	// UU forward to check
static constexpr float FloorProbeDepth = 2000.f;	// Probe trace length downward
static constexpr float FloorProbeMaxDrop = 500.f;	// Max acceptable elevation drop

#if ZP_CMC_DEBUG_LOGS
// Throttle ROUTINE logging (PreMove/END summaries). 0.25s = 4x/sec/creature.
// IMPORTANT events (vault, climb, crest, bail, wall-hit) ALWAYS log regardless.
static double LastLogTime = 0.0;
static constexpr double LogInterval = 0.25;
#endif

// --- Constructor ---

UZP_CrawlerMovementComponent::UZP_CrawlerMovementComponent()
{
	// Creatures can step over low obstacles (fences, pipes, curbs) without climbing
	MaxStepHeight = 200.f;
}

// --- Target Resolution ---

FVector UZP_CrawlerMovementComponent::GetEffectiveTarget() const
{
	if (MoveTargetActor.IsValid())
	{
		return MoveTargetActor->GetActorLocation();
	}
	return MoveTarget;
}

// --- Public API ---

void UZP_CrawlerMovementComponent::LaunchAtTarget(const FVector& TargetLocation)
{
	if (bLaunching || IsLaunchOnCooldown())
	{
#if ZP_CMC_DEBUG_LOGS
		UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: LaunchAtTarget REJECTED — launching=%d cooldown=%d"),
			*GetOwner()->GetName(), bLaunching, IsLaunchOnCooldown());
#endif
		return;
	}

	const FVector CreaturePos = UpdatedComponent->GetComponentLocation();
	const FVector ToTarget = TargetLocation - CreaturePos;
	FVector LaunchDir = ToTarget.GetSafeNormal2D();

	if (LaunchDir.IsNearlyZero())
	{
		LaunchDir = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	}

	const float HorizDist = FVector(ToTarget.X, ToTarget.Y, 0.f).Size();
	const float HorizSpeed = FMath::Clamp(HorizDist * 1.2f, 200.f, LaunchHorizontalSpeed);
	// Always a substantial arc — min 400 UU/s up guarantees visible parabola
	const float UpSpeed = FMath::Clamp(HorizDist * 1.5f, 400.f, LaunchUpSpeed);

	const float HeightDiff = ToTarget.Z;
	float AdjustedUpSpeed = UpSpeed;
	if (HeightDiff < -100.f)
	{
		AdjustedUpSpeed *= 0.5f;
	}
	else if (HeightDiff > 100.f)
	{
		AdjustedUpSpeed = FMath::Min(AdjustedUpSpeed + HeightDiff * 0.5f, LaunchUpSpeed * 1.5f);
	}

	LaunchVelocity = LaunchDir * HorizSpeed + FVector(0.f, 0.f, AdjustedUpSpeed);
	bLaunching = true;
	LaunchTimeRemaining = LaunchMaxDuration;
	LaunchStartZ = CreaturePos.Z;

	WallContactTimer = 0.f;
	ContactWallNormal = FVector::ZeroVector;

#if ZP_CMC_DEBUG_LOGS
	UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: === LAUNCH === vel=(%s) speed=%.0f horizDist=%.0f heightDiff=%.0f"),
		*GetOwner()->GetName(), *LaunchVelocity.ToCompactString(), LaunchVelocity.Size(), HorizDist, HeightDiff);
#endif
}

void UZP_CrawlerMovementComponent::BeginSlam(float RiseHeight, float HoldDuration, float FallSpeed)
{
	if (bSlamming || bLaunching)
	{
#if ZP_CMC_DEBUG_LOGS
		UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: BeginSlam REJECTED — slamming=%d launching=%d"),
			*GetOwner()->GetName(), bSlamming, bLaunching);
#endif
		return;
	}

	bSlamming = true;
	bSlamImpacted = false;
	SlamPhase = ESlamPhase::Rising;
	SlamTimer = 0.f;
	SlamHoldTime = HoldDuration;
	SlamDropSpeed = FallSpeed;

	WallContactTimer = 0.f;
	ContactWallNormal = FVector::ZeroVector;

#if ZP_CMC_DEBUG_LOGS
	UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: === SLAM BEGIN === wind-up %.1fs"),
		*GetOwner()->GetName(), HoldDuration);
#endif
}

bool UZP_CrawlerMovementComponent::IsLaunchOnCooldown() const
{
	if (LastLandingTime <= 0.0)
	{
		return false;
	}
	return (FPlatformTime::Seconds() - LastLandingTime) < PostLandingCooldown;
}

// --- PhysFlying Override (custom velocity — no CalcVelocity) ---

void UZP_CrawlerMovementComponent::PhysFlying(float deltaTime, int32 Iterations)
{
	if (deltaTime < UE_SMALL_NUMBER)
	{
		return;
	}

	deltaTime = FMath::Min(deltaTime, 0.05f);

	const double Now = FPlatformTime::Seconds();
#if ZP_CMC_DEBUG_LOGS
	const bool bLog = (Now - LastLogTime) > LogInterval;
#endif

	RestorePreAdditiveRootMotionVelocity();

	// Tick down climb bail cooldown
	if (ClimbBailCooldown > 0.f)
	{
		const float PrevCooldown = ClimbBailCooldown;
		ClimbBailCooldown -= deltaTime;
		if (ClimbBailCooldown < 0.f) { ClimbBailCooldown = 0.f; }

		// Log when cooldown expires
#if ZP_CMC_DEBUG_LOGS
		if (PrevCooldown > 0.f && ClimbBailCooldown <= 0.f)
		{
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: ClimbBailCooldown EXPIRED — can climb again"),
				*GetOwner()->GetName());
		}
#endif
	}

	// Tick down crest anti-repeat timer (prevents re-climbing same wall after crest)
	if (CrestAntiRepeatTimer > 0.f)
	{
		CrestAntiRepeatTimer -= deltaTime;
		if (CrestAntiRepeatTimer <= 0.f)
		{
			CrestAntiRepeatTimer = 0.f;
			LastCrestNormal = FVector::ZeroVector;
		}
	}

	// ========================================================================
	// VELOCITY SOURCE (4 branches: slam / launch / climb / ground pursuit)
	// ========================================================================

	if (bSlamming)
	{
		Velocity = FVector::ZeroVector;
		SlamTimer += deltaTime;

		if (SlamTimer >= SlamHoldTime)
		{
			bSlamming = false;
			bSlamImpacted = true;
#if ZP_CMC_DEBUG_LOGS
			UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: === SLAM IMPACT === (%.1fs wind-up)"),
				*GetOwner()->GetName(), SlamTimer);
#endif
		}
#if ZP_CMC_DEBUG_LOGS
		else if (bLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: SLAMMING — timer=%.2f/%.2f"),
				*GetOwner()->GetName(), SlamTimer, SlamHoldTime);
		}
#endif
	}
	else if (bLaunching)
	{
		LaunchVelocity.Z -= LaunchGravity * deltaTime;
		Velocity = LaunchVelocity;
		LaunchTimeRemaining -= deltaTime;

		const float LaunchDropLimit = 300.f;
		const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;

#if ZP_CMC_DEBUG_LOGS
		if (bLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: LAUNCHING — vel=(%s) Z=%.0f startZ=%.0f timeLeft=%.2f"),
				*GetOwner()->GetName(), *LaunchVelocity.ToCompactString(), CurrentZ, LaunchStartZ, LaunchTimeRemaining);
		}
#endif

		if (CurrentZ < LaunchStartZ - LaunchDropLimit)
		{
			bLaunching = false;
			LaunchVelocity = FVector::ZeroVector;
			LaunchTimeRemaining = 0.f;
			LastLandingTime = Now;
#if ZP_CMC_DEBUG_LOGS
			UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: LAUNCH VOID ABORT — dropped %.0f below start"),
				*GetOwner()->GetName(), LaunchStartZ - CurrentZ);
#endif
		}
		else if (LaunchTimeRemaining <= 0.f)
		{
			bLaunching = false;
			LaunchVelocity = FVector::ZeroVector;
			LaunchTimeRemaining = 0.f;
			LastLandingTime = Now;
#if ZP_CMC_DEBUG_LOGS
			UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: LAUNCH EXPIRED — timed out"),
				*GetOwner()->GetName());
#endif
		}
	}
	else if (WallContactTimer > 0.f && !ContactWallNormal.IsNearlyZero())
	{
		// CLIMBING
		const float Speed = FMath::Clamp(GetMaxSpeed(), MinClimbSpeed, MaxClimbSpeed);
		FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, ContactWallNormal).GetSafeNormal();
		FVector ClimbDir = (WallUp * 0.90f - ContactWallNormal * 0.10f).GetSafeNormal();
		Velocity = ClimbDir * Speed;

#if ZP_CMC_DEBUG_LOGS
		if (bLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: CLIMBING — vel=(%s) speed=%.0f wallTimer=%.2f wallN=(%s) targetZ=%.0f"),
				*GetOwner()->GetName(), *Velocity.ToCompactString(), Speed, WallContactTimer,
				*ContactWallNormal.ToCompactString(), ClimbTargetZ);
		}
#endif
	}
	else
	{
		// GROUND PURSUIT
		if (Velocity.Z > 0.f)
		{
			Velocity.Z = 0.f;
		}

		const FVector Target = GetEffectiveTarget();
		const FVector CurrentPos = UpdatedComponent->GetComponentLocation();
		const FVector ToTarget = Target - CurrentPos;
		FVector HorizDir = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();
		const float Speed = GetMaxSpeed();
		const float DistToTarget = ToTarget.Size();
		const float HorizDistToTarget = FVector(ToTarget.X, ToTarget.Y, 0.f).Size();
		const float ZDiffToTarget = ToTarget.Z;

#if ZP_CMC_DEBUG_LOGS
		if (bLog)
		{
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: GROUND — target=(%s) dist=%.0f hDist=%.0f zDiff=%.0f speed=%.0f climb=%d bailCD=%.2f actor=%s"),
				*GetOwner()->GetName(), *Target.ToCompactString(), DistToTarget, HorizDistToTarget,
				ZDiffToTarget, Speed, bClimbEnabled, ClimbBailCooldown,
				MoveTargetActor.IsValid() ? *MoveTargetActor->GetName() : TEXT("none"));
		}
#endif

		if (!HorizDir.IsNearlyZero() && Speed > UE_SMALL_NUMBER)
		{
			const FVector CurrentHorizVel = FVector(Velocity.X, Velocity.Y, 0.f);
			const FVector DesiredHorizVel = HorizDir * Speed;
			const float SmoothRate = 8.0f;
			const FVector SmoothedHorizVel = FMath::VInterpTo(CurrentHorizVel, DesiredHorizVel, deltaTime, SmoothRate);
			HorizDir = SmoothedHorizVel.GetSafeNormal();

			bool bSafeGround = true;
			FCollisionQueryParams ProbeParams;
			ProbeParams.AddIgnoredActor(GetOwner());
			if (bFloorProbeActive)
			{
				FHitResult FloorProbe;
				const FVector ProbeStart = CurrentPos + HorizDir * FloorProbeAhead + FVector(0.f, 0.f, 50.f);
				const FVector ProbeEnd = ProbeStart - FVector(0.f, 0.f, FloorProbeDepth);

				if (GetWorld()->LineTraceSingleByChannel(FloorProbe, ProbeStart, ProbeEnd, ECC_Visibility, ProbeParams))
				{
					const float DropAhead = CurrentPos.Z - FloorProbe.ImpactPoint.Z;
					if (DropAhead > FloorProbeMaxDrop)
					{
						bSafeGround = false;
#if ZP_CMC_DEBUG_LOGS
						if (bLog)
						{
							UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: FLOOR PROBE — drop=%.0f > max=%.0f, UNSAFE"),
								*GetOwner()->GetName(), DropAhead, FloorProbeMaxDrop);
						}
#endif
					}
				}
				else
				{
					bSafeGround = false;
#if ZP_CMC_DEBUG_LOGS
					if (bLog)
					{
						UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: FLOOR PROBE — no ground ahead, UNSAFE"),
							*GetOwner()->GetName());
					}
#endif
				}
			}

			if (bSafeGround)
			{
				Velocity.X = SmoothedHorizVel.X;
				Velocity.Y = SmoothedHorizVel.Y;
			}
			else
			{
				const FVector Perp = FVector::CrossProduct(HorizDir, FVector::UpVector).GetSafeNormal();
				const FVector AltDir = (FMath::RandBool() ? Perp : -Perp);
				const FVector AltProbeStart = CurrentPos + AltDir * FloorProbeAhead + FVector(0.f, 0.f, 50.f);
				const FVector AltProbeEnd = AltProbeStart - FVector(0.f, 0.f, FloorProbeDepth);
				FHitResult AltProbe;

				bool bAltSafe = false;
				if (GetWorld()->LineTraceSingleByChannel(AltProbe, AltProbeStart, AltProbeEnd, ECC_Visibility, ProbeParams))
				{
					bAltSafe = (CurrentPos.Z - AltProbe.ImpactPoint.Z) <= FloorProbeMaxDrop;
				}

				if (bAltSafe)
				{
					Velocity.X = AltDir.X * Speed;
					Velocity.Y = AltDir.Y * Speed;
				}
				else
				{
					Velocity.X = 0.f;
					Velocity.Y = 0.f;
				}

#if ZP_CMC_DEBUG_LOGS
				UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: DROPOFF — %s"),
					*GetOwner()->GetName(), bAltSafe ? TEXT("rerouted perpendicular") : TEXT("STOPPED"));
#endif
			}
		}
		else
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
#if ZP_CMC_DEBUG_LOGS
			if (bLog)
			{
				UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: GROUND — no direction or zero speed, zeroed horizontal"),
					*GetOwner()->GetName());
			}
#endif
		}

		Velocity.Z -= GroundGravity * deltaTime;
		Velocity.Z = FMath::Max(Velocity.Z, -TerminalVelocity);
	}

	// ========================================================================
	// PRE-MOVE SUMMARY
	// ========================================================================
#if ZP_CMC_DEBUG_LOGS
	if (bLog)
	{
		const FVector Pos = UpdatedComponent->GetComponentLocation();
		UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: PRE-MOVE — Vel=(%s) Speed=%.0f Pos=(%s) wall=%.2f climb=%d bail=%.2f launch=%d slam=%d"),
			*GetOwner()->GetName(), *Velocity.ToString(), Velocity.Size(), *Pos.ToCompactString(),
			WallContactTimer, bClimbEnabled, ClimbBailCooldown, bLaunching, bSlamming);
	}
#endif

	Iterations++;
	bJustTeleported = false;

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;

	const bool bWasClimbing = WallContactTimer > 0.f;
	const FVector PrevWallNormal = ContactWallNormal;
	if (WallContactTimer > 0.f)
	{
		WallContactTimer -= deltaTime;
		if (WallContactTimer <= 0.f)
		{
			WallContactTimer = 0.f;
			ContactWallNormal = FVector::ZeroVector;
#if ZP_CMC_DEBUG_LOGS
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: WallContactTimer EXPIRED — dropped off wall"),
				*GetOwner()->GetName());
#endif
		}
	}

	if (Adjusted.IsNearlyZero())
	{
#if ZP_CMC_DEBUG_LOGS
		if (bLog) { LastLogTime = Now; }
#endif
		return;
	}

	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

	// ========================================================================
	// HIT HANDLING
	// ========================================================================
	if (Hit.bBlockingHit && !Hit.bStartPenetrating)
	{
		const FVector HitNormal = Hit.ImpactNormal;
		const float WallZThreshold = bClimbEnabled ? 0.75f : 0.5f;
		const bool bIsWall = FMath::Abs(HitNormal.Z) < WallZThreshold;
		const bool bIsCeiling = HitNormal.Z < -0.3f;

#if ZP_CMC_DEBUG_LOGS
		// ALWAYS log hits — this is the most important diagnostic
		UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: HIT — N=(%s) |N.Z|=%.3f thresh=%.2f Wall=%d Ceil=%d Floor=%d Time=%.3f Actor=%s Comp=%s"),
			*GetOwner()->GetName(), *HitNormal.ToCompactString(),
			FMath::Abs(HitNormal.Z), WallZThreshold,
			bIsWall, bIsCeiling, (!bIsWall && !bIsCeiling),
			Hit.Time,
			Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("null"),
			Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("null"));
#endif

		if (bIsWall)
		{
			const bool bHitTarget = MoveTargetActor.IsValid() && Hit.GetActor() == MoveTargetActor.Get();

			const FVector Adjusted2D(Adjusted.X, Adjusted.Y, 0.f);
			const float IntoWall = Adjusted2D.IsNearlyZero()
				? 0.f
				: FVector::DotProduct(Adjusted2D.GetSafeNormal(), -HitNormal);
			bool bHandled = false;

#if ZP_CMC_DEBUG_LOGS
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: WALL — IntoWall=%.3f hitTarget=%d climbEnabled=%d bailCD=%.2f alreadyClimbing=%d launching=%d"),
				*GetOwner()->GetName(), IntoWall, bHitTarget, bClimbEnabled, ClimbBailCooldown, WallContactTimer > 0.f, bLaunching);
#endif

			// --- STEP 0: Cancel launch on wall impact ---
			// Lunge must NOT pass through fences/walls. Cancel the arc immediately.
			if (bLaunching && IntoWall > 0.1f)
			{
				bLaunching = false;
				LaunchVelocity = FVector::ZeroVector;
				LaunchTimeRemaining = 0.f;
				LastLandingTime = FPlatformTime::Seconds();

#if ZP_CMC_DEBUG_LOGS
				UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: LAUNCH WALL ABORT — hit wall during lunge, cancelling arc"),
					*GetOwner()->GetName());
#endif
				// Fall through to wall slide handling
			}

			const bool bAlreadyClimbing = WallContactTimer > 0.f;

			// --- STEP 1: StepUp for low obstacles (any mode, not climbing) ---
			if (!bHandled && !bLaunching && IntoWall > 0.2f && !bHitTarget && !bAlreadyClimbing)
			{
				const FVector GravDir = FVector(0.f, 0.f, -1.f);
				const FVector PreStepPos = UpdatedComponent->GetComponentLocation();
				const bool bSteppedOver = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
				const float StepGain = bSteppedOver ? (UpdatedComponent->GetComponentLocation().Z - PreStepPos.Z) : 0.f;

#if ZP_CMC_DEBUG_LOGS
				UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: PATROL STEPUP — success=%d gain=%.1f"),
					*GetOwner()->GetName(), bSteppedOver, StepGain);
#endif

				if (bSteppedOver)
				{
					WallContactTimer = 0.f;
					ContactWallNormal = FVector::ZeroVector;
					bHandled = true;
				}
			}

			// --- STEP 2: Climb if enabled (anti-repeat gates this) ---
			{
				const float ClimbThreshold = bClimbEnabled ? 0.05f : 0.2f;

				// Check if this is the same wall we just crested (anti-repeat)
				const bool bSameWallAsCrest = (CrestAntiRepeatTimer > 0.f
					&& !LastCrestNormal.IsNearlyZero()
					&& FVector::DotProduct(HitNormal, LastCrestNormal) > 0.7f);

				if (!bHandled && bClimbEnabled && !bHitTarget && !bLaunching && ClimbBailCooldown <= 0.f
					&& !bSameWallAsCrest
					&& (bAlreadyClimbing || IntoWall > ClimbThreshold))
				{
					bool bEngageClimb = false;

					if (!bAlreadyClimbing)
					{
						const float PotentialTopZ = FZP_WallMap::TraceWallTop(GetWorld(), Hit.ImpactPoint, HitNormal);
						const float CreatureZ = UpdatedComponent->GetComponentLocation().Z;
						const float WallHeight = PotentialTopZ - CreatureZ;

#if ZP_CMC_DEBUG_LOGS
						UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: WALL FIRST CONTACT — topZ=%.0f creatureZ=%.0f wallHeight=%.0f impactZ=%.0f"),
							*GetOwner()->GetName(), PotentialTopZ, CreatureZ, WallHeight, Hit.ImpactPoint.Z);
#endif

						if (WallHeight >= MinClimbWallHeight)
						{
							// Tall wall — engage full climb
							ClimbTargetZ = PotentialTopZ;
							ClimbStartZ = CreatureZ;
							ClimbStallTime = 0.f;
							ClimbLastProgressZ = CreatureZ;
							bEngageClimb = true;

#if ZP_CMC_DEBUG_LOGS
							UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: >>> CLIMB START <<< wallTop=%.0f creatureZ=%.0f height=%.0f normal=(%s)"),
								*GetOwner()->GetName(), ClimbTargetZ, ClimbLastProgressZ, WallHeight, *HitNormal.ToCompactString());
#endif
						}
						else
						{
							// Short obstacle — try StepUp first, fall back to brief climb
							const FVector GravDir(0.f, 0.f, -1.f);
							const bool bSteppedOver = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
							if (bSteppedOver)
							{
								WallContactTimer = 0.f;
								ContactWallNormal = FVector::ZeroVector;
								bHandled = true;

#if ZP_CMC_DEBUG_LOGS
								UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: SHORT WALL (%.0f UU) — StepUp SUCCESS"),
									*GetOwner()->GetName(), WallHeight);
#endif
							}
							else
							{
								// StepUp failed (thin/irregular geometry) — brief climb over
								ClimbTargetZ = PotentialTopZ;
								ClimbStartZ = CreatureZ;
								ClimbStallTime = 0.f;
								ClimbLastProgressZ = CreatureZ;
								bEngageClimb = true;

#if ZP_CMC_DEBUG_LOGS
								UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: SHORT WALL (%.0f UU) — StepUp FAILED, brief climb to Z=%.0f"),
									*GetOwner()->GetName(), WallHeight, PotentialTopZ);
#endif
							}
						}
					}
					else
					{
						// Already climbing — refresh timer
						bEngageClimb = true;
#if ZP_CMC_DEBUG_LOGS
						UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: CLIMB REFRESH — wallTimer was %.2f, refreshing to %.2f, IntoWall=%.3f"),
							*GetOwner()->GetName(), WallContactTimer, WallContactDuration, IntoWall);
#endif
					}

					if (bEngageClimb)
					{
						WallContactTimer = WallContactDuration;

						if (ContactWallNormal.IsNearlyZero()
							|| FVector::DotProduct(HitNormal, ContactWallNormal) > 0.5f)
						{
							ContactWallNormal = HitNormal;
						}
						bHandled = true;
					}
				}
				else if (!bHandled && bClimbEnabled && !bHitTarget && !bAlreadyClimbing)
				{
					// Climb didn't engage — try StepUp as fallback
					if (IntoWall > 0.2f && ClimbBailCooldown <= 0.f)
					{
						const FVector GravDir(0.f, 0.f, -1.f);
						const bool bSteppedOver = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
						if (bSteppedOver)
						{
							WallContactTimer = 0.f;
							ContactWallNormal = FVector::ZeroVector;
							bHandled = true;
#if ZP_CMC_DEBUG_LOGS
							UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: HUNT STEPUP fallback — success"), *GetOwner()->GetName());
#endif
						}
					}

					// Log WHY climbing didn't engage
#if ZP_CMC_DEBUG_LOGS
					if (!bHandled)
					{
						if (bSameWallAsCrest)
						{
							UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: CLIMB REJECTED — same wall as last crest (antiRepeat=%.2f)"),
								*GetOwner()->GetName(), CrestAntiRepeatTimer);
						}
						else
						{
							UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: CLIMB REJECTED — bailCD=%.2f IntoWall=%.3f(need>%.3f) alreadyClimb=%d"),
								*GetOwner()->GetName(), ClimbBailCooldown, IntoWall, ClimbThreshold, bAlreadyClimbing);
						}
					}
#endif
				}
			} // end STEP 2 scope

			// --- STEP 3: Slide along wall or redirect ---
			if (!bHandled)
			{
				if (!bHitTarget)
				{
					const FVector Remainder = Adjusted * (1.f - Hit.Time);
					FVector WallSlide = FVector::VectorPlaneProject(Remainder, HitNormal);

					if (WallSlide.IsNearlyZero() && !Remainder.IsNearlyZero())
					{
						const FVector Perp = FVector::CrossProduct(HitNormal, FVector::UpVector).GetSafeNormal();
						if (!Perp.IsNearlyZero())
						{
							WallSlide = Perp * Remainder.Size();
#if ZP_CMC_DEBUG_LOGS
							UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: WALL SLIDE head-on — perpendicular nudge dir=(%s)"),
								*GetOwner()->GetName(), *Perp.ToCompactString());
#endif
						}
					}

					if (!WallSlide.IsNearlyZero())
					{
						FHitResult SlideHit(1.f);
						SafeMoveUpdatedComponent(WallSlide, UpdatedComponent->GetComponentQuat(), true, SlideHit);

#if ZP_CMC_DEBUG_LOGS
						if (bLog)
						{
							UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: WALL SLIDE — dir=(%s) blocked=%d"),
								*GetOwner()->GetName(), *WallSlide.GetSafeNormal().ToCompactString(), SlideHit.bBlockingHit);
						}
#endif
					}
					else
					{
#if ZP_CMC_DEBUG_LOGS
						UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: WALL SLIDE — zero slide vector, stuck"),
							*GetOwner()->GetName());
#endif
					}
				}
				else
				{
#if ZP_CMC_DEBUG_LOGS
					UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: HIT TARGET ACTOR — not sliding"),
						*GetOwner()->GetName());
#endif
				}
			}
			else if (WallContactTimer > 0.f)
			{
				// Climbing slide: strip horizontal, go UP only
				const FVector Remainder = Adjusted * (1.f - Hit.Time);
				FVector WallSlide = FVector::VectorPlaneProject(Remainder, HitNormal);
				WallSlide = FVector(0.f, 0.f, WallSlide.Z);

				if (!WallSlide.IsNearlyZero())
				{
					FHitResult SlideHit(1.f);
					SafeMoveUpdatedComponent(WallSlide, UpdatedComponent->GetComponentQuat(), true, SlideHit);

					if (SlideHit.bBlockingHit && (SlideHit.Time * WallSlide.Size()) < MinMoveDistance)
					{
						FVector UpMove = FVector(0.f, 0.f, 1.f) * Remainder.Size();
						MoveUpdatedComponent(UpMove, UpdatedComponent->GetComponentQuat(), false);
#if ZP_CMC_DEBUG_LOGS
						UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: CLIMB SLIDE blocked — forced up %.0f"),
							*GetOwner()->GetName(), Remainder.Size());
#endif
					}
				}
			}
		}
		else if (bIsCeiling)
		{
#if ZP_CMC_DEBUG_LOGS
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: CEILING HIT — N.Z=%.3f wasClimbing=%d"),
				*GetOwner()->GetName(), HitNormal.Z, bWasClimbing);
#endif

			FVector Projected = FVector::VectorPlaneProject(Adjusted * (1.f - Hit.Time), HitNormal);
			if (!Projected.IsNearlyZero())
			{
				FHitResult ProjHit(1.f);
				SafeMoveUpdatedComponent(Projected, UpdatedComponent->GetComponentQuat(), true, ProjHit);
			}

			if (bWasClimbing && HitNormal.Z < -0.95f)
			{
				// Check if this "ceiling" is actually the wall top/overhang near our climb target.
				// If so, this is a CREST — push over the lip instead of bailing.
				const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
				const bool bNearCrest = (ClimbTargetZ > 0.f && CurrentZ >= ClimbTargetZ - 200.f);

				if (bNearCrest)
				{
					FVector ForwardDir = FVector(-PrevWallNormal.X, -PrevWallNormal.Y, 0.f).GetSafeNormal();
					if (ForwardDir.IsNearlyZero())
					{
						ForwardDir = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
					}

					const FVector PreCrestPos = UpdatedComponent->GetComponentLocation();
					const float CeilWallHeight = (ClimbStartZ > 0.f) ? (ClimbTargetZ - ClimbStartZ) : 400.f;
					const float CeilPushUp = FMath::Clamp(CeilWallHeight * 0.4f, 40.f, 150.f);
					const float CeilPushFwd = FMath::Clamp(CeilWallHeight * 0.75f, 75.f, 250.f);

					// Teleport up past the overhang (no sweep — the overhang blocks swept movement)
					MoveUpdatedComponent(FVector(0.f, 0.f, CeilPushUp),
						UpdatedComponent->GetComponentQuat(), false);

					// Sweep forward to land on the other side
					FHitResult CrestFwdHit(1.f);
					SafeMoveUpdatedComponent(ForwardDir * CeilPushFwd,
						UpdatedComponent->GetComponentQuat(), true, CrestFwdHit);

					const FVector PostCrestPos = UpdatedComponent->GetComponentLocation();
					Velocity = ForwardDir * FMath::Max(Velocity.Size(), MinClimbSpeed);
					bJustTeleported = true;

					WallContactTimer = 0.f;
					ContactWallNormal = FVector::ZeroVector;
					ClimbBailCooldown = 0.5f;
					ClimbTargetZ = 0.f;
					ClimbStartZ = 0.f;

					// Anti-repeat: block re-climbing the same wall for 3s
					LastCrestNormal = PrevWallNormal;
					CrestAntiRepeatTimer = 3.0f;

					bJustCrested = true;

#if ZP_CMC_DEBUG_LOGS
					UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: >>> CEILING CREST <<< Z=%.0f wallH=%.0f up=%.0f(tp) fwd=%.0f pos=(%s)"),
						*GetOwner()->GetName(), CurrentZ, CeilWallHeight,
						CeilPushUp,
						FVector::Dist2D(PreCrestPos, PostCrestPos),
						*PostCrestPos.ToCompactString());
#endif
				}
				else
				{
					WallContactTimer = 0.f;
					ContactWallNormal = FVector::ZeroVector;
					ClimbTargetZ = 0.f;
					ClimbBailCooldown = 0.5f;

#if ZP_CMC_DEBUG_LOGS
					UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: TRUE CEILING — cleared climb, bail 0.5s"),
						*GetOwner()->GetName());
#endif
				}
			}
			else if (bWasClimbing)
			{
#if ZP_CMC_DEBUG_LOGS
				UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: ANGLED CEILING (N.Z=%.2f) — keeping climb active"),
					*GetOwner()->GetName(), HitNormal.Z);
#endif
			}
		}
		else
		{
			// FLOOR
#if ZP_CMC_DEBUG_LOGS
			UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: FLOOR HIT — N.Z=%.3f wasClimbing=%d"),
				*GetOwner()->GetName(), HitNormal.Z, bWasClimbing);
#endif

			const FVector GravDir = FVector(0.f, 0.f, -1.f);
			const FVector VelDir = Velocity.GetSafeNormal();
			const float UpDown = GravDir | VelDir;

			bool bSteppedUp = false;
			if ((FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
			{
				bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
				if (bSteppedUp)
				{
#if ZP_CMC_DEBUG_LOGS
					UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: FLOOR STEPUP success"),
						*GetOwner()->GetName());
#endif
				}
			}

			if (!bSteppedUp)
			{
				HandleImpact(Hit, deltaTime, Adjusted);
				SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
			}

			// Cresting a wall
			if (bWasClimbing && HitNormal.Z > 0.7f)
			{
				FVector ForwardDir = FVector(-PrevWallNormal.X, -PrevWallNormal.Y, 0.f).GetSafeNormal();
				if (ForwardDir.IsNearlyZero())
				{
					ForwardDir = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
				}

				const float CrestSpeed = FMath::Max(Velocity.Size(), MinClimbSpeed);
				const float WallHeight = (ClimbStartZ > 0.f) ? (ClimbTargetZ - ClimbStartZ) : 400.f;
				const float PushUp = FMath::Clamp(WallHeight * 0.5f, 40.f, 200.f);
				const float PushForward = FMath::Clamp(WallHeight * 0.75f, 75.f, 250.f);

				const FVector PreCrestPos = UpdatedComponent->GetComponentLocation();

				FHitResult UpHit(1.f);
				SafeMoveUpdatedComponent(FVector(0.f, 0.f, PushUp), UpdatedComponent->GetComponentQuat(), true, UpHit);

				const FVector MidCrestPos = UpdatedComponent->GetComponentLocation();

				FHitResult FwdHit(1.f);
				SafeMoveUpdatedComponent(ForwardDir * PushForward, UpdatedComponent->GetComponentQuat(), true, FwdHit);

				const FVector PostCrestPos = UpdatedComponent->GetComponentLocation();

				Velocity = ForwardDir * CrestSpeed;
				bJustTeleported = true;

				WallContactTimer = 0.f;
				ContactWallNormal = FVector::ZeroVector;
				ClimbBailCooldown = 0.5f;
				ClimbStartZ = 0.f;

				// Anti-repeat: block re-climbing the same wall for 3s
				LastCrestNormal = PrevWallNormal;
				CrestAntiRepeatTimer = 3.0f;

				bJustCrested = true;

#if ZP_CMC_DEBUG_LOGS
				UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: >>> FLOOR CREST <<< wallH=%.0f actualUp=%.0f/%.0f actualFwd=%.0f/%.0f upBlocked=%d fwdBlocked=%d speed=%.0f pos=(%s)"),
					*GetOwner()->GetName(), WallHeight,
					MidCrestPos.Z - PreCrestPos.Z, PushUp,
					FVector::Dist2D(MidCrestPos, PostCrestPos), PushForward,
					UpHit.bBlockingHit, FwdHit.bBlockingHit,
					CrestSpeed, *PostCrestPos.ToCompactString());
#endif
			}

			// Floor hit: zero accumulated gravity
			if (HitNormal.Z > 0.7f)
			{
				Velocity.Z = 0.f;

				if (bLaunching)
				{
					bLaunching = false;
					LaunchVelocity = FVector::ZeroVector;
					LaunchTimeRemaining = 0.f;
					LastLandingTime = Now;
#if ZP_CMC_DEBUG_LOGS
					UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: LAUNCH LANDED on floor"),
						*GetOwner()->GetName());
#endif
				}
			}
		}
	}
	else if (Hit.bStartPenetrating && !Adjusted.IsNearlyZero())
	{
		const FVector MoveDir = Adjusted.GetSafeNormal();
		const float Dot = FVector::DotProduct(Hit.ImpactNormal, MoveDir);
		const bool bCurrentlyClimbing = WallContactTimer > 0.f;

#if ZP_CMC_DEBUG_LOGS
		UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: PENETRATING — N=(%s) dot=%.3f climbing=%d actor=%s"),
			*GetOwner()->GetName(), *Hit.ImpactNormal.ToCompactString(), Dot,
			bCurrentlyClimbing,
			Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("null"));
#endif

		if (bCurrentlyClimbing && Dot > -0.5f)
		{
			// During climbing, push UPWARD only through penetration (lip transitions).
			// Never push horizontally — that phases through walls to reach the player.
			const FVector UpOnly = FVector(0.f, 0.f, FMath::Max(Adjusted.Z, 5.f));
			MoveUpdatedComponent(UpOnly, UpdatedComponent->GetComponentQuat(), false, &Hit);
			bJustTeleported = true;
		}
		else
		{
			// Push OUT of penetration — don't phase through geometry
			MoveUpdatedComponent(Hit.ImpactNormal * 10.f, UpdatedComponent->GetComponentQuat(), false);
		}
	}
#if ZP_CMC_DEBUG_LOGS
	else if (!Hit.bBlockingHit && bLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: NO HIT — moved freely"),
			*GetOwner()->GetName());
	}
#endif

	// ========================================================================
	// POST-MOVE
	// ========================================================================
	const float Displacement = FVector::Dist(OldLocation, UpdatedComponent->GetComponentLocation());
	if (Displacement < MinMoveDistance || bLaunching)
	{
		bJustTeleported = true;

		if (!bLaunching && !bSlamming && WallContactTimer <= 0.f
			&& Velocity.Z < -GroundGravity * deltaTime * 2.f)
		{
			const float OldVelZ = Velocity.Z;
			Velocity.Z = -GroundGravity * deltaTime;
#if ZP_CMC_DEBUG_LOGS
			if (bLog)
			{
				UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: GRAVITY CAP — Z was %.0f, capped to %.0f (stuck)"),
					*GetOwner()->GetName(), OldVelZ, Velocity.Z);
			}
#endif
		}
	}

	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}

	// ========================================================================
	// TARGET-Z CRESTING
	// ========================================================================
	if (ClimbTargetZ > 0.f && !bLaunching)
	{
		const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;

		if (CurrentZ >= ClimbTargetZ - 50.f)
		{
			FVector ForwardDir = FVector(-ContactWallNormal.X, -ContactWallNormal.Y, 0.f).GetSafeNormal();
			if (ForwardDir.IsNearlyZero())
			{
				ForwardDir = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
			}

			const float CrestSpeed = FMath::Max(Velocity.Size(), MinClimbSpeed);
			const float TgtWallHeight = (ClimbStartZ > 0.f) ? (ClimbTargetZ - ClimbStartZ) : 400.f;
			const float TgtPushUp = FMath::Clamp(TgtWallHeight * 0.5f, 40.f, 200.f);
			const float TgtPushFwd = FMath::Clamp(TgtWallHeight * 0.75f, 75.f, 250.f);
			const FVector PreCrestPos = UpdatedComponent->GetComponentLocation();

			FHitResult UpHit(1.f);
			SafeMoveUpdatedComponent(FVector(0.f, 0.f, TgtPushUp), UpdatedComponent->GetComponentQuat(), true, UpHit);
			FHitResult FwdHit(1.f);
			SafeMoveUpdatedComponent(ForwardDir * TgtPushFwd, UpdatedComponent->GetComponentQuat(), true, FwdHit);

			const FVector PostCrestPos = UpdatedComponent->GetComponentLocation();

			Velocity = ForwardDir * CrestSpeed;
			bJustTeleported = true;

			// Anti-repeat: block re-climbing the same wall for 3s
			LastCrestNormal = ContactWallNormal;
			CrestAntiRepeatTimer = 3.0f;

			WallContactTimer = 0.f;
			ContactWallNormal = FVector::ZeroVector;
			ClimbBailCooldown = 0.5f;

			bJustCrested = true;

#if ZP_CMC_DEBUG_LOGS
			UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: >>> TARGET CREST <<< Z=%.0f target=%.0f wallH=%.0f actualUp=%.0f/%.0f actualFwd=%.0f/%.0f speed=%.0f pos=(%s)"),
				*GetOwner()->GetName(), CurrentZ, ClimbTargetZ, TgtWallHeight,
				PostCrestPos.Z - PreCrestPos.Z, TgtPushUp,
				FVector::Dist2D(PreCrestPos, PostCrestPos), TgtPushFwd,
				CrestSpeed, *PostCrestPos.ToCompactString());
#endif

			ClimbTargetZ = 0.f;
			ClimbStartZ = 0.f;
		}
		else
		{
			const float RecentGain = CurrentZ - ClimbLastProgressZ;
			if (RecentGain > 20.f)
			{
				ClimbStallTime = 0.f;
				ClimbLastProgressZ = CurrentZ;

#if ZP_CMC_DEBUG_LOGS
				UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: CLIMB PROGRESS — Z=%.0f gain=%.0f target=%.0f remaining=%.0f"),
					*GetOwner()->GetName(), CurrentZ, RecentGain, ClimbTargetZ, ClimbTargetZ - CurrentZ);
#endif
			}
			else
			{
				ClimbStallTime += deltaTime;

#if ZP_CMC_DEBUG_LOGS
				if (bLog)
				{
					UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: CLIMB STALLING — Z=%.0f target=%.0f stallTime=%.2f/1.0s gain=%.1f"),
						*GetOwner()->GetName(), CurrentZ, ClimbTargetZ, ClimbStallTime, RecentGain);
				}
#endif

				if (ClimbStallTime > 1.0f)
				{
					WallContactTimer = 0.f;
					ContactWallNormal = FVector::ZeroVector;
					ClimbBailCooldown = 0.8f;

#if ZP_CMC_DEBUG_LOGS
					UE_LOG(LogTemp, Warning, TEXT("[ZP_CMC] %s: >>> CLIMB BAIL <<< stuck 1s at Z=%.0f (target=%.0f), cooldown 0.8s"),
						*GetOwner()->GetName(), CurrentZ, ClimbTargetZ);
#endif

					ClimbTargetZ = 0.f;
					ClimbStartZ = 0.f;
				}
			}

			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}
	}

	// --- Void kill ---
	{
		const float VoidZ = -5000.f;
		const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
		if (CurrentZ < VoidZ)
		{
			UE_LOG(LogTemp, Error, TEXT("[ZP_CMC] %s: VOID KILL at Z=%.0f"),
				*GetOwner()->GetName(), CurrentZ);
			GetOwner()->Destroy();
			return;
		}
	}

	// --- Body rotation ---
	{
		FRotator DesiredRotation = UpdatedComponent->GetComponentRotation();
		float InterpRate = 8.f;

		if (WallContactTimer > 0.f && !ContactWallNormal.IsNearlyZero())
		{
			const FVector CreatureUp = ContactWallNormal;
			const FVector ClimbForward = FVector::VectorPlaneProject(FVector::UpVector, ContactWallNormal).GetSafeNormal();
			if (!ClimbForward.IsNearlyZero())
			{
				DesiredRotation = FRotationMatrix::MakeFromXZ(ClimbForward, CreatureUp).Rotator();
			}
		}
		else
		{
			FVector FaceDir = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
			if (!FaceDir.IsNearlyZero())
			{
				DesiredRotation = FRotationMatrix::MakeFromXZ(FaceDir, FVector::UpVector).Rotator();
			}
			InterpRate = 12.f;
		}

		const FRotator CurrentRotation = UpdatedComponent->GetComponentRotation();
		const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, deltaTime, InterpRate);
		UpdatedComponent->MoveComponent(FVector::ZeroVector, NewRotation, false);
	}

	// ========================================================================
	// END-OF-FRAME SUMMARY
	// ========================================================================
#if ZP_CMC_DEBUG_LOGS
	if (bLog)
	{
		const FVector FinalPos = UpdatedComponent->GetComponentLocation();
		UE_LOG(LogTemp, Log, TEXT("[ZP_CMC] %s: END — Disp=%.1f FinalVel=(%s) Speed=%.0f Pos=(%s) wall=%.2f targetZ=%.0f bail=%.2f"),
			*GetOwner()->GetName(), Displacement, *Velocity.ToCompactString(), Velocity.Size(),
			*FinalPos.ToCompactString(), WallContactTimer, ClimbTargetZ, ClimbBailCooldown);
		LastLogTime = Now;
	}
#endif
}
