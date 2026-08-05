// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_ObjectiveReactor
 *
 * Purpose: Data-driven "objective complete -> world reactions" — the lean slice of
 *          Docs/ObjectiveReactor_Plan.md. Place one per story beat, point AZP_ListenId at a
 *          progression flag or main-objective id, and when it completes the reactor:
 *          retints every matching emergency light in the level (red -> AZP_LightTargetColor
 *          over AZP_LightFadeTime) and plays AZP_CompleteSound as a 2D stinger. It can also
 *          play AZP_StartupSound once at BeginPlay (scene-setting cue, e.g. C1_POWER_OUT
 *          when the map starts on emergency power).
 *          Idempotent by design: if the listened id is already set when the level loads
 *          (save-load, cross-level), the END state applies instantly — no fade, no stinger,
 *          reactions never replay.
 *
 * Owner Subsystem: UZP_ObjectiveSubsystem (binds its OnFlagSet / OnObjectiveCompleted)
 *
 * Blueprint Extension Points: all AZP_ knobs; Blueprintable for per-beat BP children.
 *
 * Dependencies: UZP_ObjectiveSubsystem
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_ObjectiveReactor.generated.h"

class ULightComponent;
class USoundBase;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_ObjectiveReactor : public AActor
{
	GENERATED_BODY()

public:
	AZP_ObjectiveReactor();

	/** Progression flag OR main-objective id this reactor fires on (e.g. FUSE_BOX). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactor")
	FName AZP_ListenId = NAME_None;

	/** Main objective started once at BeginPlay — the per-level campaign bootstrap (e.g.
	 *  RESEARCH1 for the research facility). The most recently started main owns the HUD
	 *  tracker. Skipped if already active or completed. NAME_None = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactor")
	FName AZP_StartObjectiveOnBeginPlay = NAME_None;

	/** Optional one-shot played 2D once at BeginPlay (scene-setting cue, e.g. C1_POWER_OUT). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactor|Audio")
	TObjectPtr<USoundBase> AZP_StartupSound;

	/** One-shot played 2D the moment the listened id completes (e.g. C1_FUSE_BOX_COMPLETE). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactor|Audio")
	TObjectPtr<USoundBase> AZP_CompleteSound;

	/** Retint every matching light in the level when the id completes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactor|Lights")
	bool bAZP_RetintLights = true;

	/** Color the matched lights fade to (power restored = white). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactor|Lights")
	FLinearColor AZP_LightTargetColor = FLinearColor::White;

	/** Seconds the light fade takes; 0 = instant snap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactor|Lights")
	float AZP_LightFadeTime = 2.0f;

	/** Match rule: a light counts as "emergency red" when its color's R is at least this... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactor|Lights")
	int32 AZP_MatchMinRed = 100;

	/** ...and G and B are both <= R * this ratio (0.6 catches both 255/93/93 and 169/0/27). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactor|Lights")
	float AZP_MatchDominance = 0.6f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION()
	void OnObjectiveEventFired(FName Id);

	/** Re-assert AZP_StartObjectiveOnBeginPlay after a save restore replaced objective state — a slot
	 *  saved in another level doesn't contain this level's main, so the restore would wipe it. */
	UFUNCTION()
	void OnObjectiveStateRestored();

	/** Run the reactions. bInstant = end state only (save-load): no stinger, no fade. */
	void React(bool bInstant);

	struct FFadingLight
	{
		TWeakObjectPtr<ULightComponent> Light;
		FLinearColor From;
	};
	TArray<FFadingLight> FadingLights;
	float FadeElapsed = 0.f;
	bool bFired = false;
};
