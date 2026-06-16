// Copyright The Signal. All Rights Reserved.

#include "ZP_CrawlerBehaviorComponent.h"
#include "ZP_CrawlerMovementComponent.h"
#include "ZP_EnemyAudioComponent.h"
#include "ZP_HealthComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

TArray<UZP_CrawlerBehaviorComponent*> UZP_CrawlerBehaviorComponent::AllCrawlers;

UZP_CrawlerBehaviorComponent::UZP_CrawlerBehaviorComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // event/timer driven, no tick
}

void UZP_CrawlerBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();
	AllCrawlers.AddUnique(this);

	AActor* Owner = GetOwner();
	if (!Owner) { return; }

	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		CrawlerCMC = Cast<UZP_CrawlerMovementComponent>(Character->GetCharacterMovement());
	}
	if (!CrawlerCMC)
	{
		UE_LOG(LogTemp, Error, TEXT("[Crawler] %s: no UZP_CrawlerMovementComponent — won't move."), *Owner->GetName());
	}

	EnemyAudio = NewObject<UZP_EnemyAudioComponent>(Owner, TEXT("EnemyAudioComp"));
	if (EnemyAudio) { EnemyAudio->RegisterComponent(); }

	if (UZP_HealthComponent* HC = Owner->FindComponentByClass<UZP_HealthComponent>())
	{
		HC->OnHealthChanged.AddDynamic(this, &UZP_CrawlerBehaviorComponent::OnOwnerHealthChanged);
	}

	if (bAutoInitialize)
	{
		InitializeBehavior();
	}
}

void UZP_CrawlerBehaviorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AllCrawlers.Remove(this);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvalTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UZP_CrawlerBehaviorComponent::InitializeBehavior()
{
	AActor* Owner = GetOwner();
	if (!Owner || !CrawlerCMC) { return; }

	CrawlerCMC->SetMovementMode(MOVE_Flying); // PhysFlying owns everything
	HomeLocation = Owner->GetActorLocation();
	SetSpeed(HuntSpeed);

	// Adhere to the placed wall, or — if there's no wall — just start as a ground hunter.
	if (!bSpawnOnWall || !ClingToSpawnWall())
	{
		bHasLeftPerch = true;
	}

	GetWorld()->GetTimerManager().SetTimer(EvalTimer, this,
		&UZP_CrawlerBehaviorComponent::EvaluateBehavior, EvalInterval, true);
}

// ───────────────────────────── State ─────────────────────────────

void UZP_CrawlerBehaviorComponent::SetState(ECrawlerState NewState)
{
	if (NewState == CurrentState) { return; }
	const ECrawlerState Old = CurrentState;
	CurrentState = NewState;
	OnStateChanged.Broadcast(Old, NewState);
}

void UZP_CrawlerBehaviorComponent::EvaluateBehavior()
{
	AActor* Owner = GetOwner();
	if (!Owner || !CrawlerCMC) { return; }

	if (UZP_HealthComponent* HC = Owner->FindComponentByClass<UZP_HealthComponent>())
	{
		if (HC->bIsDead) { return; }
	}

	// Resolve an in-flight attack first: when the CMC flags an impact, deal damage and settle.
	if (CrawlerCMC->HasImpacted())
	{
		CrawlerCMC->ClearImpact();
		ApplyAttackDamage();
		LastAttackTime = GetWorld()->GetTimeSeconds();
		SetState(ECrawlerState::Patrol);
	}
	if (CrawlerCMC->IsLaunching() || CrawlerCMC->IsSlamming())
	{
		return; // let the attack play out
	}

	APawn* Player = nullptr;
	float Dist = TNumericLimits<float>::Max();
	const bool bExposed = GetPlayerExposure(Player, Dist);

	if (!bHasLeftPerch)
	{
		// ───────── PERCHED ─────────
		UpdateLurkAudio(Player ? Dist : TNumericLimits<float>::Max(), /*bPerchedAndUnnoticed=*/true);

		if (bExposed && Player && Dist <= PerchDormantRange)
		{
			ChaseTargetActor = Player;

			// (1) Proximity — silent drop into a ground hunt.
			if (Dist <= DropProximity)
			{
				DropToGround();
				return;
			}

			// (2) Sustained direct stare -> hiss (muffled, cooled down) + launch within range.
			if (IsPlayerLookingAtMe(Player))
			{
				const double Now = GetWorld()->GetTimeSeconds();
				if (Now - LastGazeAlertTime >= GazeAlertCooldown)
				{
					if (EnemyAudio) { EnemyAudio->PlayAlert(GazeAlertLowPassHz); }
					LastGazeAlertTime = Now;
				}

				GazeStareTimer += EvalInterval;
				if (GazeStareTimer >= GazeProvokeTime && Dist <= GazeLaunchRange)
				{
					LaunchAttack();
					return;
				}
			}
			else
			{
				GazeStareTimer = 0.f; // the stare must be unbroken
			}
		}
		else
		{
			GazeStareTimer = 0.f; // out of range or not in the same space -> dormant
		}
		return;
	}

	// ───────── GROUNDED ─────────
	if (bExposed && Player && Dist <= DetectionRange)
	{
		ChaseTargetActor = Player;

		if (!bAlerted)
		{
			bAlerted = true;
			bFirstStrike = true;
			if (EnemyAudio) { EnemyAudio->PlayAlert(); }
		}

		CrawlerCMC->MoveTargetActor = Player;
		SetSpeed(HuntSpeed);

		const double Now = GetWorld()->GetTimeSeconds();
		if (Dist <= AttackRange && CrawlerCMC->IsOnGround() && (Now - LastAttackTime) >= AttackCooldown)
		{
			GroundSlam();
		}
	}
	else
	{
		// Lost the player — hold position and wait (no re-perch). Reset the alert so it re-hisses next time.
		CrawlerCMC->MoveTargetActor = nullptr;
		SetSpeed(0.f);
		bAlerted = false;
	}
}

// ───────────────────────────── Attacks ─────────────────────────────

void UZP_CrawlerBehaviorComponent::DropToGround()
{
	bHasLeftPerch = true;
	GazeStareTimer = 0.f;
	CrawlerCMC->ReleaseWall();                 // fall under gravity, then ground-hunt (silent — no vocal)
	CrawlerCMC->MoveTargetActor = ChaseTargetActor;
	SetSpeed(HuntSpeed);
}

void UZP_CrawlerBehaviorComponent::LaunchAttack()
{
	if (!ChaseTargetActor) { return; }
	bHasLeftPerch = true;
	GazeStareTimer = 0.f;
	SetState(ECrawlerState::Attack);
	if (EnemyAudio) { EnemyAudio->PlayAttack(/*bLunge=*/true); }
	CrawlerCMC->BeginLaunch(ChaseTargetActor->GetActorLocation());
	LastAttackTime = GetWorld()->GetTimeSeconds();
}

void UZP_CrawlerBehaviorComponent::GroundSlam()
{
	SetState(ECrawlerState::Attack);
	if (EnemyAudio) { EnemyAudio->PlayAttack(/*bLunge=*/bFirstStrike); }
	bFirstStrike = false;
	CrawlerCMC->BeginSlam(SlamHoldTime);
	LastAttackTime = GetWorld()->GetTimeSeconds();
}

void UZP_CrawlerBehaviorComponent::ApplyAttackDamage()
{
	AActor* Owner = GetOwner();
	if (!Owner || !ChaseTargetActor) { return; }

	if (UZP_HealthComponent* HC = Owner->FindComponentByClass<UZP_HealthComponent>())
	{
		if (HC->bIsDead) { return; }
	}

	// Never connect through a wall, and only within the impact radius.
	if (!HasLOSTo(ChaseTargetActor)) { return; }
	if (FVector::Dist(Owner->GetActorLocation(), ChaseTargetActor->GetActorLocation()) > AttackHitRadius) { return; }

	AController* Instigator = nullptr;
	if (APawn* OwnerPawn = Cast<APawn>(Owner)) { Instigator = OwnerPawn->GetController(); }
	UGameplayStatics::ApplyDamage(ChaseTargetActor, AttackDamage, Instigator, Owner, nullptr);
}

// ───────────────────────────── Perception ─────────────────────────────

bool UZP_CrawlerBehaviorComponent::GetPlayerExposure(APawn*& OutPlayer, float& OutDist) const
{
	OutPlayer = nullptr;
	OutDist = TNumericLimits<float>::Max();
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World) { return false; }

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) { return false; }
	APawn* Pawn = PC->GetPawn();
	if (!Pawn) { return false; }

	OutPlayer = Pawn;
	OutDist = FVector::Dist(Owner->GetActorLocation(), Pawn->GetActorLocation());

	// Trace from the player's eye to the crawler (open space at the camera end avoids the self-hit a
	// wall-clung crawler would cause if we traced FROM it). Aim a step off the wall into the room.
	FVector Cam; FRotator Rot;
	PC->GetPlayerViewPoint(Cam, Rot);
	FVector Aim = Owner->GetActorLocation() + FVector(0.f, 0.f, 30.f);
	if (CrawlerCMC && CrawlerCMC->IsClinging())
	{
		Aim += CrawlerCMC->GetWallNormal() * 60.f;
	}

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	Params.AddIgnoredActor(Pawn);
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, Cam, Aim, ECC_WorldStatic, Params);
	return !bBlocked;
}

bool UZP_CrawlerBehaviorComponent::HasLOSTo(const AActor* Target) const
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !Target) { return false; }

	const FVector Start = Owner->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	Params.AddIgnoredActor(Target);
	return !World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
}

bool UZP_CrawlerBehaviorComponent::IsPlayerLookingAtMe(const APawn* Player) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !Player) { return false; }
	APlayerController* PC = Player->GetController<APlayerController>();
	if (!PC) { return false; }

	FVector Cam; FRotator Rot;
	PC->GetPlayerViewPoint(Cam, Rot);
	const FVector ToCrawler = (Owner->GetActorLocation() - Cam).GetSafeNormal();
	return FVector::DotProduct(Rot.Vector(), ToCrawler) >= GazeThreshold; // LOS already confirmed by exposure gate
}

// ───────────────────────────── Spawn-on-wall ─────────────────────────────

bool UZP_CrawlerBehaviorComponent::TraceSpawnWall(FVector& OutWallPoint, FVector& OutWallNormal) const
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World) { return false; }

	const FVector Origin = Owner->GetActorLocation();
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	float BestDist = TNumericLimits<float>::Max();
	bool bFound = false;

	// Horizontal radial sweep at the placed Z (so the hit keeps that height). Each ray starts a little
	// BEHIND the origin so a crawler placed flush against the wall still hits its front face.
	const int32 Count = 16;
	const float BackUp = 60.f;
	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = (360.f / (float)Count) * (float)i;
		const FVector Dir(FMath::Cos(FMath::DegreesToRadians(Angle)), FMath::Sin(FMath::DegreesToRadians(Angle)), 0.f);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Origin - Dir * BackUp, Origin + Dir * SpawnWallSnapRange, ECC_WorldStatic, Params))
		{
			if (FMath::Abs(Hit.ImpactNormal.Z) < 0.5f && Hit.Distance < BestDist) // vertical surfaces only
			{
				BestDist = Hit.Distance;
				OutWallPoint = Hit.ImpactPoint;
				OutWallNormal = Hit.ImpactNormal;
				bFound = true;
			}
		}
	}
	return bFound;
}

bool UZP_CrawlerBehaviorComponent::ClingToSpawnWall()
{
	AActor* Owner = GetOwner();
	FVector WallPoint, WallNormal;
	if (!Owner || !CrawlerCMC || !TraceSpawnWall(WallPoint, WallNormal)) { return false; }

	// Orient to the wall (up = wall normal) but DON'T move the crawler — keep its capsule centered on
	// where the designer placed it, so the hitbox stays on the visible body. Offsetting the (small,
	// scaled-down) capsule off the wall is what put the hitbox away from the body -> shots missed
	// ("immune on the wall"). Freezing in place makes it shoot exactly like a grounded crawler.
	const FVector Forward = FVector::VectorPlaneProject(FVector::UpVector, WallNormal).GetSafeNormal();
	if (!Forward.IsNearlyZero())
	{
		Owner->SetActorRotation(FRotationMatrix::MakeFromXZ(Forward, WallNormal).Rotator());
	}

	CrawlerCMC->SetWallCling(WallNormal);
	HomeLocation = Owner->GetActorLocation();
	UE_LOG(LogTemp, Log, TEXT("[Crawler] %s: clung to spawn wall (N=%s)"), *Owner->GetName(), *WallNormal.ToCompactString());
	return true;
}

// ───────────────────────────── Audio / damage hooks ─────────────────────────────

void UZP_CrawlerBehaviorComponent::UpdateLurkAudio(float PlayerDist, bool bPerchedAndUnnoticed)
{
	const bool bInLurk = bPerchedAndUnnoticed && !bAlerted && PlayerDist <= LurkRange;

	if (!bLurkInit)
	{
		bLurkInit = true;
		bWasInLurkRange = bInLurk;
		LurkInterval = FMath::FRandRange(LurkIntervalMin, LurkIntervalMax);
		return; // never growl on the very first eval
	}

	if (!bInLurk)
	{
		bWasInLurkRange = false;
		LurkTimer = 0.f;
		return;
	}

	if (!bWasInLurkRange)
	{
		// Just entered the room — growl.
		if (EnemyAudio) { EnemyAudio->PlayLurk(); }
		LurkTimer = 0.f;
		LurkInterval = FMath::FRandRange(LurkIntervalMin, LurkIntervalMax);
	}
	else
	{
		LurkTimer += EvalInterval;
		if (LurkTimer >= LurkInterval)
		{
			if (EnemyAudio) { EnemyAudio->PlayLurk(); }
			LurkTimer = 0.f;
			LurkInterval = FMath::FRandRange(LurkIntervalMin, LurkIntervalMax);
		}
	}
	bWasInLurkRange = true;
}

void UZP_CrawlerBehaviorComponent::OnOwnerHealthChanged(float NewHealth, float MaxHealth, float DamageAmount)
{
	if (EnemyAudio && DamageAmount > 0.f) { EnemyAudio->PlayHit(); }
}

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

// ───────────────────────────── External API ─────────────────────────────

void UZP_CrawlerBehaviorComponent::StartHunt(AActor* Target)
{
	if (!Target) { return; }
	ChaseTargetActor = Target;
	if (!bHasLeftPerch)
	{
		DropToGround(); // commit off the wall toward the target
	}
	else
	{
		CrawlerCMC->MoveTargetActor = Target;
		SetSpeed(HuntSpeed);
	}
}

void UZP_CrawlerBehaviorComponent::OnHearGunshot(const FVector& NoiseLocation, AActor* Shooter)
{
	AActor* Owner = GetOwner();
	if (!Owner) { return; }
	if (UZP_HealthComponent* HC = Owner->FindComponentByClass<UZP_HealthComponent>())
	{
		if (HC->bIsDead) { return; }
	}
	if (Shooter) { StartHunt(Shooter); }
}

void UZP_CrawlerBehaviorComponent::BroadcastGunshot(UWorld* World, const FVector& Location, float Radius, AActor* Shooter)
{
	if (!World) { return; }
	for (int32 i = AllCrawlers.Num() - 1; i >= 0; --i)
	{
		UZP_CrawlerBehaviorComponent* Crawler = AllCrawlers[i];
		if (!Crawler || !IsValid(Crawler))
		{
			AllCrawlers.RemoveAt(i);
			continue;
		}
		AActor* Owner = Crawler->GetOwner();
		if (!Owner) { continue; }

		// Cap at the smaller of the broadcast radius and the crawler's own alert range (<= 50m).
		const FVector CrawlerLoc = Owner->GetActorLocation();
		const float Reach = FMath::Min(Radius, Crawler->GunshotAlertRadius);
		if (FVector::Dist(CrawlerLoc, Location) > Reach) { continue; }

		// SAME GEOMETRY ONLY: a wall between the gunshot and the crawler blocks the alert (no waking the
		// next room). Traces the crawler-surface channel — structural walls block it, furniture doesn't,
		// so a desk in the same room won't swallow the shot.
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Owner);
		if (Shooter) { Params.AddIgnoredActor(Shooter); }
		if (World->LineTraceSingleByChannel(Hit, Location, CrawlerLoc, ECC_GameTraceChannel1, Params)) { continue; }

		Crawler->OnHearGunshot(Location, Shooter);
	}
}
