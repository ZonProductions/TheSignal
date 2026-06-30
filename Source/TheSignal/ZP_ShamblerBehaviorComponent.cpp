// Copyright The Signal. All Rights Reserved.

#include "ZP_ShamblerBehaviorComponent.h"
#include "ZP_EnemyAudioComponent.h"
#include "ZP_HealthComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UZP_ShamblerBehaviorComponent::UZP_ShamblerBehaviorComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // per-frame combat facing (state logic stays on the eval timer)

	// NOTE: anim defaults are loaded lazily in BeginPlay (LoadAnimDefaults), NOT here.
	// ConstructorHelpers::FObjectFinder loading /Game assets during CDO construction crashed
	// on editor load (EXCEPTION_ACCESS_VIOLATION in FindOrLoadObject). Lazy LoadObject mirrors
	// the sound-loading pattern below and preserves per-instance BP overrides.
}

void UZP_ShamblerBehaviorComponent::LoadAnimDefaults()
{
	// Only fill slots the Blueprint hasn't overridden (serialized overrides are present before BeginPlay).
	// NOTE: these clips must stay free of orphan skeleton curves. The retargeted Hit_Front/Hit_Back shipped
	// with stale GASP distance-matching curves (DistanceToApex/DistanceCurve/blendOrient1) that the necromorph
	// skeleton has no name mapping for; on editor load that tripped UAnimSequence::PostLoad's curve-name
	// verification and hard-crashed (EXCEPTION_ACCESS_VIOLATION) on PIE / -game / level open. The curves were
	// stripped 2026-06-29 (Shambler plays SingleNode clips, never used them). See checkpoint
	// 2026-06-29_shambler_anim_curve_postload_crash. If you re-import these clips, strip stray curves again.
	auto Fill = [](TObjectPtr<UAnimSequence>& Slot, const TCHAR* P)
	{
		if (!Slot) { Slot = LoadObject<UAnimSequence>(nullptr, P); }
	};
	Fill(WalkAnim,       TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Walk.A_Shambler_Walk"));
	Fill(IdleAnim,       TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Idle.A_Shambler_Idle"));
	Fill(AttackLAnim,    TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_L.A_Shambler_Attack_L"));
	Fill(AttackRAnim,    TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_R.A_Shambler_Attack_R"));
	Fill(ScreamAnim,     TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Scream.A_Shambler_Scream"));
	Fill(DeathFrontAnim, TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Death_Front.A_Shambler_Death_Front"));
	Fill(DeathBackAnim,  TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Death_Back.A_Shambler_Death_Back"));
	Fill(HitFrontAnim,   TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Hit_Front.A_Shambler_Hit_Front"));
	Fill(HitBackAnim,    TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Hit_Back.A_Shambler_Hit_Back"));
}

void UZP_ShamblerBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<ACharacter>(GetOwner());
	if (!Owner) { return; }

	LoadAnimDefaults(); // fill any anim slots the BP didn't override (was a crashing ctor load)

	AICon = Cast<AAIController>(Owner->GetController());
	if (AICon)
	{
		// Chain the next wander leg the INSTANT the current move completes (arrived/aborted/failed).
		// Without this, the body sits idle in the gap between MoveTo completion and the next 0.25s
		// Evaluate — which is exactly the visible "pause" the dev still saw after removing pause logic.
		AICon->ReceiveMoveCompleted.AddDynamic(this, &UZP_ShamblerBehaviorComponent::OnMoveCompleted);
	}

	// Spatial voice (lurk/alert/attack) — reuse the enemy audio component, point it at the zombie SFX.
	Audio = NewObject<UZP_EnemyAudioComponent>(Owner, TEXT("ShamblerAudio"));
	if (Audio)
	{
		Audio->RegisterComponent();
		// Room-scale spatialization + occlusion so growls/alerts don't carry through walls into other rooms.
		Audio->Attenuation = LoadObject<USoundAttenuation>(nullptr, TEXT("/Game/Audio/SA_EnemyVoice.SA_EnemyVoice"));
		Audio->AlertSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_ALERT.SFX_ZOMBIE_ALERT"));
		Audio->LurkingLoop = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_LURK1.SFX_ZOMBIE_LURK1"));
		// Two strikes, alternated with the swing side (L=Attack1, R=Attack2). Attack2 is optional —
		// until SFX_ZOMBIE_ATTACK2 is imported, both swings fall back to Attack1.
		Audio->AttackSounds.Empty();
		if (USoundBase* Atk1 = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_ATTACK.SFX_ZOMBIE_ATTACK")))
		{
			Audio->AttackSounds.Add(Atk1);
		}
		if (USoundBase* Atk2 = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_ATTACK2.SFX_ZOMBIE_ATTACK2")))
		{
			Audio->AttackSounds.Add(Atk2);
		}
	}

	// Health — create one if the BP doesn't already have it, so the Shambler can take damage + die.
	Health = Owner->FindComponentByClass<UZP_HealthComponent>();
	if (!Health)
	{
		Health = NewObject<UZP_HealthComponent>(Owner, TEXT("ShamblerHealth"));
		Health->MaxHealth = MaxHealth;
		Health->RegisterComponent();
	}
	Health->MaxHealth = MaxHealth;
	Health->ResetHealth();
	Health->OnDied.AddDynamic(this, &UZP_ShamblerBehaviorComponent::OnOwnerDied);

	// Take bullets: the weapon fires ApplyPointDamage (carries the hit bone) -> head vs body damage.
	Owner->OnTakePointDamage.AddDynamic(this, &UZP_ShamblerBehaviorComponent::OnPointDamage);
	Owner->OnTakeAnyDamage.AddDynamic(this, &UZP_ShamblerBehaviorComponent::OnAnyDamage); // PROBE

	if (USkeletalMeshComponent* M0 = Owner->GetMesh())
	{
		MeshBaseRelZ = M0->GetRelativeLocation().Z;
		M0->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); // capsule takes the bullet instead
	}

	// Tether: every wander pick is anchored around the spawn point so the Shambler doesn't
	// drift the length of the building over time. WanderRadius now reads as a true leash radius.
	SpawnLocation = Owner->GetActorLocation();
	// Shootable exactly like the Crawler: the CAPSULE blocks the hitscan's Visibility trace (the per-bone
	// mesh trace doesn't register on this rig). The Pawn profile ignores Visibility, so force it here.
	if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent())
	{
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] shootable via capsule: VisResp=%d"),
			(int32)Cap->GetCollisionResponseToChannel(ECC_Visibility));
	}

	SetState(EShamblerState::Wander);

	GetWorld()->GetTimerManager().SetTimer(EvalTimer, this, &UZP_ShamblerBehaviorComponent::Evaluate, EvalInterval, true);
	GetWorld()->GetTimerManager().SetTimer(ProbeTimer, this, &UZP_ShamblerBehaviorComponent::ProbeShootable, 2.0f, true); // PROBE
}

void UZP_ShamblerBehaviorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(EvalTimer);
		W->GetTimerManager().ClearTimer(AttackHitTimer);
		W->GetTimerManager().ClearTimer(ProbeTimer);
		W->GetTimerManager().ClearTimer(IdleLockTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UZP_ShamblerBehaviorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Owner) { return; }
	if (bDead)
	{
		// Lower the mesh over the death clip to fake the missing root-motion drop -> corpse grounds.
		if (bDropping)
		{
			if (USkeletalMeshComponent* M = Owner->GetMesh())
			{
				const float DropDur = FMath::Max(DeathAnimLen - DeathDropLead, 0.1f);
				float Alpha = FMath::Clamp((float)(GetWorld()->GetTimeSeconds() - DeathStartTime) / DropDur, 0.f, 1.f);
				Alpha = 1.f - (1.f - Alpha) * (1.f - Alpha); // ease-out: drops quick, settles
				FVector RL = M->GetRelativeLocation();
				RL.Z = MeshBaseRelZ - DeathDropZ * Alpha;
				M->SetRelativeLocation(RL);
				if (Alpha >= 1.f) { bDropping = false; }
			}
		}
		return;
	}
	// Only hand-face the player while STOPPED (scream / attack) — otherwise the walk anim points at you
	// while the body slides along the nav path. Chase faces its movement direction (natural walk).
	if (State == EShamblerState::Scream || State == EShamblerState::Attack)
	{
		FaceTargetSmooth(DeltaTime);
	}

	// Mid-leg stumble end: as soon as the stumble timer runs out, re-issue the move toward the same
	// wander target. Done here (per-frame) rather than in Evaluate (0.25s eval) so the resume isn't
	// gated on the next eval tick — keeps the stumble feeling like a hesitation, not a freeze.
	if (bStumbling && State == EShamblerState::Wander && GetWorld()->GetTimeSeconds() >= StumbleEndTime)
	{
		bStumbling = false;
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] STUMBLE END (bWanderMoving=%d)"), bWanderMoving ? 1 : 0);
		if (bWanderMoving && AICon)
		{
			AICon->MoveToLocation(WanderDest, 100.f);
		}
	}

	// Per-frame velocity probe — fires twice per second while in Wander. If the body shows vel>0
	// while bWanderMoving=0 and bStumbling=0 we've found the layered mover: something OUTSIDE this
	// component is driving CMC during a pause. Pair with the state-change logs to nail down WHO.
	if (State == EShamblerState::Wander)
	{
		static int32 sShTickLog = 0;
		if ((++sShTickLog % 30) == 0)
		{
			const float V = Owner->GetVelocity().Size();
			UE_LOG(LogTemp, Warning, TEXT("[Shambler] TICK walking=%d stumbling=%d vel=%.1f stateT=%.2f walkDur=%.2f idleDur=%.2f moveStatus=%d"),
				bWanderMoving ? 1 : 0, bStumbling ? 1 : 0, V, StateTimer, WalkDuration, IdleDuration,
				AICon ? (int32)AICon->GetMoveStatus() : -1);
		}
	}
}

void UZP_ShamblerBehaviorComponent::FaceTargetSmooth(float DeltaTime)
{
	AActor* T = Target ? Target.Get() : Cast<AActor>(GetPlayer());
	if (!T || !Owner) { return; }
	FVector ToT = T->GetActorLocation() - Owner->GetActorLocation();
	ToT.Z = 0.f;
	if (ToT.IsNearlyZero()) { return; }
	const FRotator Cur = Owner->GetActorRotation();
	const FRotator Desired(0.f, ToT.Rotation().Yaw, 0.f);
	const FRotator NewRot = FMath::RInterpConstantTo(Cur, Desired, DeltaTime, CombatTurnRate);
	Owner->SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
}

// ───────────────────────── core loop ─────────────────────────

void UZP_ShamblerBehaviorComponent::Evaluate()
{
	if (bDead || !Owner) { return; }
	if (bStaggered) { return; }
	StateTimer += EvalInterval;

	APawn* Player = GetPlayer();
	const float DistToPlayer = Player ? FVector::Dist(Owner->GetActorLocation(), Player->GetActorLocation()) : TNumericLimits<float>::Max();

	switch (State)
	{
	case EShamblerState::Wander:
	{
		UpdateLurk(DistToPlayer);

		// Spotted? Needs range + roughly facing the player + a clear sightline (same room).
		if (Player && DistToPlayer <= DetectionRange)
		{
			const FVector ToP = (Player->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
			const float Facing = FVector::DotProduct(Owner->GetActorForwardVector(), ToP);
			const bool bLOS = HasLOS(Player);
			const bool bClose = DistToPlayer <= 350.f; // right next to it -> senses you regardless of facing
			if (bLOS && (bClose || Facing >= FacingThreshold))
			{
				Target = Player;
				SetState(EShamblerState::Scream);
				break;
			}
		}

		// Two-phase wander: WALK for WalkDuration (3-6s) then IDLE for IdleDuration (6-9s).
		// During WALK, legs chain seamlessly via OnMoveCompleted so velocity never drops to zero
		// (no idle frame leaking through from the BlendSpace). During IDLE, movement is fully
		// stopped and the zombie idle anim plays via the slot.
		if (bWanderMoving)
		{
			// Walk timer up -> drop into idle.
			if (StateTimer >= WalkDuration)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Shambler] WALK→IDLE (walked %.2fs of %.2fs)"), StateTimer, WalkDuration);
				bWanderMoving = false;
				StateTimer = 0.f;
				IdleDuration = FMath::FRandRange(IdleDurationMin, IdleDurationMax);
				// Smooth deceleration: AICon->StopMovement aborts the path-following request, but we
				// do NOT call StopMovementImmediately or set MOVE_None here — the CMC's braking
				// deceleration ramps velocity from walk speed → 0 over a fraction of a second,
				// which the BlendSpace shows as a natural walk→idle transition under the slot
				// blend. After IdleLockDelay, LockIdleMovement() snaps MOVE_None to freeze anything
				// residual (RVO, accel, etc.).
				if (AICon) { AICon->StopMovement(); }
				if (IdleAnim)
				{
					PlaySlotLoop(IdleAnim);
					if (USkeletalMeshComponent* SM = Owner->GetMesh())
					{
						FVector RL = SM->GetRelativeLocation();
						RL.Z = MeshBaseRelZ + IdleMeshZOffset;
						SM->SetRelativeLocation(RL);
					}
				}
				GetWorld()->GetTimerManager().SetTimer(IdleLockTimer, this,
					&UZP_ShamblerBehaviorComponent::LockIdleMovement, IdleLockDelay, false);
			}
			else if (!bStumbling)
			{
				// Mid-leg stumble (rare). Interrupts movement for a beat then resumes the same leg.
				if (FMath::FRand() < StumbleChancePerSec * EvalInterval)
				{
					bStumbling = true;
					StumbleEndTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(StumbleMin, StumbleMax);
					UE_LOG(LogTemp, Warning, TEXT("[Shambler] STUMBLE START (will end at %.2fs)"), StumbleEndTime);
					if (AICon) { AICon->StopMovement(); }
					if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
					{
						CM->StopMovementImmediately();
					}
				}
			}
		}
		else
		{
			// Idle phase: wait IdleDuration, then resume walking.
			if (StateTimer >= IdleDuration)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Shambler] IDLE→WALK (idled %.2fs of %.2fs)"), StateTimer, IdleDuration);
				StopSlotLoop();
				if (USkeletalMeshComponent* SM = Owner->GetMesh())
				{
					FVector RL = SM->GetRelativeLocation();
					RL.Z = MeshBaseRelZ;
					SM->SetRelativeLocation(RL);
				}
				// Cancel a pending idle lock (shouldn't be pending this late but defensive),
				// and re-enable walking before issuing the next leg.
				GetWorld()->GetTimerManager().ClearTimer(IdleLockTimer);
				if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
				{
					CM->SetMovementMode(MOVE_Walking);
				}
				if (PickNewWanderPoint())
				{
					StartWanderLeg();
					bWanderMoving = true;
					StateTimer = 0.f;
					WalkDuration = FMath::FRandRange(WalkDurationMin, WalkDurationMax);
				}
				else
				{
					// Couldn't find a navmesh point — extend the idle by a short retry beat.
					StateTimer = IdleDuration - FMath::FRandRange(0.2f, 0.5f);
				}
			}
		}
		break;
	}

	case EShamblerState::Scream:
	{
		// Hold the scream a fixed beat, then break into the fast walk.
		if (StateTimer >= ScreamHoldTime)
		{
			SetState(EShamblerState::Chase);
		}
		break;
	}

	case EShamblerState::Chase:
	{
		if (!Target) { SetState(EShamblerState::Wander); break; }

		const bool bSee = HasLOS(Target);
		LostSightTimer = bSee ? 0.f : (LostSightTimer + EvalInterval);

		// Give up: lost sight too long, or leashed out.
		if (LostSightTimer >= LoseSightTime || DistToPlayer > GiveUpRange)
		{
			Target = nullptr;
			SetState(EShamblerState::Wander);
			PickNewWanderPoint();
			break;
		}

		// In range + off cooldown -> swipe.
		const double Now = GetWorld()->GetTimeSeconds();
		if (DistToPlayer <= AttackRange && bSee && (Now - LastAttackTime) >= AttackCooldown)
		{
			BeginAttack();
			break;
		}

		// Keep closing — re-path to the (possibly moving) player each eval so it tracks instead of
		// running a stale path past you. Stop WELL inside AttackRange (not at its edge) so it reliably
		// crosses the attack threshold and swings instead of parking just out of reach.
		if (AICon) { AICon->MoveToActor(Target, FMath::Max(AttackRange - 70.f, 40.f)); }
		break;
	}

	case EShamblerState::Attack:
	{
		if (StateTimer >= AttackDuration)
		{
			// Still on top of the player when the swing ends -> immediately swing again (other side,
			// other strike sound). No cooldown gap: it flurries L/R/L/R until you die or break away.
			if (Target && DistToPlayer <= AttackRange && HasLOS(Target))
			{
				BeginAttack();
			}
			else
			{
				SetState(Target ? EShamblerState::Chase : EShamblerState::Wander);
			}
		}
		break;
	}
	}
}

void UZP_ShamblerBehaviorComponent::SetState(EShamblerState NewState)
{
	State = NewState;
	StateTimer = 0.f;

	// Wander faces its movement direction; combat states are hand-faced toward the player (no shimmy,
	// fast pivots even while stopped to attack).
	if (Owner)
	{
		if (UCharacterMovementComponent* M = Owner->GetCharacterMovement())
		{
			M->bOrientRotationToMovement = (NewState == EShamblerState::Wander || NewState == EShamblerState::Chase);
			// Idle phase parks CMC in MOVE_None; any state transition must restore Walking so the
			// AI can move again (Chase/Wander-walk). Idle re-applies MOVE_None inside Evaluate.
			if (M->MovementMode == MOVE_None)
			{
				M->SetMovementMode(MOVE_Walking);
			}
		}
		// Ground the floaty scream pose (and only the scream) by nudging the mesh Z.
		if (USkeletalMeshComponent* SM = Owner->GetMesh())
		{
			FVector RL = SM->GetRelativeLocation();
			RL.Z = MeshBaseRelZ + (NewState == EShamblerState::Scream ? ScreamMeshZOffset : 0.f);
			SM->SetRelativeLocation(RL);
		}
	}

	switch (NewState)
	{
	case EShamblerState::Wander:
		EnsureLocomotion();
		SetSpeed(WanderSpeed);
		bWanderMoving = false;
		bStumbling = false;
		IdleDuration = 0.f; // pick a first leg on the very next eval — no startup pause
		StopSlotLoop();
		break;

	case EShamblerState::Scream:
		SetSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		if (Audio) { Audio->PlayAlert(); }
		PlayOneShot(ScreamAnim);
		break;

	case EShamblerState::Chase:
		EnsureLocomotion();
		SetSpeed(ChaseSpeed);
		break;

	case EShamblerState::Attack:
		SetSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		break;
	}
}

// ───────────────────────── attack ─────────────────────────

void UZP_ShamblerBehaviorComponent::BeginAttack()
{
	bAttackIsLeft = !bAttackIsLeft;
	LastAttackTime = GetWorld()->GetTimeSeconds();
	SetState(EShamblerState::Attack);
	// Alternate the strike sound with the swing side: Left -> Attack1, Right -> Attack2.
	if (Audio) { Audio->PlayAttack(/*bLunge=*/!bAttackIsLeft); }
	PlayOneShot(bAttackIsLeft ? AttackLAnim : AttackRAnim);
	GetWorld()->GetTimerManager().SetTimer(AttackHitTimer, this, &UZP_ShamblerBehaviorComponent::ApplyAttackDamage, AttackHitTime, false);
}

void UZP_ShamblerBehaviorComponent::ApplyAttackDamage()
{
	if (bDead || !Owner || !Target) { return; }
	// Must still be close and visible at the moment of impact (no hitting through walls / after the player ran).
	if (FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation()) > AttackRange * 1.3f) { return; }
	if (!HasLOS(Target)) { return; }

	AController* Inst = Owner->GetController();
	UGameplayStatics::ApplyDamage(Target, AttackDamage, Inst, Owner, nullptr);
}

// ───────────────────────── helpers ─────────────────────────

APawn* UZP_ShamblerBehaviorComponent::GetPlayer() const
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		return PC->GetPawn();
	}
	return nullptr;
}

bool UZP_ShamblerBehaviorComponent::HasLOS(const AActor* InTarget) const
{
	if (!InTarget || !Owner || !GetWorld()) { return false; }
	const FVector Start = Owner->GetActorLocation() + FVector(0, 0, 60.f);
	const FVector End = InTarget->GetActorLocation() + FVector(0, 0, 40.f);
	FCollisionQueryParams P;
	P.AddIgnoredActor(Owner);
	P.AddIgnoredActor(InTarget);
	// Ignore both actors' attached actors (weapon meshes, child actors) so they can't false-block.
	TArray<AActor*> Att;
	InTarget->GetAttachedActors(Att);
	for (AActor* A : Att) { P.AddIgnoredActor(A); }
	Owner->GetAttachedActors(Att);
	for (AActor* A : Att) { P.AddIgnoredActor(A); }

	// Walk the sightline, IGNORING pure trigger volumes (door interaction boxes, overlap zones) and
	// re-tracing past each one until we hit SOLID geometry. A single multi-trace can't do this — it
	// stops dead at the first blocker, and a door's InteractionVolume BLOCKS WorldStatic right where
	// the zombie stands (@0), so the trace would die on the trigger and never reach the wall behind it.
	bool bClear = true;
	for (int32 Iter = 0; Iter < 8; ++Iter)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, P))
		{
			break; // nothing solid left on the line -> clear sight
		}
		UPrimitiveComponent* Comp = Hit.GetComponent();
		if (Comp && Comp->GetCollisionEnabled() == ECollisionEnabled::QueryOnly)
		{
			P.AddIgnoredComponent(Comp); // trigger -> see through it, re-trace ignoring it
			continue;
		}
		bClear = false;
		break;
	}

	return bClear;
}

bool UZP_ShamblerBehaviorComponent::PickNewWanderPoint()
{
	if (!Owner) { return false; }
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav) { return false; }
	// Target-dot + spawn-tether picker. Picks the candidate whose direction from the body is CLOSEST
	// to WanderTargetDot relative to forward — default 0.5 (~60° off forward) gives curving paths
	// instead of the highest-dot picker which produced straight forward lines / circles. Anchored
	// around SpawnLocation so the body stays in its leash.
	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const FVector Anchor = SpawnLocation.IsNearlyZero() ? Origin : SpawnLocation;

	FVector BestLoc = FVector::ZeroVector;
	float BestErr = TNumericLimits<float>::Max();
	const float MinLegSq = WanderMinLegDistance * WanderMinLegDistance;

	for (int32 i = 0; i < 12; ++i)
	{
		FNavLocation R;
		if (!Nav->GetRandomReachablePointInRadius(Anchor, WanderRadius, R)) { continue; }
		FVector ToCandidate = R.Location - Origin;
		ToCandidate.Z = 0.f;
		if (ToCandidate.SizeSquared() < MinLegSq) { continue; } // too close, would end instantly
		const FVector Dir = ToCandidate.GetSafeNormal();
		const float Dot = FVector::DotProduct(Forward, Dir);
		const float Err = FMath::Abs(Dot - WanderTargetDot);
		if (Err < BestErr)
		{
			BestErr = Err;
			BestLoc = R.Location;
		}
	}
	if (BestErr < TNumericLimits<float>::Max())
	{
		WanderDest = BestLoc;
		return true;
	}
	return false;
}

void UZP_ShamblerBehaviorComponent::StartWanderLeg()
{
	if (AICon)
	{
		AICon->MoveToLocation(WanderDest, 100.f); // accept "close enough" so it doesn't orbit the exact point
	}
}

void UZP_ShamblerBehaviorComponent::SetSpeed(float Speed)
{
	if (Owner)
	{
		if (UCharacterMovementComponent* M = Owner->GetCharacterMovement())
		{
			M->MaxWalkSpeed = Speed;
		}
	}
}

void UZP_ShamblerBehaviorComponent::PlayOneShot(UAnimSequence* Anim)
{
	if (!Owner || !Anim) { return; }
	USkeletalMeshComponent* M = Owner->GetMesh();
	if (!M) { return; }
	// Slot-based playback: ABP_Shambler's AnimGraph now wires BS_Shambler -> Slot 'DefaultSlot' ->
	// Output Pose. PlaySlotAnimationAsDynamicMontage layers Anim on top of the locomotion via the
	// slot, with a short blend-in/blend-out. No mesh mode swap, no pose snap — the visible "jiggle"
	// between Wander/Chase and Scream/Attack/Hit is gone.
	if (UAnimInstance* AI = M->GetAnimInstance())
	{
		AI->StopSlotAnimation(0.f, FName(TEXT("DefaultSlot"))); // hard-cut any prior one-shot first
		AI->PlaySlotAnimationAsDynamicMontage(Anim, FName(TEXT("DefaultSlot")), /*BlendIn=*/0.1f, /*BlendOut=*/0.15f);
	}
}

void UZP_ShamblerBehaviorComponent::PlaySlotLoop(UAnimSequence* Anim)
{
	if (!Owner || !Anim) { return; }
	USkeletalMeshComponent* M = Owner->GetMesh();
	if (!M) { return; }
	if (UAnimInstance* AI = M->GetAnimInstance())
	{
		AI->StopSlotAnimation(0.f, FName(TEXT("DefaultSlot")));
		// IdleBlendInTime covers the CMC's natural braking ramp — long enough that velocity has
		// dropped to ~0 by the time the slot is fully visible, so no walk pose leaks through.
		AI->PlaySlotAnimationAsDynamicMontage(Anim, FName(TEXT("DefaultSlot")),
			/*BlendInTime=*/IdleBlendInTime, /*BlendOutTime=*/IdleBlendOutTime, /*InPlayRate=*/1.f,
			/*LoopCount=*/INT32_MAX, /*BlendOutTriggerTime=*/-1.f, /*InTimeToStartMontageAt=*/0.f);
	}
}

void UZP_ShamblerBehaviorComponent::StopSlotLoop()
{
	if (!Owner) { return; }
	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		if (UAnimInstance* AI = M->GetAnimInstance())
		{
			AI->StopSlotAnimation(IdleBlendOutTime, FName(TEXT("DefaultSlot")));
		}
	}
}

void UZP_ShamblerBehaviorComponent::LockIdleMovement()
{
	// Only lock if we actually arrived in the idle phase — a Scream/Chase could have interrupted
	// the IdleLockDelay window, and we must not freeze the body during combat.
	if (bDead || State != EShamblerState::Wander || bWanderMoving) { return; }
	if (UCharacterMovementComponent* CM = Owner ? Owner->GetCharacterMovement() : nullptr)
	{
		CM->StopMovementImmediately();
		CM->SetMovementMode(MOVE_None);
	}
}

void UZP_ShamblerBehaviorComponent::EnsureLocomotion()
{
	if (!Owner) { return; }
	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		if (M->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
		{
			M->SetAnimationMode(EAnimationMode::AnimationBlueprint); // re-activate ABP_Shambler locomotion
		}
		// Slot-based one-shots blend out on their own; nothing to clear here. The legacy mode-swap
		// path used to leave the mesh stuck in SingleNode if a one-shot was still mid-play, which is
		// why EnsureLocomotion existed.
	}
}

void UZP_ShamblerBehaviorComponent::UpdateLurk(float DistToPlayer)
{
	const bool bInRange = DistToPlayer <= LurkRange;
	if (!bLurkInit)
	{
		bLurkInit = true;
		bWasInLurkRange = bInRange;
		LurkInterval = FMath::FRandRange(LurkIntervalMin, LurkIntervalMax);
		return;
	}
	if (!bInRange)
	{
		bWasInLurkRange = false;
		LurkTimer = 0.f;
		return;
	}
	LurkTimer += EvalInterval;
	if (!bWasInLurkRange || LurkTimer >= LurkInterval)
	{
		// Randomly pick one of the two lurk growls each time.
		if (Audio)
		{
			const TCHAR* P = FMath::RandBool()
				? TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_LURK1.SFX_ZOMBIE_LURK1")
				: TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_LURK2.SFX_ZOMBIE_LURK2");
			Audio->LurkingLoop = LoadObject<USoundBase>(nullptr, P);
			Audio->PlayLurk();
		}
		LurkTimer = 0.f;
		LurkInterval = FMath::FRandRange(LurkIntervalMin, LurkIntervalMax);
	}
	bWasInLurkRange = true;
}

void UZP_ShamblerBehaviorComponent::OnAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("[Shambler] PROBE OnAnyDamage fired: %.0f raw (causer=%s)"),
		Damage, DamageCauser ? *DamageCauser->GetName() : TEXT("?"));
}

void UZP_ShamblerBehaviorComponent::ProbeShootable()
{
	if (bDead || !Owner || !GetWorld()) { return; }
	USkeletalMeshComponent* M = Owner->GetMesh();
	if (!M) { return; }
	// Fire a Visibility ray straight through the mesh's bounds centre. If the mesh's collision is
	// actually hittable, this hits Owner. If it sails through, the physics bodies aren't intercepting.
	const FVector C = M->Bounds.Origin;
	const FVector R = Owner->GetActorRightVector();
	FHitResult H;
	FCollisionQueryParams P;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(H, C + R * 120.f, C - R * 120.f, ECC_Visibility, P);
	UE_LOG(LogTemp, Warning, TEXT("[Shambler] PROBE selfRay hit=%d actor='%s' comp='%s'"),
		bHit ? 1 : 0,
		(bHit && H.GetActor()) ? *H.GetActor()->GetName() : TEXT("none"),
		(bHit && H.GetComponent()) ? *H.GetComponent()->GetName() : TEXT("-"));

	// Trace from the player's eye toward the Shambler — exactly what a shot does. Tells us if a door/wall
	// sits between them or if it's clean (then it's the aim/capsule size).
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		FVector EyeLoc; FRotator EyeRot;
		PC->GetPlayerViewPoint(EyeLoc, EyeRot);
		const FVector Tgt = Owner->GetActorLocation();
		FHitResult EH;
		FCollisionQueryParams EP;
		if (PC->GetPawn()) { EP.AddIgnoredActor(PC->GetPawn()); }
		const bool bEH = GetWorld()->LineTraceSingleByChannel(EH, EyeLoc, Tgt, ECC_Visibility, EP);
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] PROBE eye->shambler distToShambler=%.0f | firstBlock='%s' atDist=%.0f (shambler is the target)"),
			FVector::Dist(EyeLoc, Tgt),
			(bEH && EH.GetActor()) ? *EH.GetActor()->GetName() : TEXT("CLEAR-no block"),
			bEH ? EH.Distance : 0.f);
	}
}

void UZP_ShamblerBehaviorComponent::OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy,
	FVector HitLocation, UPrimitiveComponent* FHitComp, FName BoneName, FVector ShotDir,
	const UDamageType* DamageType, AActor* DamageCauser)
{
	if (bDead || !Health) { return; }
	// Remember which side took the (possibly lethal) hit so the death animation falls the right way:
	// a shot travelling AGAINST the zombie's forward came at its FRONT.
	bLastHitFront = FVector::DotProduct(ShotDir.GetSafeNormal(), Owner->GetActorForwardVector()) < 0.f;
	// Capsule hits carry no bone, so judge a headshot by how high up the body it landed.
	const float HitZAboveCentre = HitLocation.Z - Owner->GetActorLocation().Z;
	const bool bHead = HitZAboveCentre >= HeadshotMinZ;
	const float Dmg = bHead ? HeadShotDamage : BodyShotDamage;
	UE_LOG(LogTemp, Warning, TEXT("[Shambler] HIT z+%.0f -> %s, %.0f dmg (was %.0f HP)"),
		HitZAboveCentre, bHead ? TEXT("HEADSHOT") : TEXT("body"), Dmg, Health->CurrentHealth);
	Health->ApplyDamage(Dmg);

	// Hit flinch: only if the shot didn't kill, and we're not in the middle of a swipe (mid-attack
	// hits don't interrupt — same rule the Scytheer uses; otherwise the player can stunlock by
	// shooting through every swing). Cooldown gates retriggering so the Shambler can still walk + fight.
	if (!Health->bIsDead && State != EShamblerState::Attack)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if ((Now - LastHitReactTime) >= HitReactCooldown)
		{
			LastHitReactTime = Now;
			UAnimSequence* HitAnim = bLastHitFront ? HitFrontAnim : HitBackAnim;
			if (HitAnim) { PlayOneShot(HitAnim); }
		}
	}
}

void UZP_ShamblerBehaviorComponent::ReceiveStaggerHit(float Duration)
{
	if (bDead || !Owner) { return; }

	// Bypass HitReact cooldown — block stagger must always read visibly.
	LastHitReactTime = -1000.0;
	if (UAnimSequence* Anim = HitFrontAnim ? HitFrontAnim : HitBackAnim)
	{
		PlayOneShot(Anim);
	}

	bStaggered = true;

	// Cancel any in-flight AI move so the swipe truly pauses.
	if (APawn* P = Cast<APawn>(Owner))
	{
		if (AAIController* AI = Cast<AAIController>(P->GetController()))
		{
			AI->StopMovement();
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(StaggerHandle);
	GetWorld()->GetTimerManager().SetTimer(StaggerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bStaggered = false;
		if (!bDead) { EnsureLocomotion(); }
	}), FMath::Max(0.1f, Duration), false);
}

void UZP_ShamblerBehaviorComponent::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	// Chain the next leg only DURING the walk phase. In idle phase the body must stay stopped, so
	// re-issuing MoveTo would kick velocity back up and the idle pose would never show.
	// Stumble is an intentional break — let StumbleEnd in Tick re-issue the move.
	if (bDead || State != EShamblerState::Wander || bStumbling) { return; }
	if (!bWanderMoving) { return; } // in idle phase — do NOT chain
	if (PickNewWanderPoint())
	{
		StartWanderLeg();
	}
	// Note: do NOT reset StateTimer — the walk-phase timer must keep ticking across legs so the
	// transition to idle still fires at WalkDuration.
}

void UZP_ShamblerBehaviorComponent::OnOwnerDied()
{
	bDead = true;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(EvalTimer);
		W->GetTimerManager().ClearTimer(AttackHitTimer);
	}
	if (AICon) { AICon->StopMovement(); }
	SetSpeed(0.f);

	// Death cry — reuses the alert path with the (dev-swappable) death SFX.
	if (Audio)
	{
		Audio->AlertSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_DEATH.SFX_ZOMBIE_DEATH"));
		Audio->PlayAlert();
	}

	// Play the directional death animation and HOLD the final (corpse) frame. NOT ragdoll — physics
	// produces NaN at this mesh's 0.01 scale (the body vanished). Front-hit -> Death_Front, back-hit -> Death_Back.
	if (Owner)
	{
		if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent()) { Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
		if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement()) { CM->StopMovementImmediately(); CM->DisableMovement(); }
		UAnimSequence* DeathAnim = bLastHitFront ? DeathFrontAnim : DeathBackAnim;
		if (USkeletalMeshComponent* M = Owner->GetMesh())
		{
			if (DeathAnim) { M->PlayAnimation(DeathAnim, /*bLooping=*/false); } // single clip, holds last frame = corpse
		}
		DeathStartTime = GetWorld()->GetTimeSeconds();
		DeathAnimLen = DeathAnim ? DeathAnim->GetPlayLength() : 0.f;
		bDropping = (DeathAnim != nullptr && DeathDropZ != 0.f);
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] DEATH anim=%s hitFront=%d"),
			DeathAnim ? *DeathAnim->GetName() : TEXT("none-yet (retarget pending)"), bLastHitFront ? 1 : 0);
	}
}

// ───────────────────────── IZP_Revivable (save-state + revival) ─────────────────────────

void UZP_ShamblerBehaviorComponent::ApplyDeadStateInstant_Implementation()
{
	// Snap to the corpse pose without replaying the fall/SFX — used when a LOAD restores a dead Shambler.
	if (!Owner) { return; }
	bDead = true;
	bDropping = false; // no easing on restore — we jump straight to grounded
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(EvalTimer);
		W->GetTimerManager().ClearTimer(AttackHitTimer);
	}
	if (AICon) { AICon->StopMovement(); }
	SetSpeed(0.f);

	if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent()) { Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement()) { CM->StopMovementImmediately(); CM->DisableMovement(); }

	UAnimSequence* DeathAnim = DeathFrontAnim ? DeathFrontAnim : DeathBackAnim;
	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		if (DeathAnim)
		{
			M->PlayAnimation(DeathAnim, /*bLooping=*/false);
			M->SetPosition(DeathAnim->GetPlayLength(), /*bFireNotifies=*/false); // hold the final (corpse) frame
		}
		FVector RL = M->GetRelativeLocation();
		RL.Z = MeshBaseRelZ - DeathDropZ; // apply the full grounding drop instantly
		M->SetRelativeLocation(RL);
	}
	UE_LOG(LogTemp, Log, TEXT("[Shambler] dead-state restored on load: %s"), *Owner->GetName());
}

void UZP_ShamblerBehaviorComponent::ReviveEnemy_Implementation()
{
	// Bring a dead Shambler back to a fully alive, patrolling state.
	if (!Owner) { return; }
	bDead = false;
	bDropping = false;
	bStaggered = false;
	Target = nullptr;
	LostSightTimer = 0.f;

	if (Health) { Health->ResetHealth(); } // CurrentHealth=Max, bIsDead=false

	if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // re-assert shootability
	}
	if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
	{
		CM->SetMovementMode(MOVE_Walking);
	}

	// SetState(Wander) re-links the locomotion AnimBP (EnsureLocomotion), restores walk speed and the
	// mesh's base Z, and clears the idle/stumble bookkeeping.
	SetState(EShamblerState::Wander);

	// Restart the AI evaluation loop (cleared on death / dead-state restore).
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(EvalTimer, this, &UZP_ShamblerBehaviorComponent::Evaluate, EvalInterval, true);
	}
	UE_LOG(LogTemp, Log, TEXT("[Shambler] REVIVED: %s"), *Owner->GetName());
}
