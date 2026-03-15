// Copyright The Signal. All Rights Reserved.

#include "ZP_MapWidget.h"
#include "ZP_MapComponent.h"
#include "ZP_MapVolume.h"
#include "ZP_InteractDoor.h"
#include "ZP_LockableDoor.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

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
	if (!MapCanvas) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->GetPawn()) return;

	const FVector2D CanvasSize = MapCanvas->GetCachedGeometry().GetLocalSize();
	if (CanvasSize.X < 1.0f || CanvasSize.Y < 1.0f) return;

	// --- Player marker ---
	if (PlayerMarker)
	{
		const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
		const FVector2D UV = WorldToMapUV(PlayerLoc);

		// Log every 60 frames (~1 second)
		static int32 LogCounter = 0;
		if (LogCounter++ % 60 == 0)
		{
			const FVector2D WorldMin = CachedVolume->GetWorldBoundsMin();
			const FVector2D WorldMax = CachedVolume->GetWorldBoundsMax();
			UE_LOG(LogTemp, Warning, TEXT("[MAP] Player=(%.0f,%.0f,%.0f) UV=(%.3f,%.3f) VolMin=(%.0f,%.0f) VolMax=(%.0f,%.0f) Canvas=(%.0f,%.0f)"),
				PlayerLoc.X, PlayerLoc.Y, PlayerLoc.Z,
				UV.X, UV.Y,
				WorldMin.X, WorldMin.Y, WorldMax.X, WorldMax.Y,
				CanvasSize.X, CanvasSize.Y);
		}

		if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(PlayerMarker->Slot))
		{
			const FVector2D MarkerPos = UV * CanvasSize - (MarkerSize * 0.5f);
			MarkerSlot->SetPosition(MarkerPos);
			MarkerSlot->SetSize(MarkerSize);
		}

		PlayerMarker->SetRenderTransformAngle(-PC->GetControlRotation().Yaw + 90.0f);
	}

	// --- Door markers ---
	for (FZP_DoorMarkerInfo& Info : DoorMarkers)
	{
		if (!Info.MarkerWidget || !Info.DoorActor.IsValid()) continue;

		const FVector2D UV = WorldToMapUV(Info.DoorActor->GetActorLocation());
		if (UCanvasPanelSlot* DoorSlot = Cast<UCanvasPanelSlot>(Info.MarkerWidget->Slot))
		{
			const FVector2D DoorPos = UV * CanvasSize - (DoorMarkerSize * 0.5f);
			DoorSlot->SetPosition(DoorPos);
			DoorSlot->SetSize(DoorMarkerSize);
		}

		// Update color for lockable doors (state can change at runtime)
		if (Info.bIsLockable)
		{
			if (AZP_LockableDoor* LD = Cast<AZP_LockableDoor>(Info.DoorActor.Get()))
			{
				const FLinearColor Color = (LD->GetDoorState() == EZP_DoorState::Locked)
					? LockedDoorColor : UnlockedDoorColor;
				Info.MarkerWidget->SetColorAndOpacity(Color);
			}
		}
	}
}

void UZP_MapWidget::ShowMap(UZP_MapComponent* MapComp)
{
	if (!MapComp) return;

	CachedMapComp = MapComp;
	ClearDoorMarkers();

	const FName AreaID = MapComp->GetCurrentAreaID();
	AZP_MapVolume* Volume = MapComp->GetCurrentVolume();

	UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP SHOW: AreaID='%s', Volume=%s"),
		*AreaID.ToString(), Volume ? *Volume->GetName() : TEXT("NULL"));

	if (AreaID.IsNone() || !Volume)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP SHOW: → BRANCH: No map available (AreaID none=%d, Volume null=%d)"),
			AreaID.IsNone() ? 1 : 0, Volume ? 0 : 1);
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
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP SHOW: → BRANCH: Map not found yet"));
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
	UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP SHOW: → BRANCH: Showing map! MapImage=%s, Texture=%s"),
		MapImage ? TEXT("valid") : TEXT("NULL"),
		Volume->MapTexture ? *Volume->MapTexture->GetName() : TEXT("NULL"));

	if (NoMapText) NoMapText->SetVisibility(ESlateVisibility::Collapsed);
	if (AreaNameText) AreaNameText->SetText(Volume->AreaDisplayName);

	if (MapImage && Volume->MapTexture)
	{
		MapImage->SetBrushFromTexture(Volume->MapTexture);
		MapImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP SHOW: Texture SET on MapImage"));
	}
	else if (MapImage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] MAP SHOW: No texture — MapImage collapsed"));
		MapImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (PlayerMarker) PlayerMarker->SetVisibility(ESlateVisibility::HitTestInvisible);

	// Create door markers for all interactable doors in this area
	CreateDoorMarkers();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	bMapVisible = true;
}

void UZP_MapWidget::HideMap()
{
	ClearDoorMarkers();
	SetVisibility(ESlateVisibility::Collapsed);
	bMapVisible = false;
	CachedVolume = nullptr;
}

FVector2D UZP_MapWidget::WorldToMapUV(const FVector& WorldLocation) const
{
	if (!CachedVolume.IsValid()) return FVector2D(0.5f, 0.5f);

	const FVector2D WorldMin = CachedVolume->GetWorldBoundsMin();
	const FVector2D WorldMax = CachedVolume->GetWorldBoundsMax();
	const FVector2D WorldSize = WorldMax - WorldMin;
	if (WorldSize.X < 1.0f || WorldSize.Y < 1.0f) return FVector2D(0.5f, 0.5f);

	FVector2D UV;
	UV.X = FMath::Clamp((WorldLocation.X - WorldMin.X) / WorldSize.X, 0.0f, 1.0f);
	UV.Y = FMath::Clamp((WorldLocation.Y - WorldMin.Y) / WorldSize.Y, 0.0f, 1.0f);
	return UV;
}

void UZP_MapWidget::CreateDoorMarkers()
{
	ClearDoorMarkers();
	if (!CachedVolume.IsValid() || !MapCanvas) return;

	const FVector2D WorldMin = CachedVolume->GetWorldBoundsMin();
	const FVector2D WorldMax = CachedVolume->GetWorldBoundsMax();

	UWorld* World = GetWorld();
	if (!World) return;

	// Check if door position falls within the current volume XY bounds
	auto IsInBounds = [&](const FVector& Loc) -> bool
	{
		return Loc.X >= WorldMin.X && Loc.X <= WorldMax.X
			&& Loc.Y >= WorldMin.Y && Loc.Y <= WorldMax.Y;
	};

	// Create a solid-color UImage marker and add to canvas
	auto CreateMarker = [&](const FLinearColor& Color) -> UImage*
	{
		UImage* Marker = NewObject<UImage>(this);
		// Modify existing brush in-place (avoids FSlateBrush ctor linker dep on SlateCore)
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Marker->Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Marker->Brush.ImageSize = DoorMarkerSize;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		Marker->SetColorAndOpacity(Color);
		MapCanvas->AddChild(Marker);
		return Marker;
	};

	// InteractDoors — always unlocked (blue)
	for (TActorIterator<AZP_InteractDoor> It(World); It; ++It)
	{
		if (!IsInBounds(It->GetActorLocation())) continue;

		FZP_DoorMarkerInfo Info;
		Info.DoorActor = *It;
		Info.bIsLockable = false;
		Info.MarkerWidget = CreateMarker(UnlockedDoorColor);
		DoorMarkers.Add(Info);
	}

	// LockableDoors — red when locked, blue when unlocked
	for (TActorIterator<AZP_LockableDoor> It(World); It; ++It)
	{
		if (!IsInBounds(It->GetActorLocation())) continue;

		const FLinearColor Color = (It->GetDoorState() == EZP_DoorState::Locked)
			? LockedDoorColor : UnlockedDoorColor;

		FZP_DoorMarkerInfo Info;
		Info.DoorActor = *It;
		Info.bIsLockable = true;
		Info.MarkerWidget = CreateMarker(Color);
		DoorMarkers.Add(Info);
	}
}

void UZP_MapWidget::ClearDoorMarkers()
{
	for (FZP_DoorMarkerInfo& Info : DoorMarkers)
	{
		if (Info.MarkerWidget)
		{
			Info.MarkerWidget->RemoveFromParent();
		}
	}
	DoorMarkers.Empty();
}
