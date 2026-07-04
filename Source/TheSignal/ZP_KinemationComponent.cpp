// Copyright The Signal. All Rights Reserved.

#include "ZP_KinemationComponent.h"
#include "ZP_CrawlerBehaviorComponent.h"
#include "ZP_GraceCharacter.h"
#include "ZP_GrenadeProjectile.h"
#include "ZP_MeleeDamageType.h"
#include "ZP_SFXStatics.h"
#include "ZP_BloodFXComponent.h"
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
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UZP_KinemationComponent::UZP_KinemationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Default melee hand skin (flat, tunable). Overridable in BP_GraceCharacter Details.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> HandSkinFinder(
		TEXT("/Game/Core/Materials/MI_HandSkin.MI_HandSkin"));
	if (HandSkinFinder.Succeeded())
	{
		AZP_MeleeHandMaterial = HandSkinFinder.Object;
	}

	// --- Weapon / melee impact sounds (imported to /Game/Audio/Weapons) ---
	static ConstructorHelpers::FObjectFinder<USoundBase> PistolImpactFinder(
		TEXT("/Game/Audio/Weapons/SFX_RANGE_IMPACT_PISTOL.SFX_RANGE_IMPACT_PISTOL"));
	if (PistolImpactFinder.Succeeded()) { AZP_PistolImpactSound = PistolImpactFinder.Object; }

	static ConstructorHelpers::FObjectFinder<USoundBase> RifleImpactFinder(
		TEXT("/Game/Audio/Weapons/SFX_RANGE_IMPACT_RIFLE.SFX_RANGE_IMPACT_RIFLE"));
	if (RifleImpactFinder.Succeeded()) { AZP_RifleImpactSound = RifleImpactFinder.Object; }

	static ConstructorHelpers::FObjectFinder<USoundBase> ShotgunImpactFinder(
		TEXT("/Game/Audio/Weapons/SFX_RANGE_IMPACT_SHOTGUN.SFX_RANGE_IMPACT_SHOTGUN"));
	if (ShotgunImpactFinder.Succeeded()) { AZP_ShotgunImpactSound = ShotgunImpactFinder.Object; }

	static ConstructorHelpers::FObjectFinder<USoundBase> PipeMetalFinder(
		TEXT("/Game/Audio/Weapons/SFX_PIPE_HIT.SFX_PIPE_HIT"));
	if (PipeMetalFinder.Succeeded()) { AZP_PipeMetalSound = PipeMetalFinder.Object; }

	static ConstructorHelpers::FObjectFinder<USoundBase> PipeWallFinder(
		TEXT("/Game/Audio/Weapons/SFX_PIPE_SURFACE_WALL_IMPACT.SFX_PIPE_SURFACE_WALL_IMPACT"));
	if (PipeWallFinder.Succeeded()) { AZP_PipeWallImpactSound = PipeWallFinder.Object; }

	static ConstructorHelpers::FObjectFinder<USoundBase> MeleeFlesh1Finder(
		TEXT("/Game/Audio/Weapons/SFX_MELEE_IMPACT1.SFX_MELEE_IMPACT1"));
	if (MeleeFlesh1Finder.Succeeded()) { AZP_MeleeFleshImpactSounds.Add(MeleeFlesh1Finder.Object); }

	static ConstructorHelpers::FObjectFinder<USoundBase> MeleeFlesh2Finder(
		TEXT("/Game/Audio/Weapons/SFX_MELEE_IMPACT2.SFX_MELEE_IMPACT2"));
	if (MeleeFlesh2Finder.Succeeded()) { AZP_MeleeFleshImpactSounds.Add(MeleeFlesh2Finder.Object); }

	static ConstructorHelpers::FObjectFinder<USoundBase> MeleeSwingFinder(
		TEXT("/Game/Audio/Weapons/SFX_MELEE_SWING.SFX_MELEE_SWING"));
	if (MeleeSwingFinder.Succeeded()) { AZP_MeleeSwingSound = MeleeSwingFinder.Object; }
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
	// REVERTED 2026-06-20: the CCMH-retargeted Marcus_ melee set collapsed the
	// pose (arms mispositioned — "more than a tuning displacement"). Back to the
	// proven Operator-skeleton A_MeleePipe clips on SKM_Operator_Mono below.
	AZP_MeleeLightAnims.Reset();
	for (const FString& Name : { FString(TEXT("A_MeleePipe_Attack_R")),
	                             FString(TEXT("A_MeleePipe_Attack_L")) })
	{
		const FString Path = FString::Printf(TEXT("/Game/TheSignal/Animations/Melee/%s.%s"), *Name, *Name);
		if (UAnimSequenceBase* Anim = LoadObject<UAnimSequenceBase>(nullptr, *Path))
		{
			AZP_MeleeLightAnims.Add(Anim);
		}
	}
	AZP_MeleeIdleAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle.A_MeleePipe_Idle"));
	AZP_MeleeEquipAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Equip.A_MeleePipe_Equip"));
	AZP_MeleeUnequipAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Unequip.A_MeleePipe_Unequip"));
	AZP_GrenadeThrowAnim = LoadObject<UAnimSequenceBase>(nullptr,
		TEXT("/Game/Animations/FPS/AM_FP_GrenadeThrow.AM_FP_GrenadeThrow"));

	// View-model mesh: the Operator body PlayerMesh wears — PROVEN pose/skeleton.
	// (CCMH swap reverted 2026-06-20: retarget onto the CCMH skeleton broke the
	// pipe pose.) Legs + head hidden — never visible from the FP camera; head
	// sits at camera height. Reskinned to Marcus's CCMH skin so the arms read as
	// Marcus (same MI_Skin_Body_CCMH the working RANGED arms use — same Operator
	// arm geometry/UVs, so it maps the same). Restores the Marcus-skin melee arms
	// the dev had working before the per-hand block-offset knobs.
	if (MeleeViewMeshComponent && !MeleeViewMeshComponent->GetSkeletalMeshAsset())
	{
		if (USkeletalMesh* ViewMesh = LoadObject<USkeletalMesh>(nullptr,
			*AZP_MeleeViewModelMeshAsset.ToString()))
		{
			MeleeViewMeshComponent->SetSkeletalMesh(ViewMesh);
			MeleeViewMeshComponent->HideBoneByName(FName("thigh_l"), PBO_None);
			MeleeViewMeshComponent->HideBoneByName(FName("thigh_r"), PBO_None);
			MeleeViewMeshComponent->HideBoneByName(FName("head"), PBO_None);

			// NOTE: do NOT assign UZP_MeleeHandsAnimInstance via SetOverridePostProcessAnimBP
			// here — a bare C++ AnimInstance class has no AnimGraph, so as a post-process it
			// outputs the REFERENCE pose and FREEZES/clobbers the SingleNode melee clip
			// (probe-confirmed 2026-06-20: hand_r bit-identical across idle AND block). That
			// froze hand_r and made the pipe vanish. A post-process needs a real AnimBP with
			// an Input-Pose -> Output passthrough graph on SKM_Manny_Skeleton.

			// Paint the bare-skin slots (forearm MI_Skin + the hand MI_Gloves) with the
			// AZP_MeleeHandMaterial (defaults to flat MI_HandSkin; settable in BP_GraceCharacter
			// → KinemationComp Details → Kinemation|Melee). Flat = solid color = UV-independent
			// — can't mismap the glove/hand UVs (that caused the splotch) and there is NO
			// second mesh to z-fight. Tune tone on MI_HandSkin (SkinColor / Roughness params).
			UMaterialInterface* HandSkin = AZP_MeleeHandMaterial;
			if (!HandSkin)
			{
				HandSkin = LoadObject<UMaterialInterface>(nullptr,
					TEXT("/Game/Core/Materials/MI_HandSkin.MI_HandSkin"));
			}
			if (HandSkin)
			{
				const TArray<FSkeletalMaterial>& Mats = ViewMesh->GetMaterials();
				for (int32 i = 0; i < Mats.Num(); ++i)
				{
					const FString SlotName = Mats[i].MaterialSlotName.ToString();
					if (SlotName.Contains(TEXT("Skin")) || SlotName.Contains(TEXT("Gloves")))
					{
						MeleeViewMeshComponent->SetMaterial(i, HandSkin);
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: Anims — MeleeLight:%d, MeleeIdle:%s, MeleeEquip:%s, MeleeUnequip:%s, GrenadeThrow:%s, ViewMesh:%s"),
		AZP_MeleeLightAnims.Num(),
		AZP_MeleeIdleAnim ? TEXT("OK") : TEXT("NOT FOUND"),
		AZP_MeleeEquipAnim ? TEXT("OK") : TEXT("NOT FOUND"),
		AZP_MeleeUnequipAnim ? TEXT("OK") : TEXT("NOT FOUND"),
		AZP_GrenadeThrowAnim ? TEXT("OK") : TEXT("NOT FOUND"),
		(MeleeViewMeshComponent && MeleeViewMeshComponent->GetSkeletalMeshAsset()) ? TEXT("OK") : TEXT("NOT FOUND"));

	// Load grenade projectile class
	if (!AZP_GrenadeProjectileClass)
	{
		AZP_GrenadeProjectileClass = AZP_GrenadeProjectile::StaticClass();
	}

	// Initialize ammo state from config
	CurrentAmmo = AZP_MagSize;

	// Spawn and equip default weapon (only if bAZP_AutoSpawnWeapon is true)
	if (bAZP_AutoSpawnWeapon)
	{
		SpawnAndEquipWeapon();
		// Apply per-weapon config (mag size, fire rate, damage, weapon type)
		// SpawnAndEquipWeapon doesn't set CurrentWeaponType — without this,
		// reload is blocked because CurrentWeaponType remains None.
		if (ActiveWeapon)
		{
			ApplyWeaponConfig(AZP_WeaponClass);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: bAZP_AutoSpawnWeapon=false — weapon spawn deferred to inventory."));
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
	if (!AZP_WeaponClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent: No AZP_WeaponClass set — skipping."));
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

	ActiveWeapon = GetWorld()->SpawnActor<AActor>(AZP_WeaponClass, FTransform::Identity, SpawnParams);
	if (!ActiveWeapon)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] KinemationComponent: Failed to spawn weapon!"));
		return;
	}

	// Attach weapon to PlayerMesh at the gun socket
	const FName WeaponSocket = AZP_WeaponAttachSocketName;
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
		ApplyWeaponConfig(AZP_WeaponClass);
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
	if (ActiveWeapon && AZP_WeaponClass == ActorClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] KinemationComponent::EquipWeaponClass — same weapon already equipped."));
		return true;
	}

	// Save current weapon class before switching (for auto-switch-back after throwable)
	if (ActiveWeapon && CurrentWeaponType != EZP_WeaponType::Throwable)
	{
		PreviousWeaponClass = AZP_WeaponClass;
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
	}, AZP_WeaponDrawLockTime, false);
	GetWorld()->GetTimerManager().SetTimer(HeadHideHandle, [this]()
	{
		SetCameraBonePinned(false);
	}, AZP_SwapHeadHideTime, false);

	return true;
}

bool UZP_KinemationComponent::PerformWeaponSwap(TSubclassOf<AActor> ActorClass)
{
	if (ActiveWeapon)
	{
		ActiveWeapon->Destroy();
		ActiveWeapon = nullptr;
	}

	AZP_WeaponClass = ActorClass;
	SpawnAndEquipWeapon(); // sets ActiveSettings + plays the Draw montage (deferred one tick)

	if (!ActiveWeapon)
	{
		return false;
	}

	ApplyWeaponConfig(ActorClass); // melee raises its Kubold view model in here
	CurrentAmmo = AZP_MagSize;
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
		AZP_MagSize = 12;
		AZP_FireCooldownTime = 0.25f;
		AZP_HitscanBodyDamage = 10.f;
		AZP_HitscanWeakPointDamage = 50.f;
		ReserveAmmo = 48;
	}
	else if (WeaponName.Contains(TEXT("Herrington")))
	{
		// Herrington 11-87 — Shotgun (Police or standard)
		// Buckshot spray: these are the TOTAL per-shot damage (3x pistol = 30/150),
		// split across the pellets in PerformHitscan.
		AZP_MagSize = 6;
		AZP_FireCooldownTime = 0.8f;
		AZP_HitscanBodyDamage = 30.f;
		AZP_HitscanWeakPointDamage = 150.f;
		ReserveAmmo = 24;
		bShellReload = true;
	}
	else if (WeaponName.Contains(TEXT("AK105")))
	{
		// AK-105 Carbine — Assault Rifle. Semi-auto: limited by trigger pull,
		// not cycling — 0.47s read as ~1 shot/sec in play (dev call).
		AZP_MagSize = 30;
		AZP_FireCooldownTime = 0.15f;
		AZP_HitscanBodyDamage = 15.f;
		AZP_HitscanWeakPointDamage = 60.f;
		ReserveAmmo = 90;
	}
	else if (WeaponName.Contains(TEXT("TR15")))
	{
		// TR15 — Rifle
		AZP_MagSize = 20;
		AZP_FireCooldownTime = 0.15f;
		AZP_HitscanBodyDamage = 18.f;
		AZP_HitscanWeakPointDamage = 70.f;
		ReserveAmmo = 60;
	}
	else if (WeaponName.Contains(TEXT("SRM")))
	{
		// SRM-12 — Shotgun. Buckshot spray: TOTAL per-shot damage (3x pistol = 30/150),
		// split across the pellets in PerformHitscan.
		AZP_MagSize = 8;
		AZP_FireCooldownTime = 0.7f;
		AZP_HitscanBodyDamage = 30.f;
		AZP_HitscanWeakPointDamage = 150.f;
		ReserveAmmo = 32;
		bShellReload = true;
	}
	else if (WeaponName.Contains(TEXT("Pipe")))
	{
		// Pipe — Melee weapon
		CurrentWeaponType = EZP_WeaponType::Melee;
		AZP_MeleeDamage = 25.f;
		AZP_MeleeCooldown = 0.7f;
		AZP_MagSize = 0;
		CurrentAmmo = 0;
		ReserveAmmo = 0;
	}
	else if (WeaponName.Contains(TEXT("Grenade")))
	{
		// Grenade — Throwable weapon (1 throw per equip, consumed from inventory)
		CurrentWeaponType = EZP_WeaponType::Throwable;
		AZP_MagSize = 1;
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

	// Reserve cap is icon-driven (the per-weapon ReserveAmmo numbers set above are now
	// dead — kept only for clarity). The live ReserveAmmo is a mirror of the inventory
	// ammo for this weapon; the owning character sets it via SyncReserveFromInventory
	// right after equip (OnWeaponChanged), so we don't seed it here.
	MaxReserveAmmo = GetReserveCapForIcon(CurrentWeaponIcon);

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

int32 UZP_KinemationComponent::GetReserveCapForIcon(EZP_WeaponIcon Icon)
{
	switch (Icon)
	{
	case EZP_WeaponIcon::Pistol:  return 48;
	case EZP_WeaponIcon::Shotgun: return 24;
	case EZP_WeaponIcon::Rifle:   return 90;
	default:                      return 0; // melee / throwable / none — no reserve
	}
}

EZP_WeaponIcon UZP_KinemationComponent::AmmoNameToIcon(const FString& AmmoItemName)
{
	if (AmmoItemName.Contains(TEXT("9mm")))      return EZP_WeaponIcon::Pistol;
	if (AmmoItemName.Contains(TEXT("Buckshot"))) return EZP_WeaponIcon::Shotgun;
	if (AmmoItemName.Contains(TEXT("556")))      return EZP_WeaponIcon::Rifle;
	return EZP_WeaponIcon::None;
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
		// [LatchProbe] the grab latch stows the weapon through here — if a swing's damage sweep is
		// still queued at this instant, THIS is what kills it. If this line never shows on a
		// glitched latch, the sweep either already fired (see SWEEP FIRED timing) or was never queued.
		if (GetWorld()->GetTimerManager().IsTimerActive(MeleeDamageHandle))
		{
			UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] t=%.2f UnequipWeapon: PENDING melee sweep killed (was %.2fs from firing)"),
				GetWorld()->GetTimeSeconds(), GetWorld()->GetTimerManager().GetTimerRemaining(MeleeDamageHandle));
		}
		GetWorld()->GetTimerManager().ClearTimer(FireCooldownHandle);
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeCooldownHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeSwingReturnHandle);
		GetWorld()->GetTimerManager().ClearTimer(MeleeTailCancelHandle);
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
	bMeleeSwingTailCancelable = false;

	bWeaponSwitching = false;
	SetCameraBonePinned(false);

	// Destroy the weapon actor
	ActiveWeapon->Destroy();
	ActiveWeapon = nullptr;

	// Unarmed now — reset weapon identity so the HUD blanks the ammo line and the
	// weapon icon. Without broadcasting None, the HUD keeps the last weapon's
	// format and shows stale ammo (the 12/48 demo defaults on an empty hand).
	CurrentWeaponType = EZP_WeaponType::None;
	CurrentWeaponIcon = EZP_WeaponIcon::None;
	OnWeaponTypeChanged.Broadcast(CurrentWeaponType);
	OnWeaponIconChanged.Broadcast(CurrentWeaponIcon);

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

	// Per-weapon ADS FOV — the aim pose pulls the gun to the eye (the "zoom"); a wider
	// FOV counteracts it. Widen shotgun/rifle to dial their zoom back. Hip-fire = default.
	float TargetFOV = AZP_DefaultFOV;
	if (bAiming)
	{
		switch (CurrentWeaponIcon)
		{
			case EZP_WeaponIcon::Shotgun: TargetFOV = AZP_AdsFOVShotgun; break;
			case EZP_WeaponIcon::Rifle:   TargetFOV = AZP_AdsFOVRifle;   break;
			case EZP_WeaponIcon::Pistol:  TargetFOV = AZP_AdsFOVPistol;  break;
			default: break;
		}
	}
	SetTargetFOV(TargetFOV, AZP_AdsFOVInterpSpeed);

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
		AZP_GunshotAlertRadius, // default broadcast radius (creatures use their own AZP_GunshotAlertRadius for per-instance override)
		GetOwner()
	);

	// Semi-auto lockout — block next shot until animation cycles
	bFireCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(FireCooldownHandle, [this]()
	{
		bFireCooldown = false;
	}, AZP_FireCooldownTime, false);
}

void UZP_KinemationComponent::PerformHitscan()
{
	if (AZP_BulletDecalMaterials.Num() == 0)
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
	const FVector AimDir = AimRot.Vector();

	// Shotguns fire a randomized buckshot spray; everything else is a single round.
	// AZP_HitscanBodyDamage/WeakPointDamage are the TOTAL per-shot damage — split across the
	// pellets, so a full spray that all connects deals the total and partial hits scale down.
	const bool bShotgun = (CurrentWeaponIcon == EZP_WeaponIcon::Shotgun);
	const int32 PelletCount = bShotgun
		? FMath::Max(1, FMath::RandRange(AZP_ShotgunPelletMin, AZP_ShotgunPelletMax))
		: 1;
	const float SpreadRad = FMath::DegreesToRadians(FMath::Max(0.f, AZP_ShotgunSpreadDegrees));

	// Impact sound chosen by weapon identity — played ONCE per trigger pull at the
	// first surface a round connects with (a 20-pellet shotgun must not fire 20 thuds).
	USoundBase* ImpactSound = nullptr;
	switch (CurrentWeaponIcon)
	{
		case EZP_WeaponIcon::Pistol:  ImpactSound = AZP_PistolImpactSound;  break;
		case EZP_WeaponIcon::Rifle:   ImpactSound = AZP_RifleImpactSound;   break;
		case EZP_WeaponIcon::Shotgun: ImpactSound = AZP_ShotgunImpactSound; break;
		default: break;
	}
	bool bPlayedImpactSound = false;

	for (int32 Pellet = 0; Pellet < PelletCount; ++Pellet)
	{
		const FVector Dir = (bShotgun && SpreadRad > 0.f) ? FMath::VRandCone(AimDir, SpreadRad) : AimDir;
		const FVector End = Start + Dir * AZP_HitscanRange;

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
				break; // this pellet hit nothing solid
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
			continue; // pellet missed — next pellet
		}

		// First connecting round plays the bullet-impact sound at the surface. Far carry — the
		// hitscan reaches 100 m and the shooter must hear the impact at any hit distance (bare
		// PlaySoundAtLocation inherited UE5's default attenuation = silent past 40 m).
		if (!bPlayedImpactSound && ImpactSound)
		{
			UZP_SFXStatics::PlaySFXAtLocation(this, ImpactSound, Hit.ImpactPoint, EZP_SFXCarry::Far);
			bPlayedImpactSound = true;
		}

		UMaterialInterface* DecalMat = AZP_BulletDecalMaterials[FMath::RandRange(0, AZP_BulletDecalMaterials.Num() - 1)];

		// Decal projects along its local X — must face INTO the surface (opposite of impact normal)
		const FRotator DecalRotation = (-Hit.ImpactNormal).Rotation();
		if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(), DecalMat, AZP_DecalSize, Hit.ImpactPoint, DecalRotation, AZP_DecalLifetime))
		{
			Decal->SetFadeOut(AZP_DecalLifetime - AZP_DecalFadeDuration, AZP_DecalFadeDuration);
		}

		// Apply this pellet's share of the damage to the hit actor.
		if (Hit.GetActor())
		{
			const float DistFromCenter = FVector::Dist(Hit.ImpactPoint, Hit.GetActor()->GetActorLocation());
			const bool bWeakPointHit = DistFromCenter <= AZP_WeakPointRadius;
			const float FullDamage = bWeakPointHit ? AZP_HitscanWeakPointDamage : AZP_HitscanBodyDamage;
			const float Damage = FullDamage / PelletCount; // per-pellet share of the total

			UGameplayStatics::ApplyPointDamage(
				Hit.GetActor(), Damage, Dir, Hit, PC, GetOwner(), nullptr);

			// Blood on pawn hits: ranged trio "Splash + Hit + Metal" (bullet impact/ricochet) at the
			// enemy's AZP_BloodIntensity + traced splatter decals. Per-enemy via UZP_BloodFXComponent.
			if (Cast<APawn>(Hit.GetActor()))
			{
				UZP_BloodFXComponent::PlayHitBloodFor(Hit.GetActor(), Hit.ImpactPoint, Dir, /*bMeleeHit=*/false, Hit.ImpactNormal);
			}
		}
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
	if (CurrentAmmo >= AZP_MagSize || ReserveAmmo <= 0)
	{
		return;
	}

	FKinemationBridge::WeaponOnReload(ActiveWeapon);

	// Shell loaders animate per shell — lock/refill/head-hide must run for
	// the REAL duration or the head pops back mid-animation (dev-caught on
	// the shotgun: ~9s full reload vs the old flat 3s).
	float ThisReloadTime = AZP_ReloadTime;
	if (bShellReload)
	{
		const int32 Shells = FMath::Min(AZP_MagSize - CurrentAmmo, ReserveAmmo);
		const float StartTime = (CurrentAmmo == 0) ? AZP_ShellReloadEmptyStartTime : AZP_ShellReloadTacStartTime;
		ThisReloadTime = StartTime + Shells * AZP_ShellReloadLoopTime + AZP_ShellReloadEndTime;
	}

	bIsReloading = true;
	SetCameraBonePinned(true); // reload montage animates the camera's neck bone
	GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, [this]()
	{
		bIsReloading = false;
		SetCameraBonePinned(false);

		// Transfer ammo from reserve (= inventory ammo) into the magazine. ReserveAmmo
		// mirrors the inventory; the character consumes the actual items on the
		// OnReserveConsumed broadcast, then re-syncs ReserveAmmo from what's left.
		const int32 AmmoNeeded = AZP_MagSize - CurrentAmmo;
		const int32 AmmoAvailable = FMath::Min(AmmoNeeded, ReserveAmmo);
		CurrentAmmo += AmmoAvailable;
		OnReserveConsumed.Broadcast(AmmoAvailable, CurrentWeaponIcon);
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
	if (AZP_MeleeLightAnims.Num() > 0)
	{
		SwingAnim = AZP_MeleeLightAnims[MeleeLightAnimIndex % AZP_MeleeLightAnims.Num()];
		++MeleeLightAnimIndex;
	}

	// Swing whoosh — own-body foley, fires with the swing regardless of view-model state.
	if (AZP_MeleeSwingSound)
	{
		UGameplayStatics::PlaySound2D(GetOwner(), AZP_MeleeSwingSound, AZP_MeleeSwingVolume);
	}

	// --- Animation: retargeted Kubold swing on the view model (TICKET-054) ---
	float SwingDuration = 0.7f; // fallback if anim missing
	if (bMeleeViewModelActive && MeleeViewMeshComponent && SwingAnim)
	{
		PlayMeleeViewAnim(SwingAnim, false, AZP_MeleeSwingRate);
		SwingDuration = SwingAnim->GetPlayLength() / FMath::Max(AZP_MeleeSwingRate, 0.1f);

		bMeleeSwingActive = true;
		bMeleeSwingTailCancelable = false;
		GetWorld()->GetTimerManager().SetTimer(MeleeSwingReturnHandle, [this]()
		{
			if (bMeleeViewModelActive && AZP_MeleeIdleAnim)
			{
				PlayMeleeViewAnim(AZP_MeleeIdleAnim, true, 1.0f);
			}
			bMeleeSwingActive = false;
			bMeleeSwingTailCancelable = false;
		}, SwingDuration, false);
		// Past this point the strike + follow-through have visibly completed; only the
		// return-to-idle dead frames remain — a HELD block may cancel into them (no perceived
		// clipping, no perceived pause).
		GetWorld()->GetTimerManager().SetTimer(MeleeTailCancelHandle, [this]()
		{
			bMeleeSwingTailCancelable = true;
		}, SwingDuration * FMath::Clamp(AZP_MeleeBlockCancelFraction, 0.1f, 1.0f), false);
	}
	else
	{
		// Fail loud — swing still does damage, but the visual is missing.
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] PerformMeleeSwing — no view model visual (active=%d, anim=%s)"),
			bMeleeViewModelActive ? 1 : 0, SwingAnim ? TEXT("OK") : TEXT("NULL"));
	}

	// --- Damage: sweep on the impact frame, not at click time ---
	UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] t=%.2f SWING click — damage sweep queued to fire at t=%.2f (+%.2fs)"),
		GetWorld()->GetTimeSeconds(), GetWorld()->GetTimeSeconds() + AZP_MeleeDamageDelay, AZP_MeleeDamageDelay);
	GetWorld()->GetTimerManager().SetTimer(MeleeDamageHandle, [this]()
	{
		DoMeleeDamageSweep();
	}, AZP_MeleeDamageDelay, false);

	// Cooldown covers the full swing so a re-click can't restart mid-animation
	bMeleeCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(MeleeCooldownHandle, [this]()
	{
		bMeleeCooldown = false;
	}, FMath::Max(AZP_MeleeCooldown, SwingDuration), false);
}

void UZP_KinemationComponent::DoMeleeDamageSweep()
{
	UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] t=%.2f SWEEP FIRED — applying melee damage now"),
		GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0);
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* PC = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (PC && CameraComponent)
	{
		const FVector Start = CameraComponent->GetComponentLocation();
		const FRotator AimRot = PC->GetControlRotation();
		const FVector End = Start + AimRot.Vector() * AZP_MeleeRange;

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());
		if (ActiveWeapon)
		{
			Params.AddIgnoredActor(ActiveWeapon);
		}

		// Detect with the SAME channel the guns use (ECC_Visibility): enemies set their CAPSULE to
		// BLOCK Visibility (mesh IGNORES it) so hitscan connects. BUT door/trigger interaction boxes
		// are QueryOnly volumes that ALSO block Visibility while being non-solid — they were eating
		// the whole swing (the sweep stopped on BP_InteractDoor near a doorway and the Scytheer/
		// Shambler behind it took nothing). Mirror PerformHitscan: re-sweep past each QueryOnly
		// volume until we reach real geometry / a pawn. Sphere keeps a little melee aim-forgiveness.
		bool bGotSolidHit = false;
		for (int32 Iter = 0; Iter < 8; ++Iter)
		{
			if (!GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
				ECC_Visibility, FCollisionShape::MakeSphere(AZP_MeleeSweepRadius), Params))
			{
				break; // swung at nothing solid
			}
			UPrimitiveComponent* HitComp = Hit.GetComponent();
			if (HitComp && HitComp->GetCollisionEnabled() == ECollisionEnabled::QueryOnly)
			{
				Params.AddIgnoredComponent(HitComp); // interaction/trigger box — pass through it
				continue;
			}
			bGotSolidHit = true;
			break;
		}

		if (bGotSolidHit)
		{
			// An enemy is a Pawn; anything else is a wall / hard surface.
			const bool bHitEnemy = Cast<APawn>(Hit.GetActor()) != nullptr;
			const FVector ImpactLoc = Hit.ImpactPoint;

			if (bHitEnemy)
			{
				// Pipe is metal — the metal sound always plays — and a flesh impact
				// layers on top (dev: "impact + hit at the same time"). Close carry: own-melee
				// foley, always within ~2 m of the listener.
				UZP_SFXStatics::PlaySFXAtLocation(this, AZP_PipeMetalSound, ImpactLoc, EZP_SFXCarry::Close);
				if (AZP_MeleeFleshImpactSounds.Num() > 0)
				{
					USoundBase* Flesh = AZP_MeleeFleshImpactSounds[FMath::RandRange(0, AZP_MeleeFleshImpactSounds.Num() - 1)];
					UZP_SFXStatics::PlaySFXAtLocation(this, Flesh, ImpactLoc, EZP_SFXCarry::Close);
				}

				// Blood: melee trio "Splash with Burst + Hit" at the enemy's AZP_BloodIntensity +
				// traced persistent splatter decals + delayed floor pool. Per-enemy via UZP_BloodFXComponent.
				// ImpactNormal lays the body-residual stains flat on the skin.
				const FVector SwingDir = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
				UZP_BloodFXComponent::PlayHitBloodFor(Hit.GetActor(), Hit.ImpactPoint, SwingDir, /*bMeleeHit=*/true, Hit.ImpactNormal);
			}
			else
			{
				// Pipe struck a wall / hard surface.
				UZP_SFXStatics::PlaySFXAtLocation(this, AZP_PipeWallImpactSound, ImpactLoc, EZP_SFXCarry::Close);
			}

			if (Hit.GetActor())
			{
				// Tag the hit as MELEE so height-headshot enemies (Shambler/Scytheer) treat it as a
				// flat body strike (AZP_MeleeDamage), never a headshot.
				UGameplayStatics::ApplyPointDamage(
					Hit.GetActor(),
					AZP_MeleeDamage,
					AimRot.Vector(),
					Hit,
					PC,
					GetOwner(),
					UZP_MeleeDamageType::StaticClass()
				);

				UE_LOG(LogTemp, Log, TEXT("[TheSignal] Melee hit %s — %.0f dmg"),
					*Hit.GetActor()->GetName(), AZP_MeleeDamage);

				// Stagger the enemy on a landed pipe hit (AZP_HitStaggerDuration). Routed through the
				// player so the duration stays an exposed knob and the IZP_Staggerable lookup is shared.
				if (bHitEnemy)
				{
					if (AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(GetOwner()))
					{
						Grace->MeleeStaggerEnemy(Hit.GetActor());
					}
				}
			}
		}
	}

	// Alert nearby creatures — impact noise, quieter than gunshots
	UZP_CrawlerBehaviorComponent::BroadcastGunshot(
		GetWorld(),
		GetOwner()->GetActorLocation(),
		AZP_MeleeNoiseAlertRadius,
		GetOwner()
	);
}

void UZP_KinemationComponent::CancelPendingMeleeSwing()
{
	// UnequipWeapon already clears MeleeDamageHandle when a weapon is equipped; this covers the
	// grab latch's no-ActiveWeapon path and stands alone as an explicit "no queued damage" call.
	if (UWorld* W = GetWorld())
	{
		const bool bWasPending = W->GetTimerManager().IsTimerActive(MeleeDamageHandle);
		W->GetTimerManager().ClearTimer(MeleeDamageHandle);
		if (bWasPending)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CancelPendingMeleeSwing — queued melee sweep KILLED (grab latched mid-swing)"));
		}
	}
	bMeleeSwingActive = false;
	bMeleeSwingTailCancelable = false;
}

// --- Melee view model (TICKET-054) ---

void UZP_KinemationComponent::SetMeleeWeaponBlockGrip(bool bBlocking)
{
	if (!MeleeWeaponMeshComp || !MeleeViewMeshComponent) return;

	// Parent the pipe to hand_r and seat it at the idle grip. It STAYS parented to hand_r
	// for its whole life — UpdateMeleeGrip only nudges the hand_r-RELATIVE offset toward
	// the block delta. (The pipe used to re-blend onto the ik_hand_gun joint during block;
	// because hand_r and ik_hand_gun separate during the block clip, that world-space
	// blend swept the pipe OUT of the hand on un/block. Hand-local is the fix.)
	MeleeWeaponMeshComp->AttachToComponent(MeleeViewMeshComponent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_r"));
	MeleeWeaponMeshComp->SetRelativeLocation(AZP_MeleeGripOffset);
	MeleeWeaponMeshComp->SetRelativeRotation(AZP_MeleeGripRotation);
	MeleeGripBlend = bBlocking ? 1.f : 0.f;
}

void UZP_KinemationComponent::UpdateMeleeGrip(float DeltaSeconds, bool bBlocking)
{
	if (!MeleeWeaponMeshComp) return;

	MeleeGripBlend = FMath::FInterpTo(MeleeGripBlend, bBlocking ? 1.f : 0.f, DeltaSeconds, AZP_BlockGripBlendSpeed);

	// Ease the hand_r-RELATIVE grip between idle and block. Block is an additive delta on
	// the fitted idle grip, so both endpoints live in the same (hand-local) frame. We set
	// the pipe's RELATIVE transform on a hand_r-parented mesh, so the engine composes it
	// with the freshly evaluated hand pose every frame: the pipe tracks the hand 1:1 and
	// the un/block transition can only shift the grip WITHIN the hand — never out of it.
	const FVector  Loc   = AZP_MeleeGripOffset + AZP_BlockGripDeltaLocation * MeleeGripBlend;
	const FQuat    IdleQ = AZP_MeleeGripRotation.Quaternion();
	const FQuat    Rot   = FQuat::Slerp(IdleQ, IdleQ * AZP_BlockGripDeltaRotation.Quaternion(), MeleeGripBlend);

	MeleeWeaponMeshComp->SetRelativeLocation(Loc);
	MeleeWeaponMeshComp->SetRelativeRotation(Rot);
}

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
			*AZP_MeleeWeaponMeshAsset.ToString()))
		{
			MeleeWeaponMeshComp->SetStaticMesh(PipeMesh);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ActivateMeleeViewModel — SM_Pipe not found, view model swings empty-handed."));
		}
	}
	// Idle/swing grip: hand_r + AZP_MeleeGripOffset (fitted to the idle finger channel).
	// Block re-attaches to ik_hand_gun at runtime via SetMeleeWeaponBlockGrip() — only
	// block needed the weapon joint; idle must stay on hand_r or its fitted grip breaks.
	SetMeleeWeaponBlockGrip(false);
	MeleeWeaponMeshComp->SetVisibility(true);

	MeleeViewMeshComponent->SetVisibility(true);

	// --- PROBE: report the pipe's actual state right after we show it, so we can see
	// whether it's hidden, off-screen, or fine when the hand grip layer is active. ---
	{
		const int32 HRBone = MeleeViewMeshComponent->GetBoneIndex(FName("hand_r"));
		const FVector SocketWorld = MeleeViewMeshComponent->DoesSocketExist(FName("hand_r"))
			? MeleeViewMeshComponent->GetSocketLocation(FName("hand_r")) : FVector::ZeroVector;
		const UAnimInstance* PP = MeleeViewMeshComponent->GetPostProcessInstance();
		UE_LOG(LogTemp, Warning,
			TEXT("[PIPEPROBE] pipe_visible=%d pipe_world=%s mesh_visible=%d hand_r_bone=%d hand_r_world=%s postproc=%s"),
			MeleeWeaponMeshComp->IsVisible() ? 1 : 0,
			*MeleeWeaponMeshComp->GetComponentLocation().ToString(),
			MeleeViewMeshComponent->IsVisible() ? 1 : 0,
			HRBone, *SocketWorld.ToString(),
			PP ? *PP->GetClass()->GetName() : TEXT("NONE"));
	}

	// Raise: Kubold Equip → Idle loop. Swing is blocked until the raise lands.
	if (AZP_MeleeEquipAnim)
	{
		PlayMeleeViewAnim(AZP_MeleeEquipAnim, false, AZP_MeleeEquipRate);
		const float EquipTime = AZP_MeleeEquipAnim->GetPlayLength() / FMath::Max(AZP_MeleeEquipRate, 0.1f);

		GetWorld()->GetTimerManager().SetTimer(MeleeEquipIdleHandle, [this]()
		{
			if (bMeleeViewModelActive && AZP_MeleeIdleAnim)
			{
				PlayMeleeViewAnim(AZP_MeleeIdleAnim, true, 1.0f);
			}
		}, EquipTime, false);

		bMeleeCooldown = true;
		GetWorld()->GetTimerManager().SetTimer(MeleeCooldownHandle, [this]()
		{
			bMeleeCooldown = false;
		}, EquipTime, false);
	}
	else if (AZP_MeleeIdleAnim)
	{
		PlayMeleeViewAnim(AZP_MeleeIdleAnim, true, 1.0f);
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
		ActiveWeapon->SetActorRelativeLocation(AZP_ThrowableGripOffset);
		ActiveWeapon->SetActorRelativeRotation(AZP_ThrowableGripRotation);
		ActiveWeapon->SetActorRelativeScale3D(AZP_ThrowableGripScale);
		ActiveWeapon->SetActorHiddenInGame(false);
	}

	MeleeViewMeshComponent->SetVisibility(true);

	// Only the back half of the Kubold Equip — the first half mimes drawing
	// a pipe (a remnant, no pipe in hand); the simple rise from below is the
	// grenade-out animation (dev call)
	if (AZP_MeleeEquipAnim)
	{
		PlayMeleeViewAnim(AZP_MeleeEquipAnim, false, AZP_MeleeEquipRate);
		const float StartTime = AZP_MeleeEquipAnim->GetPlayLength() * AZP_ThrowableEquipStartFraction;
		MeleeViewMeshComponent->SetPosition(StartTime, false);
		const float EquipTime = (AZP_MeleeEquipAnim->GetPlayLength() - StartTime) / FMath::Max(AZP_MeleeEquipRate, 0.1f);
		GetWorld()->GetTimerManager().SetTimer(MeleeEquipIdleHandle, [this]()
		{
			if (bMeleeViewModelActive && AZP_MeleeIdleAnim)
			{
				PlayMeleeViewAnim(AZP_MeleeIdleAnim, true, 1.0f);
			}
		}, EquipTime, false);
	}
	else if (AZP_MeleeIdleAnim)
	{
		PlayMeleeViewAnim(AZP_MeleeIdleAnim, true, 1.0f);
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
	GetWorld()->GetTimerManager().ClearTimer(MeleeTailCancelHandle);
	bMeleeSwingActive = false;
	bMeleeSwingTailCancelable = false;

	if (MeleeViewMeshComponent && bPlayUnequip && AZP_MeleeUnequipAnim)
	{
		// Lower the pipe through the swap drop window; hide at the off-screen
		// moment (0.5s — matches EquipWeaponClass phase 2). Clavicle restore is
		// handled by ApplyWeaponConfig when the new weapon's config lands.
		PlayMeleeViewAnim(AZP_MeleeUnequipAnim, false, AZP_MeleeUnequipRate);
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
		}, AZP_MeleeUnequipHideDelay, false);
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
	if (!PC || !CameraComponent || !AZP_GrenadeProjectileClass)
	{
		return;
	}

	const FVector SpawnLoc = CameraComponent->GetComponentLocation()
		+ PC->GetControlRotation().Vector() * AZP_ThrowSpawnForwardOffset;
	const FRotator SpawnRot = PC->GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = OwnerPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Grenade = GetWorld()->SpawnActor<AActor>(
		AZP_GrenadeProjectileClass, SpawnLoc, SpawnRot, SpawnParams);

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
	}, AZP_ThrowCooldownTime, false);
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
	CurrentAmmo = AZP_MagSize > 0 ? AZP_MagSize : 1;
	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);

	// Bring the next grenade up like a fresh equip (dev call) — replay the
	// rise-from-below on the view model, same as the initial grenade equip.
	if (bMeleeViewModelActive && AZP_MeleeEquipAnim && MeleeViewMeshComponent)
	{
		PlayMeleeViewAnim(AZP_MeleeEquipAnim, false, AZP_MeleeEquipRate);
		const float StartTime = AZP_MeleeEquipAnim->GetPlayLength() * AZP_ThrowableEquipStartFraction;
		MeleeViewMeshComponent->SetPosition(StartTime, false);
		const float EquipTime = (AZP_MeleeEquipAnim->GetPlayLength() - StartTime) / FMath::Max(AZP_MeleeEquipRate, 0.1f);
		GetWorld()->GetTimerManager().SetTimer(MeleeEquipIdleHandle, [this]()
		{
			if (bMeleeViewModelActive && AZP_MeleeIdleAnim)
			{
				PlayMeleeViewAnim(AZP_MeleeIdleAnim, true, 1.0f);
			}
		}, EquipTime, false);
	}
}
