// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_MapComponent
 *
 * Purpose: Tracks which maps the player has discovered and which area
 *          they're currently in. Scans for AZP_MapVolume actors at
 *          BeginPlay to build the area registry.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - OnMapDiscovered / OnAreaChanged delegates for UI feedback.
 *   - DiscoverMap(FName) callable from any system (pickups, triggers, etc.).
 *
 * Dependencies:
 *   - AZP_MapVolume (level-placed area definitions)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_MapTypes.h"
#include "ZP_MapComponent.generated.h"

class AZP_MapVolume;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapDiscovered, FName, AreaID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAreaChanged, FName, NewAreaID);

UCLASS(ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_MapComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_MapComponent();

	/** Discovers (unlocks) a map for the given area. */
	UFUNCTION(BlueprintCallable, Category = "Map")
	void DiscoverMap(FName AreaID);

	/** Returns true if the player has discovered the map for this area. */
	UFUNCTION(BlueprintCallable, Category = "Map")
	bool IsMapDiscovered(FName AreaID) const;

	/** Returns the area ID the player is currently inside. NAME_None if outside all areas. */
	UFUNCTION(BlueprintCallable, Category = "Map")
	FName GetCurrentAreaID() const { return CurrentAreaID; }

	/** Returns the volume for the given area, or nullptr. */
	UFUNCTION(BlueprintCallable, Category = "Map")
	AZP_MapVolume* GetAreaVolume(FName AreaID) const;

	/** Returns the volume the player is currently inside, or nullptr. */
	UFUNCTION(BlueprintCallable, Category = "Map")
	AZP_MapVolume* GetCurrentVolume() const;

	/** Sets the current area (called by MapVolumes on overlap). */
	void SetCurrentArea(FName AreaID);

	/** Clears the current area (called by MapVolumes on end overlap). */
	void ClearCurrentArea(FName AreaID);

	/** Fired when a new map is discovered. */
	UPROPERTY(BlueprintAssignable, Category = "Map")
	FOnMapDiscovered OnMapDiscovered;

	/** Fired when the player enters a different area. */
	UPROPERTY(BlueprintAssignable, Category = "Map")
	FOnAreaChanged OnAreaChanged;

	/** All discovered area IDs. */
	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TSet<FName> DiscoveredMaps;

protected:
	virtual void BeginPlay() override;

private:
	/** Registry of all map areas in the level (AreaID -> Volume). */
	UPROPERTY()
	TMap<FName, TObjectPtr<AZP_MapVolume>> AreaVolumes;

	/** Current area the player is inside. */
	FName CurrentAreaID;
};
