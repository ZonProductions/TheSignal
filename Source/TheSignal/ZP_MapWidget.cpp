// Copyright The Signal. All Rights Reserved.

#include "ZP_MapWidget.h"
#include "ZP_MapComponent.h"
#include "ZP_MapVolume.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/PlayerController.h"

void UZP_MapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Start hidden
	SetVisibility(ESlateVisibility::Collapsed);
	bMapVisible = false;
}

void UZP_MapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bMapVisible) return;
	if (!CachedMapComp.IsValid() || !CachedVolume.IsValid()) return;
	if (!PlayerMarker || !MapCanvas) return;

	// Get player world position
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->GetPawn()) return;

	const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
	const FRotator PlayerRot = PC->GetControlRotation();

	// Calculate player UV in map space
	const FVector2D WorldMin = CachedVolume->GetWorldBoundsMin();
	const FVector2D WorldMax = CachedVolume->GetWorldBoundsMax();
	const FVector2D WorldSize = WorldMax - WorldMin;
	if (WorldSize.X < 1.0f || WorldSize.Y < 1.0f) return;

	FVector2D UV;
	UV.X = (PlayerLoc.X - WorldMin.X) / WorldSize.X;
	UV.Y = (PlayerLoc.Y - WorldMin.Y) / WorldSize.Y;

	// Clamp to map bounds
	UV.X = FMath::Clamp(UV.X, 0.0f, 1.0f);
	UV.Y = FMath::Clamp(UV.Y, 0.0f, 1.0f);

	// Flip Y: UE5 world Y+ is right, screen Y+ is down
	UV.Y = 1.0f - UV.Y;

	// Position marker in the canvas
	if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(PlayerMarker->Slot))
	{
		const FVector2D CanvasSize = MapCanvas->GetCachedGeometry().GetLocalSize();
		const FVector2D MarkerPos = UV * CanvasSize - (MarkerSize * 0.5f);
		MarkerSlot->SetPosition(MarkerPos);
		MarkerSlot->SetSize(MarkerSize);
	}

	// Rotate marker to match player facing direction
	PlayerMarker->SetRenderTransformAngle(-PlayerRot.Yaw + 90.0f);
}

void UZP_MapWidget::ShowMap(UZP_MapComponent* MapComp)
{
	if (!MapComp) return;

	CachedMapComp = MapComp;
	const FName AreaID = MapComp->GetCurrentAreaID();
	AZP_MapVolume* Volume = MapComp->GetCurrentVolume();

	if (AreaID.IsNone() || !Volume)
	{
		// Player is outside any mapped area
		if (MapImage) MapImage->SetVisibility(ESlateVisibility::Collapsed);
		if (PlayerMarker) PlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
		if (AreaNameText) AreaNameText->SetText(FText::FromString(TEXT("Unknown Area")));
		if (NoMapText)
		{
			NoMapText->SetText(FText::FromString(TEXT("No map available")));
			NoMapText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		SetVisibility(ESlateVisibility::HitTestInvisible);
		bMapVisible = true;
		CachedVolume = nullptr;
		return;
	}

	CachedVolume = Volume;

	// Check if map is discovered
	if (!MapComp->IsMapDiscovered(AreaID))
	{
		if (MapImage) MapImage->SetVisibility(ESlateVisibility::Collapsed);
		if (PlayerMarker) PlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
		if (AreaNameText) AreaNameText->SetText(Volume->AreaDisplayName);
		if (NoMapText)
		{
			NoMapText->SetText(FText::FromString(TEXT("Map not found yet")));
			NoMapText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		SetVisibility(ESlateVisibility::HitTestInvisible);
		bMapVisible = true;
		return;
	}

	// Show the map
	if (NoMapText) NoMapText->SetVisibility(ESlateVisibility::Collapsed);
	if (AreaNameText) AreaNameText->SetText(Volume->AreaDisplayName);

	if (MapImage && Volume->MapTexture)
	{
		MapImage->SetBrushFromTexture(Volume->MapTexture);
		MapImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (MapImage)
	{
		MapImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (PlayerMarker) PlayerMarker->SetVisibility(ESlateVisibility::HitTestInvisible);

	SetVisibility(ESlateVisibility::HitTestInvisible);
	bMapVisible = true;
}

void UZP_MapWidget::HideMap()
{
	SetVisibility(ESlateVisibility::Collapsed);
	bMapVisible = false;
	CachedVolume = nullptr;
}
