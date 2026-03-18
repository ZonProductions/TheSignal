// Copyright The Signal. All Rights Reserved.

#include "ZP_KinemationComponent.h"
#include "ZP_CrawlerBehaviorComponent.h"
#include "ZP_GrenadeProjectile.h"
#include "KinemationBridge.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "ZP_GracePlayerAnimInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UZP_KinemationComponent::UZP_KinemationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_KinemationComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Auto-discover camera if not explicitly set
	if (!CameraComponent)
	{
		for (UActorComponent* Comp : Owner->GetComponents())
		{
			if (UCameraComponent* Cam = Cast<UCameraComponent>(Comp))
			{
				if (Cam->GetName() == TEXT("FirstPersonCamera"))
				{
					CameraComponent = Cam;
					break;
				}
			}
		}
	}

	// Auto-discover PlayerMesh if not explicitly set
	if (!PlayerMeshComponent)
	{
		for (UActorComponent* Comp : Owner->GetComponents())
		{
			if (USkeletalMeshComponent* SK = Cast<USkeletalMeshComponent>(Comp))
			{
				if (SK->GetName() == TEXT("PlayerMesh"))
				{
					PlayerMeshComponent = SK;
					break;
				}
			}
		}
	}

	// NOTE: Do NOT wire Kinemation here. SCS Blueprint components
	// (AC_FirstPersonCamera, AC_TacticalShooterAnimation, etc.) may not have
	// had their BeginPlay yet — their init would overwrite our wiring.
	// Call InitializeKinemation() from the owning actor's BeginPlay (after Super).

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent BeginPlay — Camera=%s, Mesh=%s (wiring deferred to InitializeKinemation)"),
		CameraComponent ? *CameraComponent->GetName() : TEXT("NONE"),
		PlayerMeshComponent ? *PlayerMeshComponent->GetName() : TEXT("NONE"));
}

void UZP_KinemationComponent::InitializeKinemation()
{
	// Wire Kinemation camera (gracefully skips if AC_FirstPersonCamera not found)
	InitKinemationCamera();

	// Wire Kinemation animation components (gracefully skips if not present)
	InitKinemationAnimation();

	// Load animation sequences for melee/throwable weapons
	MeleeSwingAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/Animations/FPS/A_FP_PipeSwing.A_FP_PipeSwing"));
	GrenadeThrowAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/Animations/FPS/A_FP_GrenadeThrow.A_FP_GrenadeThrow"));

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Anims — MeleeSwing:%s, GrenadeThrow:%s"),
		MeleeSwingAnim ? TEXT("OK") : TEXT("NOT FOUND"),
		GrenadeThrowAnim ? TEXT("OK") : TEXT("NOT FOUND"));

	// Load grenade projectile class
	if (!GrenadeProjectileClass)
	{
		GrenadeProjectileClass = AZP_GrenadeProjectile::StaticClass();
	}

	// Initialize ammo state from config
	CurrentAmmo = MagSize;

	// Spawn and equip default weapon (only if bAutoSpawnWeapon is true)
	if (bAutoSpawnWeapon)
	{
		SpawnAndEquipWeapon();
		// Apply per-weapon config (mag size, fire rate, damage, weapon type)
		// SpawnAndEquipWeapon doesn't set CurrentWeaponType — without this,
		// reload is blocked because CurrentWeaponType remains None.
		if (ActiveWeapon)
		{
			ApplyWeaponConfig(WeaponClass);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: bAutoSpawnWeapon=false — weapon spawn deferred to inventory."));
	}

	// Broadcast initial ammo state (after weapon spawn so HUD binding can catch it)
	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent InitializeKinemation — Weapon=%s, Kinemation: Cam=%s Anim=%s"),
		ActiveWeapon ? *ActiveWeapon->GetName() : TEXT("NONE"),
		TacticalCameraComp ? TEXT("OK") : TEXT("OFF"),
		TacticalAnimComp ? TEXT("OK") : TEXT("OFF"));
}

// --- Kinemation Camera ---

void UZP_KinemationComponent::InitKinemationCamera()
{
	AActor* Owner = GetOwner();

	// Auto-detect AC_FirstPersonCamera if TacticalCameraComp not explicitly set
	if (!TacticalCameraComp)
	{
		for (UActorComponent* Comp : Owner->GetComponents())
		{
			if (Comp && Comp->GetClass()->GetName().Contains(TEXT("AC_FirstPersonCamera")))
			{
				TacticalCameraComp = Comp;
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Auto-detected %s"), *Comp->GetName());
				break;
			}
		}
	}

	if (!TacticalCameraComp)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: No AC_FirstPersonCamera found — skipping."));
		return;
	}

	// Wire AC_FirstPersonCamera to our camera component
	if (CameraComponent)
	{
		FKinemationBridge::UpdateTargetCamera(TacticalCameraComp, CameraComponent);
	}

	// Wire AC_FirstPersonCamera to PlayerMesh
	if (PlayerMeshComponent)
	{
		FKinemationBridge::UpdatePlayerMesh(TacticalCameraComp, PlayerMeshComponent);

		// AC_FirstPersonCamera caches OwnerAnimInstance in its BeginPlay from GetMesh(),
		// which returns the empty inherited Mesh. Force-refresh it from our actual PlayerMesh
		// so AC_FirstPersonCamera can drive the skeleton (aim rotation, bone reads).
		FKinemationBridge::RefreshOwnerAnimInstance(TacticalCameraComp, PlayerMeshComponent);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Wired AC_FirstPersonCamera — Camera: %s, Mesh: %s"),
		CameraComponent ? *CameraComponent->GetName() : TEXT("NONE"),
		PlayerMeshComponent ? *PlayerMeshComponent->GetName() : TEXT("NONE"));
}

void UZP_KinemationComponent::TriggerCameraShake(UObject* ShakeData)
{
	if (!TacticalCameraComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KinemationComponent::TriggerCameraShake: TacticalCameraComp not set."));
		return;
	}
	FKinemationBridge::PlayCameraShake(TacticalCameraComp, ShakeData);
}

void UZP_KinemationComponent::SetTargetFOV(float NewFOV, float InterpSpeed)
{
	if (!TacticalCameraComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KinemationComponent::SetTargetFOV: TacticalCameraComp not set."));
		return;
	}
	FKinemationBridge::UpdateTargetFOV(TacticalCameraComp, NewFOV, InterpSpeed);
}

// --- Kinemation Animation Components ---

void UZP_KinemationComponent::InitKinemationAnimation()
{
	AActor* Owner = GetOwner();

	for (UActorComponent* Comp : Owner->GetComponents())
	{
		if (!Comp) continue;

		const FString ClassName = Comp->GetClass()->GetName();

		if (!TacticalAnimComp && ClassName.Contains(TEXT("AC_TacticalShooterAnimation")))
		{
			TacticalAnimComp = Comp;
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Auto-detected TacticalAnimComp: %s"), *Comp->GetName());
		}
		else if (!RecoilAnimComp && ClassName.Contains(TEXT("AC_RecoilAnimation")))
		{
			RecoilAnimComp = Comp;
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Auto-detected RecoilAnimComp: %s"), *Comp->GetName());
		}
		else if (!IKMotionComp && ClassName.Contains(TEXT("AC_IKMotionPlayer")))
		{
			IKMotionComp = Comp;
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Auto-detected IKMotionComp: %s"), *Comp->GetName());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: TacticalAnim=%s, Recoil=%s, IK=%s"),
		TacticalAnimComp ? TEXT("OK") : TEXT("NONE"),
		RecoilAnimComp ? TEXT("OK") : TEXT("NONE"),
		IKMotionComp ? TEXT("OK") : TEXT("NONE"));
}

// --- Weapon ---

void UZP_KinemationComponent::SpawnAndEquipWeapon()
{
	if (!WeaponClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: No WeaponClass set — skipping."));
		return;
	}

	if (!PlayerMeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KinemationComponent: PlayerMesh is null — cannot attach weapon."));
		return;
	}

	AActor* Owner = GetOwner();

	// Spawn weapon with Owner = the character (not this component)
	// Weapon's BeginPlay uses GetOwner() → GetComponentByClass() to find Kinemation components
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ActiveWeapon = GetWorld()->SpawnActor<AActor>(WeaponClass, FTransform::Identity, SpawnParams);
	if (!ActiveWeapon)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] KinemationComponent: Failed to spawn weapon!"));
		return;
	}

	// Attach weapon to PlayerMesh at the gun socket
	const FName WeaponSocket(TEXT("VB ik_hand_gun"));
	ActiveWeapon->AttachToComponent(PlayerMeshComponent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Spawned %s, attached to %s at socket %s"),
		*ActiveWeapon->GetName(), *PlayerMeshComponent->GetName(), *WeaponSocket.ToString());

	// Feed weapon's view settings to AC_TacticalShooterAnimation
	if (TacticalAnimComp)
	{
		UObject* Settings = FKinemationBridge::WeaponGetSettings(ActiveWeapon);
		if (Settings)
		{
			FKinemationBridge::AnimSetActiveSettings(TacticalAnimComp, Settings);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Set ActiveSettings to %s"), *Settings->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KinemationComponent: WeaponGetSettings returned null"));
		}
	}

	// Defer weapon draw to next tick — weapon internals + AnimInstance
	// need one frame to fully initialize after SpawnActor
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		if (ActiveWeapon)
		{
			FKinemationBridge::WeaponDraw(ActiveWeapon);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: WeaponDraw called (deferred)."));
		}
	});
}

// --- Equip / Unequip ---

bool UZP_KinemationComponent::EquipWeapon()
{
	if (ActiveWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KinemationComponent::EquipWeapon — weapon already active, ignoring."));
		return false;
	}

	SpawnAndEquipWeapon();

	if (ActiveWeapon)
	{
		ApplyWeaponConfig(WeaponClass);
		OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);
		OnWeaponTypeChanged.Broadcast(CurrentWeaponType);
		OnWeaponChanged.Broadcast(ActiveWeapon);
		return true;
	}
	return false;
}

bool UZP_KinemationComponent::EquipWeaponClass(TSubclassOf<UObject> NewWeaponClass)
{
	if (!NewWeaponClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KinemationComponent::EquipWeaponClass — null weapon class."));
		return false;
	}

	// Validate it's an Actor subclass (Moonville stores as TSubclassOf<UObject>)
	if (!NewWeaponClass->IsChildOf(AActor::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KinemationComponent::EquipWeaponClass — %s is not an Actor subclass."), *NewWeaponClass->GetName());
		return false;
	}

	TSubclassOf<AActor> ActorClass = *NewWeaponClass;

	// Block weapon switch during reload — animation must finish first
	if (bIsReloading)
	{
		return false;
	}

	// If same weapon type is already equipped, skip
	if (ActiveWeapon && WeaponClass == ActorClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent::EquipWeaponClass — same weapon already equipped."));
		return true;
	}

	// Save current weapon class before switching (for auto-switch-back after throwable)
	if (ActiveWeapon && CurrentWeaponType != EZP_WeaponType::Throwable)
	{
		PreviousWeaponClass = WeaponClass;
	}

	// --- Deferred weapon switch: drop arms → swap weapon off screen → raise arms ---
	// Phase 1 (0.0s): Arms drop off screen with current weapon
	// Phase 2 (0.5s): Destroy old weapon, spawn new weapon (off screen)
	// Phase 3 (1.5s): Hold off screen so player registers the change
	// Phase 4 (2.0s): Arms rise back with new weapon

	bWeaponSwitching = true;
	PendingSwapWeaponClass = ActorClass;

	// Cancel any in-progress switch timers
	GetWorld()->GetTimerManager().ClearTimer(WeaponSwitchAnimHandle);
	GetWorld()->GetTimerManager().ClearTimer(WeaponSwapDeferredHandle);
	GetWorld()->GetTimerManager().ClearTimer(WeaponSwitchRiseHandle);

	// Cancel melee/grenade timers that may still be running
	GetWorld()->GetTimerManager().ClearTimer(MeleeCooldownHandle);
	bMeleeCooldown = false;

	// Reset anim overlays — grenade/melee overlays apply bone rotations
	// that would carry over to the new weapon if not cleared
	if (USkeletalMeshComponent* PMesh = PlayerMeshComponent)
	{
		if (UZP_GracePlayerAnimInstance* AnimInst = Cast<UZP_GracePlayerAnimInstance>(PMesh->GetAnimInstance()))
		{
			AnimInst->ResetOverlays();
		}
	}

	if (TacticalAnimComp)
	{
		// Release ADS + drop to low ready for visual transition
		SetAiming(false);
		FKinemationBridge::AnimToggleReadyPose(TacticalAnimComp, false);
	}

	// Hide old weapon immediately so it disappears as arms drop
	if (ActiveWeapon)
	{
		ActiveWeapon->SetActorHiddenInGame(true);
	}

	// Phase 2: After arms are off screen, do the actual weapon swap
	GetWorld()->GetTimerManager().SetTimer(WeaponSwapDeferredHandle, [this]()
	{
		TSubclassOf<AActor> SwapClass = PendingSwapWeaponClass;
		PendingSwapWeaponClass = nullptr;

		if (!SwapClass) { bWeaponSwitching = false; return; }

		// Destroy old weapon
		if (ActiveWeapon)
		{
			ActiveWeapon->Destroy();
			ActiveWeapon = nullptr;
		}

		// Spawn new weapon
		WeaponClass = SwapClass;
		SpawnAndEquipWeapon();

		if (!ActiveWeapon) { bWeaponSwitching = false; return; }

		// Apply config + broadcast
		ApplyWeaponConfig(SwapClass);
		CurrentAmmo = MagSize;
		OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);
		OnWeaponTypeChanged.Broadcast(CurrentWeaponType);
		OnWeaponChanged.Broadcast(ActiveWeapon);

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent — swapped to %s (off screen)"), *SwapClass->GetName());

		// Hide new weapon until arms rise
		if (ActiveWeapon)
		{
			ActiveWeapon->SetActorHiddenInGame(true);
		}

		// Phase 3+4: Hold off screen, then rise with new weapon visible
		GetWorld()->GetTimerManager().SetTimer(WeaponSwitchRiseHandle, [this]()
		{
			if (ActiveWeapon)
			{
				ActiveWeapon->SetActorHiddenInGame(false);
			}
			// Only raise ready pose for melee/throwable — ranged weapons
			// use WeaponDraw which handles the pose. Calling ToggleReadyPose
			// on top of WeaponDraw creates a tilt conflict.
			if (TacticalAnimComp && CurrentWeaponType != EZP_WeaponType::Ranged)
			{
				FKinemationBridge::AnimToggleReadyPose(TacticalAnimComp, true);
			}
			bWeaponSwitching = false;
		}, 0.4f, false);

	}, 0.5f, false);

	return true;
}

void UZP_KinemationComponent::ApplyWeaponConfig(TSubclassOf<AActor> InWeaponClass)
{
	if (!InWeaponClass)
	{
		return;
	}

	const FString WeaponName = InWeaponClass->GetName();

	// Default all to ranged — overridden for melee/throwable below
	CurrentWeaponType = EZP_WeaponType::Ranged;

	if (WeaponName.Contains(TEXT("Viper")))
	{
		// WK-11 Viper — Pistol
		MagSize = 12;
		FireCooldownTime = 0.25f;
		HitscanBodyDamage = 10.f;
		HitscanWeakPointDamage = 50.f;
		ReserveAmmo = 48;
	}
	else if (WeaponName.Contains(TEXT("Herrington")))
	{
		// Herrington 11-87 — Shotgun (Police or standard)
		MagSize = 6;
		FireCooldownTime = 0.8f;
		HitscanBodyDamage = 35.f;
		HitscanWeakPointDamage = 100.f;
		ReserveAmmo = 24;
	}
	else if (WeaponName.Contains(TEXT("AK105")))
	{
		// AK-105 Carbine — Assault Rifle (128 BPM = 60/128 = 0.47s)
		MagSize = 30;
		FireCooldownTime = 0.47f;
		HitscanBodyDamage = 15.f;
		HitscanWeakPointDamage = 60.f;
		ReserveAmmo = 90;
	}
	else if (WeaponName.Contains(TEXT("TR15")))
	{
		// TR15 — Rifle
		MagSize = 20;
		FireCooldownTime = 0.15f;
		HitscanBodyDamage = 18.f;
		HitscanWeakPointDamage = 70.f;
		ReserveAmmo = 60;
	}
	else if (WeaponName.Contains(TEXT("SRM")))
	{
		// SRM-12 — Shotgun
		MagSize = 8;
		FireCooldownTime = 0.7f;
		HitscanBodyDamage = 30.f;
		HitscanWeakPointDamage = 90.f;
		ReserveAmmo = 32;
	}
	else if (WeaponName.Contains(TEXT("Pipe")))
	{
		// Pipe — Melee weapon
		CurrentWeaponType = EZP_WeaponType::Melee;
		MeleeDamage = 25.f;
		MeleeCooldown = 0.7f;
		MagSize = 0;
		CurrentAmmo = 0;
		ReserveAmmo = 0;
	}
	else if (WeaponName.Contains(TEXT("Grenade")))
	{
		// Grenade — Throwable weapon (1 throw per equip, consumed from inventory)
		CurrentWeaponType = EZP_WeaponType::Throwable;
		MagSize = 1;
		CurrentAmmo = 1;
		ReserveAmmo = 0;
	}
	else
	{
		// Unknown weapon — keep defaults
		CurrentWeaponType = EZP_WeaponType::Ranged;
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] KinemationComponent::ApplyWeaponConfig — unknown weapon '%s', using defaults."), *WeaponName);
	}

	// Throwables are one-handed (right hand only) — hide left arm on PlayerMesh
	if (PlayerMeshComponent)
	{
		if (CurrentWeaponType == EZP_WeaponType::Throwable)
		{
			PlayerMeshComponent->HideBoneByName(FName("clavicle_l"), PBO_None);
		}
		else
		{
			PlayerMeshComponent->UnHideBoneByName(FName("clavicle_l"));
		}
	}
}

void UZP_KinemationComponent::AddReserveAmmo(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	ReserveAmmo += Amount;
	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent::AddReserveAmmo — added %d rounds, reserve now %d"),
		Amount, ReserveAmmo);
}

void UZP_KinemationComponent::UnequipWeapon()
{
	if (!ActiveWeapon)
	{
		return;
	}

	// Exit ADS if active
	if (bIsAiming)
	{
		SetAiming(false);
	}

	// Cancel pending timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireCooldownHandle);
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeCooldownHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeSwingReturnHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeWindupHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeReadyStanceHandle);
		GetWorld()->GetTimerManager().ClearTimer(WeaponSwitchAnimHandle);
		GetWorld()->GetTimerManager().ClearTimer(WeaponSwapDeferredHandle);
		GetWorld()->GetTimerManager().ClearTimer(WeaponSwitchRiseHandle);
	}
	bFireCooldown = false;
	bIsReloading = false;
	bMeleeCooldown = false;
	bMeleeSwingActive = false;

	bWeaponSwitching = false;

	// Destroy the weapon actor
	ActiveWeapon->Destroy();
	ActiveWeapon = nullptr;

	// Drop arms to low ready so empty hands aren't held up at camera level
	if (TacticalAnimComp)
	{
		FKinemationBridge::AnimToggleReadyPose(TacticalAnimComp, false);
	}

	OnWeaponChanged.Broadcast(nullptr);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent::UnequipWeapon — weapon destroyed, arms lowered."));
}

// --- ADS ---

void UZP_KinemationComponent::SetAiming(bool bAiming)
{
	bIsAiming = bAiming;

	// Tell Kinemation animation system to enter/exit ADS pose
	if (TacticalAnimComp)
	{
		FKinemationBridge::AnimSetAiming(TacticalAnimComp, bAiming);
	}

	// Adjust recoil pattern for ADS (tighter when aiming)
	if (RecoilAnimComp)
	{
		FKinemationBridge::RecoilSetAiming(RecoilAnimComp, bAiming);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent::SetAiming(%s)"),
		bAiming ? TEXT("true") : TEXT("false"));
}

// --- Weapon ---

void UZP_KinemationComponent::FirePressed()
{
	// Block all fire input during weapon switch animation
	if (bWeaponSwitching) return;

	// Route to weapon-type-specific action
	if (CurrentWeaponType == EZP_WeaponType::Melee)
	{
		PerformMeleeSwing();
		return;
	}
	if (CurrentWeaponType == EZP_WeaponType::Throwable)
	{
		ThrowProjectile();
		return;
	}

	// --- Ranged (existing hitscan logic) ---
	if (bIsReloading || bFireCooldown)
	{
		return;
	}

	if (CurrentAmmo <= 0)
	{
		// Click — no ammo. Could trigger dry-fire sound here.
		return;
	}

	if (ActiveWeapon)
	{
		FKinemationBridge::WeaponOnFirePressed(ActiveWeapon);
	}

	PerformHitscan();

	CurrentAmmo--;
	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);

	// Alert all nearby creatures — gunshots are LOUD
	UZP_CrawlerBehaviorComponent::BroadcastGunshot(
		GetWorld(),
		GetOwner()->GetActorLocation(),
		8000.f, // default broadcast radius (creatures use their own GunshotAlertRadius for per-instance override)
		GetOwner()
	);

	// Semi-auto lockout — block next shot until animation cycles
	bFireCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(FireCooldownHandle, [this]()
	{
		bFireCooldown = false;
	}, FireCooldownTime, false);
}

void UZP_KinemationComponent::PerformHitscan()
{
	if (BulletDecalMaterials.Num() == 0)
	{
		return;
	}

	// Use controller's control rotation for trace direction — always matches
	// where the player is looking, regardless of camera tick ordering.
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* PC = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (!PC || !CameraComponent)
	{
		return;
	}

	const FVector Start = CameraComponent->GetComponentLocation();
	const FRotator AimRot = PC->GetControlRotation();
	const FVector End = Start + AimRot.Vector() * HitscanRange;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.AddIgnoredActor(ActiveWeapon);

	// Channel trace: hits anything that BLOCKS Visibility (walls, pawns, physics).
	// Volumes set to Overlap pass through — no more MapVolume eating bullets.
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return;
	}

	UMaterialInterface* DecalMat = BulletDecalMaterials[FMath::RandRange(0, BulletDecalMaterials.Num() - 1)];

	// Decal projects along its local X — must face INTO the surface (opposite of impact normal)
	const FRotator DecalRotation = (-Hit.ImpactNormal).Rotation();

	UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
		GetWorld(), DecalMat, DecalSize, Hit.ImpactPoint, DecalRotation, DecalLifetime);

	if (Decal)
	{
		Decal->SetFadeOut(DecalLifetime - 2.0f, 2.0f);
	}

	// Apply damage to hit actor
	if (Hit.GetActor())
	{
		const float DistFromCenter = FVector::Dist(Hit.ImpactPoint, Hit.GetActor()->GetActorLocation());
		const bool bWeakPointHit = DistFromCenter <= WeakPointRadius;
		const float Damage = bWeakPointHit ? HitscanWeakPointDamage : HitscanBodyDamage;

		UGameplayStatics::ApplyPointDamage(
			Hit.GetActor(),
			Damage,
			AimRot.Vector(),
			Hit,
			PC,
			GetOwner(),
			nullptr
		);

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Hitscan hit %s — %s (%.0f dmg, dist from center: %.0f UU)"),
			*Hit.GetActor()->GetName(),
			bWeakPointHit ? TEXT("WEAK POINT") : TEXT("body"),
			Damage, DistFromCenter);
	}
}

void UZP_KinemationComponent::FireReleased()
{
	if (ActiveWeapon)
	{
		FKinemationBridge::WeaponOnFireReleased(ActiveWeapon);
	}
}

void UZP_KinemationComponent::Reload()
{
	// Melee and throwable don't reload
	if (CurrentWeaponType != EZP_WeaponType::Ranged)
	{
		return;
	}

	if (bIsReloading || !ActiveWeapon)
	{
		return;
	}

	// Skip reload if mag is full or no reserve ammo
	if (CurrentAmmo >= MagSize || ReserveAmmo <= 0)
	{
		return;
	}

	FKinemationBridge::WeaponOnReload(ActiveWeapon);

	bIsReloading = true;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, [this]()
	{
		bIsReloading = false;

		// Transfer ammo from reserve to magazine
		const int32 AmmoNeeded = MagSize - CurrentAmmo;
		const int32 AmmoAvailable = FMath::Min(AmmoNeeded, ReserveAmmo);
		CurrentAmmo += AmmoAvailable;
		ReserveAmmo -= AmmoAvailable;
		OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);
	}, 3.0f, false);
}

// --- Melee ---

void UZP_KinemationComponent::PerformMeleeSwing()
{
	if (bMeleeCooldown)
	{
		return;
	}

	// --- Animation: ADS wind-up → strike (low ready) → return ---
	// Phase 1 (0ms):    SetAiming(true)           = arms pull BACK (wind-up)
	// Phase 2 (250ms):  SetAiming(false) + LowReady = FORWARD+DOWN (strike)
	// Phase 3 (500ms):  HighReady                  = return to idle
	bMeleeSwingActive = true;
	if (TacticalAnimComp)
	{
		SetAiming(true); // Wind-up: arms pull back
	}

	// After wind-up completes: strike outward + downward
	GetWorld()->GetTimerManager().SetTimer(MeleeWindupHandle, [this]()
	{
		if (TacticalAnimComp)
		{
			SetAiming(false); // Release = arms push OUTWARD
			FKinemationBridge::AnimToggleReadyPose(TacticalAnimComp, false); // + DOWN
		}

		// Return to high ready after strike follow-through
		GetWorld()->GetTimerManager().SetTimer(MeleeSwingReturnHandle, [this]()
		{
			if (TacticalAnimComp)
			{
				FKinemationBridge::AnimToggleReadyPose(TacticalAnimComp, true);
			}
			bMeleeSwingActive = false;
		}, 0.25f, false);
	}, 0.25f, false);

	// --- Damage: sphere sweep at click time (instant hit, animation is cosmetic) ---
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* PC = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (PC && CameraComponent)
	{
		const FVector Start = CameraComponent->GetComponentLocation();
		const FRotator AimRot = PC->GetControlRotation();
		const FVector End = Start + AimRot.Vector() * MeleeRange;

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());
		if (ActiveWeapon)
		{
			Params.AddIgnoredActor(ActiveWeapon);
		}

		FCollisionObjectQueryParams MeleeObjectParams;
		MeleeObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
		MeleeObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		MeleeObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		MeleeObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

		if (GetWorld()->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity,
			MeleeObjectParams, FCollisionShape::MakeSphere(MeleeSweepRadius), Params))
		{
			if (Hit.GetActor())
			{
				UGameplayStatics::ApplyPointDamage(
					Hit.GetActor(),
					MeleeDamage,
					AimRot.Vector(),
					Hit,
					PC,
					GetOwner(),
					nullptr
				);

				UE_LOG(LogTemp, Log, TEXT("[TheSignal] Melee hit %s — %.0f dmg"),
					*Hit.GetActor()->GetName(), MeleeDamage);
			}
		}
	}

	// Alert nearby creatures — melee is quieter than gunshots
	UZP_CrawlerBehaviorComponent::BroadcastGunshot(
		GetWorld(),
		GetOwner()->GetActorLocation(),
		2000.f,
		GetOwner()
	);

	// Start cooldown
	bMeleeCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(MeleeCooldownHandle, [this]()
	{
		bMeleeCooldown = false;
	}, MeleeCooldown, false);
}

// --- Throwable ---

void UZP_KinemationComponent::ThrowProjectile()
{
	if (CurrentAmmo <= 0)
	{
		return;
	}

	if (bFireCooldown)
	{
		return;
	}

	// Trigger bone-level throw overlay (runs post-Kinemation)
	if (PlayerMeshComponent)
	{
		if (UZP_GracePlayerAnimInstance* AnimInst = Cast<UZP_GracePlayerAnimInstance>(PlayerMeshComponent->GetAnimInstance()))
		{
			AnimInst->StartGrenadeThrow(0.4f);
		}
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* PC = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (!PC || !CameraComponent || !GrenadeProjectileClass)
	{
		return;
	}

	const FVector SpawnLoc = CameraComponent->GetComponentLocation()
		+ PC->GetControlRotation().Vector() * 100.f;
	const FRotator SpawnRot = PC->GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = OwnerPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Grenade = GetWorld()->SpawnActor<AActor>(
		GrenadeProjectileClass, SpawnLoc, SpawnRot, SpawnParams);

	if (Grenade)
	{
		// Set velocity via ProjectileMovement (it uses initial rotation as direction)
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Grenade thrown — spawned %s at %s"),
			*Grenade->GetName(), *SpawnLoc.ToString());
	}

	// Decrement ammo
	CurrentAmmo--;
	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);

	// Notify owner to remove 1 grenade from inventory
	OnThrowableConsumed.Broadcast();

	// Auto-switch back to previous weapon after a short delay (let throw anim play)
	if (CurrentAmmo <= 0 && PreviousWeaponClass)
	{
		TSubclassOf<UObject> SwitchBackClass = PreviousWeaponClass;
		GetWorld()->GetTimerManager().SetTimerForNextTick([this, SwitchBackClass]()
		{
			EquipWeaponClass(SwitchBackClass);
		});
	}

	// Cooldown to prevent rapid throwing
	bFireCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(FireCooldownHandle, [this]()
	{
		bFireCooldown = false;
	}, 0.8f, false);
}
