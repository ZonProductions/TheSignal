// Copyright The Signal. All Rights Reserved.

#include "ZP_ScytheerBase.h"
#include "ZP_ScytheerClimbPath.h"
#include "ZP_HealthComponent.h"
#include "ZP_SFXStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AZP_ScytheerBase::AZP_ScytheerBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule: half-scale Scytheer is roughly 134 UU tall. Cap fits the visible body and BLOCKS
	// the player's ECC_Visibility hitscan so OnTakePointDamage fires (same pattern as Shambler).
	UCapsuleComponent* Cap = GetCapsuleComponent();
	if (Cap)
	{
		Cap->SetCapsuleHalfHeight(65.f);
		Cap->SetCapsuleRadius(40.f);
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	// Mesh: re-anchor + scale 0.5. Capsule eats the bullet, mesh ignores Visibility so we never
	// double-trigger damage. SingleNode mode so PlayAnimation() + SetPosition control the slice.
	USkeletalMeshComponent* SM = GetMesh();
	if (SM)
	{
		SM->SetRelativeLocation(FVector(0.f, 0.f, -65.f));
		// (Pitch=90, Yaw=-90, Roll=0): Scytheer FBX imports lying on its back facing +X. Pitch=90
		// stands it upright, Yaw=-90 matches the standard Character forward (+X). Tweak the Mesh
		// component (NOT the actor) in the child BP if a future model has different import axes.
		SM->SetRelativeRotation(FRotator(90.f, -90.f, 0.f));
		SM->SetRelativeScale3D(FVector(0.5f));
		SM->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		SM->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	}

	// Possess with a stock AIController so MoveToLocation/MoveToActor work out of the box.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Movement defaults — wander pace until aggro flips it to chase.
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (CM)
	{
		CM->bOrientRotationToMovement = true;
		CM->bUseControllerDesiredRotation = false;
		CM->MaxWalkSpeed = WanderSpeed;
		CM->RotationRate = FRotator(0.f, 360.f, 0.f);
		// RVO avoidance — smoother steering around obstacles (doors that just-closed, props in
		// doorways, the player). Doesn't fix navmesh issues directly but stops the body from
		// punching into door geometry while a path reroute is happening. AvoidanceWeight=0.5 so
		// the Scytheer doesn't shove other AI off course.
		CM->bUseRVOAvoidance = true;
		CM->AvoidanceConsiderationRadius = 200.f;
		CM->AvoidanceWeight = 0.5f;
	}
	bUseControllerRotationYaw = false;

	// NOTE: SFX defaults are loaded lazily in BeginPlay (LoadSFXDefaults), NOT here.
	// ConstructorHelpers::FObjectFinder loading /Game assets during CDO construction crashed the
	// editor when the Shambler hit the same pattern with anim assets this session
	// (EXCEPTION_ACCESS_VIOLATION in FindOrLoadObject). Lazy LoadObject is the safe path and
	// preserves per-instance BP overrides via if (!Slot) guards.
}

void AZP_ScytheerBase::LoadSFXDefaults()
{
	auto FillSnd = [](TObjectPtr<USoundBase>& Slot, const TCHAR* P)
	{
		if (!Slot) { Slot = LoadObject<USoundBase>(nullptr, P); }
	};
	// Scytheer-owned duplicates of the Crawler/Shambler placeholders — sit in /Game/Audio/Scytheer/
	// with Scytheer-specific names so the dev can swap each one for real audio independently of the
	// Crawler. Earlier wiring re-used the Crawler assets directly which made true creature-specific
	// audio swap impossible without breaking the Crawler.
	FillSnd(AlertSound,  TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Alert.SFX_Scytheer_Alert"));
	FillSnd(AttackSound, TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Attack.SFX_Scytheer_Attack"));
	FillSnd(HitSound,    TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Hit.SFX_Scytheer_Hit"));
	FillSnd(DeathSound,  TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Death.SFX_Scytheer_Death"));
	FillSnd(LurkSound,   TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Lurk.SFX_Scytheer_Lurk"));

	// Carry/attenuation is C++-owned (UZP_SFXStatics Far profile, ~120 m natural falloff) — the old
	// SA_EnemyVoice asset silently fell silent at ~15 m, which killed alert/hit/death audibility.
	// AudioAttenuation is legacy and no longer loaded or passed anywhere.
}

void AZP_ScytheerBase::BeginPlay()
{
	Super::BeginPlay();

	LoadSFXDefaults(); // fill any SFX slots the BP didn't override (was a crashing ctor load)

	// Force-spawn the AIController if AutoPossessAI somehow didn't fire (BP override, timing
	// race, simulate mode, ...). Without a controller, MoveToLocation is a no-op and the body
	// just plays its walk anim in place — exactly the symptom.
	if (!GetController())
	{
		SpawnDefaultController();
	}

	// ACharacter constructs with MovementMode=MOVE_None and only transitions to Walking on a
	// Falling→Landed event. If the editor places us ON the floor (no fall), we'd stay in NONE
	// forever — capsule won't translate, MoveToLocation no-ops, body plays walk anim in place.
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		if (CM->MovementMode == MOVE_None)
		{
			CM->SetMovementMode(MOVE_Walking);
		}
	}
	AICon = Cast<AAIController>(GetController());
	UE_LOG(LogTemp, Warning, TEXT("[Scytheer] BeginPlay controller=%s AICon=%s PatrolPath=%s splineLen=%.0f"),
		GetController() ? *GetController()->GetName() : TEXT("NULL"),
		AICon ? *AICon->GetName() : TEXT("NULL"),
		PatrolPath ? *PatrolPath->GetName() : TEXT("NULL"),
		PatrolPath ? PatrolPath->GetSplineLength() : 0.f);

	// Anim slice timing — anchor SecondsPerFrame to DieEndFrame so every user-typed frame marker
	// maps to the same fraction of the clip regardless of sampler-reported fps.
	if (SingleAnim)
	{
		ClipLen = SingleAnim->GetPlayLength();
		if (DieEndFrame > 0)
		{
			SecondsPerFrame = ClipLen / (float)DieEndFrame;
		}
		else
		{
			const double FPS = SingleAnim->GetSamplingFrameRate().AsDecimal();
			SecondsPerFrame = (FPS > 0.0) ? (float)(1.0 / FPS) : (1.f / 30.f);
		}
		UE_LOG(LogTemp, Warning, TEXT("[Scytheer] anim=%s len=%.3fs spf=%.4f (Walk=%.2fs-%.2fs, Die=%.2fs-%.2fs)"),
			*SingleAnim->GetName(), ClipLen, SecondsPerFrame,
			Frame2Time(WalkStartFrame), Frame2Time(WalkEndFrame),
			Frame2Time(DieStartFrame), Frame2Time(DieEndFrame));
		GetMesh()->PlayAnimation(SingleAnim, true);
	}

	// Health — auto-attach; OnDied -> Die state.
	Health = FindComponentByClass<UZP_HealthComponent>();
	if (!Health)
	{
		Health = NewObject<UZP_HealthComponent>(this, TEXT("ScytheerHealth"));
		Health->MaxHealth = MaxHealth;
		Health->RegisterComponent();
	}
	Health->MaxHealth = MaxHealth;
	Health->ResetHealth();
	Health->OnDied.AddDynamic(this, &AZP_ScytheerBase::OnOwnerDied);
	OnTakePointDamage.AddDynamic(this, &AZP_ScytheerBase::OnPointDamage);

	// Capsule re-assert (post-BP-load) — anything the editor flipped goes back to shootable.
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	if (USkeletalMeshComponent* M = GetMesh())
	{
		M->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	EnterState(EScytheerState::Wander);
}

void AZP_ScytheerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Lazy AIController cache — AutoPossessAI = PlacedInWorldOrSpawned can run AFTER BeginPlay, so
	// the BeginPlay cache misses. Re-Cast every tick until we have one, then never again. Also
	// force-spawn the default controller if no controller appeared at all.
	if (!AICon)
	{
		if (!GetController())
		{
			SpawnDefaultController();
		}
		AICon = Cast<AAIController>(GetController());
		if (AICon)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Scytheer] AIController possessed: %s"), *AICon->GetName());
		}
	}

	// Throttled state probe — ~once per second at 60fps. Print exactly what the wander state sees
	// so we can tell whether the patrol-branch even runs, what AICon/PatrolPath are, and whether
	// the body is translating.
	{
		static int32 sLogCounter = 0;
		if ((++sLogCounter % 60) == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Scytheer] TICK state=%d AICon=%s PatrolPath=%s splineLen=%.0f bWanderMoving=%d loc=(%.0f,%.0f,%.0f) vel=%.0f"),
				(int32)State,
				AICon ? *AICon->GetName() : TEXT("NULL"),
				PatrolPath ? *PatrolPath->GetName() : TEXT("NULL"),
				PatrolPath ? PatrolPath->GetSplineLength() : 0.f,
				bWanderMoving ? 1 : 0,
				GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z,
				GetVelocity().Size());
		}
	}

	if (bDead) { TickAnim(); return; }

	APawn* Player = GetPlayer();
	const float Dist = Player ? FVector::Dist(GetActorLocation(), Player->GetActorLocation()) : TNumericLimits<float>::Max();
	const bool bSee = Player ? HasLOS(Player) : false;

	// "On wall" test = is the body currently elevated above the patrol spline's ground point?
	// If yes, aggro should run DOWN the spline first (WallDescent) instead of instant-dropping +
	// transitioning to Chase (which gravity-yeets the body off the wall).
	const bool bOnWall = (PatrolPath
		&& (GetActorLocation().Z - PatrolPath->GetLocationAtDistance(0.f).Z) > OnWallZThreshold);

	// Detection / aggro management.
	if (!bAggro && State != EScytheerState::Die)
	{
		// "Same geometry" gate: LOS trace + navmesh reachability. A closed door blocks both. An
		// open door lets the navmesh path through and aggro fires only then.
		const bool bReachable = (Player && bSee) ? IsPlayerReachable(Player) : false;
		if (Player && Dist <= DetectionRange && bSee && bReachable)
		{
			// Log what the LOS trace actually hit (or didn't). If HasLOS thinks it sees the player
			// through a wall, this fires whenever bAggro flips on — we can see the trace start/end +
			// the first blocker so we know whether the wall has no collision, isn't on WorldStatic,
			// or LOS is being fooled by a doorway.
			FHitResult DbgHit;
			FCollisionQueryParams DbgP;
			DbgP.AddIgnoredActor(this);
			DbgP.AddIgnoredActor(Player);
			const FVector Eye = GetActorLocation() + FVector(0, 0, 30.f);
			const FVector End = Player->GetActorLocation() + FVector(0, 0, 40.f);
			const bool bAnyHit = GetWorld()->LineTraceSingleByChannel(DbgHit, Eye, End, ECC_WorldStatic, DbgP);
			UE_LOG(LogTemp, Warning, TEXT("[Scytheer] AGGRO eye=(%.0f,%.0f,%.0f) -> player=(%.0f,%.0f,%.0f) Dist=%.0f bSee=1 | rawTrace blocked=%d first='%s' atDist=%.0f"),
				Eye.X, Eye.Y, Eye.Z, End.X, End.Y, End.Z, Dist,
				bAnyHit ? 1 : 0,
				(bAnyHit && DbgHit.GetActor()) ? *DbgHit.GetActor()->GetName() : TEXT("CLEAR-no block"),
				bAnyHit ? DbgHit.Distance : 0.f);
			bAggro = true;
			LostSightTimer = 0.f;
			if (State != EScytheerState::Attack && State != EScytheerState::Hit)
			{
				// Route aggro: on the wall -> WallDescent (run DOWN the spline at chase speed).
				// On the ground -> normal Alert (idle hold + SFX, then Chase).
				EnterState(bOnWall ? EScytheerState::WallDescent : EScytheerState::Alert);
			}
		}
	}
	else if (bAggro)
	{
		LostSightTimer = bSee ? 0.f : (LostSightTimer + DeltaTime);
		if (LostSightTimer >= LoseSightTime || Dist > GiveUpRange)
		{
			bAggro = false;
			LostSightTimer = 0.f;
			if (State == EScytheerState::Chase)
			{
				// Patrol path set -> walk back to it (no teleport). No path -> straight to Wander
				// (which falls back to random navmesh wander).
				EnterState(PatrolPath ? EScytheerState::ReturnToPatrol : EScytheerState::Wander);
			}
		}
	}

	const double Now = GetWorld()->GetTimeSeconds();

	switch (State)
	{
	case EScytheerState::Wander:
	{
		if (PatrolPath && PatrolPath->GetSplineLength() > KINDA_SMALL_NUMBER)
		{
			// SPLINE-DIRECT patrol: drive the body's position + rotation straight from the spline,
			// ping-ponging end to end at WanderSpeed. Uses the per-point WallNormals on the path so
			// the Scytheer can walk up walls / across ceilings — CharacterMovement is in MOVE_None
			// during patrol (set in EnterState(Wander)) so gravity doesn't pull the body off the wall
			// and the capsule doesn't try to step-up world geometry while clinging.
			const float Total = PatrolPath->GetSplineLength();
			PatrolDistance += (float)PatrolDir * WanderSpeed * DeltaTime;
			if (PatrolDistance >= Total)      { PatrolDistance = Total; PatrolDir = -1; }
			else if (PatrolDistance <= 0.f)   { PatrolDistance = 0.f;   PatrolDir =  1; }

			const FVector Loc = PatrolPath->GetLocationAtDistance(PatrolDistance);
			const FVector Up  = PatrolPath->GetWallNormalAtDistance(PatrolDistance);
			FVector Fwd       = PatrolPath->GetForwardAtDistance(PatrolDistance);
			if (PatrolDir < 0) { Fwd = -Fwd; }

			// Smooth orientation: at the spline endpoints (and any other moment Fwd flips
			// direction), capping the per-frame angular speed avoids the 1-frame 180° snap.
			// PatrolTurnRate is deg/s — high enough that normal patrol along a curving spline
			// looks instantaneous, low enough that endpoint reversals turn over ~0.5s.
			const FRotator DesiredRot = MakeOrientation(Fwd, Up);
			const FRotator NewRot     = FMath::RInterpConstantTo(GetActorRotation(), DesiredRot, DeltaTime, PatrolTurnRate);
			SetActorLocationAndRotation(Loc, NewRot, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			// Fallback: random navmesh wander when no PatrolPath is wired.
			if (bWanderMoving)
			{
				if (FVector::Dist2D(GetActorLocation(), WanderDest) < 120.f)
				{
					bWanderMoving = false;
					PauseTimer = FMath::FRandRange(PauseMin, PauseMax);
					if (AICon) { AICon->StopMovement(); }
				}
			}
			else
			{
				PauseTimer -= DeltaTime;
				if (PauseTimer <= 0.f)
				{
					PickNewWanderPoint();
					bWanderMoving = true;
				}
			}
		}
		break;
	}

	case EScytheerState::Alert:
	{
		// Sit in the idle pose; face the player; advance to Chase after AlertHoldTime.
		FaceTargetSmooth(DeltaTime);
		if ((Now - StateEnteredAt) >= AlertHoldTime)
		{
			EnterState(EScytheerState::Chase);
		}
		break;
	}

	case EScytheerState::WallDescent:
	{
		// Drive the body backward along the spline (PatrolDistance -> 0) at ChaseSpeed, holding
		// the wall-normal orientation so it reads as "running down the wall". On arrival within a
		// short threshold of the ground point, hand off to Chase (which restores MOVE_Walking and
		// starts the navmesh pursuit).
		if (PatrolPath && PatrolPath->GetSplineLength() > KINDA_SMALL_NUMBER)
		{
			PatrolDistance -= ChaseSpeed * DeltaTime;
			if (PatrolDistance < 0.f) { PatrolDistance = 0.f; }

			const FVector Loc = PatrolPath->GetLocationAtDistance(PatrolDistance);
			const FVector Up  = PatrolPath->GetWallNormalAtDistance(PatrolDistance);
			// Forward = direction of motion = -spline tangent (we're going backward through the spline).
			FVector Fwd       = -PatrolPath->GetForwardAtDistance(PatrolDistance);

			const FRotator Desired = MakeOrientation(Fwd, Up);
			const FRotator NewRot  = FMath::RInterpConstantTo(GetActorRotation(), Desired, DeltaTime, PatrolTurnRate);
			SetActorLocationAndRotation(Loc, NewRot, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

			// Drive the descent ALL THE WAY to the spline's start point (PatrolDistance == 0). The
			// earlier Z-threshold early-out was bailing at PatrolDistance ~ pt2 (Z still elevated
			// above the floor), which left the Scytheer mid-wall when Chase began and gravity
			// dropped it a few feet to the floor. Following the full spline back to pt0 lets the
			// "curl from wall onto floor" motion play out the same way the climb-up did.
			if (PatrolDistance <= 0.f)
			{
				EnterState(EScytheerState::Chase);
			}
		}
		else
		{
			// No path somehow -> can't descend cleanly; fall straight to Chase. CMC restored to
			// Walking by EnterState's transition guard.
			EnterState(EScytheerState::Chase);
		}
		break;
	}

	case EScytheerState::Chase:
	{
		if (!Player) { EnterState(EScytheerState::Wander); break; }

		// In range + off cooldown -> swipe.
		if (Dist <= AttackRange && bSee && (Now - LastAttackTime) >= AttackCooldown)
		{
			PendingAttackVariant = FMath::RandRange(1, 3);
			LastAttackTime = Now;
			AttackStartTime = Now;
			bAttackHitFired = false;
			EnterState(EScytheerState::Attack);
			break;
		}

		// Stuck detection: if the body hasn't moved more than 30 UU since the last check, count the
		// time. At 2s try a wider acceptance radius (path may have been too tight against geometry).
		// At 4s give up and return to Wander before whatever 7s watchdog respawns the actor — getting
		// caught against door geometry was the dev's repeat complaint.
		if (FVector::DistSquared(GetActorLocation(), LastChaseStuckLoc) < 30.f * 30.f)
		{
			ChaseStuckTimer += DeltaTime;
		}
		else
		{
			ChaseStuckTimer = 0.f;
			LastChaseStuckLoc = GetActorLocation();
		}
		if (ChaseStuckTimer >= 4.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Scytheer] CHASE stuck 4s -> de-aggro at (%.0f,%.0f,%.0f)"),
				GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
			bAggro = false;
			LostSightTimer = 0.f;
			ChaseStuckTimer = 0.f;
			// Walk back to the patrol spline if we have one (no teleport). No path -> wander.
			EnterState(PatrolPath ? EScytheerState::ReturnToPatrol : EScytheerState::Wander);
			break;
		}
		if (ChaseStuckTimer >= 2.f && AICon)
		{
			// Wider acceptance radius — often the AI is parked one capsule-width from a door because
			// the goal is set to AttackRange-60; relaxing it lets path-following declare success and
			// the next frame's MoveToActor recomputes.
			AICon->StopMovement();
			AICon->MoveToActor(Player, FMath::Max(AttackRange - 60.f, 200.f));
			ChaseStuckTimer = 2.5f; // hold above the threshold so we don't re-issue every frame
			break;
		}

		// Keep closing — re-path each frame so the chase tracks the moving player.
		if (AICon) { AICon->MoveToActor(Player, FMath::Max(AttackRange - 60.f, 40.f)); }
		break;
	}

	case EScytheerState::Attack:
	{
		FaceTargetSmooth(DeltaTime);
		// Apply damage at the slice midpoint (one strike per swing).
		if (!bAttackHitFired && Player)
		{
			const float Elapsed = (float)(Now - AttackStartTime);
			const float Midpoint = (SegEndT - SegStartT) * 0.5f;
			if (Elapsed >= Midpoint)
			{
				bAttackHitFired = true;
				if (Dist <= AttackRange * 1.4f && bSee)
				{
					UGameplayStatics::ApplyDamage(Player, AttackDamage, GetInstigatorController(), this, nullptr);
				}
			}
		}
		break;
	}

	case EScytheerState::Hit:
	{
		FaceTargetSmooth(DeltaTime);
		break;
	}

	case EScytheerState::ReturnToPatrol:
	{
		// Walk back to the closest spline point via CharacterMovement. When close enough, hand off
		// to Wander (which will set MOVE_None and start the spline-direct patrol from here — no
		// teleport, the body is already at the spline).
		if (!PatrolPath)
		{
			EnterState(EScytheerState::Wander);
			break;
		}
		const float ClosestD = PatrolPath->GetClosestDistanceToPoint(GetActorLocation());
		const FVector Goal = PatrolPath->GetLocationAtDistance(ClosestD);
		if (FVector::Dist(GetActorLocation(), Goal) < 80.f)
		{
			EnterState(EScytheerState::Wander);
		}
		else
		{
			// Re-issue MoveTo each tick in case the chosen point shifted or got blocked.
			if (AICon) { AICon->MoveToLocation(Goal, 80.f); }
		}
		break;
	}

	default:
		break;
	}

	TickAnim();
}

// ───────────────────────── state / anim slicing ─────────────────────────

void AZP_ScytheerBase::EnterState(EScytheerState NewState)
{
	State = NewState;
	StateEnteredAt = GetWorld()->GetTimeSeconds();

	// Any non-Wander, non-WallDescent state needs real CharacterMovement (walking + gravity +
	// navmesh chase). Wander + WallDescent both drive position directly from the spline and need
	// MOVE_None so gravity doesn't fight the SetActorLocationAndRotation calls. ReturnToPatrol
	// is the bridge between Chase (gravity + navmesh) and Wander (spline-direct): keep MOVE_Walking
	// and navmesh-walk toward the closest spline point so the body doesn't teleport on hand-off.
	if (NewState != EScytheerState::Wander && NewState != EScytheerState::WallDescent)
	{
		if (UCharacterMovementComponent* CM = GetCharacterMovement())
		{
			if (CM->MovementMode == MOVE_None)
			{
				CM->SetMovementMode(MOVE_Walking);
			}
		}
	}

	switch (NewState)
	{
	case EScytheerState::Wander:
		StartSegment(WalkStartFrame, WalkEndFrame, true);
		SetMaxWalkSpeed(WanderSpeed);
		bWanderMoving = false;
		PauseTimer = 0.f; // pick a leg on the next tick
		// Rejoin nearest spot on the patrol path so the resume point isn't always 0.
		if (PatrolPath)
		{
			PatrolDistance = PatrolPath->GetClosestDistanceToPoint(GetActorLocation());
			// Spline-direct patrol: park CharacterMovement so gravity / collision pushes don't fight
			// the SetActorLocationAndRotation we issue from the patrol's wall normals.
			if (UCharacterMovementComponent* CM = GetCharacterMovement())
			{
				CM->StopMovementImmediately();
				CM->SetMovementMode(MOVE_None);
			}
		}
		break;

	case EScytheerState::Alert:
		StartSegment(IdleStartFrame, IdleEndFrame, true);
		SetMaxWalkSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		UZP_SFXStatics::PlaySFXAttached(AlertSound, GetRootComponent(), EZP_SFXCarry::Far);
		break;

	case EScytheerState::WallDescent:
		// Sprint down the spline using the same spline-direct mechanism patrol uses, just at chase
		// speed and with the Run anim. CharacterMovement stays in MOVE_None (set above via the
		// !=Wander && !=WallDescent guard) so the body doesn't fall off the wall mid-descent. The
		// per-tick logic below drives PatrolDistance toward 0 (the ground end of the spline) and
		// hands off to Chase once we reach it.
		StartSegment(RunStartFrame, RunEndFrame, true);
		SetMaxWalkSpeed(0.f); // movement is driven by SetActorLocation, not CMC, during descent
		if (AICon) { AICon->StopMovement(); }
		UZP_SFXStatics::PlaySFXAttached(AlertSound, GetRootComponent(), EZP_SFXCarry::Far);
		break;

	case EScytheerState::Chase:
		StartSegment(RunStartFrame, RunEndFrame, true);
		SetMaxWalkSpeed(ChaseSpeed);
		ChaseStuckTimer = 0.f;
		LastChaseStuckLoc = GetActorLocation();
		break;

	case EScytheerState::Attack:
	{
		int32 SF = Attack1StartFrame, EF = Attack1EndFrame;
		if (PendingAttackVariant == 2) { SF = Attack2StartFrame; EF = Attack2EndFrame; }
		else if (PendingAttackVariant == 3) { SF = Attack3StartFrame; EF = Attack3EndFrame; }
		StartSegment(SF, EF, false);
		SetMaxWalkSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		UZP_SFXStatics::PlaySFXAttached(AttackSound, GetRootComponent(), EZP_SFXCarry::Far);
		break;
	}

	case EScytheerState::Hit:
		StartSegment(HitStartFrame, HitEndFrame, false);
		// Far carry matters here: the player can hitscan-snipe from well past room scale and needs
		// the hit-confirm to be audible at the shot distance.
		UZP_SFXStatics::PlaySFXAttached(HitSound, GetRootComponent(), EZP_SFXCarry::Far);
		break;

	case EScytheerState::Die:
		StartSegment(DieStartFrame, DieEndFrame, false);
		SetMaxWalkSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		// Silent when a LOAD restores a corpse (ApplyDeadStateInstant re-enters Die for the pose) —
		// with Far carry, replaying the cry would make every dead Scytheer audibly "die" on load.
		if (!bRestoringDeadState)
		{
			UZP_SFXStatics::PlaySFXAttached(DeathSound, GetRootComponent(), EZP_SFXCarry::Far);
		}
		break;

	case EScytheerState::ReturnToPatrol:
		// Walking back to the closest spline point via the navmesh — no spline-direct teleport.
		StartSegment(WalkStartFrame, WalkEndFrame, true);
		SetMaxWalkSpeed(WanderSpeed);
		if (PatrolPath && AICon)
		{
			const float ClosestD = PatrolPath->GetClosestDistanceToPoint(GetActorLocation());
			const FVector Goal = PatrolPath->GetLocationAtDistance(ClosestD);
			AICon->MoveToLocation(Goal, 80.f);
		}
		break;
	}
}

void AZP_ScytheerBase::StartSegment(int32 FrameStart, int32 FrameEnd, bool bLoop)
{
	SegStartT = Frame2Time(FrameStart);
	SegEndT = Frame2Time(FrameEnd);
	bSegLoops = bLoop;

	USkeletalMeshComponent* SM = GetMesh();
	if (!SM) { return; }

	if (UAnimSingleNodeInstance* SNI = SM->GetSingleNodeInstance())
	{
		SNI->SetPlayRate(1.0f);
		SNI->SetLooping(false); // we manually loop by seeking — exact slice boundaries
		SNI->SetPosition(SegStartT, false);
		SNI->SetPlaying(true);
	}
	else if (SingleAnim)
	{
		SM->PlayAnimation(SingleAnim, true);
		if (UAnimSingleNodeInstance* SNI2 = SM->GetSingleNodeInstance())
		{
			SNI2->SetLooping(false);
			SNI2->SetPosition(SegStartT, false);
			SNI2->SetPlaying(true);
		}
	}
}

void AZP_ScytheerBase::TickAnim()
{
	USkeletalMeshComponent* SM = GetMesh();
	UAnimSingleNodeInstance* SNI = SM ? SM->GetSingleNodeInstance() : nullptr;
	if (!SNI) { return; }

	if (SNI->GetCurrentTime() >= SegEndT)
	{
		if (bSegLoops)
		{
			SNI->SetPosition(SegStartT, false);
		}
		else
		{
			SNI->SetPosition(SegEndT, false);
			SNI->SetPlaying(false);
			OnSegmentComplete();
		}
	}
}

void AZP_ScytheerBase::OnSegmentComplete()
{
	switch (State)
	{
	case EScytheerState::Hit:
		// A forced stagger holds past the flinch clip — the StaggerHandle timer owns the exit.
		if (bStaggerHold) { break; }
		EnterState(bAggro ? EScytheerState::Chase : EScytheerState::Wander);
		break;
	case EScytheerState::Attack:
		EnterState(bAggro ? EScytheerState::Chase : EScytheerState::Wander);
		break;
	case EScytheerState::Die:
		// Stay dead. Final frame held.
		break;
	default:
		break;
	}
}

float AZP_ScytheerBase::Frame2Time(int32 Frame) const
{
	const float T = (float)Frame * SecondsPerFrame;
	return (ClipLen > 0.f) ? FMath::Clamp(T, 0.f, ClipLen) : T;
}

// ───────────────────────── helpers ─────────────────────────

APawn* AZP_ScytheerBase::GetPlayer() const
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		return PC->GetPawn();
	}
	return nullptr;
}

bool AZP_ScytheerBase::HasLOS(const AActor* Target) const
{
	if (!Target || !GetWorld()) { return false; }
	const FVector Start = GetActorLocation() + FVector(0, 0, 30.f);
	const FVector End = Target->GetActorLocation() + FVector(0, 0, 40.f);
	FCollisionQueryParams P;
	P.AddIgnoredActor(this);
	P.AddIgnoredActor(Target);
	TArray<AActor*> Att;
	Target->GetAttachedActors(Att);
	for (AActor* A : Att) { P.AddIgnoredActor(A); }
	GetAttachedActors(Att);
	for (AActor* A : Att) { P.AddIgnoredActor(A); }

	// Walk the line, skipping QueryOnly trigger volumes (door interaction boxes, overlap zones).
	// BOTH directions must be clear — complex-as-simple wall collision has no BACKFACES, so a body
	// hugging a wall can trace out through it unblocked; the reverse trace catches the front face
	// (same fix as the Shambler's through-wall aggro).
	auto DirClear = [this](FVector A, FVector B, FCollisionQueryParams Params) -> bool
	{
		for (int32 Iter = 0; Iter < 8; ++Iter)
		{
			FHitResult Hit;
			if (!GetWorld()->LineTraceSingleByChannel(Hit, A, B, ECC_WorldStatic, Params))
			{
				return true;
			}
			UPrimitiveComponent* Comp = Hit.GetComponent();
			if (Comp && Comp->GetCollisionEnabled() == ECollisionEnabled::QueryOnly)
			{
				Params.AddIgnoredComponent(Comp);
				continue;
			}
			return false;
		}
		return false;
	};
	return DirClear(Start, End, P) && DirClear(End, Start, P);
}

bool AZP_ScytheerBase::IsPlayerReachable(const AActor* Target) const
{
	if (!Target || !GetWorld()) { return false; }
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav) { return true; } // no nav system -> degrade to LOS-only (don't lock down aggro)
	const ANavigationData* NavData = Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	if (!NavData) { return true; }

	FPathFindingQuery Query(this, *NavData, GetActorLocation(), Target->GetActorLocation());
	Query.SetAllowPartialPaths(false); // partial paths still mean "blocked somewhere" — reject
	const FPathFindingResult Result = Nav->FindPathSync(Query, EPathFindingMode::Regular);
	if (Result.Result != ENavigationQueryResult::Success || !Result.Path.IsValid())
	{
		return false; // closed door / unreachable region
	}
	const float PathLen = Result.Path->GetLength();
	return PathLen <= MaxReachablePathLength;
}

void AZP_ScytheerBase::PickNewWanderPoint()
{
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav || !AICon) { return; }
	FNavLocation Result;
	if (Nav->GetRandomReachablePointInRadius(GetActorLocation(), WanderRadius, Result))
	{
		WanderDest = Result.Location;
		AICon->MoveToLocation(WanderDest, 80.f);
	}
}

void AZP_ScytheerBase::SetMaxWalkSpeed(float Speed)
{
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		CM->MaxWalkSpeed = Speed;
	}
}

void AZP_ScytheerBase::FaceTargetSmooth(float DeltaTime)
{
	APawn* P = GetPlayer();
	if (!P) { return; }
	FVector ToP = P->GetActorLocation() - GetActorLocation();
	ToP.Z = 0.f;
	if (ToP.IsNearlyZero()) { return; }
	const FRotator Cur = GetActorRotation();
	const FRotator Desired(0.f, ToP.Rotation().Yaw, 0.f);
	const FRotator NewRot = FMath::RInterpConstantTo(Cur, Desired, DeltaTime, CombatTurnRate);
	SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
}

// ───────────────────────── damage ─────────────────────────

void AZP_ScytheerBase::OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy,
	FVector HitLocation, UPrimitiveComponent* FHitComp, FName BoneName, FVector ShotDir,
	const UDamageType* DamageType, AActor* DamageCauser)
{
	if (bDead || !Health) { return; }
	const float HitZAboveCentre = HitLocation.Z - GetActorLocation().Z;
	const bool bHead = HitZAboveCentre >= HeadshotMinZ;
	const float Dmg = bHead ? HeadShotDamage : BodyShotDamage;
	UE_LOG(LogTemp, Warning, TEXT("[Scytheer] HIT z+%.0f -> %s, %.0f dmg (was %.0f HP)"),
		HitZAboveCentre, bHead ? TEXT("HEADSHOT") : TEXT("body"), Dmg, Health->CurrentHealth);
	Health->ApplyDamage(Dmg);

	if (!bDead)
	{
		bAggro = true;
		LostSightTimer = 0.f;
		// Cooldown-gated flinch. Without it, sustained fire chains the Hit state every bullet ->
		// perma-stunlock. 1.5s lets the body act between flinches.
		const double NowDmg = GetWorld()->GetTimeSeconds();
		if (State != EScytheerState::Attack && State != EScytheerState::Die
		 && (NowDmg - LastHitReactTime) >= HitReactCooldown)
		{
			LastHitReactTime = NowDmg;
			EnterState(EScytheerState::Hit);
		}
	}
}

void AZP_ScytheerBase::ReceiveStagger_Implementation(float Duration)
{
	if (bDead || State == EScytheerState::Die) { return; }

	// A landed hit/block means the player is right here — wake up and react.
	bAggro = true;
	LostSightTimer = 0.f;
	LastHitReactTime = GetWorld()->GetTimeSeconds(); // refresh so the normal flinch path stays gated

	bStaggerHold = true;
	EnterState(EScytheerState::Hit); // plays the flinch clip + Hit SFX; FaceTargetSmooth runs in Tick
	if (AICon) { AICon->StopMovement(); }

	// Hold the stagger for Duration, then resume. OnSegmentComplete won't auto-exit while bStaggerHold.
	GetWorld()->GetTimerManager().ClearTimer(StaggerHandle);
	GetWorld()->GetTimerManager().SetTimer(StaggerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bStaggerHold = false;
		if (!bDead && State == EScytheerState::Hit)
		{
			EnterState(bAggro ? EScytheerState::Chase : EScytheerState::Wander);
		}
	}), FMath::Max(0.1f, Duration), false);
}

FRotator AZP_ScytheerBase::MakeOrientation(const FVector& Forward, const FVector& Up)
{
	const FVector SafeFwd = Forward.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
	FVector SafeUp = Up.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);

	// Orthonormalize Up against Forward — if the dev's wall normal isn't perfectly perpendicular to
	// the spline tangent at this point, the matrix would otherwise be invalid.
	SafeUp = (SafeUp - FVector::DotProduct(SafeUp, SafeFwd) * SafeFwd)
		.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ZeroVector);

	// Up collapsed to zero == the wall normal was (near-)parallel to the spline tangent — e.g. a
	// vertical climb section whose normal was still left at world-up. Without a real Up, MakeFromXZ
	// produces a garbage/flipped frame (the "back to the wall, walking on air" bug). Derive ANY
	// stable perpendicular instead so the body stays sanely oriented until the normal is fixed.
	if (SafeUp.IsNearlyZero())
	{
		SafeUp = FVector::CrossProduct(SafeFwd, FVector::RightVector);
		if (SafeUp.IsNearlyZero())
		{
			SafeUp = FVector::CrossProduct(SafeFwd, FVector::UpVector);
		}
		SafeUp = SafeUp.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
	}

	return FRotationMatrix::MakeFromXZ(SafeFwd, SafeUp).Rotator();
}

void AZP_ScytheerBase::OnOwnerDied()
{
	if (bDead) { return; }
	bDead = true;
	bAggro = false;
	if (UCapsuleComponent* Cap = GetCapsuleComponent()) { Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (UCharacterMovementComponent* CM = GetCharacterMovement()) { CM->StopMovementImmediately(); CM->DisableMovement(); }
	EnterState(EScytheerState::Die);
}

// ───────────────────────── IZP_Revivable (save-state + revival) ─────────────────────────

void AZP_ScytheerBase::ApplyDeadStateInstant_Implementation()
{
	// Snap to the corpse pose without replaying the die clip from the start — used on a LOAD restore.
	bDead = true;
	bAggro = false;
	if (UCapsuleComponent* Cap = GetCapsuleComponent()) { Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (UCharacterMovementComponent* CM = GetCharacterMovement()) { CM->StopMovementImmediately(); CM->DisableMovement(); }
	bRestoringDeadState = true; // suppress the death-cry replay inside EnterState(Die)
	EnterState(EScytheerState::Die); // sets the Die anim segment (SegStartT..SegEndT)
	bRestoringDeadState = false;
	if (USkeletalMeshComponent* M = GetMesh()) { M->SetPosition(SegEndT, /*bFireNotifies=*/false); } // hold final frame
	UE_LOG(LogTemp, Log, TEXT("[Scytheer] dead-state restored on load: %s"), *GetName());
}

void AZP_ScytheerBase::ReviveEnemy_Implementation()
{
	// Bring a dead Scytheer back to a fully alive, patrolling state.
	bDead = false;
	bAggro = false;
	LostSightTimer = 0.f;

	if (Health) { Health->ResetHealth(); } // CurrentHealth=Max, bIsDead=false

	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // re-assert shootability
	}
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		CM->SetMovementMode(MOVE_Walking);
	}

	// EnterState(Wander) restores the walk segment + speed, rejoins the patrol path, and parks
	// CharacterMovement appropriately (MOVE_None for spline-direct patrol).
	EnterState(EScytheerState::Wander);
	UE_LOG(LogTemp, Log, TEXT("[Scytheer] REVIVED: %s"), *GetName());
}
