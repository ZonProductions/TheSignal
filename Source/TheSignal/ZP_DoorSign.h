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
class UStaticMesh;
class UMaterialInterface;

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

	/** Mesh used for the solid background panel that covers the original door text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Sign|Appearance")
	TObjectPtr<UStaticMesh> AZP_SignBackgroundMesh;

	/** Material on the background panel (dark cover material created by Scripts/Python/create_doorsign_material.py). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Sign|Appearance")
	TObjectPtr<UMaterialInterface> AZP_SignBackgroundMaterial;

	/** World scale of the background panel: ~0.5cm thick (X), 30cm wide (Y), 10cm tall (Z) — the sign's physical footprint on the door. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Sign|Appearance")
	FVector AZP_SignPanelScale = FVector(0.005f, 0.3f, 0.1f);

	/** Neutral placeholder text the sign spawns with; the real text is authored per-instance on the SignText component (matches the dev's expose-player-facing-text rule). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Sign|Text")
	FText AZP_DefaultSignText = FText::FromString(TEXT("ROOM 101"));

	/** World size (cm) of the rendered sign text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Sign|Text")
	float AZP_SignTextSize = 8.f;

	/** How far backward (-X, cm) the sign traces to find the door/wall surface it auto-attaches to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Sign|Attach")
	float AZP_AttachTraceDistance = 300.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif

private:
	void TryAttachToSurface();
};
