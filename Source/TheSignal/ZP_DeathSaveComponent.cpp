// Copyright The Signal. All Rights Reserved.

#include "ZP_DeathSaveComponent.h"
#include "ZP_HealthComponent.h"
#include "ZP_ObjectiveSubsystem.h"
#include "ZP_Revivable.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

static const FString GZP_DeadKey(TEXT("ZP_Dead"));

UZP_DeathSaveComponent::UZP_DeathSaveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_DeathSaveComponent::BeginPlay()
{
	Super::BeginPlay();

	// Defer one tick so the sibling BP_EasySaveGameComponent exists + has registered, and so the
	// GameInstance ObjectiveSubsystem is ready.
	if (UWorld* W = GetWorld())
	{
		TWeakObjectPtr<UZP_DeathSaveComponent> Weak(this);
		W->GetTimerManager().SetTimerForNextTick([Weak]()
		{
			if (UZP_DeathSaveComponent* Self = Weak.Get())
			{
				Self->BindSaveComponent();
				Self->BindObjectiveEvents();
			}
		});
	}
}

void UZP_DeathSaveComponent::BindSaveComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UActorComponent* SaveComp = nullptr;
	for (UActorComponent* C : Owner->GetComponents())
	{
		if (C && C->GetClass()->GetName().Contains(TEXT("EasySaveGameComponent"))) { SaveComp = C; break; }
	}
	if (!SaveComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeathSave] %s: no BP_EasySaveGameComponent — enemy death will NOT persist (add it: ActorSaveType=Persistent, HasVariablesToSave=true, RegisterDestruction=false)."), *Owner->GetName());
		return;
	}

	FMulticastDelegateProperty* DProp = CastField<FMulticastDelegateProperty>(
		SaveComp->GetClass()->FindPropertyByName(FName("LoadingOrSavingVariables")));
	if (!DProp) { UE_LOG(LogTemp, Warning, TEXT("[DeathSave] %s: LoadingOrSavingVariables not found"), *Owner->GetName()); return; }

	FScriptDelegate Del;
	Del.BindUFunction(this, FName("OnEguiSaveLoadVariables"));
	DProp->AddDelegate(MoveTemp(Del), SaveComp, DProp->ContainerPtrToValuePtr<void>(SaveComp));
	UE_LOG(LogTemp, Log, TEXT("[DeathSave] %s: bound to EGUI save/load"), *Owner->GetName());
}

void UZP_DeathSaveComponent::BindObjectiveEvents()
{
	if (ReviveOnObjective.IsNone()) return; // nothing to listen for — this enemy never auto-revives

	UZP_ObjectiveSubsystem* Obj = GetObjectiveSubsystem();
	if (!Obj)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeathSave] %s: ReviveOnObjective='%s' set but no ObjectiveSubsystem — cannot bind revive."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"), *ReviveOnObjective.ToString());
		return;
	}

	Obj->OnObjectiveCompleted.AddDynamic(this, &UZP_DeathSaveComponent::OnObjectiveCompletedEvt);
	Obj->OnSubObjectiveCompleted.AddDynamic(this, &UZP_DeathSaveComponent::OnSubObjectiveCompletedEvt);
	Obj->OnFlagSet.AddDynamic(this, &UZP_DeathSaveComponent::OnFlagSetEvt);
	UE_LOG(LogTemp, Log, TEXT("[DeathSave] %s: listening to revive on '%s'"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"), *ReviveOnObjective.ToString());
}

void UZP_DeathSaveComponent::OnEguiSaveLoadVariables(uint8 OperationType, FJsonObjectWrapper JsonObject)
{
	if (!JsonObject.JsonObject.IsValid()) return;

	UZP_HealthComponent* Health = GetHealth();

	if (OperationType == 0) // SAVE — record whether this enemy is dead
	{
		JsonObject.JsonObject->SetBoolField(GZP_DeadKey, Health && Health->bIsDead);
		return;
	}

	// LOAD — restore corpse state if it was dead at save time.
	bool bWasDead = false;
	JsonObject.JsonObject->TryGetBoolField(GZP_DeadKey, bWasDead);
	if (!bWasDead) return;

	// If the revive beat has ALREADY happened (objective/flag complete by the time this slot loads), the
	// enemy belongs alive — don't re-corpse it. Covers load-order / cross-level robustness.
	if (IsReviveConditionMet())
	{
		UE_LOG(LogTemp, Log, TEXT("[DeathSave] %s: was dead at save but revive beat '%s' already met — staying alive."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"), *ReviveOnObjective.ToString());
		return;
	}

	// Defer a tick so the just-restored actor/components are settled before we pose it as a corpse.
	if (UWorld* W = GetWorld())
	{
		TWeakObjectPtr<UZP_DeathSaveComponent> Weak(this);
		W->GetTimerManager().SetTimerForNextTick([Weak]()
		{
			if (UZP_DeathSaveComponent* Self = Weak.Get()) { Self->RestoreDeadState(); }
		});
	}
}

void UZP_DeathSaveComponent::RestoreDeadState()
{
	if (UObject* Rev = FindRevivable())
	{
		IZP_Revivable::Execute_ApplyDeadStateInstant(Rev);
		UE_LOG(LogTemp, Log, TEXT("[DeathSave] %s: restored dead/corpse state on load."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"));
		return;
	}

	// Generic fallback for an enemy that doesn't implement IZP_Revivable: re-apply lethal damage so its
	// own OnDied corpse path runs. (The death anim replays, but the enemy correctly stays dead.)
	if (UZP_HealthComponent* H = GetHealth())
	{
		if (!H->bIsDead) { H->ApplyDamage(H->MaxHealth * 10.f); }
		UE_LOG(LogTemp, Warning, TEXT("[DeathSave] %s: no IZP_Revivable — used generic re-kill to restore dead state (revival unavailable for this enemy)."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"));
	}
}

void UZP_DeathSaveComponent::OnObjectiveCompletedEvt(FName ObjectiveId)   { HandleReviveTrigger(ObjectiveId); }
void UZP_DeathSaveComponent::OnSubObjectiveCompletedEvt(FName SubObjectiveId) { HandleReviveTrigger(SubObjectiveId); }
void UZP_DeathSaveComponent::OnFlagSetEvt(FName Flag)                     { HandleReviveTrigger(Flag); }

void UZP_DeathSaveComponent::HandleReviveTrigger(FName Id)
{
	if (Id.IsNone() || Id != ReviveOnObjective) return;

	UZP_HealthComponent* H = GetHealth();
	if (!H || !H->bIsDead) return; // only a dead enemy can be revived

	if (UObject* Rev = FindRevivable())
	{
		IZP_Revivable::Execute_ReviveEnemy(Rev);
		UE_LOG(LogTemp, Log, TEXT("[DeathSave] %s: REVIVED by '%s'."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"), *Id.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeathSave] %s: revive beat '%s' fired but enemy has no IZP_Revivable — cannot revive."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"), *Id.ToString());
	}
}

bool UZP_DeathSaveComponent::IsReviveConditionMet() const
{
	if (ReviveOnObjective.IsNone()) return false;
	if (UZP_ObjectiveSubsystem* Obj = GetObjectiveSubsystem())
	{
		return Obj->IsObjectiveComplete(ReviveOnObjective)
			|| Obj->IsSubObjectiveComplete(ReviveOnObjective)
			|| Obj->HasFlag(ReviveOnObjective);
	}
	return false;
}

UZP_HealthComponent* UZP_DeathSaveComponent::GetHealth() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UZP_HealthComponent>() : nullptr;
}

UZP_ObjectiveSubsystem* UZP_DeathSaveComponent::GetObjectiveSubsystem() const
{
	UWorld* W = GetWorld();
	UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr;
}

UObject* UZP_DeathSaveComponent::FindRevivable() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	if (Owner->GetClass()->ImplementsInterface(UZP_Revivable::StaticClass())) { return Owner; }

	for (UActorComponent* C : Owner->GetComponents())
	{
		if (C && C->GetClass()->ImplementsInterface(UZP_Revivable::StaticClass())) { return C; }
	}
	return nullptr;
}
