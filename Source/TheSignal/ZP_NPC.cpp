// Copyright The Signal. All Rights Reserved.

#include "ZP_NPC.h"
#include "ZP_DialogueManager.h"
#include "ZP_DialogueTypes.h"
#include "ZP_GraceCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogZPNPC, Log, All);

AZP_NPC::AZP_NPC()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetBoxExtent(FVector(120.f, 120.f, 100.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	InteractionVolume->SetGenerateOverlapEvents(true);

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_NPC::OnInteractionBeginOverlap);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_NPC::OnInteractionEndOverlap);
}

void AZP_NPC::BeginPlay()
{
	Super::BeginPlay();

	// Crowd NPCs don't need interaction
	if (NPCRole == EZP_NPCNPCRole::Crowd)
	{
		InteractionVolume->SetGenerateOverlapEvents(false);
		InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

// --- IZP_Interactable ---

FText AZP_NPC::GetInteractionPrompt_Implementation()
{
	// Custom override takes priority
	if (!InteractionPromptOverride.IsEmpty())
	{
		return InteractionPromptOverride;
	}

	// Auto-generate from role
	switch (NPCRole)
	{
	case EZP_NPCNPCRole::Interactive:
		return FText::FromString(TEXT("Talk"));
	case EZP_NPCNPCRole::Corpse:
		return DialogueData ? FText::FromString(TEXT("Examine")) : FText::GetEmpty();
	case EZP_NPCNPCRole::Crowd:
	default:
		return FText::GetEmpty();
	}
}

void AZP_NPC::OnInteract_Implementation(ACharacter* Interactor)
{
	if (!Interactor) return;

	// One-shot guard
	if (bInteractOnce && bHasBeenInteracted) return;

	// Crowd NPCs are not interactable
	if (NPCRole == EZP_NPCNPCRole::Crowd) return;

	// Need dialogue data to do anything
	if (!DialogueData)
	{
		UE_LOG(LogZPNPC, Warning, TEXT("NPC '%s': OnInteract but no DialogueData assigned."), *GetName());
		return;
	}

	// Find DialogueManager on the player's controller
	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (!PC) return;

	UZP_DialogueManager* Manager = PC->FindComponentByClass<UZP_DialogueManager>();
	if (!Manager)
	{
		UE_LOG(LogZPNPC, Warning, TEXT("NPC '%s': No DialogueManager on PlayerController."), *GetName());
		return;
	}

	UE_LOG(LogZPNPC, Log, TEXT("NPC '%s' (%s) — playing dialogue '%s'."),
		*GetName(),
		NPCRole == EZP_NPCNPCRole::Interactive ? TEXT("Interactive") : TEXT("Corpse"),
		*DialogueData->DialogueID.ToString());

	Manager->PlayDialogue(DialogueData);

	if (bInteractOnce)
	{
		bHasBeenInteracted = true;
	}
}

// --- Overlap ---

void AZP_NPC::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->SetCurrentInteractable(this);
}

void AZP_NPC::OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->ClearCurrentInteractable(this);
}
