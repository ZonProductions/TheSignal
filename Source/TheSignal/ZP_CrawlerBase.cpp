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
	HealthComp->MaxHealth = 50.f;
	BehaviorComp = CreateDefaultSubobject<UZP_CrawlerBehaviorComponent>(TEXT("BehaviorComp"));

	// Pawn profile ignores Visibility by default — hitscan uses Visibility channel.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
}

void AZP_CrawlerBase::BeginPlay()
{
	Super::BeginPlay();

	// Force capsule to block Visibility at runtime (Pawn profile ignores it by default,
	// and placed instances may have stale CDO). Hitscan uses ECC_Visibility channel.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	// Disable BPC_Climbing — we handle all movement in ZP_CrawlerMovementComponent::PhysFlying.
	// BPC_Climbing spams Sqrt() on negative numbers every frame and conflicts with our CMC.
	{
		TArray<UActorComponent*> AllComps;
		GetComponents(AllComps);
		for (UActorComponent* Comp : AllComps)
		{
			if (Comp && Comp->GetClass()->GetName().Contains(TEXT("BPC_Climbing")))
			{
				Comp->PrimaryComponentTick.SetTickFunctionEnable(false);
				Comp->SetComponentTickEnabled(false);
			}
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

void AZP_CrawlerBase::SetClimbingEnabled(bool bEnabled)
{
	TArray<UActorComponent*> AllComps;
	GetComponents(AllComps);
	for (UActorComponent* Comp : AllComps)
	{
		if (Comp && Comp->GetClass()->GetName().Contains(TEXT("BPC_Climbing")))
		{
			FBoolProperty* EnabledProp = CastField<FBoolProperty>(
				Comp->GetClass()->FindPropertyByName(TEXT("Enabled")));
			if (EnabledProp)
			{
				EnabledProp->SetPropertyValue_InContainer(Comp, bEnabled);
			}
			return;
		}
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
 * Stops: tick, animation, timelines (breathing), timers (blink loops).
 * bRagdollLegs: if true, skeletal meshes get physics sim (for detached leg actors).
 *               if false, skeletal meshes just stop animation (for main body).
 */
static void FreezeActorHierarchy(AActor* Actor, bool bRagdollLegs = false)
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
			// Freeze in last animated pose — no ragdoll (tentacle legs lack physics bodies)
			SK->Stop();
		}

		// Stop timeline components — kills breathing (BodyMovement timeline), etc.
		if (UTimelineComponent* TL = Cast<UTimelineComponent>(Comp))
		{
			TL->Stop();
		}

		if (UChildActorComponent* ChildAC = Cast<UChildActorComponent>(Comp))
		{
			FreezeActorHierarchy(ChildAC->GetChildActor(), bRagdollLegs);
		}
	}

	// Freeze dynamically-spawned attached actors (eyes on body surface, etc.)
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors);
	for (AActor* Attached : AttachedActors)
	{
		FreezeActorHierarchy(Attached, bRagdollLegs);
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

	// 1. Disable behavior and clear its timers (eval timer is bound to component, not actor)
	if (BehaviorComp)
	{
		BehaviorComp->Deactivate();
		GetWorldTimerManager().ClearAllTimersForObject(BehaviorComp);
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

	// 5. Drop body to ground FIRST — while children are still attached, so everything moves together.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		const float OrigHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		Capsule->SetCapsuleHalfHeight(10.f);

		const FVector DeathPos = GetActorLocation();
		FVector GroundPos = DeathPos;
		GroundPos.Z -= (OrigHalfHeight - 10.f);

		FHitResult GroundHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(GroundHit, DeathPos, DeathPos - FVector(0.f, 0.f, 5000.f), ECC_Visibility, Params))
		{
			GroundPos = GroundHit.ImpactPoint + FVector(0.f, 0.f, 10.f);
		}

		SetActorLocation(GroundPos);

		FRotator DeathRot = GetActorRotation();
		DeathRot.Pitch = 0.f;
		DeathRot.Roll = 0.f;
		SetActorRotation(DeathRot);

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] %s: Death drop — from Z=%.0f to Z=%.0f (fell %.0f)"),
			*GetName(), DeathPos.Z, GroundPos.Z, DeathPos.Z - GroundPos.Z);
	}

	// 6. NOW detach all child/attached actors (legs, body, eyes) so the Pieria plugin's
	//    cleanup can't destroy them. They're already at ground level from step 5.
	TArray<AActor*> OrphanedActors;
	{
		TArray<AActor*> ChildrenToOrphan;
		GetAttachedActors(ChildrenToOrphan, false, true);
		for (AActor* Child : ChildrenToOrphan)
		{
			if (Child)
			{
				Child->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				OrphanedActors.AddUnique(Child);
			}
		}
		TArray<UChildActorComponent*> ChildActorComps;
		GetComponents(ChildActorComps);
		for (UChildActorComponent* CAC : ChildActorComps)
		{
			if (AActor* ChildActor = CAC->GetChildActor())
			{
				ChildActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				OrphanedActors.AddUnique(ChildActor);
				FreezeActorHierarchy(ChildActor);
			}
		}
		for (AActor* Child : ChildrenToOrphan)
		{
			FreezeActorHierarchy(Child);
		}
	}

	// Freeze self (body stays frozen, no ragdoll)
	FreezeActorHierarchy(this, false);

	// 7. World-scan: catch dynamically-spawned attached actors
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

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] CrawlerBase %s: Dead — frozen on ground"), *GetName());

	// 8. Destroy corpse + orphaned parts after 30 seconds
	SetLifeSpan(30.0f);
	for (AActor* Orphan : OrphanedActors)
	{
		if (Orphan)
		{
			Orphan->SetLifeSpan(30.0f);
		}
	}
}
