// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_FloorSign
 *
 * Purpose: Drag-and-drop floor number sign. Set FloorNumber in Details,
 *          material updates automatically in editor and at runtime.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Blueprint Extension Points:
 *   - FloorNumber (1-6) editable per instance
 *
 * Dependencies:
 *   - MI_FloorSign_1 through MI_FloorSign_6 in /Game/TheSignal/Materials/
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_FloorSign.generated.h"

UCLASS(Blueprintable)
class THESIGNAL_API AZP_FloorSign : public AActor
{
	GENERATED_BODY()

public:
	AZP_FloorSign();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floor Sign")
	TObjectPtr<UStaticMeshComponent> SignMesh;

	/** Floor number to display (1-6). Changes material automatically. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Sign", meta = (ClampMin = "1", ClampMax = "6"))
	int32 FloorNumber = 1;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void UpdateMaterial();
};
