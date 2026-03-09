// Copyright The Signal. All Rights Reserved.

#include "ZP_DoorSign.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AZP_DoorSign::AZP_DoorSign()
{
	PrimaryActorTick.bCanEverTick = false;

	// Scene root so text position isn't affected by mesh scale
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Solid background panel — covers existing door text
	Background = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Background"));
	Background->SetupAttachment(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		Background->SetStaticMesh(CubeMesh.Object);
	}

	// Dark material (created by Scripts/Python/create_doorsign_material.py)
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SignMat(
		TEXT("/Game/TheSignal/Materials/M_DoorSignDark"));
	if (SignMat.Succeeded())
	{
		Background->SetMaterial(0, SignMat.Object);
	}

	// Flat panel facing +X: ~0.5cm thick (X), ~30cm wide (Y), ~10cm tall (Z)
	Background->SetWorldScale3D(FVector(0.005f, 0.3f, 0.1f));
	Background->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Sign text on the front face (+X), attached to root (not scaled mesh)
	SignText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SignText"));
	SignText->SetupAttachment(Root);
	SignText->SetRelativeLocation(FVector(1.f, 0.f, 0.f));
	SignText->SetText(FText::FromString(TEXT("ROOM 101")));
	SignText->SetWorldSize(8.f);
	SignText->SetTextRenderColor(FColor::White);
	SignText->SetHorizontalAlignment(EHTA_Center);
	SignText->SetVerticalAlignment(EVRTA_TextCenter);
}

void AZP_DoorSign::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	TryAttachToSurface();
}

#if WITH_EDITOR
void AZP_DoorSign::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (bFinished)
	{
		TryAttachToSurface();
	}
}
#endif

void AZP_DoorSign::BeginPlay()
{
	Super::BeginPlay();

	// Re-attach at runtime if editor attachment didn't persist
	if (!GetAttachParentActor())
	{
		TryAttachToSurface();
	}
}

void AZP_DoorSign::TryAttachToSurface()
{
	if (!GetWorld())
	{
		return;
	}

	FHitResult Hit;
	// Start slightly in front (+X) to avoid self-intersection
	const FVector Start = GetActorLocation() + GetActorForwardVector() * 5.f;
	// Trace backward (-X) into the door/wall surface
	const FVector End = Start - GetActorForwardVector() * 300.f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor != this && GetAttachParentActor() != HitActor)
		{
			AttachToActor(HitActor, FAttachmentTransformRules::KeepWorldTransform);
		}
	}
}
