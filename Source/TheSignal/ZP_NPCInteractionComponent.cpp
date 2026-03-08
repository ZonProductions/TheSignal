// Copyright The Signal. All Rights Reserved.

#include "ZP_NPCInteractionComponent.h"
#include "ZP_DialogueManager.h"
#include "ZP_DialogueTypes.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SaveGame.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimInstance.h"
#include "Components/AudioComponent.h"
#include "ZP_LipSyncComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPCInteraction, Log, All);

UZP_NPCInteractionComponent::UZP_NPCInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Auto-load our dialogue widget (duplicated from CodeSpartan's demo, customized)
	static ConstructorHelpers::FClassFinder<UUserWidget> DefaultWidget(
		TEXT("/DialoguePlugin/UI/DemoDialogueWidget"));
	if (DefaultWidget.Succeeded())
	{
		DialogueWidgetClass = DefaultWidget.Class;
	}
}

TArray<FString> UZP_NPCInteractionComponent::GetSavedCharacterNames() const
{
	TArray<FString> Names;
	Names.Add(TEXT("None"));

	USaveGame* RawSave = UGameplayStatics::LoadGameFromSlot(TEXT("CC_SaveGame"), 0);
	if (!RawSave) return Names;

	// CC_SaveObject is a Blueprint class — access "Saved Characters" TMap via reflection
	FMapProperty* MapProp = CastField<FMapProperty>(
		RawSave->GetClass()->FindPropertyByName(TEXT("Saved Characters")));
	if (!MapProp) return Names;

	FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(RawSave));
	for (int32 i = 0; i < MapHelper.Num(); ++i)
	{
		if (MapHelper.IsValidIndex(i))
		{
			const FName* Key = (const FName*)MapHelper.GetKeyPtr(i);
			if (Key && *Key != NAME_None)
			{
				Names.Add(Key->ToString());
			}
		}
	}

	return Names;
}

void UZP_NPCInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Make NPC immovable — cannot be pushed by player
	if (ACharacter* Char = Cast<ACharacter>(Owner))
	{
		if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
		{
			CMC->SetMovementMode(MOVE_None);
			CMC->StopMovementImmediately();
			CMC->bEnablePhysicsInteraction = false;
			UE_LOG(LogNPCInteraction, Log, TEXT("NPC '%s': Movement disabled (immovable)."), *Owner->GetName());
		}
	}

	// Create the interaction overlap volume at runtime
	InteractionVolume = NewObject<UBoxComponent>(Owner, TEXT("NPCInteractionVolume"));
	InteractionVolume->SetBoxExtent(FVector(300.f, 300.f, 150.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	InteractionVolume->SetGenerateOverlapEvents(true);
	InteractionVolume->SetupAttachment(Owner->GetRootComponent());
	InteractionVolume->RegisterComponent();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &UZP_NPCInteractionComponent::OnBeginOverlap);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &UZP_NPCInteractionComponent::OnEndOverlap);
}

void UZP_NPCInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInDialogue) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// --- Rotate towards player ---
	if (DialogueInteractor.IsValid())
	{
		FVector NPCLoc = Owner->GetActorLocation();
		FVector PlayerLoc = DialogueInteractor->GetActorLocation();
		FVector Direction = PlayerLoc - NPCLoc;
		Direction.Z = 0.f; // only yaw

		if (!Direction.IsNearlyZero())
		{
			FRotator TargetRot = Direction.Rotation();
			FRotator CurrentRot = Owner->GetActorRotation();
			FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, FacePlayerSpeed);
			Owner->SetActorRotation(NewRot);
		}
	}

	// --- Gesture animations (only when dialogue audio is playing) ---
	if (GestureAnimations.Num() > 0)
	{
		// Check if dialogue audio is actually playing
		bool bAudioPlaying = false;
		if (ActiveDialogueWidget.IsValid() && GestureSound2DProp)
		{
			UObject* AudioObj = GestureSound2DProp->GetObjectPropertyValue(
				GestureSound2DProp->ContainerPtrToValuePtr<void>(ActiveDialogueWidget.Get()));
			UAudioComponent* AudioComp = Cast<UAudioComponent>(AudioObj);
			bAudioPlaying = AudioComp && AudioComp->IsPlaying();
		}

		if (bAudioPlaying)
		{
			GestureTimer -= DeltaTime;
			if (GestureTimer <= 0.f)
			{
				if (ACharacter* Char = Cast<ACharacter>(Owner))
				{
					UAnimInstance* AnimInst = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
					if (!AnimInst)
					{
						// CC may not use default Mesh — find first mesh with an AnimInstance
						TArray<USkeletalMeshComponent*> Meshes;
						Owner->GetComponents<USkeletalMeshComponent>(Meshes);
						for (USkeletalMeshComponent* M : Meshes)
						{
							if (M && M->GetAnimInstance())
							{
								AnimInst = M->GetAnimInstance();
								break;
							}
						}
					}

					if (AnimInst && !AnimInst->IsAnyMontagePlaying())
					{
						int32 Idx = FMath::RandRange(0, GestureAnimations.Num() - 1);
						if (GestureAnimations[Idx])
						{
							AnimInst->PlaySlotAnimationAsDynamicMontage(
								GestureAnimations[Idx], FName("DefaultSlot"), 0.25f, 0.25f);
						}
					}
				}
				GestureTimer = FMath::RandRange(3.f, 7.f);
			}
		}
	}
}

void UZP_NPCInteractionComponent::HandleInteract(ACharacter* Interactor)
{
	if (!Interactor) return;
	if (bInteractOnce && bHasBeenInteracted) return;

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (!PC) return;

	// --- Path 1: CodeSpartan Dialogue Plugin (preferred) ---
	if (PluginDialogue && DialogueWidgetClass)
	{
		// Don't double-create if a dialogue widget is already active
		if (ActiveDialogueWidget.IsValid() && ActiveDialogueWidget->IsInViewport())
		{
			UE_LOG(LogNPCInteraction, Log, TEXT("NPC '%s': Dialogue widget already active, skipping."), *GetOwner()->GetName());
			return;
		}

		UUserWidget* Widget = CreateWidget<UUserWidget>(PC, DialogueWidgetClass);
		if (!Widget)
		{
			UE_LOG(LogNPCInteraction, Error, TEXT("NPC '%s': Failed to create dialogue widget."), *GetOwner()->GetName());
			return;
		}

		// Set NPCActor on the widget via reflection (plugin's UDialogueUserWidget::NPCActor)
		FObjectProperty* NPCActorProp = CastField<FObjectProperty>(
			Widget->GetClass()->FindPropertyByName(TEXT("NPCActor")));
		if (NPCActorProp)
		{
			NPCActorProp->SetObjectPropertyValue(
				NPCActorProp->ContainerPtrToValuePtr<void>(Widget), GetOwner());
		}

		// Set the Dialogue asset via reflection — plugin's DemoDialogueWidget uses "InDialogue"
		FObjectProperty* DialogueProp = CastField<FObjectProperty>(
			Widget->GetClass()->FindPropertyByName(TEXT("InDialogue")));
		if (DialogueProp)
		{
			DialogueProp->SetObjectPropertyValue(
				DialogueProp->ContainerPtrToValuePtr<void>(Widget), PluginDialogue.Get());
		}
		else
		{
			UE_LOG(LogNPCInteraction, Warning, TEXT("NPC '%s': Widget has no 'InDialogue' property — dialogue won't load."), *GetOwner()->GetName());
		}

		Widget->AddToViewport(100);
		ActiveDialogueWidget = Widget;

		// Start dialogue behaviors (face player, gestures)
		bInDialogue = true;
		DialogueInteractor = Interactor;
		GestureTimer = FMath::RandRange(2.f, 5.f);
		GestureSound2DProp = CastField<FObjectProperty>(
			Widget->GetClass()->FindPropertyByName(TEXT("Sound2D")));
		SetComponentTickEnabled(true);

		// Start lip sync if the NPC has a LipSyncComponent
		if (UZP_LipSyncComponent* LipSync = GetOwner()->FindComponentByClass<UZP_LipSyncComponent>())
		{
			LipSync->SetDialogueWidget(Widget);
		}

		// Switch to UI input so player can click dialogue choices
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(Widget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;

		// Poll for widget self-close (e.g. end of dialogue RemoveFromParent)
		GetWorld()->GetTimerManager().SetTimer(DialogueWatchTimer, this,
			&UZP_NPCInteractionComponent::CheckDialogueWidget, 0.25f, true);

		UE_LOG(LogNPCInteraction, Log, TEXT("NPC '%s': Plugin dialogue started."), *GetOwner()->GetName());

		if (bInteractOnce)
		{
			bHasBeenInteracted = true;
		}
		return;
	}

	// --- Path 2: Custom ZP_DialogueManager (fallback) ---
	if (!DialogueData)
	{
		UE_LOG(LogNPCInteraction, Warning, TEXT("NPC '%s': No PluginDialogue or DialogueData assigned."), *GetOwner()->GetName());
		return;
	}

	UZP_DialogueManager* Manager = PC->FindComponentByClass<UZP_DialogueManager>();
	if (!Manager)
	{
		UE_LOG(LogNPCInteraction, Warning, TEXT("NPC '%s': No DialogueManager on PlayerController."), *GetOwner()->GetName());
		return;
	}

	UE_LOG(LogNPCInteraction, Log, TEXT("NPC '%s' — playing dialogue '%s'."),
		*GetOwner()->GetName(), *DialogueData->DialogueID.ToString());

	Manager->PlayDialogue(DialogueData);

	if (bInteractOnce)
	{
		bHasBeenInteracted = true;
	}
}

void UZP_NPCInteractionComponent::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->SetCurrentInteractable(GetOwner());

	// Show prompt on HUD
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->ShowInteractionPrompt(InteractionPrompt);
	}
}

void UZP_NPCInteractionComponent::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->ClearCurrentInteractable(GetOwner());

	// Hide prompt
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->HideInteractionPrompt();
	}

	// Close active dialogue if player walks away
	CloseDialogue();
}

void UZP_NPCInteractionComponent::CloseDialogue()
{
	// Stop dialogue behaviors
	bInDialogue = false;
	DialogueInteractor.Reset();
	GestureSound2DProp = nullptr;
	SetComponentTickEnabled(false);

	// Stop any playing gesture animation
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		if (UAnimInstance* AnimInst = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInst->StopAllMontages(0.25f);
		}
	}

	// Stop the watch timer
	GetWorld()->GetTimerManager().ClearTimer(DialogueWatchTimer);

	// Stop lip sync
	if (UZP_LipSyncComponent* LipSync = GetOwner()->FindComponentByClass<UZP_LipSyncComponent>())
	{
		LipSync->ClearDialogueWidget();
	}

	if (ActiveDialogueWidget.IsValid() && ActiveDialogueWidget->IsInViewport())
	{
		ActiveDialogueWidget->RemoveFromParent();
	}
	ActiveDialogueWidget.Reset();

	// Restore game-only input
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}

	UE_LOG(LogNPCInteraction, Log, TEXT("NPC '%s': Dialogue closed."), *GetOwner()->GetName());
}

void UZP_NPCInteractionComponent::CheckDialogueWidget()
{
	// Widget removed itself (end of dialogue) — restore input
	if (!ActiveDialogueWidget.IsValid() || !ActiveDialogueWidget->IsInViewport())
	{
		// Stop dialogue behaviors
		bInDialogue = false;
		DialogueInteractor.Reset();
		GestureSound2DProp = nullptr;
		SetComponentTickEnabled(false);
		if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
		{
			if (UAnimInstance* AnimInst = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
			{
				AnimInst->StopAllMontages(0.25f);
			}
		}

		GetWorld()->GetTimerManager().ClearTimer(DialogueWatchTimer);
		ActiveDialogueWidget.Reset();

		// Stop lip sync
		if (UZP_LipSyncComponent* LipSync = GetOwner()->FindComponentByClass<UZP_LipSyncComponent>())
		{
			LipSync->ClearDialogueWidget();
		}

		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}

		UE_LOG(LogNPCInteraction, Log, TEXT("NPC '%s': Dialogue widget self-closed, input restored."), *GetOwner()->GetName());
	}
}
