// Copyright The Signal. All Rights Reserved.

#include "ZP_MapPickup.h"
#include "ZP_MapComponent.h"
#include "ZP_MapVolume.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"

AZP_MapPickup::AZP_MapPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(Root);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetBoxExtent(FVector(100.f, 100.f, 80.f));
	InteractionVolume->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);

	PickupGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("PickupGlow"));
	PickupGlow->SetupAttachment(Root);
	PickupGlow->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	PickupGlow->SetIntensity(300.f);
	PickupGlow->SetAttenuationRadius(150.f);
	PickupGlow->SetLightColor(FLinearColor(0.4f, 0.7f, 0.9f)); // cool blue for maps
}

void AZP_MapPickup::BeginPlay()
{
	Super::BeginPlay();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_MapPickup::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_MapPickup::OnOverlapEnd);

	// Auto-generate AreaID from actor name if empty
	if (AreaID.IsNone())
	{
		AreaID = FName(*GetName());
	}

	// Auto-spawn a MapVolume covering this floor
	if (bAutoCreateVolume && MapTexture)
	{
		float MinX, MinY, MaxX, MaxY;
		int32 FoundActors = 0;
		const float MyZ = GetActorLocation().Z;
		bool bUsedFile = false;

		// Determine which floor (by Z height)
		int32 FloorNum = 1;
		if (MyZ > 1400) FloorNum = 5;
		else if (MyZ > 900) FloorNum = 4;
		else if (MyZ > 400) FloorNum = 3;
		else if (MyZ > 0) FloorNum = 2;

		// Try to read bounds from Dev Tools export file
		FString BoundsPath = FPaths::ProjectDir() / FString::Printf(TEXT("Scripts/FloorPlans/map_bounds_F%d.txt"), FloorNum);
		FString BoundsContent;
		if (FFileHelper::LoadFileToString(BoundsContent, *BoundsPath))
		{
			TArray<FString> Parts;
			BoundsContent.ParseIntoArray(Parts, TEXT(","));
			if (Parts.Num() == 4)
			{
				MinX = FCString::Atof(*Parts[0]);
				MinY = FCString::Atof(*Parts[1]);
				MaxX = FCString::Atof(*Parts[2]);
				MaxY = FCString::Atof(*Parts[3]);
				bUsedFile = true;
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] MapPickup %s: Loaded bounds from %s: (%.0f,%.0f) to (%.0f,%.0f)"),
					*GetName(), *BoundsPath, MinX, MinY, MaxX, MaxY);
			}
		}

		if (!bUsedFile)
		{
			UE_LOG(LogTemp, Error, TEXT("[TheSignal] MapPickup %s: No bounds file found at %s. Export from Dev Tools first!"), *GetName(), *BoundsPath);
			// Use the entire level as a rough fallback
			MinX = -5000.f; MinY = -2000.f;
			MaxX = 3000.f; MaxY = 6000.f;
		}

		const FVector Center((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f, MyZ);
		const FVector HalfExtent((MaxX - MinX) * 0.5f, (MaxY - MinY) * 0.5f, 300.f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		AZP_MapVolume* Volume = GetWorld()->SpawnActor<AZP_MapVolume>(
			AZP_MapVolume::StaticClass(), Center, FRotator::ZeroRotator, SpawnParams);

		if (Volume)
		{
			Volume->AreaID = AreaID;
			Volume->AreaDisplayName = AreaDisplayName;
			Volume->MapTexture = MapTexture;

			if (Volume->AreaBounds)
			{
				Volume->AreaBounds->SetBoxExtent(HalfExtent);
			}

			UE_LOG(LogTemp, Log, TEXT("[TheSignal] MapPickup %s: Auto-created MapVolume '%s' from %d actors. Bounds: (%.0f,%.0f) to (%.0f,%.0f)"),
				*GetName(), *AreaID.ToString(), FoundActors, MinX, MinY, MaxX, MaxY);

			// Write actual volume bounds for debugging
			FString VolBoundsPath = FPaths::ProjectDir() / TEXT("Scripts/FloorPlans/volume_bounds.txt");
			FString VolBoundsData = FString::Printf(TEXT("%.1f,%.1f,%.1f,%.1f"), MinX, MinY, MaxX, MaxY);
			FFileHelper::SaveStringToFile(VolBoundsData, *VolBoundsPath);
		}
	}
}

FText AZP_MapPickup::GetInteractionPrompt_Implementation()
{
	return PromptText;
}

void AZP_MapPickup::OnInteract_Implementation(ACharacter* Interactor)
{
	if (bPickedUp) return;

	UZP_MapComponent* MapComp = Interactor ? Interactor->FindComponentByClass<UZP_MapComponent>() : nullptr;
	if (!MapComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MapPickup %s: No MapComponent on interactor"), *GetName());
		return;
	}

	MapComp->DiscoverMap(AreaID);
	bPickedUp = true;

	// Clear interaction prompt
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Interactor);
	if (Grace)
	{
		Grace->ClearCurrentInteractable(this);
		AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
		if (PC && PC->HUDWidget)
		{
			PC->HUDWidget->HideInteractionPrompt();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] MapPickup %s: Discovered map for area '%s'"),
		*GetName(), *AreaID.ToString());

	Destroy();
}

void AZP_MapPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bPickedUp) return;

	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->SetCurrentInteractable(this);

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->ShowInteractionPrompt(PromptText);
	}
}

void AZP_MapPickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (bPickedUp) return;

	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->ClearCurrentInteractable(this);

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->HideInteractionPrompt();
	}
}
