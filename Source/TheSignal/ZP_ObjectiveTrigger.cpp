// Copyright The Signal. All Rights Reserved.

#include "ZP_ObjectiveTrigger.h"
#include "ZP_ObjectiveSubsystem.h"
#include "ZP_GraceCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AZP_ObjectiveTrigger::AZP_ObjectiveTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(200.f, 200.f, 150.f));
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerVolume->SetGenerateOverlapEvents(true);
}

void AZP_ObjectiveTrigger::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_ObjectiveTrigger::OnOverlapBegin);
}

void AZP_ObjectiveTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[ObjTrigger] %s OVERLAP by %s (isPlayer=%d, bFired=%d, bAZP_OneShot=%d)"),
		*GetName(), OtherActor ? *OtherActor->GetName() : TEXT("NULL"),
		Cast<AZP_GraceCharacter>(OtherActor) != nullptr, bFired, bAZP_OneShot);
	if (bAZP_OneShot && bFired) return;
	if (!Cast<AZP_GraceCharacter>(OtherActor)) return; // players only
	Fire();
}

void AZP_ObjectiveTrigger::Fire()
{
	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UZP_ObjectiveSubsystem* Obj = GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr;
	if (!Obj) { UE_LOG(LogTemp, Warning, TEXT("[ObjTrigger] %s: no ObjectiveSubsystem"), *GetName()); return; }

	// Optional gate: only fire while the named main objective is active.
	if (!AZP_RequiresActiveObjective.IsNone() && !Obj->IsObjectiveActive(AZP_RequiresActiveObjective))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ObjTrigger] %s: GATED — AZP_RequiresActiveObjective '%s' is NOT active right now, so nothing fired. (Did you reach the step / is OFFICE1 active?)"),
			*GetName(), *AZP_RequiresActiveObjective.ToString());
		return;
	}

	if (AZP_TargetId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ObjectiveTrigger %s: AZP_TargetId is None — nothing to fire"), *GetName());
		return;
	}

	switch (AZP_Action)
	{
	case EZP_ObjectiveTriggerAction::SetFlag:              Obj->SetFlag(AZP_TargetId); break;
	case EZP_ObjectiveTriggerAction::CompleteSubObjective: Obj->CompleteSubObjective(AZP_TargetId); break;
	case EZP_ObjectiveTriggerAction::CompleteObjective:    Obj->CompleteObjective(AZP_TargetId); break;
	case EZP_ObjectiveTriggerAction::StartObjective:       Obj->StartObjective(AZP_TargetId); break;
	}

	bFired = true;
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveTrigger %s: fired %s '%s'"),
		*GetName(), *UEnum::GetValueAsString(AZP_Action), *AZP_TargetId.ToString());
}
