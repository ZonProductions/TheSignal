// Copyright The Signal. All Rights Reserved.

#include "ZP_GrenadeProjectile.h"
#include "ZP_SFXStatics.h"
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
	ProjectileMovement->InitialSpeed = AZP_GrenadeThrowSpeed;
	ProjectileMovement->MaxSpeed = AZP_GrenadeThrowSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = AZP_GrenadeBounciness;
	ProjectileMovement->Friction = AZP_GrenadeFriction;
	ProjectileMovement->ProjectileGravityScale = AZP_GrenadeGravityScale;

	// Load explosion Niagara system
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FXFinder(
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Effects/Particles/Explosion/NS_Grenade_Explosion"));
	if (FXFinder.Succeeded())
	{
		AZP_ExplosionFX = FXFinder.Object;
	}

	// Load explosion sound cue
	static ConstructorHelpers::FObjectFinder<USoundBase> SoundFinder(
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Sounds/Weapons/Explosions/SC_Grenade_Explosion"));
	if (SoundFinder.Succeeded())
	{
		AZP_ExplosionSound = SoundFinder.Object;
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
		&AZP_GrenadeProjectile::Explode, AZP_FuseTime, false);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] GrenadeProjectile spawned — fuse %.1fs, velocity %s"),
		AZP_FuseTime, *GetVelocity().ToString());
}

void AZP_GrenadeProjectile::Explode()
{
	const FVector Location = GetActorLocation();

	// Two-tier radial damage with falloff
	UGameplayStatics::ApplyRadialDamageWithFalloff(
		GetWorld(),
		AZP_InnerDamage,           // BaseDamage (at center)
		AZP_OuterDamage,           // MinimumDamage (at outer edge)
		Location,              // Origin
		AZP_InnerRadius,       // DamageInnerRadius (full damage)
		AZP_OuterRadius,       // DamageOuterRadius (falloff ends)
		1.f,                   // DamageFalloff exponent (linear)
		nullptr,               // DamageTypeClass
		TArray<AActor*>(),     // IgnoreActors
		this,                  // DamageCauser
		GetInstigatorController(), // InstigatedBy
		ECollisionChannel::ECC_Visibility // DamagePreventionChannel
	);

	// Debug spheres removed — explosion VFX handles visual feedback

	// Spawn explosion VFX
	if (AZP_ExplosionFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), AZP_ExplosionFX, Location, FRotator::ZeroRotator,
			FVector(1.f), true, true, ENCPoolMethod::None);
	}

	// Play explosion sound — Far carry (loudest world event in the game; a bare PlaySoundAtLocation
	// was at the mercy of the pack cue's internal attenuation, or silent past 40 m without one).
	UZP_SFXStatics::PlaySFXAtLocation(GetWorld(), AZP_ExplosionSound, Location, EZP_SFXCarry::Far);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] GrenadeProjectile EXPLODED at %s — Inner: %.0f dmg/%.0f UU, Outer: %.0f dmg/%.0f UU"),
		*Location.ToString(), AZP_InnerDamage, AZP_InnerRadius, AZP_OuterDamage, AZP_OuterRadius);

	Destroy();
}
