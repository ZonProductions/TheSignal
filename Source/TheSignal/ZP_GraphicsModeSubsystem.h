// Copyright The Signal. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZP_GraphicsModeSubsystem.generated.h"

/**
 * Purpose: The game's two graphics modes (dev design 2026-08-04, "like an actual game I buy"):
 *   QUALITY     — Epic scalability at NATIVE resolution (sg.ResolutionQuality=100). The look the
 *                 dev signed off on: rock-steady vsync-paced ~18-20fps at 4K on a 3080.
 *   PERFORMANCE — IDENTICAL Epic look (GI/Shadow tier is calibrated and must never drop — DEAD
 *                 ENDS 2026-08-04), but internal render at 50% (4K -> 1080p, TSR upscales).
 *                 Roughly 3x GPU headroom; fps then limited by the game thread (~45, rising as
 *                 game-thread work lands).
 * The mode is APPLIED AND ENFORCED at every game/PIE start — this is also the anti-drift guard
 * (editor scalability silently ran Epic-default for weeks because nothing owned it; now this
 * subsystem owns the play-time state and logs what it applied).
 * Owned by: Core / graphics settings.
 * Extension: console `zp.Mode Performance` / `zp.Mode Quality` (persists to GameUserSettings.ini
 *   [TheSignal.GraphicsMode]); future: hook into the EGUI settings menu (dev authors the UI).
 * Dependencies: Scalability console vars only.
 */
UCLASS()
class THESIGNAL_API UZP_GraphicsModeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Applies the given mode's cvars now, persists the choice. */
	void ApplyMode(bool bPerformance, bool bPersist);

	/** EGUI settings-menu hookup: drop a selector in the EasyGameUI options widget and call this
	 *  (applies immediately + persists). Pair with IsPerformanceMode() to show the current state. */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Graphics")
	void SetGraphicsMode(bool bPerformance) { ApplyMode(bPerformance, /*bPersist=*/true); }

	UFUNCTION(BlueprintPure, Category = "TheSignal|Graphics")
	bool IsPerformanceMode() const { return LoadSavedModeIsPerformance(); }

	static UZP_GraphicsModeSubsystem* Get(const UObject* WorldContext);

private:
	bool LoadSavedModeIsPerformance() const;
	void OnPostLoadMap(UWorld* LoadedWorld);

	/** Editor-PIE plumbing + the LOW enforcement (immediate + delayed re-assert). */
	void EnforceSavedSettings();

	/** EVERY graphics scalability group -> LOW, native res scale. Dev directive 2026-08-04:
	 *  low on every PIE start, always; the EGUI menu can raise things per-session on top. */
	void ApplyLowSeed();

	FDelegateHandle PostLoadMapHandle;
	FTimerHandle ReapplyHandle;
};
