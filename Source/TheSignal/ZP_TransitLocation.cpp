// Copyright The Signal. All Rights Reserved.

#include "ZP_TransitLocation.h"
#include "Components/SceneComponent.h"
#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#endif

AZP_TransitLocation::AZP_TransitLocation()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(SceneRoot);
	Billboard->bIsScreenSizeScaled = true;
#endif
}
