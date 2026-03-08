// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_MapWidget
 *
 * Purpose: Full-screen map overlay displaying the current area's captured
 *          texture with a player position marker. Toggled via M key.
 *          Silent Hill / RE style: pre-authored floor plan per area.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - All BindWidget references configurable in WBP_Map.
 *   - MarkerSize tunable in editor.
 *
 * Dependencies:
 *   - UZP_MapComponent (map area data, discovery state)
 *   - UMG (UUserWidget, UImage, UTextBlock, UCanvasPanel)
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_MapWidget.generated.h"

class UImage;
class UTextBlock;
class UCanvasPanel;
class UZP_MapComponent;
class AZP_MapVolume;

UCLASS()
class THESIGNAL_API UZP_MapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- BindWidget (names MUST match UMG designer widget names) ---

	/** Canvas containing the map image and player marker. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MapCanvas;

	/** The map texture image. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> MapImage;

	/** Player position indicator (small dot/arrow). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PlayerMarker;

	/** Area name displayed at the top. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AreaNameText;

	/** Shown when the player hasn't found a map for the current area. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NoMapText;

	// --- Config ---

	/** Size of the player marker widget (pixels). */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Marker")
	FVector2D MarkerSize = FVector2D(12.0f, 12.0f);

	// --- API ---

	/** Show the map for the current area. Shows "no map" if not discovered. */
	UFUNCTION(BlueprintCallable, Category = "Map")
	void ShowMap(UZP_MapComponent* MapComp);

	/** Hide the map overlay. */
	UFUNCTION(BlueprintCallable, Category = "Map")
	void HideMap();

	/** Is the map currently visible? */
	UFUNCTION(BlueprintCallable, Category = "Map")
	bool IsMapVisible() const { return bMapVisible; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	bool bMapVisible = false;

	UPROPERTY()
	TWeakObjectPtr<UZP_MapComponent> CachedMapComp;

	UPROPERTY()
	TWeakObjectPtr<AZP_MapVolume> CachedVolume;
};
