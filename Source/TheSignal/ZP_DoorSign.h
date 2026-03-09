// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_DoorSign
 *
 * Purpose: Solid sign that covers existing door text. Place against a door
 *          surface — auto-attaches at runtime so it moves when the door opens.
 *          Edit text on the SignText component in Details.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Dependencies: None
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_DoorSign.generated.h"

class UTextRenderComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_DoorSign : public AActor
{
	GENERATED_BODY()

public:
	AZP_DoorSign();

	virtual void BeginPlay() override;

	/** Solid background panel that covers existing door text. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door Sign")
	TObjectPtr<UStaticMeshComponent> Background;

	/** The sign text. Click this component in Details, change the Text property. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door Sign")
	TObjectPtr<UTextRenderComponent> SignText;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif

private:
	void TryAttachToSurface();
};
