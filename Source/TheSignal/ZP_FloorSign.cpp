// Copyright The Signal. All Rights Reserved.

#include "ZP_FloorSign.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AZP_FloorSign::AZP_FloorSign()
{
	PrimaryActorTick.bCanEverTick = false;

	SignMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SignMesh"));
	SetRootComponent(SignMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane"));
	if (PlaneMesh.Succeeded())
	{
		SignMesh->SetStaticMesh(PlaneMesh.Object);
	}

	SignMesh->SetWorldScale3D(FVector(1.f, 1.f, 1.f));
}

void AZP_FloorSign::BeginPlay()
{
	Super::BeginPlay();
	UpdateMaterial();
}

void AZP_FloorSign::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateMaterial();
}

#if WITH_EDITOR
void AZP_FloorSign::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateMaterial();
}
#endif

void AZP_FloorSign::UpdateMaterial()
{
	const int32 Clamped = FMath::Clamp(FloorNumber, 1, 6);
	const FString Path = FString::Printf(
		TEXT("/Game/TheSignal/Materials/MI_FloorSign_%d.MI_FloorSign_%d"), Clamped, Clamped);

	UMaterialInterface* MI = Cast<UMaterialInterface>(
		StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *Path));

	if (MI)
	{
		SignMesh->SetMaterial(0, MI);
	}
}
