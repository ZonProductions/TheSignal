// Copyright The Signal. All Rights Reserved.

#include "ZP_Ladder.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/StaticMesh.h"

// Spacing values from BP_MasterLadder (consistent across all 5 styles)
static constexpr float AZP_FootBarSpread = 23.5f;
static constexpr float AZP_SideDistance = 23.5f;
// NOTE: Rail meshes already contain correct X offsets in their geometry (~±28 UU from origin).
// Components are placed at root origin (0,0,0) — no additional X offset needed.

AZP_Ladder::AZP_Ladder()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// LadderMesh: kept for backward compat (BP instances may reference it).
	// No mesh, no collision, no visibility — purely a placeholder.
	LadderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LadderMesh"));
	LadderMesh->SetupAttachment(RootComponent);
	LadderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LadderMesh->SetVisibility(false);
	LadderMesh->SetCastShadow(false);

	// --- Modular visual components (hierarchy mirrors BP_MasterLadder) ---

	// Footbar rungs — ISM attached to root
	FootBarISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FootBarISM"));
	FootBarISM->SetupAttachment(RootComponent);
	FootBarISM->SetRelativeLocation(FVector(0.f, 0.f, -2.059338f)); // from BP_MasterLadder
	FootBarISM->SetCollisionProfileName(TEXT("BlockAll"));

	// Left rail chain: BotL → MidL (ISM) → TopL
	BottomLeftRail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BottomLeftRail"));
	BottomLeftRail->SetupAttachment(RootComponent);
	BottomLeftRail->SetCollisionProfileName(TEXT("BlockAll"));

	MidLeftISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MidLeftISM"));
	MidLeftISM->SetupAttachment(BottomLeftRail); // child of BotL (like BP_MasterLadder)
	MidLeftISM->SetRelativeLocation(FVector(0.f, 0.f, AZP_SideDistance));
	MidLeftISM->SetCollisionProfileName(TEXT("BlockAll"));

	TopLeftCap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopLeftCap"));
	TopLeftCap->SetupAttachment(RootComponent); // attach to root, position manually
	TopLeftCap->SetCollisionProfileName(TEXT("BlockAll"));

	// Right rail chain: BotR → MidR (ISM) → TopR
	BottomRightRail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BottomRightRail"));
	BottomRightRail->SetupAttachment(RootComponent);
	BottomRightRail->SetCollisionProfileName(TEXT("BlockAll"));

	MidRightISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MidRightISM"));
	MidRightISM->SetupAttachment(BottomRightRail); // child of BotR
	MidRightISM->SetRelativeLocation(FVector(0.f, 0.f, AZP_SideDistance));
	MidRightISM->SetCollisionProfileName(TEXT("BlockAll"));

	TopRightCap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopRightCap"));
	TopRightCap->SetupAttachment(RootComponent);
	TopRightCap->SetCollisionProfileName(TEXT("BlockAll"));

	// Arrow shows the direction player faces while climbing (points INTO ladder surface)
	ClimbDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ClimbDirection"));
	ClimbDirection->SetupAttachment(RootComponent);
	ClimbDirection->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	ClimbDirection->SetArrowColor(FLinearColor::Green);

	// Bottom: where player mounts from ground level
	BottomAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BottomAttachPoint"));
	BottomAttachPoint->SetupAttachment(RootComponent);
	BottomAttachPoint->SetRelativeLocation(FVector(AZP_MountStandoffX, 0.f, 0.f));

	// Top: where player exits after climbing up — Z auto-adjusted in BuildLadderAssembly
	TopExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TopExitPoint"));
	TopExitPoint->SetupAttachment(RootComponent);
	TopExitPoint->SetRelativeLocation(FVector(AZP_MountStandoffX, 0.f, 585.f));

	// Interaction trigger volume — auto-adjusted in BuildLadderAssembly
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetBoxExtent(FVector(AZP_InteractionPocketExtent.X, AZP_InteractionPocketExtent.Y, 350.f)); // small front pocket (see OnConstruction)
	InteractionVolume->SetRelativeLocation(FVector(AZP_MountStandoffX, 0.f, 300.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);
}

// ---------- Geometry helpers ----------

FVector AZP_Ladder::GetLadderCenter() const
{
	// Center between left/right rails — mesh geometry is symmetric around X=0
	FVector LocalCenter(0.f, 0.f, AZP_LadderHeight * 0.5f);
	return GetActorLocation() + GetActorRotation().RotateVector(LocalCenter);
}

FVector AZP_Ladder::GetLadderSurfaceNormal() const
{
	// Ladder surface is perpendicular to the local Y axis (the thin/depth dimension).
	// Returns the +Y direction in world space. GraceCharacter picks +/- based on player position.
	FVector WorldNormal = GetActorRotation().RotateVector(FVector(0.f, 1.f, 0.f));
	WorldNormal.Z = 0.f;
	WorldNormal.Normalize();
	return WorldNormal;
}

// ---------- Mesh path resolution ----------

FString AZP_Ladder::GetMeshPath(EZP_LadderStyle Style, const FString& PieceName)
{
	const int32 N = static_cast<int32>(Style) + 1; // enum is 0-4, folders are Ladder1-5
	const FString BaseDir = FString::Printf(TEXT("/Game/LadderClimbingSystem/Meshes/Ladders/Ladder%d"), N);

	// The pack has inconsistent naming across styles. Map each piece to its actual asset name.
	if (PieceName == TEXT("FootBar"))
	{
		if (N == 2 || N == 4)
			return BaseDir / FString::Printf(TEXT("SM_Ladder%d_Footbar"), N);
		return BaseDir / FString::Printf(TEXT("SM_Ladder%d_FootBar"), N);
	}
	if (PieceName == TEXT("BottomLeft"))
	{
		return BaseDir / FString::Printf(TEXT("SM_Ladder%d_BottomLeft"), N);
	}
	if (PieceName == TEXT("BottomRight"))
	{
		if (N == 2)
			return BaseDir / FString::Printf(TEXT("SM_Ladder%d_BottomRIght"), N);
		return BaseDir / FString::Printf(TEXT("SM_Ladder%d_BottomRight"), N);
	}
	if (PieceName == TEXT("MidLeftWithoutPanel"))
	{
		if (N == 2)
			return BaseDir / FString::Printf(TEXT("SM_Ladder%d_LeftMidWithoutPanel"), N);
		return BaseDir / FString::Printf(TEXT("SM_Ladder%d_MidLeft_WithoutPanel"), N);
	}
	if (PieceName == TEXT("MidRightWithoutPanel"))
	{
		if (N == 2)
			return BaseDir / FString::Printf(TEXT("SM_Ladder%d_RightMidWithoutPanel"), N);
		return BaseDir / FString::Printf(TEXT("SM_Ladder%d_MidRight_WithoutPanel"), N);
	}
	if (PieceName == TEXT("TopLeft"))
	{
		if (N == 5)
			return BaseDir / FString::Printf(TEXT("SM_Ladder%d_TopL"), N);
		return BaseDir / FString::Printf(TEXT("SM_Ladder%d_TopLeft"), N);
	}
	if (PieceName == TEXT("TopRight"))
	{
		if (N == 5)
			return BaseDir / FString::Printf(TEXT("SM_Ladder%d_TopR"), N);
		return BaseDir / FString::Printf(TEXT("SM_Ladder%d_TopRight"), N);
	}

	return FString();
}

bool AZP_Ladder::StyleHasTopCaps(EZP_LadderStyle Style)
{
	return (Style == EZP_LadderStyle::Style2 || Style == EZP_LadderStyle::Style4 || Style == EZP_LadderStyle::Style5);
}

// ---------- Assembly ----------

void AZP_Ladder::BuildLadderAssembly()
{
	// Load meshes for the selected style
	auto LoadMesh = [](const FString& Path) -> UStaticMesh*
	{
		if (Path.IsEmpty()) return nullptr;
		return LoadObject<UStaticMesh>(nullptr, *Path);
	};

	UStaticMesh* FootBarMesh = LoadMesh(GetMeshPath(AZP_LadderStyle, TEXT("FootBar")));
	UStaticMesh* BotLeftMesh = LoadMesh(GetMeshPath(AZP_LadderStyle, TEXT("BottomLeft")));
	UStaticMesh* BotRightMesh = LoadMesh(GetMeshPath(AZP_LadderStyle, TEXT("BottomRight")));
	UStaticMesh* MidLeftMesh = LoadMesh(GetMeshPath(AZP_LadderStyle, TEXT("MidLeftWithoutPanel")));
	UStaticMesh* MidRightMesh = LoadMesh(GetMeshPath(AZP_LadderStyle, TEXT("MidRightWithoutPanel")));
	UStaticMesh* TopLeftMesh = StyleHasTopCaps(AZP_LadderStyle) ? LoadMesh(GetMeshPath(AZP_LadderStyle, TEXT("TopLeft"))) : nullptr;
	UStaticMesh* TopRightMesh = StyleHasTopCaps(AZP_LadderStyle) ? LoadMesh(GetMeshPath(AZP_LadderStyle, TEXT("TopRight"))) : nullptr;

	// Assign meshes
	if (FootBarISM) { FootBarISM->SetStaticMesh(FootBarMesh); }
	if (BottomLeftRail) { BottomLeftRail->SetStaticMesh(BotLeftMesh); }
	if (BottomRightRail) { BottomRightRail->SetStaticMesh(BotRightMesh); }
	if (MidLeftISM) { MidLeftISM->SetStaticMesh(MidLeftMesh); }
	if (MidRightISM) { MidRightISM->SetStaticMesh(MidRightMesh); }
	if (TopLeftCap) { TopLeftCap->SetStaticMesh(TopLeftMesh); TopLeftCap->SetVisibility(TopLeftMesh != nullptr); }
	if (TopRightCap) { TopRightCap->SetStaticMesh(TopRightMesh); TopRightCap->SetVisibility(TopRightMesh != nullptr); }

	// Clear previous instances
	FootBarISM->ClearInstances();
	MidLeftISM->ClearInstances();
	MidRightISM->ClearInstances();

	// Spawn rung instances: (0, 0, Z) relative to FootBarISM
	float BarZ = 0.f;
	while (BarZ < AZP_LadderHeight)
	{
		FootBarISM->AddInstance(FTransform(FVector(0.f, 0.f, BarZ)));
		BarZ += AZP_FootBarSpread;
	}

	// Spawn mid rail section instances (replicating BP_MasterLadder AdjustHeight)
	// MidL/R are children of BotL/R respectively. Instances placed at (0, 0, Z) relative to ISM.
	// bSyncedFootBar=true: top bound = AZP_LadderHeight - AZP_FootBarSpread
	const float MidTopZ = AZP_LadderHeight - AZP_FootBarSpread;
	float SideZ = -23.5f; // LocalSideHeightStored initial from BP_MasterLadder
	while (SideZ < MidTopZ)
	{
		MidLeftISM->AddInstance(FTransform(FVector(0.f, 0.f, SideZ)));
		MidRightISM->AddInstance(FTransform(FVector(0.f, 0.f, SideZ)));
		SideZ += AZP_SideDistance;
	}

	// Position top caps at ladder top (mesh geometry contains X offset)
	if (TopLeftCap && TopLeftMesh)
	{
		TopLeftCap->SetRelativeLocation(FVector(0.f, 0.f, AZP_LadderHeight));
	}
	if (TopRightCap && TopRightMesh)
	{
		TopRightCap->SetRelativeLocation(FVector(0.f, 0.f, AZP_LadderHeight));
	}

	// Auto-adjust TopExitPoint, InteractionVolume based on AZP_LadderHeight
	if (TopExitPoint)
	{
		TopExitPoint->SetRelativeLocation(FVector(AZP_MountStandoffX, 0.f, AZP_LadderHeight));
	}
	if (InteractionVolume)
	{
		const float HalfHeight = AZP_LadderHeight * 0.5f;
		// Small FRONT-facing trigger (container-fix pattern): can only mount from in front, not
		// through a wall / from the side. Offset -100 with X-extent 60 keeps the box from -40..-160
		// local — its +X edge never reaches the ladder face / mounting wall at local 0, and the
		// narrow Y stops it poking sideways into an adjacent room.
		InteractionVolume->SetBoxExtent(FVector(AZP_InteractionPocketExtent.X, AZP_InteractionPocketExtent.Y, HalfHeight + AZP_InteractionPocketExtent.Z));
		InteractionVolume->SetRelativeLocation(FVector(AZP_MountStandoffX, 0.f, HalfHeight));
	}
}

void AZP_Ladder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildLadderAssembly();
}

void AZP_Ladder::BeginPlay()
{
	Super::BeginPlay();

	// Rebuild at runtime to ensure correct state
	BuildLadderAssembly();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_Ladder::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_Ladder::OnOverlapEnd);

	UE_LOG(LogTemp, Log, TEXT("[Ladder] %s: Style=%d Height=%.0f Rungs=%d"),
		*GetName(), static_cast<int32>(AZP_LadderStyle), AZP_LadderHeight,
		FootBarISM ? FootBarISM->GetInstanceCount() : 0);
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
	return AZP_InteractionPromptText;
}

void AZP_Ladder::OnInteract_Implementation(ACharacter* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("[Ladder] %s: OnInteract — Interactor=%s"),
		*GetName(), Interactor ? *Interactor->GetName() : TEXT("NULL"));

	if (AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Interactor))
	{
		Grace->EnterLadder(this);
	}
}

void AZP_Ladder::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->SetCurrentInteractable(this);

	// Don't show prompt while actively climbing
	if (Grace->bOnLadder) return;

	// Show interaction prompt on HUD
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		FText Prompt = IZP_Interactable::Execute_GetInteractionPrompt(this);
		PC->HUDWidget->ShowInteractionPrompt(Prompt);
	}
}

void AZP_Ladder::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->ClearCurrentInteractable(this);

	// Hide interaction prompt
	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->HideInteractionPrompt();
	}
}
