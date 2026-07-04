// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_SFXPropagationSubsystem
 *
 * Purpose: Makes world SFX propagation DYNAMIC. Every sound spawned through UZP_SFXStatics is
 *          registered here; the subsystem re-evaluates its propagation (Direct / Diffracted /
 *          Transmitted — see UZP_SFXStatics::ComputePropagation) several times a second for the
 *          sound's whole lifetime and smoothly interpolates the audio component's volume and
 *          low-pass toward the new targets. Walk behind a wall mid-scream and it muffles WHILE
 *          playing; round the corner and it opens back up. Without this, propagation was a
 *          spawn-time snapshot — the "I hear every SFX in a couple modes" bug.
 *
 * Owner Subsystem: Audio
 *
 * Dependencies: UZP_SFXStatics (target computation). Auto-created per world; no setup.
 */

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZP_SFXPropagationSubsystem.generated.h"

class UAudioComponent;

UCLASS()
class THESIGNAL_API UZP_SFXPropagationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Seconds between full propagation re-evaluations (trace + nav query per live sound).
	 *  Interpolation runs every tick regardless, so transitions stay smooth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Propagation")
	float AZP_RetargetInterval = 0.15f;
	/** Interp speed (per second) for volume and low-pass toward their current targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Propagation")
	float AZP_InterpSpeed = 9.f;
	/** "No filtering" low-pass frequency — effectively bypass. */
	static constexpr float AZP_LPFOpenHz = 20000.f;

	/** Track a freshly spawned one-shot. BaseVolume = the volume the caller asked for (propagation
	 *  multiplies on top). MaxLowPassHz > 0 = caller-forced ceiling (e.g. gaze hiss) that the
	 *  dynamic low-pass can go below but never above. */
	void Track(UAudioComponent* AC, const AActor* IgnoreActor, float BaseVolume, float MaxLowPassHz = 0.f);

	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return Tracked.Num() > 0; }

private:
	struct FTrackedSFX
	{
		TWeakObjectPtr<UAudioComponent> AC;
		TWeakObjectPtr<const AActor> Ignore;
		float BaseVolume = 1.f;
		float MaxLowPassHz = 0.f; // 0 = no forced ceiling
		float CurVolMul = 1.f;
		float TargetVolMul = 1.f;
		float CurLPF = 20000.f;
		float TargetLPF = 20000.f;
	};

	TArray<FTrackedSFX> Tracked;
	float RetargetAccum = 0.f;
};
