// Copyright The Signal. All Rights Reserved.

#include "ZP_ScytheerBase.h"
#include "ZP_HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
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
		SM->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
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
	}
	bUseControllerRotationYaw = false;

	// Placeholder SFX from existing Crawler + Shambler audio. Dev swaps in editor with real audio later.
	struct FFind { static USoundBase* Get(const TCHAR* P) { ConstructorHelpers::FObjectFinder<USoundBase> F(P); return F.Succeeded() ? F.Object : nullptr; } };
	AlertSound  = FFind::Get(TEXT("/Game/Audio/Crawler/SFX_Crawler_Alert.SFX_Crawler_Alert"));
	AttackSound = FFind::Get(TEXT("/Game/Audio/Crawler/SFX_Crawler_Attack.SFX_Crawler_Attack"));
	HitSound    = FFind::Get(TEXT("/Game/Audio/Crawler/SFX_Crawler_Hit.SFX_Crawler_Hit"));
	DeathSound  = FFind::Get(TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_DEATH.SFX_ZOMBIE_DEATH"));
	LurkSound   = FFind::Get(TEXT("/Game/Audio/Crawler/SFX_Crawler_Lurking.SFX_Crawler_Lurking"));
}

void AZP_ScytheerBase::BeginPlay()
{
	Super::BeginPlay();

	AICon = Cast<AAIController>(GetController());

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

	if (bDead) { TickAnim(); return; }

	APawn* Player = GetPlayer();
	const float Dist = Player ? FVector::Dist(GetActorLocation(), Player->GetActorLocation()) : TNumericLimits<float>::Max();
	const bool bSee = Player ? HasLOS(Player) : false;

	// Detection / aggro management.
	if (!bAggro && State != EScytheerState::Die)
	{
		if (Player && Dist <= DetectionRange && bSee)
		{
			bAggro = true;
			LostSightTimer = 0.f;
			if (State != EScytheerState::Attack && State != EScytheerState::Hit)
			{
				EnterState(EScytheerState::Alert);
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
				EnterState(EScytheerState::Wander);
			}
		}
	}

	const double Now = GetWorld()->GetTimeSeconds();

	switch (State)
	{
	case EScytheerState::Wander:
	{
		// Slow leg-by-leg roam — pick a navmesh point, walk to it, pause, repeat.
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

	switch (NewState)
	{
	case EScytheerState::Wander:
		StartSegment(WalkStartFrame, WalkEndFrame, true);
		SetMaxWalkSpeed(WanderSpeed);
		bWanderMoving = false;
		PauseTimer = 0.f; // pick a leg on the next tick
		break;

	case EScytheerState::Alert:
		StartSegment(IdleStartFrame, IdleEndFrame, true);
		SetMaxWalkSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		if (AlertSound) { UGameplayStatics::PlaySoundAtLocation(this, AlertSound, GetActorLocation()); }
		break;

	case EScytheerState::Chase:
		StartSegment(RunStartFrame, RunEndFrame, true);
		SetMaxWalkSpeed(ChaseSpeed);
		break;

	case EScytheerState::Attack:
	{
		int32 SF = Attack1StartFrame, EF = Attack1EndFrame;
		if (PendingAttackVariant == 2) { SF = Attack2StartFrame; EF = Attack2EndFrame; }
		else if (PendingAttackVariant == 3) { SF = Attack3StartFrame; EF = Attack3EndFrame; }
		StartSegment(SF, EF, false);
		SetMaxWalkSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		if (AttackSound) { UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation()); }
		break;
	}

	case EScytheerState::Hit:
		StartSegment(HitStartFrame, HitEndFrame, false);
		if (HitSound) { UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation()); }
		break;

	case EScytheerState::Die:
		StartSegment(DieStartFrame, DieEndFrame, false);
		SetMaxWalkSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		if (DeathSound) { UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation()); }
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
	case EScytheerState::Attack:
	case EScytheerState::Hit:
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
	for (int32 Iter = 0; Iter < 8; ++Iter)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, P))
		{
			return true;
		}
		UPrimitiveComponent* Comp = Hit.GetComponent();
		if (Comp && Comp->GetCollisionEnabled() == ECollisionEnabled::QueryOnly)
		{
			P.AddIgnoredComponent(Comp);
			continue;
		}
		return false;
	}
	return false;
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
		if (State != EScytheerState::Attack && State != EScytheerState::Die)
		{
			EnterState(EScytheerState::Hit);
		}
	}
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
