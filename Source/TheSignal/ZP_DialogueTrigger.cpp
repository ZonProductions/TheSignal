// Copyright The Signal. All Rights Reserved.

#include "ZP_DialogueTrigger.h"
#include "ZP_DialogueManager.h"
#include "ZP_DialogueTypes.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogDialogueTrigger, Log, All);

AZP_DialogueTrigger::AZP_DialogueTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	TriggerBox->SetGenerateOverlapEvents(true);

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AZP_DialogueTrigger::OnTriggerBeginOverlap);
}

void AZP_DialogueTrigger::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bEnabled || !DialogueData) return;

	// Only trigger for player characters
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Character->IsPlayerControlled()) return;

	// Find DialogueManager on the player's controller
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC) return;

	UZP_DialogueManager* Manager = PC->FindComponentByClass<UZP_DialogueManager>();
	if (!Manager)
	{
		UE_LOG(LogDialogueTrigger, Warning, TEXT("DialogueTrigger '%s': No DialogueManager on PlayerController."), *GetName());
		return;
	}

	UE_LOG(LogDialogueTrigger, Log, TEXT("DialogueTrigger '%s' activated — playing '%s'."),
		*GetName(), *DialogueData->DialogueID.ToString());

	Manager->PlayDialogue(DialogueData);

	if (bTriggerOnce)
	{
		bEnabled = false;
	}
}
