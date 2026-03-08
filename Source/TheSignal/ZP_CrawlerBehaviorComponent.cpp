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
	PrimaryComponentTick.bCanEverTick = false;
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

	// Ensure Flying movement mode — our CMC only works in PhysFlying
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

	// Initialize stall tracking
	StallCheckLocation = Owner->GetActorLocation();

	// Start in Patrol state
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
	if (!Target)
	{
		return;
	}

	ChaseTargetActor = Target;
	LastKnownPlayerLocation = Target->GetActorLocation();
	LastSeenTimer = 0.f;

	// No Alert delay — creature sees you, comes for you. Period.
	if (CurrentState != ECrawlerState::Hunt && CurrentState != ECrawlerState::Attack)
	{
		SetState(ECrawlerState::Hunt);
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

	// Clean up attack state when leaving Attack
	if (OldState == ECrawlerState::Attack)
	{
		GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
		CurrentAttackType = EAttackType::None;
		bAttackDamageApplied = false;
	}

	CurrentState = NewState;
	StateTimer = 0.f;
	bIsPaused = false;
	bPerching = false;
	PerchTimer = 0.f;
	bBeingObserved = false;
	ObservedTimer = 0.f;

	// Clear any waypoint pause timer on state change
	GetWorld()->GetTimerManager().ClearTimer(WaypointPauseTimerHandle);

	switch (NewState)
	{
	case ECrawlerState::Patrol:
		SetSpeed(PatrolSpeed);
		bSeekingWall = false;
		if (PatrolPoints.Num() > 0)
		{
			AdvanceToNextWaypoint();
		}
		else if (CrawlerCMC)
		{
			// Start with ground wander — wall climbing happens via 30% roll in eval tick.
			// No immediate wall-seek on patrol entry. Ground movement is primary.
			const FVector2D RandCircle = FMath::RandPointInCircle(WanderRadius);
			const FVector WanderTarget = HomeLocation + FVector(RandCircle.X, RandCircle.Y, 0.f);
			CrawlerCMC->MoveTarget = WanderTarget;
			CrawlerCMC->MoveTargetActor = nullptr;
		}
		break;

	case ECrawlerState::Hunt:
		SetSpeed(HuntSpeed);
		LastSeenTimer = 0.f;
		// Force immediate retarget on Hunt entry
		LastRetargetTime = 0.0;
		DistanceAtLastRetarget = MAX_FLT;
		if (CrawlerCMC && ChaseTargetActor)
		{
			CrawlerCMC->MoveTargetActor = ChaseTargetActor;
		}
		break;

	case ECrawlerState::Stalk:
		SetSpeed(StalkSpeed);
		if (CrawlerCMC)
		{
			if (IsOnGround())
			{
				// On ground — find a wall first, then climb to elevation
				FVector WallPoint, WallNormal;
				if (FindNearestWall(WallPoint, WallNormal))
				{
					CrawlerCMC->MoveTarget = WallPoint - WallNormal * 100.f;
					CrawlerCMC->MoveTargetActor = nullptr;
				}
				else
				{
					// No walls — target elevated position as fallback
					FVector StalkTarget = LastKnownPlayerLocation + FVector(0.f, 0.f, StalkVerticalBias);
					StalkTarget.Z = FMath::Min(StalkTarget.Z, LastKnownPlayerLocation.Z + MaxStalkHeight);
					CrawlerCMC->MoveTarget = StalkTarget;
					CrawlerCMC->MoveTargetActor = nullptr;
				}
			}
			else
			{
				// Already climbing/elevated — target elevated position
				FVector StalkTarget = LastKnownPlayerLocation + FVector(0.f, 0.f, StalkVerticalBias);
				StalkTarget.Z = FMath::Min(StalkTarget.Z, LastKnownPlayerLocation.Z + MaxStalkHeight);
				CrawlerCMC->MoveTarget = StalkTarget;
				CrawlerCMC->MoveTargetActor = nullptr;
			}
		}
		break;

	case ECrawlerState::Attack:
		SetSpeed(0.f);
		if (CrawlerCMC)
		{
			CrawlerCMC->MoveTargetActor = nullptr;
		}
		break;
	}

	// Floor probe: only active during Patrol to avoid casual wandering into water/void.
	// Hunt/Stalk creatures charge aggressively and don't care about edges.
	if (CrawlerCMC)
	{
		CrawlerCMC->bFloorProbeActive = (NewState == ECrawlerState::Patrol);

		// Climb control: only allow climbing when behavior explicitly wants it.
		// Patrol: disabled (ground wander primary). Hunt/Stalk: enabled (walls as shortcuts/territory).
		// Attack: disabled (stay on ground and fight).
		CrawlerCMC->bClimbEnabled = (NewState == ECrawlerState::Hunt || NewState == ECrawlerState::Stalk);
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
	if (!Owner || !CrawlerCMC)
	{
		return;
	}

	StateTimer += EvalInterval;

	// --- Update indoor/outdoor status each eval tick ---
	UpdateIndoorStatus();

	// --- Pounce post-landing damage check (any state) ---
	if (bPounceInFlight && CrawlerCMC && !CrawlerCMC->IsLaunching())
	{
		bPounceInFlight = false;

		if (ChaseTargetActor)
		{
			const float Dist = FVector::Dist(Owner->GetActorLocation(), ChaseTargetActor->GetActorLocation());
			if (Dist <= LungeDamageRadius)
			{
				ApplyAttackDamage(PounceDamage, 0.f);
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: POUNCE HIT — %.0f damage at %.0f UU"),
					*Owner->GetName(), PounceDamage, Dist);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: POUNCE MISSED — player at %.0f UU (need %.0f)"),
					*Owner->GetName(), Dist, LungeDamageRadius);
			}
		}
	}

	// Stall detection — if creature hasn't moved enough, handle it
	const FVector CurrentLocation = Owner->GetActorLocation();
	StallTimer += EvalInterval;
	if (StallTimer >= StallTimeout)
	{
		// Dist2D: ignore vertical movement so falling creatures still stall-detect
		const float Moved = FVector::Dist2D(CurrentLocation, StallCheckLocation);
		if (Moved < StallDistance && !bPerching && !bBeingObserved
			&& !(CrawlerCMC && CrawlerCMC->IsClimbing()))
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
			case ECrawlerState::Stalk:
				{
					// Stall during stalk — retreat away from last known and climb
					FVector AwayDir = (CurrentLocation - LastKnownPlayerLocation).GetSafeNormal2D();
					if (AwayDir.IsNearlyZero())
					{
						AwayDir = Owner->GetActorForwardVector().GetSafeNormal2D();
					}
					FVector StallTarget = CurrentLocation + AwayDir * 600.f + FVector(0.f, 0.f, StalkVerticalBias);
					// Cap stalk height to prevent vertical spiraling
					StallTarget.Z = FMath::Min(StallTarget.Z, LastKnownPlayerLocation.Z + MaxStalkHeight);
					CrawlerCMC->MoveTarget = StallTarget;
					CrawlerCMC->MoveTargetActor = nullptr;
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
						else if (CurrentState == ECrawlerState::Stalk)
						{
							ChaseTargetActor = PlayerPawn;
							LastKnownPlayerLocation = PlayerPawn->GetActorLocation();
							LastSeenTimer = 0.f;
						}
					}
				}
			}
		}

		// --- Threat accumulation (Patrol and Stalk only — Hunt already committed) ---
		if (CurrentState == ECrawlerState::Patrol || CurrentState == ECrawlerState::Stalk)
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
		if (bIsPaused)
		{
			return; // Waiting at waypoint/wander point, timer will advance us
		}

		if (PatrolPoints.Num() > 0)
		{
			// --- Waypoint patrol (existing behavior) ---
			const float DistToWaypoint = FVector::Dist2D(CurrentLocation, PatrolPoints[CurrentPatrolIndex]);
			if (DistToWaypoint <= ArrivalThreshold)
			{
				bIsPaused = true;
				const float PauseDuration = FMath::RandRange(WaypointPauseMin, WaypointPauseMax);

				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Arrived at waypoint %d, pausing %.1fs."),
					*Owner->GetName(), CurrentPatrolIndex, PauseDuration);

				GetWorld()->GetTimerManager().SetTimer(
					WaypointPauseTimerHandle,
					[this]()
					{
						bIsPaused = false;
						CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
						AdvanceToNextWaypoint();
					},
					PauseDuration,
					false
				);
			}
		}
		else
		{
			// --- Wall crawler patrol: seek walls, climb them, repeat ---
			const float HeightAboveHome = CurrentLocation.Z - HomeLocation.Z;

			if (HeightAboveHome > MaxStalkHeight)
			{
				// Too high — target ground to come back down, disable climbing
				bSeekingWall = false;
				CrawlerCMC->bClimbEnabled = false;
				CrawlerCMC->MoveTarget = HomeLocation;
				CrawlerCMC->MoveTargetActor = nullptr;

				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: PATROL HEIGHT CAP — %.0f UU above home, returning to ground"),
					*Owner->GetName(), HeightAboveHome);
			}
			else if (CrawlerCMC && CrawlerCMC->IsClimbing())
			{
				// Currently climbing — let CMC handle it
				bSeekingWall = false;
			}
			else if (!bSeekingWall || FVector::Dist2D(CurrentLocation, CrawlerCMC->MoveTarget) <= ArrivalThreshold)
			{
				// Arrived at destination — pause, then pick next action.
				// Ground wander is primary (70%). Wall seek is occasional (30%).
				bSeekingWall = false;
				if (CrawlerCMC) { CrawlerCMC->bClimbEnabled = false; } // Done with wall, back to ground
				bIsPaused = true;
				const float PauseDuration = FMath::RandRange(WaypointPauseMin, WaypointPauseMax);

				GetWorld()->GetTimerManager().SetTimer(
					WaypointPauseTimerHandle,
					[this]()
					{
						bIsPaused = false;

						// 30% chance: seek a wall to climb
						if (FMath::FRand() < 0.3f)
						{
							FVector WallPoint, WallNormal;
							if (FindNearestWall(WallPoint, WallNormal))
							{
								const FVector PastWall = WallPoint - WallNormal * 100.f;
								CrawlerCMC->MoveTarget = PastWall;
								CrawlerCMC->MoveTargetActor = nullptr;
								bSeekingWall = true;
								CrawlerCMC->bClimbEnabled = true; // Enable climbing for wall-seek

								UE_LOG(LogTemp, Log, TEXT("[Crawler] %s: WALL SEEK — targeting wall (30%% roll)"),
									*GetOwner()->GetName());
								return;
							}
						}

						// 70% (or wall seek failed): ground wander
						const FVector2D RandCircle = FMath::RandPointInCircle(WanderRadius);
						const FVector WanderTarget = HomeLocation + FVector(RandCircle.X, RandCircle.Y, 0.f);
						CrawlerCMC->MoveTarget = WanderTarget;
						CrawlerCMC->MoveTargetActor = nullptr;
					},
					PauseDuration,
					false
				);
			}
		}
		break;
	}

	case ECrawlerState::Hunt:
	{
		// --- Perch & Pounce check ---
		if (bPerching)
		{
			PerchTimer += EvalInterval;
			if (PerchTimer >= PerchDuration)
			{
				bPerching = false;
				PerchTimer = 0.f;

				// POUNCE — re-check distance (player may have moved during 2.5s perch)
				if (CrawlerCMC && ChaseTargetActor)
				{
					const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
					const float PounceElev = CurrentLocation.Z - PlayerLoc.Z;
					const float PounceHDist = FVector::Dist2D(CurrentLocation, PlayerLoc);

					if (PounceHDist > MaxPounceHorizDist || PounceElev > MaxPounceElevation)
					{
						UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: POUNCE CANCELLED — too far (horiz=%.0f elev=%.0f)"),
							*Owner->GetName(), PounceHDist, PounceElev);
					}
					else
					{
					FVector OffsetDir = (CurrentLocation - PlayerLoc).GetSafeNormal2D();

					if (OffsetDir.IsNearlyZero(0.1f))
					{
						if (APlayerController* PouncPC = GetWorld()->GetFirstPlayerController())
						{
							FRotator CamRot;
							FVector CamLoc;
							PouncPC->GetPlayerViewPoint(CamLoc, CamRot);
							OffsetDir = CamRot.Vector().GetSafeNormal2D();
						}
						if (OffsetDir.IsNearlyZero(0.1f))
						{
							OffsetDir = FVector(1.f, 0.f, 0.f);
						}
					}

					const FVector OffsetTarget = PlayerLoc + OffsetDir * LungeOffset;
					CrawlerCMC->LaunchAtTarget(OffsetTarget);
					bPounceInFlight = true;

					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: POUNCE from %.0f above, %.0f horiz! (Hunt)"),
						*Owner->GetName(), PounceElev, PounceHDist);
					} // else (distance check passed)
				}

				SetSpeed(HuntSpeed);
			}
			break; // Don't do anything else while perching
		}

		// --- Post-crest perch: brief pause at fence/wall top ---
		if (CrawlerCMC && CrawlerCMC->bJustCrested)
		{
			CrawlerCMC->bJustCrested = false;

			const float ElevAbovePlayer = ChaseTargetActor
				? (CurrentLocation.Z - ChaseTargetActor->GetActorLocation().Z) : 0.f;

			if (ElevAbovePlayer >= LaunchElevationThreshold && !CrawlerCMC->IsLaunchOnCooldown())
			{
				// Elevated after crest — enter full perch (triggers pounce)
				bPerching = true;
				PerchTimer = 0.f;
				SetSpeed(0.f);
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: POST-CREST PERCH (elevated %.0f) — pounce in %.1fs"),
					*Owner->GetName(), ElevAbovePlayer, PerchDuration);
				break;
			}
			else
			{
				// Not elevated — brief 0.5s pause at fence top, then resume
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
		}

		// --- Ground attack check FIRST: close enough -> attack even when observed ---
		// Close range (0-AttackRange): alternating drop/lunge
		// Mid range (AttackRange-LungeMaxRange): lunge only
		// Same Z axis required (ZDiff <= 200)
		if (ChaseTargetActor && CrawlerCMC && !CrawlerCMC->IsLaunching() && !CrawlerCMC->IsSlamming())
		{
			const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
			const float HorizDist = FVector::Dist2D(CurrentLocation, PlayerLoc);
			const float ZDiff = FMath::Abs(CurrentLocation.Z - PlayerLoc.Z);
			const double AttackNow = GetWorld()->GetTimeSeconds();

			if (ZDiff <= 200.f && HorizDist <= LungeMaxRange && (AttackNow - LastAttackTime) >= AttackCooldown)
			{
				bBeingObserved = false; // Break freeze for attack
				if (HorizDist <= AttackRange)
				{
					// Close range — alternating drop/lunge
					if (bLastAttackWasLunge)
					{
						BeginSlamAttack();
					}
					else
					{
						BeginLungeAttack();
					}
				}
				else
				{
					// Mid-to-long range — lunge only
					BeginLungeAttack();
				}
				break;
			}
		}

		// --- Gaze behavior: WALL = freeze when watched. GROUND = aggressive charge. ---
		{
			const bool bOnWall = CrawlerCMC && CrawlerCMC->IsClimbing();
			const bool bLookedAt = IsPlayerLookingAtCreature();

			if (bOnWall && bLookedAt)
			{
				// ON WALL + being watched → freeze (SH2 horror moment)
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

			// ON GROUND + direct LOS for 2s + within lunge range + same Z → lunge attack
			// Mid-to-long range lunge from sustained eye contact. Same Z axis required.
			if (!bOnWall && bLookedAt && ChaseTargetActor)
			{
				const FVector PlayerLoc2 = ChaseTargetActor->GetActorLocation();
				const float HorizDist2 = FVector::Dist2D(CurrentLocation, PlayerLoc2);
				const float ZDiff2 = FMath::Abs(CurrentLocation.Z - PlayerLoc2.Z);

				if (HorizDist2 <= LungeMaxRange && ZDiff2 <= 200.f)
				{
					ObservedTimer += EvalInterval;
					if (ObservedTimer >= 2.0f)
					{
						ObservedTimer = 0.f;
						const double AttackNow = GetWorld()->GetTimeSeconds();
						if ((AttackNow - LastAttackTime) >= AttackCooldown)
						{
							bBeingObserved = false;
							if (HorizDist2 <= AttackRange)
							{
								// Close range — alternating
								if (bLastAttackWasLunge)
								{
									UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: GROUND LOS 2s at %.0f UU — DROP ATTACK!"),
										*Owner->GetName(), HorizDist2);
									BeginSlamAttack();
								}
								else
								{
									UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: GROUND LOS 2s at %.0f UU — LUNGE!"),
										*Owner->GetName(), HorizDist2);
									BeginLungeAttack();
								}
							}
							else
							{
								// Mid range — lunge only
								UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: GROUND LOS 2s at %.0f UU — MID-RANGE LUNGE!"),
									*Owner->GetName(), HorizDist2);
								BeginLungeAttack();
							}
							break;
						}
					}
				}
				else
				{
					ObservedTimer = 0.f; // Too far or wrong Z — reset timer
				}
			}
			else if (!bOnWall)
			{
				ObservedTimer = 0.f; // Reset if LOS broken on ground
			}
		}

		// Check if we should start perching (elevated above player, within pounce range)
		// Must be within MaxPounceHorizDist horizontally — no cross-map pounces.
		if (ChaseTargetActor && CrawlerCMC && !CrawlerCMC->IsLaunching() && !CrawlerCMC->IsLaunchOnCooldown())
		{
			const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
			const float ElevationAbovePlayer = CurrentLocation.Z - PlayerLoc.Z;
			const float PounceHorizDist = FVector::Dist2D(CurrentLocation, PlayerLoc);

			if (ElevationAbovePlayer >= LaunchElevationThreshold && ElevationAbovePlayer <= MaxPounceElevation
				&& PounceHorizDist <= MaxPounceHorizDist)
			{
				bPerching = true;
				PerchTimer = 0.f;
				SetSpeed(0.f);

				UE_LOG(LogTemp, Log, TEXT("[Crawler] %s: PERCHING at %.0f UU above, %.0f UU horiz — pounce in %.1fs (Hunt)"),
					*Owner->GetName(), ElevationAbovePlayer, PounceHorizDist, PerchDuration);
				break;
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
				// Reached last known position, player not found — switch to stalk
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Reached last known pos, player gone — stalking."),
					*Owner->GetName());
				SetState(ECrawlerState::Stalk);
			}
			else
			{
				// Still en route to last known location — direct pursuit
				CrawlerCMC->MoveTarget = LastKnownPlayerLocation;
				CrawlerCMC->MoveTargetActor = nullptr;
			}
		}
		break;
	}

	case ECrawlerState::Stalk:
	{
		// --- Perch & Pounce check (lower threshold — more aggressive) ---
		if (bPerching)
		{
			PerchTimer += EvalInterval;
			if (PerchTimer >= PerchDuration)
			{
				bPerching = false;
				PerchTimer = 0.f;

				// POUNCE — re-check distance (player may have moved during perch)
				if (CrawlerCMC && ChaseTargetActor)
				{
					const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
					const float PounceElev = CurrentLocation.Z - PlayerLoc.Z;
					const float PounceHDist = FVector::Dist2D(CurrentLocation, PlayerLoc);

					if (PounceHDist > MaxPounceHorizDist || PounceElev > MaxPounceElevation)
					{
						UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: POUNCE CANCELLED (Stalk) — too far (horiz=%.0f elev=%.0f)"),
							*Owner->GetName(), PounceHDist, PounceElev);
					}
					else
					{
					FVector OffsetDir = (CurrentLocation - PlayerLoc).GetSafeNormal2D();

					// If directly above, use player's forward to prevent head-landing
					if (OffsetDir.IsNearlyZero(0.1f))
					{
						if (APlayerController* PouncPC = GetWorld()->GetFirstPlayerController())
						{
							FRotator CamRot;
							FVector CamLoc;
							PouncPC->GetPlayerViewPoint(CamLoc, CamRot);
							OffsetDir = CamRot.Vector().GetSafeNormal2D();
						}
						if (OffsetDir.IsNearlyZero(0.1f))
						{
							OffsetDir = FVector(1.f, 0.f, 0.f);
						}
					}

					const FVector OffsetTarget = PlayerLoc + OffsetDir * LungeOffset;
					CrawlerCMC->LaunchAtTarget(OffsetTarget);
					bPounceInFlight = true;

					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: POUNCE from %.0f above, %.0f horiz! (Stalk)"),
						*Owner->GetName(), PounceElev, PounceHDist);
					} // else (distance check passed)
				}

				SetSpeed(StalkSpeed);
			}
			break;
		}

		// Check perch with Stalk's lower threshold — but only after MinStalkTime on the wall
		// Must be within MaxPounceHorizDist horizontally — no cross-map pounces.
		if (ChaseTargetActor && CrawlerCMC && !CrawlerCMC->IsLaunching() && !CrawlerCMC->IsLaunchOnCooldown()
			&& StateTimer >= MinStalkTimeBeforePounce)
		{
			const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
			const float ElevationAbovePlayer = CurrentLocation.Z - PlayerLoc.Z;
			const float StalkPounceHDist = FVector::Dist2D(CurrentLocation, PlayerLoc);

			if (ElevationAbovePlayer >= StalkPounceElevation && ElevationAbovePlayer <= MaxPounceElevation
				&& StalkPounceHDist <= MaxPounceHorizDist)
			{
				bPerching = true;
				PerchTimer = 0.f;
				SetSpeed(0.f);

				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: PERCHING at %.0f above, %.0f horiz (Stalk)"),
					*Owner->GetName(), ElevationAbovePlayer, StalkPounceHDist);
				break;
			}
		}

		// --- Post-crest perch in Stalk ---
		if (CrawlerCMC && CrawlerCMC->bJustCrested)
		{
			CrawlerCMC->bJustCrested = false;

			const float ElevAbovePlayer = ChaseTargetActor
				? (CurrentLocation.Z - ChaseTargetActor->GetActorLocation().Z) : 0.f;

			if (ElevAbovePlayer >= StalkPounceElevation && !CrawlerCMC->IsLaunchOnCooldown()
				&& StateTimer >= MinStalkTimeBeforePounce)
			{
				bPerching = true;
				PerchTimer = 0.f;
				SetSpeed(0.f);
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: STALK POST-CREST PERCH (elevated %.0f)"),
					*Owner->GetName(), ElevAbovePlayer);
				break;
			}
			else
			{
				// Brief pause at crest, then resume stalk
				SetSpeed(0.f);
				bIsPaused = true;

				FTimerHandle StalkCrestHandle;
				TWeakObjectPtr<UZP_CrawlerBehaviorComponent> WeakThis(this);
				GetWorld()->GetTimerManager().SetTimer(StalkCrestHandle,
					[WeakThis]()
					{
						if (WeakThis.IsValid() && WeakThis->CurrentState == ECrawlerState::Stalk)
						{
							WeakThis->bIsPaused = false;
							WeakThis->SetSpeed(WeakThis->StalkSpeed);
						}
					},
					0.5f, false);

				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: STALK POST-CREST PAUSE 0.5s"),
					*Owner->GetName());
				break;
			}
		}

		// --- Gaze awareness: freeze when watched, aggro on prolonged stare ---
		{
			const bool bLookedAt = IsPlayerLookingAtCreature();
			if (bLookedAt)
			{
				if (!bBeingObserved)
				{
					bBeingObserved = true;
					ObservedTimer = 0.f;
					SetSpeed(0.f);
					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Player is WATCHING — freezing."), *Owner->GetName());
				}

				ObservedTimer += EvalInterval;

				if (ObservedTimer >= GazeAggroDuration)
				{
					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: STARE AGGRO — player stared %.1fs, attacking!"),
						*Owner->GetName(), ObservedTimer);
					SetState(ECrawlerState::Hunt);
					break;
				}
			}
			else if (bBeingObserved)
			{
				bBeingObserved = false;
				ObservedTimer = 0.f;
				SetSpeed(StalkSpeed);
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Player looked away — resuming stalk."), *Owner->GetName());
			}
		}

		// --- Proximity aggro: too close -> Hunt ---
		if (ChaseTargetActor)
		{
			const float DistToPlayer = GetEffectiveDistance(CurrentLocation, ChaseTargetActor->GetActorLocation());
			if (DistToPlayer <= ProximityAggroRange)
			{
				UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: PROXIMITY AGGRO — player at %.0f UU (threshold %.0f)"),
					*Owner->GetName(), DistToPlayer, ProximityAggroRange);
				SetState(ECrawlerState::Hunt);
				break;
			}
		}

		// Timeout — return to patrol
		if (StateTimer >= StalkDuration)
		{
			UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Stalk timeout (%.0fs) — returning to patrol."),
				*Owner->GetName(), StalkDuration);
			SetState(ECrawlerState::Patrol);
		}
		else if (!bBeingObserved)
		{
			// Only reposition when NOT being observed — creature freezes when watched
			const float DistToSearch = FVector::Dist(CurrentLocation, CrawlerCMC->MoveTarget);
			if (DistToSearch <= ArrivalThreshold)
			{
				if (IsOnGround())
				{
					// On ground during Stalk — find a wall to climb first
					FVector WallPoint, WallNormal;
					if (FindNearestWall(WallPoint, WallNormal))
					{
						CrawlerCMC->MoveTarget = WallPoint - WallNormal * 100.f;
						CrawlerCMC->MoveTargetActor = nullptr;

						UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Stalk WALL SEEK — finding wall before climbing to elevation"),
							*Owner->GetName());
					}
					else
					{
						// No wall — elevated retreat as fallback
						FVector AwayFromPlayer = (CurrentLocation - LastKnownPlayerLocation).GetSafeNormal2D();
						if (AwayFromPlayer.IsNearlyZero())
						{
							AwayFromPlayer = FMath::VRand();
							AwayFromPlayer.Z = 0.f;
							AwayFromPlayer.Normalize();
						}
						const float RetreatDistance = FMath::RandRange(400.f, 800.f);
						FVector NewTarget = CurrentLocation + AwayFromPlayer * RetreatDistance + FVector(0.f, 0.f, StalkVerticalBias);
						NewTarget.Z = FMath::Min(NewTarget.Z, LastKnownPlayerLocation.Z + MaxStalkHeight);
						CrawlerCMC->MoveTarget = NewTarget;
						CrawlerCMC->MoveTargetActor = nullptr;
					}
				}
				else
				{
					// Already on wall/elevated — retreat away from player at height
					FVector AwayFromPlayer = (CurrentLocation - LastKnownPlayerLocation).GetSafeNormal2D();
					if (AwayFromPlayer.IsNearlyZero())
					{
						AwayFromPlayer = FMath::VRand();
						AwayFromPlayer.Z = 0.f;
						AwayFromPlayer.Normalize();
					}

					const float RandomAngle = FMath::RandRange(-45.f, 45.f);
					const FVector RotatedDir = AwayFromPlayer.RotateAngleAxis(RandomAngle, FVector::UpVector);

					const float RetreatDistance = FMath::RandRange(400.f, 800.f);
					FVector NewTarget = CurrentLocation + RotatedDir * RetreatDistance + FVector(0.f, 0.f, StalkVerticalBias);
					NewTarget.Z = FMath::Min(NewTarget.Z, LastKnownPlayerLocation.Z + MaxStalkHeight);

					CrawlerCMC->MoveTarget = NewTarget;
					CrawlerCMC->MoveTargetActor = nullptr;

					UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: Stalking — retreat+climb to %s (%.0f UU away, +%.0f Z)"),
						*Owner->GetName(), *NewTarget.ToString(), RetreatDistance, StalkVerticalBias);
				}
			}
		}
		break;
	}

	case ECrawlerState::Attack:
	{
		AttackPhaseTimer += EvalInterval;

		switch (CurrentAttackType)
		{
		case EAttackType::Lunge:
		{
			// Phase 1: Wind-up — creature freezes, builds tension
			if (!bLungeExecuted && AttackPhaseTimer >= LungeWindUpTime)
			{
				ExecuteLunge();
				bLungeExecuted = true;
			}

			// Phase 2: Post-launch — wait for landing, check damage
			if (bLungeExecuted && CrawlerCMC && !CrawlerCMC->IsLaunching()
				&& AttackPhaseTimer > LungeWindUpTime + 0.3f)
			{
				if (!bAttackDamageApplied && ChaseTargetActor)
				{
					const float Dist = FVector::Dist(CurrentLocation, ChaseTargetActor->GetActorLocation());
					if (Dist <= LungeDamageRadius)
					{
						ApplyAttackDamage(LungeDamage, 0.f);
						UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: LUNGE HIT — %.0f damage at %.0f UU"),
							*Owner->GetName(), LungeDamage, Dist);
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: LUNGE MISSED — player at %.0f UU"),
							*Owner->GetName(), Dist);
					}
					bAttackDamageApplied = true;
				}

				LastAttackTime = GetWorld()->GetTimeSeconds();
				SetState(ECrawlerState::Hunt);
			}

			// Safety timeout
			if (AttackPhaseTimer > 4.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ZP_Behavior] %s: LUNGE TIMEOUT — forcing Hunt"),
					*Owner->GetName());
				LastAttackTime = GetWorld()->GetTimeSeconds();
				SetState(ECrawlerState::Hunt);
			}
			break;
		}

		case EAttackType::Slam:
		{
			// Drop attack: CMC handles hold/impact, we wait for it
			if (CrawlerCMC && CrawlerCMC->HasSlamImpacted())
			{
				CrawlerCMC->ClearSlamImpact();

				// Drop attack damage — single target (not AoE)
				if (ChaseTargetActor)
				{
					const float Dist = FVector::Dist(CurrentLocation, ChaseTargetActor->GetActorLocation());
					if (Dist <= LungeDamageRadius)
					{
						ApplyAttackDamage(LungeDamage, 0.f);
						UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: DROP ATTACK HIT — %.0f damage at %.0f UU"),
							*Owner->GetName(), LungeDamage, Dist);
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: DROP ATTACK MISSED — player at %.0f UU"),
							*Owner->GetName(), Dist);
					}
				}

				LastAttackTime = GetWorld()->GetTimeSeconds();
				SetState(ECrawlerState::Hunt);
			}

			// Safety timeout
			if (AttackPhaseTimer > 3.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ZP_Behavior] %s: DROP TIMEOUT — forcing Hunt"),
					*Owner->GetName());
				LastAttackTime = GetWorld()->GetTimeSeconds();
				SetState(ECrawlerState::Hunt);
			}
			break;
		}

		default:
			SetState(ECrawlerState::Hunt);
			break;
		}
		break;
	}

	}
}

// --- Perception ---

void UZP_CrawlerBehaviorComponent::OnSeePawn(APawn* SeenPawn)
{
	if (!SeenPawn)
	{
		return;
	}

	// Update last known position every time we see the player
	LastKnownPlayerLocation = SeenPawn->GetActorLocation();

	if (CurrentState == ECrawlerState::Hunt)
	{
		// Reset sight timer — still tracking player
		LastSeenTimer = 0.f;
		return;
	}

	if (CurrentState == ECrawlerState::Stalk)
	{
		// Stay in Stalk — Hunt only via gaze/proximity/pounce
		ChaseTargetActor = SeenPawn;
		LastKnownPlayerLocation = SeenPawn->GetActorLocation();
		LastSeenTimer = 0.f;
		return;
	}

	// Patrol — start hunt directly (no Alert)
	StartHunt(SeenPawn);
}

// --- Helpers ---

void UZP_CrawlerBehaviorComponent::SetSpeed(float Speed)
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
		{
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

	bIsIndoors = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
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
		if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, TraceEnd, ECC_Visibility, Params))
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

	// Spike threat — one shot maxes it out for Patrol/Stalk creatures
	ThreatLevel = FMath::Min(ThreatLevel + ThreatFromGunshot, ThreatThreshold * 1.5f);
	LastKnownPlayerLocation = NoiseLocation;

	// Update chase target to the shooter regardless of current state
	ChaseTargetActor = Shooter;
	LastKnownPlayerLocation = NoiseLocation;

	// If already hunting, refresh tracking and redirect toward shooter
	if (CurrentState == ECrawlerState::Hunt)
	{
		LastSeenTimer = 0.f;
		if (CrawlerCMC)
		{
			CrawlerCMC->MoveTargetActor = ChaseTargetActor;
		}
		UE_LOG(LogTemp, Log, TEXT("[Crawler] %s: GUNSHOT while hunting — retarget to shooter at %.0f UU"),
			*GetOwner()->GetName(),
			FVector::Dist(GetOwner()->GetActorLocation(), NoiseLocation));
		return;
	}

	// Any other state: instant Hunt
	UE_LOG(LogTemp, Log, TEXT("[Crawler] %s: GUNSHOT HEARD at %.0f UU — instant Hunt!"),
		*GetOwner()->GetName(),
		FVector::Dist(GetOwner()->GetActorLocation(), NoiseLocation));

	SetState(ECrawlerState::Hunt);
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

void UZP_CrawlerBehaviorComponent::BeginLungeAttack()
{
	CurrentAttackType = EAttackType::Lunge;
	bAttackDamageApplied = false;
	bLungeExecuted = false;
	AttackPhaseTimer = 0.f;
	bLastAttackWasLunge = true;
	SetState(ECrawlerState::Attack);
	SetSpeed(0.f);

	// Wind-up: creature freezes for LungeWindUpTime before launching.
	// ExecuteLunge called from Attack tick after wind-up completes.

	UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: LUNGE WIND-UP — launching in %.1fs"),
		*GetOwner()->GetName(), LungeWindUpTime);
}

void UZP_CrawlerBehaviorComponent::ExecuteLunge()
{
	if (!CrawlerCMC || !ChaseTargetActor)
	{
		return;
	}

	const FVector PlayerLoc = ChaseTargetActor->GetActorLocation();
	const FVector CreatureLoc = GetOwner()->GetActorLocation();
	FVector OffsetDir = (CreatureLoc - PlayerLoc).GetSafeNormal2D();

	// If directly above/below, use player forward to avoid landing on head
	if (OffsetDir.IsNearlyZero(0.1f))
	{
		if (APlayerController* LungePC = GetWorld()->GetFirstPlayerController())
		{
			FRotator CamRot;
			FVector CamLoc;
			LungePC->GetPlayerViewPoint(CamLoc, CamRot);
			OffsetDir = CamRot.Vector().GetSafeNormal2D();
		}
		if (OffsetDir.IsNearlyZero(0.1f))
		{
			OffsetDir = FVector(1.f, 0.f, 0.f);
		}
	}

	const FVector OffsetTarget = PlayerLoc + OffsetDir * LungeOffset;
	CrawlerCMC->LaunchAtTarget(OffsetTarget);

	UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: LUNGE launched — offset target %s"),
		*GetOwner()->GetName(), *OffsetTarget.ToString());
}

void UZP_CrawlerBehaviorComponent::BeginSlamAttack()
{
	CurrentAttackType = EAttackType::Slam;
	bAttackDamageApplied = false;
	AttackPhaseTimer = 0.f;
	bLastAttackWasLunge = false;
	SetState(ECrawlerState::Attack);
	SetSpeed(0.f);

	if (CrawlerCMC)
	{
		// Drop attack: 0 rise, short hold (rear-up), 0 explicit fall speed (uses default)
		CrawlerCMC->BeginSlam(0.f, DropAttackHoldTime, 0.f);
	}

	UE_LOG(LogTemp, Log, TEXT("[ZP_Behavior] %s: DROP ATTACK — rear-up %.1fs then impact"),
		*GetOwner()->GetName(), DropAttackHoldTime);
}

void UZP_CrawlerBehaviorComponent::ApplyAttackDamage(float Damage, float Radius)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
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
