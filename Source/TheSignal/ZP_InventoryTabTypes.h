// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * Data types for the unified inventory tab system (Maps / Inventory / Notes).
 * Used by UZP_InventoryTabWidget and UZP_NoteComponent.
 */

#include "CoreMinimal.h"
#include "ZP_InventoryTabTypes.generated.h"

/** Which tab is currently active in the inventory menu. */
UENUM(BlueprintType)
enum class EZP_InventoryTab : uint8
{
	Map,
	Inventory,
	Notes
};

/**
 * A single collected note/document. Found in the world via pickups or triggers.
 * Notes with bIsCode=true feed into the combination safe system (TICKET-057).
 */
USTRUCT(BlueprintType)
struct FZP_NoteEntry
{
	GENERATED_BODY()

	/** Unique ID for deduplication and save/load. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Note")
	FName NoteID;

	/** Display title shown in the Notes tab list. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Note")
	FText Title;

	/** Full note content shown in the detail view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Note")
	FText Content;

	/** If true, this note contains a code for a combination lock. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Note")
	bool bIsCode = false;

	/** The actual code string (e.g. "4281"). Only meaningful when bIsCode is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Note", meta = (EditCondition = "bIsCode"))
	FString CodeValue;
};
