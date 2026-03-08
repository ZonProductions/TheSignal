// Copyright The Signal. All Rights Reserved.

#include "ZP_GrenadeProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"

AZP_GrenadeProjectile::AZP_GrenadeProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Collision sphere — root
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(8.f);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionSphere->SetSimulatePhysics(false); // ProjectileMovement handles movement
	SetRootComponent(CollisionSphere);

	// Grenade mesh
	GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	GrenadeMesh->SetupAttachment(CollisionSphere);
	GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Load grenade mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Art/Weapons/Grenade/Mesh/SM_grenade"));
	if (MeshFinder.Succeeded())
	{
		GrenadeMesh->SetStaticMesh(MeshFinder.Object);
	}

	// Projectile movement — lobbed arc, not a rocket
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 800.f;
	ProjectileMovement->MaxSpeed = 800.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->Friction = 0.5f;
	ProjectileMovement->ProjectileGravityScale = 1.5f;

	// Load explosion Niagara system
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FXFinder(
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Effects/Particles/Explosion/NS_Grenade_Explosion"));
	if (FXFinder.Succeeded())
	{
		ExplosionFX = FXFinder.Object;
	}

	// Load explosion sound cue
	static ConstructorHelpers::FObjectFinder<USoundBase> SoundFinder(
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Sounds/Weapons/Explosions/SC_Grenade_Explosion"));
	if (SoundFinder.Succeeded())
	{
		ExplosionSound = SoundFinder.Object;
	}

	// Don't block the thrower
	InitialLifeSpan = 10.f; // Safety cleanup if explosion somehow fails
}

void AZP_GrenadeProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Ignore the instigator (player) for collision
	if (GetInstigator())
	{
		CollisionSphere->MoveIgnoreActors.Add(GetInstigator());
	}

	// Start fuse timer
	GetWorldTimerManager().SetTimer(FuseTimerHandle, this,
		&AZP_GrenadeProjectile::Explode, FuseTime, false);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] GrenadeProjectile spawned — fuse %.1fs, velocity %s"),
		FuseTime, *GetVelocity().ToString());
}

void AZP_GrenadeProjectile::Explode()
{
	const FVector Location = GetActorLocation();

	// Two-tier radial damage with falloff
	UGameplayStatics::ApplyRadialDamageWithFalloff(
		GetWorld(),
		InnerDamage,           // BaseDamage (at center)
		OuterDamage,           // MinimumDamage (at outer edge)
		Location,              // Origin
		InnerRadius,           // DamageInnerRadius (full damage)
		OuterRadius,           // DamageOuterRadius (falloff ends)
		1.f,                   // DamageFalloff exponent (linear)
		nullptr,               // DamageTypeClass
		TArray<AActor*>(),     // IgnoreActors
		this,                  // DamageCauser
		GetInstigatorController(), // InstigatedBy
		ECollisionChannel::ECC_Visibility // DamagePreventionChannel
	);

	// Debug spheres removed — explosion VFX handles visual feedback

	// Spawn explosion VFX
	if (ExplosionFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ExplosionFX, Location, FRotator::ZeroRotator,
			FVector(1.f), true, true, ENCPoolMethod::None);
	}

	// Play explosion sound
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, Location);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] GrenadeProjectile EXPLODED at %s — Inner: %.0f dmg/%.0f UU, Outer: %.0f dmg/%.0f UU"),
		*Location.ToString(), InnerDamage, InnerRadius, OuterDamage, OuterRadius);

	Destroy();
}
