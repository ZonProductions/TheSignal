// Copyright The Signal. All Rights Reserved.
#include "KinemationBridge.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY(LogKinemation);

// ── Helpers ──────────────────────────────────────────────────

void FKinemationBridge::CallNoParams(UObject* Target, FName FuncName)
{
	if (!Target) return;
	UFunction* Func = Target->FindFunction(FuncName);
	if (!Func)
	{
		UE_LOG(LogKinemation, Warning, TEXT("Function '%s' not found on %s"),
			*FuncName.ToString(), *Target->GetName());
		return;
	}
	Target->ProcessEvent(Func, nullptr);
}

// ── AC_FirstPersonCamera ────────────────────────────────────

void FKinemationBridge::UpdateTargetCamera(UObject* CameraComp, UCameraComponent* Camera)
{
	if (!CameraComp || !Camera) return;
	UFunction* Func = CameraComp->FindFunction(FName("UpdateTargetCamera"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("UpdateTargetCamera not found")); return; }

	struct { UCameraComponent* Camera; } Params;
	Params.Camera = Camera;
	CameraComp->ProcessEvent(Func, &Params);
}

void FKinemationBridge::UpdatePlayerMesh(UObject* CameraComp, USkeletalMeshComponent* Mesh)
{
	if (!CameraComp || !Mesh) return;
	UFunction* Func = CameraComp->FindFunction(FName("UpdatePlayerMesh"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("UpdatePlayerMesh not found")); return; }

	struct { USkeletalMeshComponent* Mesh; } Params;
	Params.Mesh = Mesh;
	CameraComp->ProcessEvent(Func, &Params);
}

void FKinemationBridge::RefreshOwnerAnimInstance(UObject* CameraComp, USkeletalMeshComponent* Mesh)
{
	if (!CameraComp || !Mesh) return;

	UAnimInstance* AnimInst = Mesh->GetAnimInstance();

	FProperty* Prop = CameraComp->GetClass()->FindPropertyByName(FName("OwnerAnimInstance"));
	if (!Prop)
	{
		UE_LOG(LogKinemation, Warning, TEXT("OwnerAnimInstance property not found on %s"), *CameraComp->GetClass()->GetName());
		return;
	}

	FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop);
	if (!ObjProp) return;

	void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(CameraComp);
	ObjProp->SetObjectPropertyValue(ValuePtr, AnimInst);

	UE_LOG(LogKinemation, Log, TEXT("RefreshOwnerAnimInstance on %s → %s"),
		*CameraComp->GetName(), AnimInst ? *AnimInst->GetClass()->GetName() : TEXT("NULL"));
}

void FKinemationBridge::UpdateTargetFOV(UObject* CameraComp, float NewFOV, float InterpSpeed)
{
	if (!CameraComp) return;
	UFunction* Func = CameraComp->FindFunction(FName("UpdateTargetFOV"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("UpdateTargetFOV not found")); return; }

	struct { float NewFOV; float InterpSpeed; } Params;
	Params.NewFOV = NewFOV;
	Params.InterpSpeed = InterpSpeed;
	CameraComp->ProcessEvent(Func, &Params);
}

void FKinemationBridge::EnableFreeLook(UObject* CameraComp)
{
	CallNoParams(CameraComp, FName("EnableFreeLook"));
}

void FKinemationBridge::DisableFreeLook(UObject* CameraComp)
{
	CallNoParams(CameraComp, FName("DisableFreeLook"));
}

void FKinemationBridge::AddFreeLookInput(UObject* CameraComp, float InputX, float InputY)
{
	if (!CameraComp) return;
	UFunction* Func = CameraComp->FindFunction(FName("AddFreeLookInput"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("AddFreeLookInput not found")); return; }

	struct { float Input_X; float Input_Y; } Params;
	Params.Input_X = InputX;
	Params.Input_Y = InputY;
	CameraComp->ProcessEvent(Func, &Params);
}

void FKinemationBridge::PlayCameraShake(UObject* CameraComp, UObject* ShakeData)
{
	if (!CameraComp) return;
	UFunction* Func = CameraComp->FindFunction(FName("PlayCameraShake"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("PlayCameraShake not found")); return; }

	struct { UObject* NewShake; } Params;
	Params.NewShake = ShakeData;
	CameraComp->ProcessEvent(Func, &Params);
}

// ── AC_TacticalShooterAnimation ─────────────────────────────

void FKinemationBridge::AnimSetAiming(UObject* AnimComp, bool bIsAiming)
{
	if (!AnimComp) return;

	// AC_TacticalShooterAnimation has an IsAiming PROPERTY (not a SetAiming function).
	// Blueprint booleans are bitfield-packed — must use FBoolProperty::SetPropertyValue
	// with the exact address, not the generic FProperty API.
	FBoolProperty* Prop = CastField<FBoolProperty>(AnimComp->GetClass()->FindPropertyByName(FName("IsAiming")));
	if (!Prop)
	{
		UE_LOG(LogKinemation, Warning, TEXT("IsAiming property not found on %s"), *AnimComp->GetClass()->GetName());
		return;
	}

	void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(AnimComp);
	Prop->SetPropertyValue(ValuePtr, bIsAiming);

	UE_LOG(LogKinemation, Log, TEXT("AnimSetAiming on %s → %s (verified: %s)"),
		*AnimComp->GetName(),
		bIsAiming ? TEXT("true") : TEXT("false"),
		Prop->GetPropertyValue(ValuePtr) ? TEXT("true") : TEXT("false"));
}

void FKinemationBridge::AnimSetActiveSettings(UObject* AnimComp, UObject* Settings)
{
	if (!AnimComp || !Settings) return;

	FProperty* Prop = AnimComp->GetClass()->FindPropertyByName(TEXT("ActiveSettings"));
	if (!Prop)
	{
		UE_LOG(LogKinemation, Warning, TEXT("ActiveSettings property not found on %s"), *AnimComp->GetClass()->GetName());
		return;
	}

	FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop);
	if (!ObjProp)
	{
		UE_LOG(LogKinemation, Warning, TEXT("ActiveSettings is not an object property"));
		return;
	}

	void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(AnimComp);
	ObjProp->SetObjectPropertyValue(ValuePtr, Settings);
	UE_LOG(LogKinemation, Log, TEXT("Set ActiveSettings on %s to %s"), *AnimComp->GetName(), *Settings->GetName());
}

void FKinemationBridge::AnimToggleReadyPose(UObject* AnimComp, bool bUseHighReady)
{
	if (!AnimComp) return;
	UFunction* Func = AnimComp->FindFunction(FName("ToggleReadyPose"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("ToggleReadyPose not found")); return; }

	struct { bool UseHighReady; } Params;
	Params.UseHighReady = bUseHighReady;
	AnimComp->ProcessEvent(Func, &Params);
}

// ── AC_RecoilAnimation ──────────────────────────────────────

void FKinemationBridge::RecoilPlay(UObject* RecoilComp)
{
	CallNoParams(RecoilComp, FName("Play"));
}

void FKinemationBridge::RecoilStop(UObject* RecoilComp)
{
	CallNoParams(RecoilComp, FName("Stop"));
}

void FKinemationBridge::RecoilSetAiming(UObject* RecoilComp, bool bIsAiming)
{
	if (!RecoilComp) return;
	UFunction* Func = RecoilComp->FindFunction(FName("SetAiming"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("SetAiming not found on RecoilComp")); return; }

	struct { bool IsAiming; } Params;
	Params.IsAiming = bIsAiming;
	RecoilComp->ProcessEvent(Func, &Params);
}

// ── AC_IKMotionPlayer ───────────────────────────────────────

void FKinemationBridge::IKPlay(UObject* IKComp, UObject* MotionData)
{
	if (!IKComp) return;
	UFunction* Func = IKComp->FindFunction(FName("Play"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("Play not found on IKComp")); return; }

	struct { UObject* NewMotion; } Params;
	Params.NewMotion = MotionData;
	IKComp->ProcessEvent(Func, &Params);
}

// ── Weapon ──────────────────────────────────────────────────

void FKinemationBridge::WeaponDraw(AActor* Weapon)
{
	if (!Weapon) return;
	UFunction* Func = Weapon->FindFunction(FName("Draw"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("Draw not found on weapon")); return; }

	struct { bool PlayAnimation; } Params;
	Params.PlayAnimation = true;
	Weapon->ProcessEvent(Func, &Params);
}

void FKinemationBridge::WeaponOnFirePressed(AActor* Weapon)
{
	CallNoParams(Weapon, FName("OnFirePressed"));
}

void FKinemationBridge::WeaponOnFireReleased(AActor* Weapon)
{
	CallNoParams(Weapon, FName("OnFireReleased"));
}

void FKinemationBridge::WeaponOnReload(AActor* Weapon)
{
	CallNoParams(Weapon, FName("OnReload"));
}

void FKinemationBridge::WeaponChangeFireMode(AActor* Weapon)
{
	CallNoParams(Weapon, FName("ChangeFireMode"));
}

void FKinemationBridge::WeaponOnInspect(AActor* Weapon)
{
	CallNoParams(Weapon, FName("OnInspect"));
}

void FKinemationBridge::WeaponOnMagCheck(AActor* Weapon)
{
	CallNoParams(Weapon, FName("OnMagCheck"));
}

void FKinemationBridge::WeaponOnToggleAttachment(AActor* Weapon)
{
	CallNoParams(Weapon, FName("OnToggleAttachment"));
}

UObject* FKinemationBridge::WeaponGetSettings(AActor* Weapon)
{
	if (!Weapon) return nullptr;
	UFunction* Func = Weapon->FindFunction(FName("GetSettings"));
	if (!Func) { UE_LOG(LogKinemation, Warning, TEXT("GetSettings not found on weapon")); return nullptr; }

	struct { UObject* ReturnValue; } Params;
	Params.ReturnValue = nullptr;
	Weapon->ProcessEvent(Func, &Params);
	return Params.ReturnValue;
}
