// Copyright The Signal. All Rights Reserved.

#include "ZP_KinemationComponent.h"
#include "ZP_CrawlerBehaviorComponent.h"
#include "ZP_GrenadeProjectile.h"
#include "KinemationBridge.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
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

// NEVER move PlayerMesh to animate weapon transitions — the first-person
// camera is socketed to it (FPCamera), so any Z offset moves the VIEW and
// reads as a crouch. Proven the hard way, session 63.

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

	// Auto-discover melee view mesh (TICKET-054)
	if (!MeleeViewMeshComponent)
	{
		for (UActorComponent* Comp : Owner->GetComponents())
		{
			if (USkeletalMeshComponent* SK = Cast<USkeletalMeshComponent>(Comp))
			{
				if (SK->GetName() == TEXT("MeleeViewMesh"))
				{
					MeleeViewMeshComponent = SK;
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

	// Load animation sequences for melee/throwable weapons.
	// Melee: Kubold Longsword set RETARGETED onto the Operator skeleton
	// (Scripts/Python/retarget_melee_anims.py). ONE BODY — the view model
	// wears the same SKM_Operator_Mono the player sees with every weapon.
	// Plays on the dedicated MeleeViewMesh (TICKET-054), never on PlayerMesh.
	// Attack_F dropped from the cycle (dev call): it's a longsword forward
	// stab and reads wrong with a pipe. R/L swings only.
	MeleeLightAnims.Reset();
	for (const FString& Name : { FString(TEXT("A_MeleePipe_Attack_R")),
	                             FString(TEXT("A_MeleePipe_Attack_L")) })
	{
		const FString Path = FString::Printf(TEXT("/Game/TheSignal/Animations/Melee/%s.%s"), *Name, *Name);
		if (UAnimSequenceBase* Anim = LoadObject<UAnimSequenceBase>(nullptr, *Path))
		{
			MeleeLightAnims.Add(Anim);
		}
	}
	MeleeIdleAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle.A_MeleePipe_Idle"));
	MeleeEquipAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Equip.A_MeleePipe_Equip"));
	MeleeUnequipAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Unequip.A_MeleePipe_Unequip"));
	GrenadeThrowAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/Animations/FPS/AM_FP_GrenadeThrow.AM_FP_GrenadeThrow"));

	// View-model mesh: the SAME Operator body PlayerMesh wears — identical
	// arms, identical materials. Legs hidden: never visible from the FP camera.
	if (MeleeViewMeshComponent && !MeleeViewMeshComponent->GetSkeletalMeshAsset())
	{
		if (USkeletalMesh* ViewMesh = LoadObject<USkeletalMesh>(nullptr,
			TEXT("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono.SKM_Operator_Mono")))
		{
			MeleeViewMeshComponent->SetSkeletalMesh(ViewMesh);
			MeleeViewMeshComponent->HideBoneByName(FName("thigh_l"), PBO_None);
			MeleeViewMeshComponent->HideBoneByName(FName("thigh_r"), PBO_None);
			// The Operator's face geometry sits at camera height on the view
			// model — without this the player sees inside their own head.
			MeleeViewMeshComponent->HideBoneByName(FName("head"), PBO_None);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Anims — MeleeLight:%d, MeleeIdle:%s, MeleeEquip:%s, MeleeUnequip:%s, GrenadeThrow:%s, ViewMesh:%s"),
		MeleeLightAnims.Num(),
		MeleeIdleAnim ? TEXT("OK") : TEXT("NOT FOUND"),
		MeleeEquipAnim ? TEXT("OK") : TEXT("NOT FOUND"),
		MeleeUnequipAnim ? TEXT("OK") : TEXT("NOT FOUND"),
		GrenadeThrowAnim ? TEXT("OK") : TEXT("NOT FOUND"),
		(MeleeViewMeshComponent && MeleeViewMeshComponent->GetSkeletalMeshAsset()) ? TEXT("OK") : TEXT("NOT FOUND"));

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

	// The player starts UNARMED, so no weapon view-settings are applied and
	// AC_TacticalShooterAnimation.ActiveSettings stays null. Its ComputeAimSway/
	// MoveSway run every frame and dereference that null → ~3 "Accessed None" BP
	// runtime errors PER FRAME → a 50MB+ log → PIE that's fine at first and
	// progressively laggier. Disable the component's tick while unarmed; it's
	// re-enabled the moment a weapon's settings are applied (SpawnAndEquipWeapon).
	if (TacticalAnimComp)
	{
		TacticalAnimComp->SetComponentTickEnabled(false);
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
			// Settings are now valid — re-enable the tactical anim tick (disabled
			// while unarmed to avoid per-frame null-ActiveSettings BP errors).
			TacticalAnimComp->SetComponentTickEnabled(true);
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

	// --- Traditional Kinemation swap (session 63): the new weapon's Draw
	// montage IS the transition, exactly like the pack's demo character —
	// set ActiveSettings, call Draw, done. Replaces the old 4-phase
	// drop/hold/raise hack (ToggleReadyPose lower + off-screen timers).

	bWeaponSwitching = true;

	// Cancel any in-progress switch/melee timers
	GetWorld()->GetTimerManager().ClearTimer(WeaponSwitchAnimHandle);
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

	// Release ADS — the draw montage starts from the hip pose
	if (TacticalAnimComp)
	{
		SetAiming(false);
	}

	// Leaving melee: pop the view model off instantly — the incoming
	// weapon's draw animation covers the transition
	if (bMeleeViewModelActive)
	{
		DeactivateMeleeViewModel(false);
	}

	// Swap immediately — the transition is the Draw montage (ranged) or the
	// Kubold view-model raise (melee AND throwable, via ApplyWeaponConfig)
	SetCameraBonePinned(true); // hide head region through the draw animation
	GetWorld()->GetTimerManager().ClearTimer(HeadHideHandle);
	if (!PerformWeaponSwap(ActorClass))
	{
		bWeaponSwitching = false;
		SetCameraBonePinned(false);
		return false;
	}

	// Release the fire lock once the draw animation lands. The head hide runs
	// on its OWN longer timer — draw montages outlast the fire lock, and
	// unhiding early flashes the misaligned head (dev-caught).
	GetWorld()->GetTimerManager().SetTimer(WeaponSwitchAnimHandle, [this]()
	{
		bWeaponSwitching = false;
	}, WeaponDrawLockTime, false);
	GetWorld()->GetTimerManager().SetTimer(HeadHideHandle, [this]()
	{
		SetCameraBonePinned(false);
	}, SwapHeadHideTime, false);

	return true;
}

bool UZP_KinemationComponent::PerformWeaponSwap(TSubclassOf<AActor> ActorClass)
{
	if (ActiveWeapon)
	{
		ActiveWeapon->Destroy();
		ActiveWeapon = nullptr;
	}

	WeaponClass = ActorClass;
	SpawnAndEquipWeapon(); // sets ActiveSettings + plays the Draw montage (deferred one tick)

	if (!ActiveWeapon)
	{
		return false;
	}

	ApplyWeaponConfig(ActorClass); // melee raises its Kubold view model in here
	CurrentAmmo = MagSize;
	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);
	OnWeaponTypeChanged.Broadcast(CurrentWeaponType);
	OnWeaponChanged.Broadcast(ActiveWeapon);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent — swapped to %s"), *ActorClass->GetName());
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
	bShellReload = false;

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
		bShellReload = true;
	}
	else if (WeaponName.Contains(TEXT("AK105")))
	{
		// AK-105 Carbine — Assault Rifle. Semi-auto: limited by trigger pull,
		// not cycling — 0.47s read as ~1 shot/sec in play (dev call).
		MagSize = 30;
		FireCooldownTime = 0.15f;
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
		bShellReload = true;
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

	// HUD weapon icon — finer than CurrentWeaponType (Rifle vs Shotgun are both
	// Ranged). Derived from the weapon name; default Rifle covers AK105/TR15 and
	// any unknown ranged weapon (bullets icon).
	// Melee/throwable keyed off the resolved type (reliable); ranged keyed off
	// the weapon name (Rifle vs Shotgun can't be told apart by type).
	if (CurrentWeaponType == EZP_WeaponType::Throwable)
	{
		CurrentWeaponIcon = EZP_WeaponIcon::Grenade;
	}
	else if (CurrentWeaponType == EZP_WeaponType::Melee)
	{
		CurrentWeaponIcon = EZP_WeaponIcon::Pipe;
	}
	else if (WeaponName.Contains(TEXT("Viper")))
	{
		CurrentWeaponIcon = EZP_WeaponIcon::Pistol;
	}
	else if (WeaponName.Contains(TEXT("Herrington")) || WeaponName.Contains(TEXT("SRM")))
	{
		CurrentWeaponIcon = EZP_WeaponIcon::Shotgun;
	}
	else
	{
		CurrentWeaponIcon = EZP_WeaponIcon::Rifle;
	}
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] WeaponIcon: '%s' -> icon %d (type %d)"),
		*WeaponName, (int32)CurrentWeaponIcon, (int32)CurrentWeaponType);
	OnWeaponIconChanged.Broadcast(CurrentWeaponIcon);

	// View model lifecycle (TICKET-054): the Kubold mesh replaces PlayerMesh
	// arms while a melee weapon (both arms + pipe) or a throwable (right arm
	// + grenade in the same fist) is held.
	if (CurrentWeaponType == EZP_WeaponType::Melee)
	{
		ActivateMeleeViewModel();
	}
	else if (CurrentWeaponType == EZP_WeaponType::Throwable)
	{
		ActivateThrowableViewModel();
	}
	else
	{
		// No-op when not active. During a swap away from the view model,
		// EquipWeaponClass already popped it off.
		DeactivateMeleeViewModel(false);
	}

	// PlayerMesh arm visibility — ranged weapons show both Kinemation arms;
	// melee/throwable hide them inside their view-model activation.
	if (PlayerMeshComponent
		&& CurrentWeaponType != EZP_WeaponType::Melee
		&& CurrentWeaponType != EZP_WeaponType::Throwable)
	{
		PlayerMeshComponent->UnHideBoneByName(FName("clavicle_r"));
		PlayerMeshComponent->UnHideBoneByName(FName("clavicle_l"));
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

	// Retire the melee view model immediately (no lower animation on hard unequip)
	DeactivateMeleeViewModel(false);

	// Cancel pending timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireCooldownHandle);
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeCooldownHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeSwingReturnHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeWindupHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeReadyStanceHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeEquipIdleHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeDamageHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeUnequipHideHandle);
		GetWorld()->GetTimerManager().ClearTimer(WeaponSwitchAnimHandle);
		GetWorld()->GetTimerManager().ClearTimer(HeadHideHandle);
	}
	bFireCooldown = false;
	bIsReloading = false;
	bMeleeCooldown = false;
	bMeleeSwingActive = false;

	bWeaponSwitching = false;
	SetCameraBonePinned(false);

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
		// Swing fires immediately on press (heavy/hold mechanic removed by design)
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

	// Channel trace, but SKIP QueryOnly trigger volumes (door interaction boxes, overlap zones). They
	// BLOCK Visibility yet aren't solid — they were eating bullets near doorways, so the shot never
	// reached the enemy behind them. Re-trace past each trigger until we hit real geometry / a pawn.
	bool bGotSolidHit = false;
	for (int32 TraceIter = 0; TraceIter < 8; ++TraceIter)
	{
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			return; // nothing solid on the line
		}
		UPrimitiveComponent* HitComp = Hit.GetComponent();
		if (HitComp && HitComp->GetCollisionEnabled() == ECollisionEnabled::QueryOnly)
		{
			Params.AddIgnoredComponent(HitComp); // trigger — pass through it
			continue;
		}
		bGotSolidHit = true;
		break;
	}
	if (!bGotSolidHit)
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

	// Shell loaders animate per shell — lock/refill/head-hide must run for
	// the REAL duration or the head pops back mid-animation (dev-caught on
	// the shotgun: ~9s full reload vs the old flat 3s).
	float ThisReloadTime = ReloadTime;
	if (bShellReload)
	{
		const int32 Shells = FMath::Min(MagSize - CurrentAmmo, ReserveAmmo);
		const float StartTime = (CurrentAmmo == 0) ? ShellReloadEmptyStartTime : ShellReloadTacStartTime;
		ThisReloadTime = StartTime + Shells * ShellReloadLoopTime + ShellReloadEndTime;
	}

	bIsReloading = true;
	SetCameraBonePinned(true); // reload montage animates the camera's neck bone
	GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, [this]()
	{
		bIsReloading = false;
		SetCameraBonePinned(false);

		// Transfer ammo from reserve to magazine
		const int32 AmmoNeeded = MagSize - CurrentAmmo;
		const int32 AmmoAvailable = FMath::Min(AmmoNeeded, ReserveAmmo);
		CurrentAmmo += AmmoAvailable;
		ReserveAmmo -= AmmoAvailable;
		OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);
	}, ThisReloadTime, false);
}

// --- Melee ---

void UZP_KinemationComponent::PerformMeleeSwing()
{
	if (bMeleeCooldown)
	{
		return;
	}

	// --- Pick the swing: cycles F → R → L (variety is dev-approved) ---
	UAnimSequenceBase* SwingAnim = nullptr;
	if (MeleeLightAnims.Num() > 0)
	{
		SwingAnim = MeleeLightAnims[MeleeLightAnimIndex % MeleeLightAnims.Num()];
		++MeleeLightAnimIndex;
	}

	// --- Animation: retargeted Kubold swing on the view model (TICKET-054) ---
	float SwingDuration = 0.7f; // fallback if anim missing
	if (bMeleeViewModelActive && MeleeViewMeshComponent && SwingAnim)
	{
		PlayMeleeViewAnim(SwingAnim, false, MeleeSwingRate);
		SwingDuration = SwingAnim->GetPlayLength() / FMath::Max(MeleeSwingRate, 0.1f);

		bMeleeSwingActive = true;
		GetWorld()->GetTimerManager().SetTimer(MeleeSwingReturnHandle, [this]()
		{
			if (bMeleeViewModelActive && MeleeIdleAnim)
			{
				PlayMeleeViewAnim(MeleeIdleAnim, true, 1.0f);
			}
			bMeleeSwingActive = false;
		}, SwingDuration, false);
	}
	else
	{
		// Fail loud — swing still does damage, but the visual is missing.
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] PerformMeleeSwing — no view model visual (active=%d, anim=%s)"),
			bMeleeViewModelActive ? 1 : 0, SwingAnim ? TEXT("OK") : TEXT("NULL"));
	}

	// --- Damage: sweep on the impact frame, not at click time ---
	GetWorld()->GetTimerManager().SetTimer(MeleeDamageHandle, [this]()
	{
		DoMeleeDamageSweep();
	}, MeleeDamageDelay, false);

	// Cooldown covers the full swing so a re-click can't restart mid-animation
	bMeleeCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(MeleeCooldownHandle, [this]()
	{
		bMeleeCooldown = false;
	}, FMath::Max(MeleeCooldown, SwingDuration), false);
}

void UZP_KinemationComponent::DoMeleeDamageSweep()
{
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

	// Alert nearby creatures — impact noise, quieter than gunshots
	UZP_CrawlerBehaviorComponent::BroadcastGunshot(
		GetWorld(),
		GetOwner()->GetActorLocation(),
		2000.f,
		GetOwner()
	);
}

// --- Melee view model (TICKET-054) ---

void UZP_KinemationComponent::ActivateMeleeViewModel()
{
	if (bMeleeViewModelActive)
	{
		return;
	}
	if (!MeleeViewMeshComponent || !MeleeViewMeshComponent->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ActivateMeleeViewModel — MeleeViewMesh missing or has no skeletal mesh."));
		return;
	}
	bMeleeViewModelActive = true;
	GetWorld()->GetTimerManager().ClearTimer(MeleeUnequipHideHandle);

	// Kinemation arms off — the view model IS the arms while melee is held.
	// Same HideBoneByName pattern as the throwable one-handed clavicle hide.
	if (PlayerMeshComponent)
	{
		PlayerMeshComponent->HideBoneByName(FName("clavicle_l"), PBO_None);
		PlayerMeshComponent->HideBoneByName(FName("clavicle_r"), PBO_None);
	}

	// The Moonville pipe actor stays alive for inventory bookkeeping but is
	// never rendered — its hand attachment collapses once clavicles are hidden.
	if (ActiveWeapon)
	{
		ActiveWeapon->SetActorHiddenInGame(true);
	}

	// Pipe rigid in the right hand — Kubold convention ("parent weapons
	// directly to hands"), proven in Reliquary. The anims are authored so the
	// weapon follows hand_r; left hand tracks the weapon by authoring.
	if (!MeleeWeaponMeshComp)
	{
		MeleeWeaponMeshComp = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("MeleeViewWeapon"));
		MeleeWeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeleeWeaponMeshComp->SetOnlyOwnerSee(true);
		MeleeWeaponMeshComp->bCastDynamicShadow = false;
		MeleeWeaponMeshComp->CastShadow = false;
		MeleeWeaponMeshComp->RegisterComponent();
		if (UStaticMesh* PipeMesh = LoadObject<UStaticMesh>(nullptr,
			TEXT("/Game/InventorySystemPro/ExampleContent/Common/Art/Pipe/SM_Pipe.SM_Pipe")))
		{
			MeleeWeaponMeshComp->SetStaticMesh(PipeMesh);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ActivateMeleeViewModel — SM_Pipe not found, view model swings empty-handed."));
		}
	}
	MeleeWeaponMeshComp->AttachToComponent(MeleeViewMeshComponent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_r"));
	MeleeWeaponMeshComp->SetRelativeLocation(MeleeGripOffset);
	MeleeWeaponMeshComp->SetRelativeRotation(MeleeGripRotation);
	MeleeWeaponMeshComp->SetVisibility(true);

	MeleeViewMeshComponent->SetVisibility(true);

	// Raise: Kubold Equip → Idle loop. Swing is blocked until the raise lands.
	if (MeleeEquipAnim)
	{
		PlayMeleeViewAnim(MeleeEquipAnim, false, MeleeEquipRate);
		const float EquipTime = MeleeEquipAnim->GetPlayLength() / FMath::Max(MeleeEquipRate, 0.1f);

		GetWorld()->GetTimerManager().SetTimer(MeleeEquipIdleHandle, [this]()
		{
			if (bMeleeViewModelActive && MeleeIdleAnim)
			{
				PlayMeleeViewAnim(MeleeIdleAnim, true, 1.0f);
			}
		}, EquipTime, false);

		bMeleeCooldown = true;
		GetWorld()->GetTimerManager().SetTimer(MeleeCooldownHandle, [this]()
		{
			bMeleeCooldown = false;
		}, EquipTime, false);
	}
	else if (MeleeIdleAnim)
	{
		PlayMeleeViewAnim(MeleeIdleAnim, true, 1.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Melee view model ACTIVE (Kubold FPP)"));
}

void UZP_KinemationComponent::ActivateThrowableViewModel()
{
	// "One half of the pipe animation" (dev design, session 63): the Kubold
	// view model's RIGHT arm holds the grenade in its fist — the same fist
	// grip the pipe uses — while the left arm is hidden. Reuses the melee
	// view-model lifecycle so every existing teardown path applies.
	if (bMeleeViewModelActive)
	{
		return;
	}
	if (!MeleeViewMeshComponent || !MeleeViewMeshComponent->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ActivateThrowableViewModel — MeleeViewMesh missing or has no skeletal mesh."));
		return;
	}
	bMeleeViewModelActive = true;
	bViewModelThrowable = true;
	GetWorld()->GetTimerManager().ClearTimer(MeleeUnequipHideHandle);

	// Kinemation arms off — the view model IS the arm while the grenade is held
	if (PlayerMeshComponent)
	{
		PlayerMeshComponent->HideBoneByName(FName("clavicle_l"), PBO_None);
		PlayerMeshComponent->HideBoneByName(FName("clavicle_r"), PBO_None);
	}

	// Right arm only — hide the view model's left arm
	MeleeViewMeshComponent->HideBoneByName(FName("clavicle_l"), PBO_None);

	// No pipe — the grenade is the held prop
	if (MeleeWeaponMeshComp)
	{
		MeleeWeaponMeshComp->SetVisibility(false);
	}
	if (ActiveWeapon)
	{
		ActiveWeapon->AttachToComponent(MeleeViewMeshComponent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_r"));
		ActiveWeapon->SetActorRelativeLocation(ThrowableGripOffset);
		ActiveWeapon->SetActorRelativeRotation(ThrowableGripRotation);
		ActiveWeapon->SetActorRelativeScale3D(ThrowableGripScale);
		ActiveWeapon->SetActorHiddenInGame(false);
	}

	MeleeViewMeshComponent->SetVisibility(true);

	// Only the back half of the Kubold Equip — the first half mimes drawing
	// a pipe (a remnant, no pipe in hand); the simple rise from below is the
	// grenade-out animation (dev call)
	if (MeleeEquipAnim)
	{
		PlayMeleeViewAnim(MeleeEquipAnim, false, MeleeEquipRate);
		const float StartTime = MeleeEquipAnim->GetPlayLength() * ThrowableEquipStartFraction;
		MeleeViewMeshComponent->SetPosition(StartTime, false);
		const float EquipTime = (MeleeEquipAnim->GetPlayLength() - StartTime) / FMath::Max(MeleeEquipRate, 0.1f);
		GetWorld()->GetTimerManager().SetTimer(MeleeEquipIdleHandle, [this]()
		{
			if (bMeleeViewModelActive && MeleeIdleAnim)
			{
				PlayMeleeViewAnim(MeleeIdleAnim, true, 1.0f);
			}
		}, EquipTime, false);
	}
	else if (MeleeIdleAnim)
	{
		PlayMeleeViewAnim(MeleeIdleAnim, true, 1.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Throwable view model ACTIVE (Kubold right fist)"));
}

void UZP_KinemationComponent::DeactivateMeleeViewModel(bool bPlayUnequip)
{
	if (!bMeleeViewModelActive)
	{
		return;
	}
	bMeleeViewModelActive = false;

	// Restore the view model's left arm for the next melee equip
	if (bViewModelThrowable)
	{
		bViewModelThrowable = false;
		if (MeleeViewMeshComponent)
		{
			MeleeViewMeshComponent->UnHideBoneByName(FName("clavicle_l"));
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(MeleeEquipIdleHandle);
	GetWorld()->GetTimerManager().ClearTimer(MeleeDamageHandle);
	GetWorld()->GetTimerManager().ClearTimer(MeleeSwingReturnHandle);
	bMeleeSwingActive = false;

	if (MeleeViewMeshComponent && bPlayUnequip && MeleeUnequipAnim)
	{
		// Lower the pipe through the swap drop window; hide at the off-screen
		// moment (0.5s — matches EquipWeaponClass phase 2). Clavicle restore is
		// handled by ApplyWeaponConfig when the new weapon's config lands.
		PlayMeleeViewAnim(MeleeUnequipAnim, false, MeleeUnequipRate);
		GetWorld()->GetTimerManager().SetTimer(MeleeUnequipHideHandle, [this]()
		{
			if (!bMeleeViewModelActive && MeleeViewMeshComponent)
			{
				MeleeViewMeshComponent->SetVisibility(false);
				if (MeleeWeaponMeshComp)
				{
					MeleeWeaponMeshComp->SetVisibility(false);
				}
			}
		}, 0.5f, false);
	}
	else
	{
		// Hard deactivate: hide now, restore Kinemation arms now.
		if (MeleeViewMeshComponent)
		{
			MeleeViewMeshComponent->SetVisibility(false);
		}
		if (MeleeWeaponMeshComp)
		{
			MeleeWeaponMeshComp->SetVisibility(false);
		}
		if (PlayerMeshComponent)
		{
			PlayerMeshComponent->UnHideBoneByName(FName("clavicle_l"));
			PlayerMeshComponent->UnHideBoneByName(FName("clavicle_r"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Melee view model INACTIVE (unequip anim: %s)"),
		bPlayUnequip ? TEXT("yes") : TEXT("no"));
}

void UZP_KinemationComponent::PlayMeleeViewAnim(UAnimSequenceBase* Anim, bool bLoop, float Rate)
{
	if (!MeleeViewMeshComponent || !Anim)
	{
		return;
	}
	MeleeViewMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MeleeViewMeshComponent->PlayAnimation(Anim, bLoop);
	MeleeViewMeshComponent->SetPlayRate(Rate);
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

	// No throw gesture (dev call): the swing anims read as sword moves. The
	// hand stays in the idle hold and the grenade just flies — same look the
	// last-grenade swap produces.

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

void UZP_KinemationComponent::SetCameraBonePinned(bool bPinned)
{
	// Reload/draw montages take over the spine/neck — the body stops tracking
	// the view and the player can see inside their own head (session 63).
	// Neck-bone pinning in PostEval FAILED (Kinemation overwrites the neck,
	// same dead end as the arms). Render-level bone hide is the mechanism
	// that always works (proven by the clavicle hides): hide the head region
	// during these windows — it's never legitimately visible in first person.
	if (PlayerMeshComponent)
	{
		if (bPinned)
		{
			PlayerMeshComponent->HideBoneByName(FName("neck_01"), PBO_None);
		}
		else
		{
			PlayerMeshComponent->UnHideBoneByName(FName("neck_01"));
		}
	}
}

void UZP_KinemationComponent::RestockThrowable()
{
	// Called by the owner when inventory still holds more of the equipped
	// throwable after a throw — rearm so the auto-switch-back doesn't fire.
	if (CurrentWeaponType != EZP_WeaponType::Throwable)
	{
		return;
	}
	CurrentAmmo = MagSize > 0 ? MagSize : 1;
	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);

	// Bring the next grenade up like a fresh equip (dev call) — replay the
	// rise-from-below on the view model, same as the initial grenade equip.
	if (bMeleeViewModelActive && MeleeEquipAnim && MeleeViewMeshComponent)
	{
		PlayMeleeViewAnim(MeleeEquipAnim, false, MeleeEquipRate);
		const float StartTime = MeleeEquipAnim->GetPlayLength() * ThrowableEquipStartFraction;
		MeleeViewMeshComponent->SetPosition(StartTime, false);
		const float EquipTime = (MeleeEquipAnim->GetPlayLength() - StartTime) / FMath::Max(MeleeEquipRate, 0.1f);
		GetWorld()->GetTimerManager().SetTimer(MeleeEquipIdleHandle, [this]()
		{
			if (bMeleeViewModelActive && MeleeIdleAnim)
			{
				PlayMeleeViewAnim(MeleeIdleAnim, true, 1.0f);
			}
		}, EquipTime, false);
	}
}
