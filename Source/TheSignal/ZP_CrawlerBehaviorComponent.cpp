// Copyright The Signal. All Rights Reserved.

#include "ZP_CrawlerBehaviorComponent.h"
#include "ZP_CrawlerMovementComponent.h"
#include "ZP_WallMap.h"
#include "ZP_HealthComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Perception/PawnSensingComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

// Static registry — all active crawler behavior components
TArray<UZP_CrawlerBehaviorComponent*> UZP_CrawlerBehaviorComponent::AllCrawlers;

UZP_CrawlerBehaviorComponent::UZP_CrawlerBehaviorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UZP_CrawlerBehaviorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Movement is handled by ZP_CrawlerMovementComponent::PhysFlying directly reading MoveTarget.
	// No AddMovementInput needed.
}

void UZP_CrawlerBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Register in static registry FIRST — before anything else can early-return.
	// BroadcastGunshot uses this instead of FindComponentByClass (which fails due to CoreRedirect).
	AllCrawlers.AddUnique(this);
	UE_LOG(LogTemp, Warning, TEXT("[ZP_Behavior] REGISTRY: %s registered (class=%s, owner=%s, total=%d)"),
		*GetName(),
		*GetClass()->GetName(),
		GetOwner() ? *GetOwner()->GetName() : TEXT("NO_OWNER"),
		AllCrawlers.Num());

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Cache our custom CMC
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		CrawlerCMC = Cast<UZP_CrawlerMovementComponent>(Character->GetCharacterMovement());
		if (CrawlerCMC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZP_Behavior] %s: CrawlerCMC OK (class=%s)"),
				*Owner->GetName(), *CrawlerCMC->GetClass()->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ZP_Behavior] %s: CrawlerCMC CAST FAILED! Actual CMC class=%s — creature WILL NOT MOVE. BP parent must be ZP_CrawlerBase."),
				*Owner->GetName(), *Character->GetCharacterMovement()->GetClass()->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ZP_Behavior] %s: Owner is NOT a Character! class=%s"),
			*Owner->GetName(), *Owner->GetClass()->GetName());
	}

	// Disable plugin climbing system — we use ZP_CrawlerMovementComponent for all climbing.
	// BPC_Climbing (from BP_ClimbingSystem_Character) spams Sqrt() warnings and tanks FPS.
	TArray<UActorComponent*> AllComponents;
	Owner->GetComponents(AllComponents);
	for (UActorComponent* Comp : AllComponents)
	{
		if (Comp && Comp->GetClass()->GetName().Contains(TEXT("BPC_Climbing")))
		{
			Comp->Deactivate();
			Comp->SetComponentTickEnabled(false);
			UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Disabled plugin climbing component: %s"),
				*Owner->GetName(), *Comp->GetName());
		}
	}

	// Auto-bind PawnSensing
	if (UPawnSensingComponent* Sensing = Owner->FindComponentByClass<UPawnSensingComponent>())
	{
		Sensing->OnSeePawn.AddDynamic(this, &UZP_CrawlerBehaviorComponent::OnSeePawn);
		UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Bound to PawnSensingComponent."),
			*Owner->GetName());
	}

	// Diagnostic: check if controller exists (PawnSensing requires it for LOS)
	if (APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Controller=%s AutoPossessAI=%d"),
			*Owner->GetName(),
			OwnerPawn->GetController() ? *OwnerPawn->GetController()->GetName() : TEXT("NONE"),
			(int32)OwnerPawn->AutoPossessAI);
	}

	// Build wall map on first creature spawn — shared across all creatures
	if (!FZP_WallMap::IsBuilt())
	{
		FZP_WallMap::Build(GetWorld(), Owner->GetActorLocation());
	}

	if (bAutoInitialize)
	{
		if (InitialPatrolDelay > 0.f)
		{
			// Freeze movement during delay — creature stays put until behavior starts
			if (ACharacter* Character = Cast<ACharacter>(Owner))
			{
				if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
				{
					CMC->StopMovementImmediately();
					CMC->SetMovementMode(MOVE_None);
				}
			}

			// Hide creature during delay — prevents visible settling (body bobs, legs find ground)
			Owner->SetActorHiddenInGame(true);

			FTimerHandle DelayHandle;
			GetWorld()->GetTimerManager().SetTimer(DelayHandle, this,
				&UZP_CrawlerBehaviorComponent::InitializeBehavior, InitialPatrolDelay, false);

			UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Patrol delayed %.1fs (hidden + movement frozen)"),
				*Owner->GetName(), InitialPatrolDelay);
		}
		else
		{
			InitializeBehavior();
		}
	}
}

void UZP_CrawlerBehaviorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister from static registry
	AllCrawlers.Remove(this);
	UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Unregistered from crawler registry (%d remaining)"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("unknown"), AllCrawlers.Num());

	Super::EndPlay(EndPlayReason);

	// Only clear wall map when the LAST crawler is destroyed.
	// On PIE restart, old crawlers EndPlay runs AFTER new crawlers BeginPlay.
	// If every crawler clears the map, it gets wiped after new crawlers skip Build().
	if (AllCrawlers.Num() == 0)
	{
		FZP_WallMap::Clear();
	}
}

// --- Public API ---

void UZP_CrawlerBehaviorComponent::InitializeBehavior()
{
	AActor* Owner = GetOwner();
	if (!Owner || !CrawlerCMC)
	{
		UE_LOG(LogTemp, Error, TEXT("[ZP_Behavior] InitializeBehavior ABORTED — Owner=%s CrawlerCMC=%s. This crawler has NO behavior loop!"),
			Owner ? *Owner->GetName() : TEXT("NULL"),
			CrawlerCMC ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	// Reveal creature — was hidden during InitialPatrolDelay to prevent visible settling
	Owner->SetActorHiddenInGame(false);

	// MOVE_Flying: our CMC PhysFlying handles ground pursuit with manual gravity.
	CrawlerCMC->SetMovementMode(MOVE_Flying);

	// Record spawn location as home for wandering (used when no patrol points)
	HomeLocation = Owner->GetActorLocation();

	// Floor trace: find actual ground level below spawn point.
	// Prevents spawn-over-void (creature teleport loops when HomeLocation is above water/void).
	{
		FHitResult FloorHit;
		FCollisionQueryParams FloorParams;
		FloorParams.AddIgnoredActor(Owner);
		const FVector TraceStart = HomeLocation;
		const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, 2000.f);

		if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, FloorParams))
		{
			float CapsuleHalfHeight = 90.f;
			if (ACharacter* Character = Cast<ACharacter>(Owner))
			{
				if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
				}
			}
			const float GroundZ = FloorHit.ImpactPoint.Z + CapsuleHalfHeight;

			if (FMath::Abs(HomeLocation.Z - GroundZ) > 50.f)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ZP_Behavior] %s: HomeLocation adjusted — spawn Z=%.0f, ground Z=%.0f"),
					*Owner->GetName(), HomeLocation.Z, GroundZ);
				HomeLocation.Z = GroundZ;
				Owner->SetActorLocation(HomeLocation);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZP_Behavior] %s: NO GROUND found below spawn! Creature may struggle."),
				*Owner->GetName());
		}
	}

	// Find ceiling above spawn — this is the creature's perch/ambush spot.
	{
		FHitResult CeilingHit;
		FCollisionQueryParams CeilParams;
		CeilParams.AddIgnoredActor(Owner);
		const FVector CeilTraceStart = HomeLocation;
		const FVector CeilTraceEnd = HomeLocation + FVector(0.f, 0.f, 3000.f);

		if (GetWorld()->LineTraceSingleByChannel(CeilingHit, CeilTraceStart, CeilTraceEnd, ECC_GameTraceChannel1, CeilParams))
		{
			// Offset down from ceiling so capsule fits
			float CapsuleHH = 90.f;
			if (ACharacter* Char = Cast<ACharacter>(Owner))
			{
				if (UCapsuleComponent* Cap = Char->GetCapsuleComponent())
				{
					CapsuleHH = Cap->GetScaledCapsuleHalfHeight();
				}
			}
			PerchLocation = CeilingHit.ImpactPoint - FVector(0.f, 0.f, CapsuleHH + 10.f);
			UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Ceiling perch found at Z=%.0f (ceiling Z=%.0f)"),
				*Owner->GetName(), PerchLocation.Z, CeilingHit.ImpactPoint.Z);
		}
		else
		{
			// No ceiling — perch at home (ground creature)
			PerchLocation = HomeLocation;
			UE_LOG(LogTemp, Warning, TEXT("[ZP_Behavior] %s: No ceiling found — perching at ground level"),
				*Owner->GetName());
		}
	}

	// Initialize stall tracking
	StallCheckLocation = Owner->GetActorLocation();

	// Start in Patrol state (moves to perch, then waits)
	SetState(ECrawlerState::Patrol);

	// Start eval timer
	GetWorld()->GetTimerManager().SetTimer(
		EvalTimerHandle,
		this,
		&UZP_CrawlerBehaviorComponent::EvaluateBehavior,
		EvalInterval,
		true
	);

	if (PatrolPoints.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Initialized with %d waypoints."),
			*Owner->GetName(), PatrolPoints.Num());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Initialized — wandering within %.0f UU of home."),
			*Owner->GetName(), WanderRadius);
	}
}

void UZP_CrawlerBehaviorComponent::StartHunt(AActor* Target)
{
	if (!Target) return;

	// Ambush predator: StartHunt triggers an attack drop, not a chase.
	ChaseTargetActor = Target;
	if (CurrentState != ECrawlerState::Attack)
	{
		BeginSlamAttack();
	}
}

void UZP_CrawlerBehaviorComponent::ReturnToPatrol()
{
	ChaseTargetActor = nullptr;
	SetState(ECrawlerState::Patrol);
}

// --- State Machine ---

void UZP_CrawlerBehaviorComponent::SetState(ECrawlerState NewState)
{
	if (NewState == CurrentState)
	{
		return;
	}

	const ECrawlerState OldState = CurrentState;

	if (OldState == ECrawlerState::Attack)
	{
		GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
		CurrentAttackType = EAttackType::None;
		bAttackDamageApplied = false;
	}

	CurrentState = NewState;
	StateTimer = 0.f;

	switch (NewState)
	{
	case ECrawlerState::Patrol:
		// Return to ceiling perch
		SetSpeed(PatrolSpeed);
		bSeekingWall = false;
		bReachedPerch = false;
		if (CrawlerCMC)
		{
			CrawlerCMC->bOnCeiling = false;
			CrawlerCMC->bClimbEnabled = true;
			CrawlerCMC->MoveTarget = PerchLocation;
			CrawlerCMC->MoveTargetActor = nullptr;
		}
		break;

	case ECrawlerState::Attack:
		SetSpeed(0.f);
		if (CrawlerCMC)
		{
			CrawlerCMC->bClimbEnabled = false;
			CrawlerCMC->bOnCeiling = false;
			CrawlerCMC->MoveTargetActor = nullptr;
		}
		break;
	}

	UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: %s -> %s"),
		*GetOwner()->GetName(),
		*UEnum::GetValueAsString(OldState),
		*UEnum::GetValueAsString(NewState));

	OnStateChanged.Broadcast(OldState, NewState);
}

void UZP_CrawlerBehaviorComponent::EvaluateBehavior()
{
	AActor* Owner = GetOwner();
	if (!Owner || !CrawlerCMC) return;

	// Death guard
	if (UZP_HealthComponent* HC = Owner->FindComponentByClass<UZP_HealthComponent>())
	{
		if (HC->bIsDead) return;
	}

	const FVector CurrentLocation = Owner->GetActorLocation();

	// --- Player detection: LOS + within range ---
	bool bPlayerInRange = false;

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* PlayerPawn = PC->GetPawn())
		{
			const float Dist = FVector::Dist(CurrentLocation, PlayerPawn->GetActorLocation());
			if (Dist <= DetectionRange)
			{
				FHitResult LOSHit;
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(Owner);

				const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
					LOSHit, CurrentLocation, PlayerPawn->GetActorLocation(),
					ECC_Visibility, QueryParams);
				const bool bClearLOS = !bBlocked || LOSHit.GetActor() == PlayerPawn;

				if (bClearLOS)
				{
					bPlayerInRange = true;
					ChaseTargetActor = PlayerPawn;
				}
			}
		}
	}

	// --- State logic ---
	switch (CurrentState)
	{
	case ECrawlerState::Patrol:
	{
		// PERCH ON CEILING AND WAIT

		if (bReachedPerch || CrawlerCMC->bOnCeiling)
		{
			// On ceiling — check for player
			bReachedPerch = true;
			SetSpeed(0.f);
			CrawlerCMC->bClimbEnabled = false;

			if (bPlayerInRange)
			{
				// Player detected! Drop and attack.
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: PLAYER DETECTED — dropping from ceiling!"),
					*Owner->GetName());
				BeginSlamAttack();
			}
			break;
		}

		if (CrawlerCMC->IsClimbing())
		{
			// Climbing wall toward ceiling — let CMC handle
			CrawlerCMC->MoveTarget = PerchLocation;
			break;
		}

		// On ground — find wall and head toward it
		if (!bSeekingWall)
		{
			FVector WallPoint, WallNormal;
			if (FindNearestWall(WallPoint, WallNormal))
			{
				CrawlerCMC->MoveTarget = WallPoint - WallNormal * 50.f;
				CrawlerCMC->MoveTargetActor = nullptr;
				CrawlerCMC->bClimbEnabled = true;
				bSeekingWall = true;
			}
			else
			{
				bReachedPerch = true;
				SetSpeed(0.f);
			}
		}
		break;
	}

	case ECrawlerState::Attack:
	{
		if (!CrawlerCMC) break;

		// --- Check for slam impact (may complete between eval ticks) ---
		if (CrawlerCMC->HasSlamImpacted())
		{
			CrawlerCMC->ClearSlamImpact();

			if (ChaseTargetActor)
			{
				const float Dist = FVector::Dist(CurrentLocation, ChaseTargetActor->GetActorLocation());
				if (Dist <= SlamDamageRadius)
				{
					ApplyAttackDamage(LungeDamage, 0.f);
					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: SLAM HIT — %.0f damage at %.0f UU"),
						*Owner->GetName(), LungeDamage, Dist);
				}
			}

			LastAttackTime = GetWorld()->GetTimeSeconds();
		}

		// --- Ground chase: follow floor path, slam when close ---
		if (!ChaseTargetActor)
		{
			SetState(ECrawlerState::Patrol);
			break;
		}

		const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
		const float DistToPlayer = FVector::Dist(CurrentLocation, PlayerLoc);

		// Player escaped — return to ceiling
		if (DistToPlayer > DetectionRange * 3.f)
		{
			UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Player escaped — returning to ceiling"),
				*Owner->GetName());
			SetState(ECrawlerState::Patrol);
			break;
		}

		// Stall detection — stuck on obstacle, give up
		StallTimer += EvalInterval;
		if (StallTimer >= 2.0f)
		{
			const float Moved = FVector::Dist2D(CurrentLocation, StallCheckLocation);
			if (Moved < 30.f)
			{
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: STUCK — returning to ceiling"),
					*Owner->GetName());
				SetState(ECrawlerState::Patrol);
				break;
			}
			StallCheckLocation = CurrentLocation;
			StallTimer = 0.f;
		}

		// Close enough to slam?
		const float ZDiff = FMath::Abs(CurrentLocation.Z - PlayerLoc.Z);
		const double Now = GetWorld()->GetTimeSeconds();
		if (ZDiff <= 200.f && DistToPlayer <= AttackRange
			&& (Now - LastAttackTime) >= AttackCooldown)
		{
			// Keep chasing during slam — no stop
			CrawlerCMC->BeginSlam(0.f, DropAttackHoldTime, 0.f);
			break;
		}

		// Chase on ground — no climbing
		CrawlerCMC->bClimbEnabled = false;
		SetSpeed(HuntSpeed);
		CrawlerCMC->MoveTargetActor = ChaseTargetActor;
		break;
	}

	} // end switch
}

// Dead code removed — old stall/threat/hunt/patrol eval logic.
// Replaced by clean 2-state ambush predator above.

#if 0 // === DEAD CODE START — kept for reference, will not compile ===
static void DeadCode_OldEvalBehavior()
		{
			UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: STALL detected (moved %.1f UU in %.1fs) — re-evaluating target."),
				*Owner->GetName(), Moved, StallTimeout);

			const bool bBelowHome = CurrentLocation.Z < HomeLocation.Z - 100.f;

			switch (CurrentState)
			{
			case ECrawlerState::Patrol:
				if (bBelowHome && CrawlerCMC)
				{
					// Below home during patrol — navigate toward home to escape water/pits
					CrawlerCMC->MoveTarget = HomeLocation;
					CrawlerCMC->MoveTargetActor = nullptr;
					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Patrol STALL below home (Z=%.0f vs Home=%.0f) — targeting home"),
						*Owner->GetName(), CurrentLocation.Z, HomeLocation.Z);
				}
				else if (PatrolPoints.Num() > 0)
				{
					CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
					AdvanceToNextWaypoint();
				}
				else if (CrawlerCMC)
				{
					// Pick new random wander target
					const FVector2D RandCircle = FMath::RandPointInCircle(WanderRadius);
					const FVector WanderTarget = HomeLocation + FVector(RandCircle.X, RandCircle.Y, 0.f);
					CrawlerCMC->MoveTarget = WanderTarget;
					CrawlerCMC->MoveTargetActor = nullptr;
				}
				break;
			case ECrawlerState::Hunt:
				{
					if (ChaseTargetActor && CrawlerCMC)
					{
						const FVector TargetLoc = ChaseTargetActor->GetActorLocation();
						const float ZDiff = TargetLoc.Z - CurrentLocation.Z;

						if (ZDiff > 200.f)
						{
							// Target is above — find a wall to climb instead of lateral oscillation.
							const FClimbableWall* BestWall = FZP_WallMap::FindBestToward(
								CurrentLocation, TargetLoc, 3000.f);

							if (BestWall)
							{
								CrawlerCMC->MoveTarget = BestWall->Location;
								CrawlerCMC->MoveTargetActor = nullptr;
								CrawlerCMC->bClimbEnabled = true;

								FTimerHandle WallRetargetHandle;
								TWeakObjectPtr<AActor> WeakTarget = ChaseTargetActor;
								TWeakObjectPtr<UZP_CrawlerMovementComponent> WeakCMC = CrawlerCMC;
								GetWorld()->GetTimerManager().SetTimer(WallRetargetHandle,
									[WeakTarget, WeakCMC]()
									{
										if (WeakCMC.IsValid() && WeakTarget.IsValid())
										{
											WeakCMC->MoveTargetActor = WeakTarget;
										}
									},
									1.5f, false);

								UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Hunt STALL — target above (Z+%.0f), seeking wall at (%.0f, %.0f)"),
									*Owner->GetName(), ZDiff, BestWall->Location.X, BestWall->Location.Y);
							}
							else
							{
								// No wall found — lateral break
								const FVector ToPlayer = (TargetLoc - CurrentLocation).GetSafeNormal2D();
								const FVector Lateral = FVector::CrossProduct(ToPlayer, FVector::UpVector).GetSafeNormal();
								const float Side = FMath::RandBool() ? 1.f : -1.f;
								CrawlerCMC->MoveTarget = CurrentLocation + Lateral * Side * 400.f + ToPlayer * 200.f;
								CrawlerCMC->MoveTargetActor = nullptr;

								FTimerHandle RetargetHandle;
								TWeakObjectPtr<AActor> WeakTarget = ChaseTargetActor;
								TWeakObjectPtr<UZP_CrawlerMovementComponent> WeakCMC = CrawlerCMC;
								GetWorld()->GetTimerManager().SetTimer(RetargetHandle,
									[WeakTarget, WeakCMC]()
									{
										if (WeakCMC.IsValid() && WeakTarget.IsValid())
										{
											WeakCMC->MoveTargetActor = WeakTarget;
										}
									},
									0.5f, false);

								UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Hunt STALL — target above (Z+%.0f) but no wall, lateral break"),
									*Owner->GetName(), ZDiff);
							}
						}
						else
						{
							// Standard lateral break for same-height stalls
							const FVector ToPlayer = (TargetLoc - CurrentLocation).GetSafeNormal2D();
							const FVector Lateral = FVector::CrossProduct(ToPlayer, FVector::UpVector).GetSafeNormal();
							const float Side = FMath::RandBool() ? 1.f : -1.f;
							CrawlerCMC->MoveTarget = CurrentLocation + Lateral * Side * 400.f + ToPlayer * 200.f;
							CrawlerCMC->MoveTargetActor = nullptr;

							FTimerHandle RetargetHandle;
							TWeakObjectPtr<AActor> WeakTarget = ChaseTargetActor;
							TWeakObjectPtr<UZP_CrawlerMovementComponent> WeakCMC = CrawlerCMC;
							GetWorld()->GetTimerManager().SetTimer(RetargetHandle,
								[WeakTarget, WeakCMC]()
								{
									if (WeakCMC.IsValid() && WeakTarget.IsValid())
									{
										WeakCMC->MoveTargetActor = WeakTarget;
									}
								},
								0.5f, false);

							UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Hunt STALL — lateral break, re-lock player in 0.5s"),
								*Owner->GetName());
						}
					}
				}
				break;
			default:
				break;
			}
		}
		StallCheckLocation = CurrentLocation;
		StallTimer = 0.f;
	}

	// Increment last-seen timer when not seeing player
	if (CurrentState == ECrawlerState::Hunt)
	{
		LastSeenTimer += EvalInterval;
	}

	// --- Direct player detection + threat accumulation ---
	{
		bool bPlayerVisibleThisTick = false;
		float DistToPlayerThisTick = MAX_FLT;

		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				DistToPlayerThisTick = GetEffectiveDistance(CurrentLocation, PlayerPawn->GetActorLocation());

				if (DistToPlayerThisTick <= DetectionRange)
				{
					// LOS check
					FHitResult LOSHit;
					FCollisionQueryParams QueryParams;
					QueryParams.AddIgnoredActor(Owner);

					const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
						LOSHit, CurrentLocation, PlayerPawn->GetActorLocation(),
						ECC_Visibility, QueryParams);
					const bool bClearLOS = !bBlocked || LOSHit.GetActor() == PlayerPawn;

					if (bClearLOS)
					{
						bPlayerVisibleThisTick = true;

						if (CurrentState == ECrawlerState::Patrol)
						{
							UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: DETECTED player at %.0f UU (direct check)"),
								*Owner->GetName(), DistToPlayerThisTick);
							StartHunt(PlayerPawn);
						}
					}
				}
			}
		}

		// --- Threat accumulation (Patrol only — Hunt already committed) ---
		if (CurrentState == ECrawlerState::Patrol)
		{
			if (bPlayerVisibleThisTick)
			{
				// Closer = faster threat buildup: 1x at max range, 3x at point blank
				const float DistFactor = 1.0f - FMath::Clamp(DistToPlayerThisTick / DetectionRange, 0.f, 1.f);
				const float ThreatGain = ThreatPerSecondLOS * (1.0f + DistFactor * 2.0f) * EvalInterval;
				ThreatLevel = FMath::Min(ThreatLevel + ThreatGain, ThreatThreshold * 1.5f);
			}
			else
			{
				// Decay when hidden — but slowly, tension doesn't evaporate instantly
				ThreatLevel = FMath::Max(ThreatLevel - ThreatDecayRate * EvalInterval, 0.f);
			}

			// Threshold crossed -> forced Hunt
			if (ThreatLevel >= ThreatThreshold)
			{
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: THREAT AGGRO — threat %.0f hit threshold %.0f!"),
					*Owner->GetName(), ThreatLevel, ThreatThreshold);

				ThreatLevel = 0.f; // Reset after triggering

				if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
				{
					if (APawn* PlayerPawn = PC->GetPawn())
					{
						ChaseTargetActor = PlayerPawn;
						LastKnownPlayerLocation = PlayerPawn->GetActorLocation();
						SetState(ECrawlerState::Hunt);
					}
				}
			}
		}
		else
		{
			// In Hunt/Attack: reset threat (already committed)
			ThreatLevel = 0.f;
		}
	}

	// State-specific evaluation
	switch (CurrentState)
	{
	case ECrawlerState::Patrol:
	{
		// AMBUSH BEHAVIOR: find wall → climb to ceiling → perch and wait.
		if (!CrawlerCMC) break;

		if (bReachedPerch || CrawlerCMC->bOnCeiling)
		{
			// On ceiling — stop and wait for prey.
			bReachedPerch = true;
			SetSpeed(0.f);
			CrawlerCMC->bClimbEnabled = false;
			break;
		}

		if (CrawlerCMC->IsClimbing())
		{
			// Currently climbing a wall toward ceiling — let CMC handle it.
			// Target is the ceiling perch point (set when we started climbing).
			break;
		}

		// On ground — find nearest wall and head toward it
		if (!bSeekingWall)
		{
			FVector WallPoint, WallNormal;
			if (FindNearestWall(WallPoint, WallNormal))
			{
				// Target: slightly past the wall so we collide with it
				const FVector WallTarget = WallPoint - WallNormal * 50.f;
				CrawlerCMC->MoveTarget = WallTarget;
				CrawlerCMC->MoveTargetActor = nullptr;
				CrawlerCMC->bClimbEnabled = true;
				bSeekingWall = true;

				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Heading to wall for ceiling climb"),
					*Owner->GetName());
			}
			else
			{
				// No wall found — just sit at home
				bReachedPerch = true;
				SetSpeed(0.f);
				UE_LOG(LogTemp, Warning, TEXT("[ZP_Behavior] %s: No wall found — perching at ground"),
					*Owner->GetName());
			}
		}
		else
		{
			// Approaching wall — once we start climbing, target the ceiling
			if (CrawlerCMC->IsClimbing())
			{
				CrawlerCMC->MoveTarget = PerchLocation;
				bSeekingWall = false;
			}
		}
		break;
	}

	case ECrawlerState::Hunt:
	{
		// --- Post-crest: brief pause at fence/wall top ---
		if (CrawlerCMC && CrawlerCMC->bJustCrested)
		{
			CrawlerCMC->bJustCrested = false;

			// Brief 0.5s pause at fence top, then resume
			SetSpeed(0.f);
			bIsPaused = true;

			FTimerHandle CrestPauseHandle;
			TWeakObjectPtr<UZP_CrawlerBehaviorComponent> WeakThis(this);
			GetWorld()->GetTimerManager().SetTimer(CrestPauseHandle,
				[WeakThis]()
				{
					if (WeakThis.IsValid() && WeakThis->CurrentState == ECrawlerState::Hunt)
					{
						WeakThis->bIsPaused = false;
						WeakThis->SetSpeed(WeakThis->HuntSpeed);
						if (WeakThis->CrawlerCMC && WeakThis->ChaseTargetActor)
						{
							WeakThis->CrawlerCMC->MoveTargetActor = WeakThis->ChaseTargetActor;
						}
					}
				},
				0.5f, false);

			UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: POST-CREST PAUSE 0.5s at fence top"),
				*Owner->GetName());
			break;
		}

		// --- Ground attack check: close enough -> slam attack ---
		if (ChaseTargetActor && CrawlerCMC && !CrawlerCMC->IsSlamming())
		{
			const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
			const float HorizDist = FVector::Dist2D(CurrentLocation, PlayerLoc);
			const float ZDiff = FMath::Abs(CurrentLocation.Z - PlayerLoc.Z);
			const double AttackNow = GetWorld()->GetTimeSeconds();

			if (ZDiff <= 200.f && HorizDist <= AttackRange && (AttackNow - LastAttackTime) >= AttackCooldown)
			{
				bBeingObserved = false; // Break freeze for attack
				BeginSlamAttack();
				break;
			}
		}

		// --- Gaze behavior: WALL = freeze when watched (long range only). GROUND = aggressive charge. ---
		{
			const bool bOnWall = CrawlerCMC && CrawlerCMC->IsClimbing();
			const bool bLookedAt = IsPlayerLookingAtCreature();
			const float DistToPlayer = ChaseTargetActor
				? FVector::Dist(CurrentLocation, ChaseTargetActor->GetActorLocation()) : 0.f;
			const bool bFarEnoughToFreeze = DistToPlayer >= GazeFreezeMinDistance;

			if (bOnWall && bLookedAt && bFarEnoughToFreeze)
			{
				// ON WALL + being watched at distance → freeze ("is that... something?")
				if (!bBeingObserved)
				{
					bBeingObserved = true;
					ObservedTimer = 0.f;
					SetSpeed(0.f);
					if (CrawlerCMC)
					{
						CrawlerCMC->MoveTargetActor = nullptr;
					}
					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: ON WALL — player watching — FREEZE."), *Owner->GetName());
				}
				break; // Skip all other Hunt logic while frozen on wall
			}
			else if (bBeingObserved && (!bOnWall || !bLookedAt))
			{
				// Was frozen, now either off wall or player looked away — resume
				bBeingObserved = false;
				ObservedTimer = 0.f;
				SetSpeed(HuntSpeed);
				if (CrawlerCMC && ChaseTargetActor)
				{
					CrawlerCMC->MoveTargetActor = ChaseTargetActor;
				}
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Unfreeze — resuming hunt."), *Owner->GetName());
			}
		}

		// --- Hearing ---
		if (ChaseTargetActor)
		{
			const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
			const float DistToPlayer = GetEffectiveDistance(CurrentLocation, PlayerLoc);

			// Hearing: within range, creature tracks player even without visual LOS
			if (DistToPlayer <= HearingRange)
			{
				LastKnownPlayerLocation = PlayerLoc;
				LastSeenTimer = 0.f;
			}
		}

		// (Attack check moved above gaze freeze — attacks bypass freeze at close range)

		// --- Immediate wall-seek when target is far above ---
		// Without this, the creature does ground pursuit toward the player's XY (nearly overhead),
		// overshoots, turns, overshoots → fidget spinner. Seeking a wall immediately breaks the loop.
		if (ChaseTargetActor && CrawlerCMC && !CrawlerCMC->IsClimbing() && !bSeekingWall)
		{
			const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
			const float ZAbove = PlayerLoc.Z - CurrentLocation.Z;

			if (ZAbove > 200.f)
			{
				const FClimbableWall* BestWall = FZP_WallMap::FindBestToward(
					CurrentLocation, PlayerLoc, 3000.f);

				if (BestWall)
				{
					CrawlerCMC->MoveTarget = BestWall->Location;
					CrawlerCMC->MoveTargetActor = nullptr;
					CrawlerCMC->bClimbEnabled = true;
					bSeekingWall = true;

					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Hunt WALL-SEEK — target %.0f UU above, heading to wall at (%.0f, %.0f)"),
						*Owner->GetName(), ZAbove, BestWall->Location.X, BestWall->Location.Y);
				}
			}
		}

		// Clear wall-seek flag once climbing has engaged (wall reached, now climbing it)
		if (bSeekingWall && CrawlerCMC && CrawlerCMC->IsClimbing())
		{
			bSeekingWall = false;
			// Re-lock player target — creature is on the wall, climbing toward player
			if (ChaseTargetActor)
			{
				CrawlerCMC->MoveTargetActor = ChaseTargetActor;
			}
		}

		// --- Throttled retargeting ---
		if (ChaseTargetActor && !bSeekingWall)
		{
			const double Now = GetWorld()->GetTimeSeconds();
			if (Now - LastRetargetTime >= RetargetInterval)
			{
				const float CurrentDist = FVector::Dist(CurrentLocation, ChaseTargetActor->GetActorLocation());

				// Only retarget if NOT making progress (distance didn't close by threshold)
				if (CurrentDist >= DistanceAtLastRetarget * RetargetProgressThreshold)
				{
					// Re-lock onto player (MoveTargetActor tracks every frame)
					CrawlerCMC->MoveTargetActor = ChaseTargetActor;

					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Hunt retarget — dist %.0f (was %.0f, threshold %.0f)"),
						*Owner->GetName(), CurrentDist, DistanceAtLastRetarget,
						DistanceAtLastRetarget * RetargetProgressThreshold);
				}

				DistanceAtLastRetarget = CurrentDist;
				LastRetargetTime = Now;
			}
		}

		// If haven't seen player for HuntLoseSightTime, chase to last known position
		if (LastSeenTimer >= HuntLoseSightTime)
		{
			const float DistToLastKnown = FVector::Dist(CurrentLocation, LastKnownPlayerLocation);
			if (DistToLastKnown <= ArrivalThreshold)
			{
				// Reached last known position, player not found — give up
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Reached last known pos, player gone — returning to patrol."),
					*Owner->GetName());
				SetState(ECrawlerState::Patrol);
			}
			else
			{
				// Still en route to last known location — direct pursuit
				CrawlerCMC->MoveTarget = LastKnownPlayerLocation;
				CrawlerCMC->MoveTargetActor = nullptr;
			}
		}

		// If player is beyond HuntMaxRange, give up pursuit
		if (ChaseTargetActor)
		{
			const float DistToPlayer = FVector::Dist(CurrentLocation, ChaseTargetActor->GetActorLocation());
			if (DistToPlayer > HuntMaxRange)
			{
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Player at %.0f UU — beyond hunt range (%.0f), returning to patrol."),
					*Owner->GetName(), DistToPlayer, HuntMaxRange);
				SetState(ECrawlerState::Patrol);
			}
		}
		break;
	}

	case ECrawlerState::Attack:
	{
		AttackPhaseTimer += EvalInterval;

		// Slam attack: CMC handles hold/impact, we wait for it
		if (CrawlerCMC && CrawlerCMC->HasSlamImpacted())
		{
			CrawlerCMC->ClearSlamImpact();

			if (ChaseTargetActor)
			{
				const float Dist = FVector::Dist(CurrentLocation, ChaseTargetActor->GetActorLocation());
				if (Dist <= SlamDamageRadius)
				{
					ApplyAttackDamage(LungeDamage, 0.f);
					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: SLAM HIT — %.0f damage at %.0f UU"),
						*Owner->GetName(), LungeDamage, Dist);
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: SLAM MISSED — player at %.0f UU"),
						*Owner->GetName(), Dist);
				}
			}

			LastAttackTime = GetWorld()->GetTimeSeconds();
			SetState(ECrawlerState::Hunt);
		}

		// Safety timeout
		if (AttackPhaseTimer > 3.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ZP_Behavior] %s: SLAM TIMEOUT — forcing Hunt"),
				*Owner->GetName());
			LastAttackTime = GetWorld()->GetTimeSeconds();
			SetState(ECrawlerState::Hunt);
		}
		break;
	}

	}
}
#endif // === DEAD CODE END ===

// --- Perception ---

void UZP_CrawlerBehaviorComponent::OnSeePawn(APawn* SeenPawn)
{
	if (!SeenPawn)
	{
		return;
	}

	LastKnownPlayerLocation = SeenPawn->GetActorLocation();

	// Ambush predator: seeing player triggers attack if perched
	StartHunt(SeenPawn);
}

// --- Helpers ---

void UZP_CrawlerBehaviorComponent::SetSpeed(float Speed)
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
		{
			CMC->MaxWalkSpeed = Speed;
			CMC->MaxFlySpeed = Speed;
		}
	}
}

void UZP_CrawlerBehaviorComponent::AdvanceToNextWaypoint()
{
	if (CrawlerCMC && PatrolPoints.IsValidIndex(CurrentPatrolIndex))
	{
		CrawlerCMC->MoveTarget = PatrolPoints[CurrentPatrolIndex];
		CrawlerCMC->MoveTargetActor = nullptr;

		UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Targeting waypoint %d at %s"),
			*GetOwner()->GetName(), CurrentPatrolIndex,
			*PatrolPoints[CurrentPatrolIndex].ToString());
	}
}

void UZP_CrawlerBehaviorComponent::UpdateIndoorStatus()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	const FVector Start = Owner->GetActorLocation();
	const FVector End = Start + FVector(0.f, 0.f, IndoorTraceDistance);

	bIsIndoors = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_GameTraceChannel1, Params);
}

float UZP_CrawlerBehaviorComponent::GetEffectiveDistance(const FVector& From, const FVector& To) const
{
	if (bIsIndoors)
	{
		return FVector::Dist(From, To);
	}
	return FVector::Dist2D(From, To);
}

bool UZP_CrawlerBehaviorComponent::FindNearestWall(FVector& OutWallPoint, FVector& OutWallNormal) const
{
	AActor* Owner = GetOwner();
	if (!Owner) return false;

	// Query the pre-built wall map instead of doing live traces.
	// The map was built at level start and contains every climbable surface
	// with its position, normal, and height.
	const FVector Origin = Owner->GetActorLocation();

	if (FZP_WallMap::IsBuilt())
	{
		// If we have a chase target, find walls on the path toward them
		const FClimbableWall* Wall = nullptr;
		if (ChaseTargetActor)
		{
			Wall = FZP_WallMap::FindBestToward(Origin, ChaseTargetActor->GetActorLocation(), WallSeekRadius);
		}

		// Fallback to nearest wall
		if (!Wall)
		{
			Wall = FZP_WallMap::FindNearest(Origin, WallSeekRadius);
		}

		if (Wall)
		{
			OutWallPoint = Wall->Location;
			OutWallNormal = Wall->Normal;
			return true;
		}
	}

	// Fallback: live radial traces if wall map not available
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	float BestDist = MAX_FLT;
	bool bFound = false;

	const float AngleStep = 360.f / (float)WallSeekTraceCount;
	for (int32 i = 0; i < WallSeekTraceCount; ++i)
	{
		const float Angle = AngleStep * i;
		const FVector Dir = FVector(
			FMath::Cos(FMath::DegreesToRadians(Angle)),
			FMath::Sin(FMath::DegreesToRadians(Angle)),
			0.f
		);
		const FVector TraceEnd = Origin + Dir * WallSeekRadius;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, TraceEnd, ECC_GameTraceChannel1, Params))
		{
			if (FMath::Abs(Hit.ImpactNormal.Z) < 0.5f)
			{
				const float Dist = Hit.Distance;
				if (Dist < BestDist && Dist > 300.f)
				{
					BestDist = Dist;
					OutWallPoint = Hit.ImpactPoint;
					OutWallNormal = Hit.ImpactNormal;
					bFound = true;
				}
			}
		}
	}

	return bFound;
}

bool UZP_CrawlerBehaviorComponent::IsOnGround() const
{
	if (!CrawlerCMC)
	{
		return true;
	}
	return !CrawlerCMC->IsClimbing() && !CrawlerCMC->IsLaunching() && !CrawlerCMC->IsSlamming();
}

// --- Gunshot Alert System ---

void UZP_CrawlerBehaviorComponent::OnHearGunshot(const FVector& NoiseLocation, AActor* Shooter)
{
	// NOTE: Do NOT check IsActive() here — components may report inactive even when
	// their eval timer is running and behavior is functional. The dead check below
	// is sufficient to filter out non-responsive creatures.

	// Dead creatures don't react
	if (UZP_HealthComponent* HC = GetOwner()->FindComponentByClass<UZP_HealthComponent>())
	{
		if (HC->bIsDead)
		{
			return;
		}
	}

	// Don't interrupt an active attack
	if (CurrentState == ECrawlerState::Attack)
	{
		return;
	}

	// Ambush predator: gunshot triggers attack drop if perched
	ChaseTargetActor = Shooter;
	LastKnownPlayerLocation = NoiseLocation;

	if (CurrentState == ECrawlerState::Patrol && bReachedPerch)
	{
		UE_LOG(LogTemp, Log, TEXT("[Crawler] %s: GUNSHOT HEARD — dropping from perch to attack!"),
			*GetOwner()->GetName());
		BeginSlamAttack();
	}
}

void UZP_CrawlerBehaviorComponent::BroadcastGunshot(UWorld* World, const FVector& Location, float Radius, AActor* Shooter)
{
	if (!World)
	{
		return;
	}

	// Diagnostic: registry size at broadcast time
	UE_LOG(LogTemp, Warning, TEXT("[Crawler] GUNSHOT: Registry has %d entries. Shooter=%s"),
		AllCrawlers.Num(), Shooter ? *Shooter->GetName() : TEXT("unknown"));

	int32 AlertedCount = 0;
	int32 SkippedCount = 0;

	// Use static registry — bypasses FindComponentByClass which fails due to
	// CoreRedirect class rename (ZP_Creature* → ZP_Crawler*) not resolving at runtime.
	for (int32 i = AllCrawlers.Num() - 1; i >= 0; --i)
	{
		UZP_CrawlerBehaviorComponent* Crawler = AllCrawlers[i];
		if (!Crawler || !IsValid(Crawler))
		{
			// Stale entry — remove it
			AllCrawlers.RemoveAt(i);
			SkippedCount++;
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("[Crawler] GUNSHOT: Alerting %s (owner=%s, active=%d)"),
			*Crawler->GetName(),
			Crawler->GetOwner() ? *Crawler->GetOwner()->GetName() : TEXT("NO_OWNER"),
			Crawler->IsActive() ? 1 : 0);

		Crawler->OnHearGunshot(Location, Shooter);
		AlertedCount++;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Crawler] GUNSHOT RESULT: %d alerted, %d stale removed, %d remaining in registry"),
		AlertedCount, SkippedCount, AllCrawlers.Num());
}

bool UZP_CrawlerBehaviorComponent::IsPlayerLookingAtCreature() const
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return false;

	FVector CameraLoc;
	FRotator CameraRot;
	PC->GetPlayerViewPoint(CameraLoc, CameraRot);

	const FVector ToCreature = (GetOwner()->GetActorLocation() - CameraLoc).GetSafeNormal();
	const float Dot = FVector::DotProduct(CameraRot.Vector(), ToCreature);
	if (Dot < GazeThreshold) return false;

	// LOS check — creature doesn't react to observation through walls
	FHitResult LOSHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		LOSHit, CameraLoc, GetOwner()->GetActorLocation(), ECC_Visibility, QueryParams);
	return !bBlocked || LOSHit.GetActor() == GetOwner();
}

// --- Attack System ---

void UZP_CrawlerBehaviorComponent::BeginSlamAttack()
{
	CurrentAttackType = EAttackType::Slam;
	bAttackDamageApplied = false;
	AttackPhaseTimer = 0.f;
	SetState(ECrawlerState::Attack);
	// No SetSpeed(0) — creature keeps moving during slam

	if (CrawlerCMC)
	{
		CrawlerCMC->BeginSlam(0.f, DropAttackHoldTime, 0.f);
	}

	UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: SLAM ATTACK"),
		*GetOwner()->GetName());
}

void UZP_CrawlerBehaviorComponent::ApplyAttackDamage(float Damage, float Radius)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Death guard — never deal damage after death
	if (UZP_HealthComponent* HC = Owner->FindComponentByClass<UZP_HealthComponent>())
	{
		if (HC->bIsDead)
		{
			return;
		}
	}

	AController* CreatureController = nullptr;
	if (APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		CreatureController = OwnerPawn->GetController();
	}

	if (Radius > 0.f)
	{
		// AoE damage
		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(Owner);

		UGameplayStatics::ApplyRadialDamage(
			GetWorld(),
			Damage,
			Owner->GetActorLocation(),
			Radius,
			nullptr, // DamageTypeClass
			IgnoreActors,
			Owner, // DamageCauser
			CreatureController,
			true // bDoFullDamage — no falloff
		);
	}
	else if (ChaseTargetActor)
	{
		// Single target damage (lunge / drop / pounce)
		UGameplayStatics::ApplyDamage(
			ChaseTargetActor,
			Damage,
			CreatureController,
			Owner,
			nullptr // DamageTypeClass
		);
	}
}
