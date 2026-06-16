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

	// Default the anim slots to the retargeted Shambler clips (overridable per-instance).
	struct FFind { static UAnimSequence* Get(const TCHAR* P) { ConstructorHelpers::FObjectFinder<UAnimSequence> F(P); return F.Succeeded() ? F.Object : nullptr; } };
	WalkAnim    = FFind::Get(TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Walk.A_Shambler_Walk"));
	IdleAnim    = FFind::Get(TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Idle.A_Shambler_Idle"));
	AttackLAnim = FFind::Get(TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_L.A_Shambler_Attack_L"));
	AttackRAnim = FFind::Get(TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_R.A_Shambler_Attack_R"));
	ScreamAnim  = FFind::Get(TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Scream.A_Shambler_Scream"));
	DeathFrontAnim = FFind::Get(TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Death_Front.A_Shambler_Death_Front"));
	DeathBackAnim  = FFind::Get(TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Death_Back.A_Shambler_Death_Back"));
}

void UZP_ShamblerBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<ACharacter>(GetOwner());
	if (!Owner) { return; }

	AICon = Cast<AAIController>(Owner->GetController());

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

		// Walk one smooth leg -> stop and pause (sometimes a long idle beat) -> slow-turn into the
		// next leg. No jerky re-pathing mid-walk.
		if (bWanderMoving)
		{
			const float DistToDest = FVector::Dist2D(Owner->GetActorLocation(), WanderDest);
			if (DistToDest < 140.f || StateTimer > 7.f)
			{
				bWanderMoving = false;
				StateTimer = 0.f;
				if (AICon) { AICon->StopMovement(); }
				PauseTime = (FMath::FRand() < LongPauseChance)
					? FMath::FRandRange(LongPauseMin, LongPauseMax)
					: FMath::FRandRange(PauseMin, PauseMax);
			}
		}
		else
		{
			if (StateTimer >= PauseTime)
			{
				PickNewWanderPoint();
				bWanderMoving = true;
				StateTimer = 0.f;
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
		PauseTime = 0.f; // pick a leg on the next eval
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

void UZP_ShamblerBehaviorComponent::PickNewWanderPoint()
{
	if (!Owner || !AICon) { return; }
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav) { return; }
	FNavLocation Result;
	if (Nav->GetRandomReachablePointInRadius(Owner->GetActorLocation(), WanderRadius, Result))
	{
		WanderDest = Result.Location;
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
	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		// Forces single-clip mode, plays once, holds the last frame. The zombie is stationary during
		// scream/attack, so there's no locomotion to blend with — EnsureLocomotion() restores the
		// AnimBP when it next wanders/chases. No AnimGraph slot required.
		M->PlayAnimation(Anim, false);
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
