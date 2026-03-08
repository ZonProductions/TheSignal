// Copyright The Signal. All Rights Reserved.

#include "ZP_CrawlerBase.h"
#include "ZP_CrawlerMovementComponent.h"
#include "ZP_HealthComponent.h"
#include "ZP_CrawlerBehaviorComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "TimerManager.h"
#include "EngineUtils.h"

AZP_CrawlerBase::AZP_CrawlerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UZP_CrawlerMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	HealthComp = CreateDefaultSubobject<UZP_HealthComponent>(TEXT("HealthComp"));
	BehaviorComp = CreateDefaultSubobject<UZP_CrawlerBehaviorComponent>(TEXT("BehaviorComp"));
}

void AZP_CrawlerBase::BeginPlay()
{
	Super::BeginPlay();

	// Disable tick on the purchased plugin's BPC_Climbing component.
	// We handle all climbing in ZP_CrawlerMovementComponent.
	// BPC_Climbing spams Sqrt() on negative numbers every frame → FPS killer.
	TArray<UActorComponent*> AllComps;
	GetComponents(AllComps);
	for (UActorComponent* Comp : AllComps)
	{
		if (Comp && Comp->GetName().Contains(TEXT("BPC_Climbing")))
		{
			Comp->PrimaryComponentTick.SetTickFunctionEnable(false);
			Comp->SetComponentTickEnabled(false);
		}
	}

	if (HealthComp)
	{
		HealthComp->OnDied.AddDynamic(this, &AZP_CrawlerBase::OnDied);
	}

	// Apply tentacle material after Monster Randomizer completes.
	// BP_Monster_Pawn's BeginPlay has a 3s delay before Monster Randomizer runs,
	// then legs spawn and get their materials. We wait 4.5s to ensure legs exist.
	if (TentacleMaterial)
	{
		GetWorldTimerManager().SetTimer(
			MaterialSwapTimerHandle, this,
			&AZP_CrawlerBase::ApplyTentacleMaterial,
			4.5f, false
		);
	}
}

float AZP_CrawlerBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (HealthComp && HealthComp->bIsDead)
	{
		return 0.f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HealthComp)
	{
		HealthComp->ApplyDamage(ActualDamage);
	}

	return ActualDamage;
}

void AZP_CrawlerBase::ApplyTentacleMaterial()
{
	if (!TentacleMaterial)
	{
		return;
	}

	// If RuntimeRoughnessOverride > 0, create a DMI with boosted roughness to blur
	// SSR floor reflections while keeping the wet ink visual intact.
	UMaterialInterface* MatToApply = TentacleMaterial;
	if (RuntimeRoughnessOverride > 0.f)
	{
		UMaterialInstanceDynamic* DMI = UMaterialInstanceDynamic::Create(TentacleMaterial, this);
		DMI->SetScalarParameterValue(FName("Roughness"), RuntimeRoughnessOverride);
		MatToApply = DMI;
	}

	int32 SwapCount = 0;

	// Walk all attached actors (legs, body, eyes) and swap materials on skeletal + static meshes
	TArray<AActor*> AllAttached;
	GetAttachedActors(AllAttached, false, true); // bResetArray=false, bRecursivelyIncludeAttachedActors=true

	for (AActor* Attached : AllAttached)
	{
		if (!Attached)
		{
			continue;
		}

		// Skeletal meshes (tentacle legs)
		TArray<USkeletalMeshComponent*> SKMeshes;
		Attached->GetComponents(SKMeshes);
		for (USkeletalMeshComponent* SK : SKMeshes)
		{
			const int32 NumMats = SK->GetNumMaterials();
			for (int32 i = 0; i < NumMats; ++i)
			{
				SK->SetMaterial(i, MatToApply);
			}
			++SwapCount;
		}

		// Static meshes (body joints if present)
		TArray<UStaticMeshComponent*> SMMeshes;
		Attached->GetComponents(SMMeshes);
		for (UStaticMeshComponent* SM : SMMeshes)
		{
			const int32 NumMats = SM->GetNumMaterials();
			for (int32 i = 0; i < NumMats; ++i)
			{
				SM->SetMaterial(i, MatToApply);
			}
			++SwapCount;
		}
	}

	// Also swap on child actor components (body is a ChildActorComponent)
	TArray<UChildActorComponent*> ChildActors;
	GetComponents(ChildActors);
	for (UChildActorComponent* CAC : ChildActors)
	{
		if (AActor* ChildActor = CAC->GetChildActor())
		{
			TArray<USkeletalMeshComponent*> ChildSKs;
			ChildActor->GetComponents(ChildSKs);
			for (USkeletalMeshComponent* SK : ChildSKs)
			{
				const int32 NumMats = SK->GetNumMaterials();
				for (int32 i = 0; i < NumMats; ++i)
				{
					SK->SetMaterial(i, MatToApply);
				}
				++SwapCount;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] %s: Applied TentacleMaterial to %d mesh components (Roughness override: %.2f)"), *GetName(), SwapCount, RuntimeRoughnessOverride);
}

/**
 * Recursively freeze an actor and all its child/attached actors.
 * Stops: tick, animation (SK->Stop), timelines (breathing), timers (blink loops).
 * Also handles dynamically-spawned attached actors (eyes on body surface).
 */
static void FreezeActorHierarchy(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	Actor->SetActorTickEnabled(false);

	TArray<UActorComponent*> AllComponents;
	Actor->GetComponents(AllComponents);

	for (UActorComponent* Comp : AllComponents)
	{
		Comp->SetComponentTickEnabled(false);

		if (USkeletalMeshComponent* SK = Cast<USkeletalMeshComponent>(Comp))
		{
			SK->Stop();
		}

		// Stop timeline components — kills breathing (BodyMovement timeline), etc.
		if (UTimelineComponent* TL = Cast<UTimelineComponent>(Comp))
		{
			TL->Stop();
		}

		if (UChildActorComponent* ChildAC = Cast<UChildActorComponent>(Comp))
		{
			FreezeActorHierarchy(ChildAC->GetChildActor());
		}
	}

	// Freeze dynamically-spawned attached actors (eyes on body surface, etc.)
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors);
	for (AActor* Attached : AttachedActors)
	{
		FreezeActorHierarchy(Attached);
	}

	// Clear all timers on this actor — kills blink loops, IK stepping, etc.
	if (Actor->GetWorld())
	{
		Actor->GetWorldTimerManager().ClearAllTimersForObject(Actor);
	}
}

void AZP_CrawlerBase::OnDied()
{
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] CrawlerBase %s: Death sequence started"), *GetName());

	// 1. Disable behavior
	if (BehaviorComp)
	{
		BehaviorComp->Deactivate();
	}

	// 2. Stop movement
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->StopMovementImmediately();
		CMC->SetMovementMode(MOVE_None);
	}

	// 3. Detach from controller
	DetachFromControllerPendingDestroy();

	// 4. Disable capsule collision (player can walk through corpse)
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 5. Freeze entire hierarchy in place: stop timelines (breathing), timers (blink loops),
	//    tick, animation, leg IK. Everything stops where it is — no ragdoll, no detach.
	FreezeActorHierarchy(this);

	// 6. World-scan: catch ALL actors in our attachment chain.
	//    FreezeActorHierarchy walks ChildActorComponent + GetAttachedActors, but eye actors
	//    spawned dynamically by BP_MonsterBody may be missed if the plugin attaches them
	//    in a way the component tree doesn't capture. Walk every world actor's attachment
	//    parent chain — if it leads back to us, freeze it.
	int32 ExtraFrozen = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* WorldActor = *It;
		if (!WorldActor || WorldActor == this)
		{
			continue;
		}

		AActor* Parent = WorldActor->GetAttachParentActor();
		while (Parent)
		{
			if (Parent == this)
			{
				FreezeActorHierarchy(WorldActor);
				ExtraFrozen++;
				break;
			}
			Parent = Parent->GetAttachParentActor();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] %s: World-scan froze %d additional actors"), *GetName(), ExtraFrozen);

	// 7. Drop body to ground — trace down to find the floor, teleport there.
	//    Handles wall deaths: creature falls to ground instead of sticking to wall.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		const float OrigHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		Capsule->SetCapsuleHalfHeight(10.f);

		const FVector DeathPos = GetActorLocation();
		FVector GroundPos = DeathPos;
		GroundPos.Z -= (OrigHalfHeight - 10.f); // default: just shrink capsule

		// Trace down to find actual ground
		FHitResult GroundHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		const FVector TraceStart = DeathPos;
		const FVector TraceEnd = DeathPos - FVector(0.f, 0.f, 5000.f);

		if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			// Land on the ground + small offset so body doesn't clip into floor
			GroundPos = GroundHit.ImpactPoint + FVector(0.f, 0.f, 10.f);
		}

		SetActorLocation(GroundPos);

		// Reset rotation to upright (creature may be wall-tilted)
		FRotator DeathRot = GetActorRotation();
		DeathRot.Pitch = 0.f;
		DeathRot.Roll = 0.f;
		SetActorRotation(DeathRot);

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] %s: Death drop — from Z=%.0f to Z=%.0f (fell %.0f)"),
			*GetName(), DeathPos.Z, GroundPos.Z, DeathPos.Z - GroundPos.Z);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] CrawlerBase %s: Dead — frozen on ground"), *GetName());

	// 8. Destroy corpse after 30 seconds
	SetLifeSpan(30.0f);
}
