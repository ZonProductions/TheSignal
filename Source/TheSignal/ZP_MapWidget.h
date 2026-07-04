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
 *   - AZP_MarkerSize tunable in editor.
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

/** Tracks a dynamically-created door marker on the map. */
USTRUCT()
struct FZP_DoorMarkerInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> AZP_DoorActor;

	UPROPERTY()
	TObjectPtr<UImage> MarkerWidget = nullptr;

	/** True = AZP_LockableDoor (color updates per state). False = AZP_InteractDoor (always unlocked). */
	bool bIsLockable = false;
};

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
	FVector2D AZP_MarkerSize = FVector2D(12.0f, 12.0f);

	/** Degrees added to the negated control yaw so the player-marker art points the correct way; must change if the marker texture's authored orientation changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Marker")
	float AZP_PlayerMarkerAngleOffset = 90.0f;

	/** Size of door marker icons (pixels). */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Doors")
	FVector2D AZP_DoorMarkerSize = FVector2D(8.0f, 8.0f);

	/** Color for unlocked / interactable doors (RE-style blue). */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Doors")
	FLinearColor AZP_UnlockedDoorColor = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);

	/** Color for locked doors (RE-style red). */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Doors")
	FLinearColor AZP_LockedDoorColor = FLinearColor(1.0f, 0.15f, 0.15f, 1.0f);

	/** Player-facing area title shown when no MapVolume covers the player (hardcoded UI string; violates the expose-player-facing-text rule). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Text")
	FText AZP_UnknownAreaText = FText::FromString(TEXT("Unknown Area"));

	/** Player-facing message shown when no map exists for the current location (hardcoded UI string; violates the expose-player-facing-text rule). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Text")
	FText AZP_NoMapAvailableText = FText::FromString(TEXT("No map available"));

	/** Player-facing message shown when the area has a map but the player has not picked it up yet (hardcoded UI string; violates the expose-player-facing-text rule). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Text")
	FText AZP_MapNotFoundText = FText::FromString(TEXT("Map not found yet"));

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

	UPROPERTY()
	TArray<FZP_DoorMarkerInfo> DoorMarkers;

	/** Converts world XY to map UV (with Y-flip for screen coordinates). */
	FVector2D WorldToMapUV(const FVector& WorldLocation) const;

	/** Scans level for doors within current volume, creates marker widgets. */
	void CreateDoorMarkers();

	/** Removes all door marker widgets and empties the array. */
	void ClearDoorMarkers();
};
