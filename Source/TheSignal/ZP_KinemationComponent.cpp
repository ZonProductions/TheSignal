// Copyright The Signal. All Rights Reserved.

#include "ZP_KinemationComponent.h"
#include "KinemationBridge.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

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

	// Spawn and equip default weapon (gracefully skips if WeaponClass not set)
	SpawnAndEquipWeapon();

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

void UZP_KinemationComponent::FirePressed()
{
	if (ActiveWeapon)
	{
		FKinemationBridge::WeaponOnFirePressed(ActiveWeapon);
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
	if (ActiveWeapon)
	{
		FKinemationBridge::WeaponOnReload(ActiveWeapon);
	}
}
