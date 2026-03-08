// Copyright The Signal. All Rights Reserved.

#include "ZP_PatrolWaypoint.h"
#include "Components/SceneComponent.h"

AZP_PatrolWaypoint::AZP_PatrolWaypoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}
