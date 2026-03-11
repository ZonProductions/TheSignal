// Copyright The Signal. All Rights Reserved.

#include "ZP_Ladder.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"

AZP_Ladder::AZP_Ladder()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	LadderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LadderMesh"));
	LadderMesh->SetupAttachment(RootComponent);
	LadderMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Default mesh — can be overridden per-instance in editor
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultLadderMesh(
		TEXT("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_Ladder"));
	if (DefaultLadderMesh.Succeeded())
	{
		LadderMesh->SetStaticMesh(DefaultLadderMesh.Object);
	}

	// Arrow shows the direction player faces while climbing (points INTO ladder surface)
	ClimbDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ClimbDirection"));
	ClimbDirection->SetupAttachment(RootComponent);
	ClimbDirection->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	ClimbDirection->SetArrowColor(FLinearColor::Green);

	// Bottom: where player mounts from ground level
	BottomAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BottomAttachPoint"));
	BottomAttachPoint->SetupAttachment(RootComponent);
	// X=-100: capsule radius=55, so player's back is 45 UU from ladder root.
	// Prevents clipping into the wall behind the ladder.
	// Y=0 — SM_Ladder mesh is centered at Y≈-1, essentially on-center.
	BottomAttachPoint->SetRelativeLocation(FVector(-100.f, 0.f, 0.f));

	// Top: where player exits after climbing up
	TopExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TopExitPoint"));
	TopExitPoint->SetupAttachment(RootComponent);
	// Z=585 is just above the SM_Ladder mesh top (582.4 UU). ExitLadder adds
	// capsule half-height so the player's feet land on the upper floor surface.
	TopExitPoint->SetRelativeLocation(FVector(-100.f, 0.f, 585.f));

	// Interaction trigger volume — tall box covering the ladder area
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetBoxExtent(FVector(100.f, 100.f, 350.f));
	InteractionVolume->SetRelativeLocation(FVector(-100.f, 0.f, 300.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);
}

void AZP_Ladder::BeginPlay()
{
	Super::BeginPlay();

	// Force volume size at runtime — overrides any baked BP values
	InteractionVolume->SetBoxExtent(FVector(100.f, 100.f, 350.f));
	InteractionVolume->SetRelativeLocation(FVector(-100.f, 0.f, 300.f));

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_Ladder::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_Ladder::OnOverlapEnd);

	UE_LOG(LogTemp, Log, TEXT("[ZP-BUG] Ladder %s: BeginPlay — InteractionVolume extent=(%.0f, %.0f, %.0f) at (%.0f, %.0f, %.0f)"),
		*GetName(),
		InteractionVolume->GetScaledBoxExtent().X, InteractionVolume->GetScaledBoxExtent().Y, InteractionVolume->GetScaledBoxExtent().Z,
		InteractionVolume->GetComponentLocation().X, InteractionVolume->GetComponentLocation().Y, InteractionVolume->GetComponentLocation().Z);
}

float AZP_Ladder::GetBottomZ() const
{
	return BottomAttachPoint->GetComponentLocation().Z;
}

float AZP_Ladder::GetTopZ() const
{
	return TopExitPoint->GetComponentLocation().Z;
}

FRotator AZP_Ladder::GetClimbFacingRotation() const
{
	return ClimbDirection->GetComponentRotation();
}

FVector AZP_Ladder::GetBottomAttachLocation() const
{
	return BottomAttachPoint->GetComponentLocation();
}

FVector AZP_Ladder::GetTopExitLocation() const
{
	return TopExitPoint->GetComponentLocation();
}

FText AZP_Ladder::GetInteractionPrompt_Implementation()
{
	return FText::FromString(TEXT("Climb Ladder"));
}

void AZP_Ladder::OnInteract_Implementation(ACharacter* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Ladder %s: OnInteract fired — Interactor=%s"),
		*GetName(), Interactor ? *Interactor->GetName() : TEXT("NULL"));

	if (AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Interactor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Ladder %s: Calling Grace->EnterLadder()"), *GetName());
		Grace->EnterLadder(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Ladder %s: Interactor is NOT GraceCharacter!"), *GetName());
	}
}

void AZP_Ladder::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Ladder %s: Player entered interaction range"), *GetName());

	Grace->SetCurrentInteractable(this);

	// Don't show prompt while actively climbing
	if (Grace->bOnLadder) return;

	// Show interaction prompt on HUD
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		FText Prompt = IZP_Interactable::Execute_GetInteractionPrompt(this);
		PC->HUDWidget->ShowInteractionPrompt(Prompt);
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Ladder %s: Showing prompt '%s'"), *GetName(), *Prompt.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Ladder %s: No PC/HUDWidget — can't show prompt (PC=%s, HUD=%s)"),
			*GetName(), PC ? TEXT("OK") : TEXT("NULL"), (PC && PC->HUDWidget) ? TEXT("OK") : TEXT("NULL"));
	}
}

void AZP_Ladder::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Ladder %s: Player left interaction range"), *GetName());

	Grace->ClearCurrentInteractable(this);

	// Hide interaction prompt
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->HideInteractionPrompt();
	}
}
