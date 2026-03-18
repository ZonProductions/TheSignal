// Copyright The Signal. All Rights Reserved.

#include "ZP_MapVolume.h"
#include "ZP_MapComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"

AZP_MapVolume::AZP_MapVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	AreaBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaBounds"));
	SetRootComponent(AreaBounds);
	AreaBounds->SetBoxExtent(FVector(1000.f, 1000.f, 500.f));
	AreaBounds->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	AreaBounds->SetGenerateOverlapEvents(true);
	AreaBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// Block NO traces — this volume is for overlap detection only.
	// Without this, hitscan LineTraceByObjectType hits the box before reaching actors inside it.
	AreaBounds->SetCollisionResponseToAllChannels(ECR_Overlap);

	// Visible in editor wireframe, hidden at runtime
	AreaBounds->SetHiddenInGame(true);
	AreaBounds->ShapeColor = FColor(0, 200, 200, 128);
}

void AZP_MapVolume::BeginPlay()
{
	Super::BeginPlay();

	// Force overlap-only at runtime — placed instances may have stale collision from old CDO.
	// Without this, hitscan LineTraceByObjectType hits the box before reaching actors inside it.
	AreaBounds->SetCollisionResponseToAllChannels(ECR_Overlap);

	AreaBounds->OnComponentBeginOverlap.AddDynamic(this, &AZP_MapVolume::OnOverlapBegin);
	AreaBounds->OnComponentEndOverlap.AddDynamic(this, &AZP_MapVolume::OnOverlapEnd);

	// Detect actors already inside the volume at spawn (player may start inside)
	AreaBounds->UpdateOverlaps();
}

FVector2D AZP_MapVolume::GetWorldBoundsMin() const
{
	const FVector Center = GetActorLocation();
	const FVector Extent = AreaBounds->GetScaledBoxExtent();
	return FVector2D(Center.X - Extent.X, Center.Y - Extent.Y);
}

FVector2D AZP_MapVolume::GetWorldBoundsMax() const
{
	const FVector Center = GetActorLocation();
	const FVector Extent = AreaBounds->GetScaledBoxExtent();
	return FVector2D(Center.X + Extent.X, Center.Y + Extent.Y);
}

void AZP_MapVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* Char = Cast<ACharacter>(OtherActor);
	if (!Char || !Char->IsPlayerControlled()) return;

	if (UZP_MapComponent* MapComp = Char->FindComponentByClass<UZP_MapComponent>())
	{
		MapComp->SetCurrentArea(AreaID);
	}
}

void AZP_MapVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ACharacter* Char = Cast<ACharacter>(OtherActor);
	if (!Char || !Char->IsPlayerControlled()) return;

	if (UZP_MapComponent* MapComp = Char->FindComponentByClass<UZP_MapComponent>())
	{
		MapComp->ClearCurrentArea(AreaID);
	}
}
