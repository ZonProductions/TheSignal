// Copyright The Signal. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZP_FrameStatsSubsystem.generated.h"

/**
 * Purpose: zero-friction perf telemetry. During play (PIE/game, never shipping) logs a
 *   [FrameStats] line every zp.FrameStats.Interval seconds with the averaged thread split
 *   (frame/game/render/RHI/GPU ms) plus hitch count and worst spike in the window — so a perf
 *   session leaves real numbers in Saved/Logs/TheSignal.log without the dev touching the console.
 *   Read afterward with: grep "\[FrameStats\]" Saved/Logs/TheSignal.log
 * Owned subsystem: DebugOverlay (diagnostics).
 * Blueprint extension points: none (log-only). Console knobs: zp.FrameStats.Enabled (1),
 *   zp.FrameStats.Interval (5s), zp.FrameStats.HitchMs (100).
 * Dependencies: Core (thread cycle globals), RHI (GPU frame cycles) — both already in Build.cs.
 * Tick justification (poll-driven by design): it samples per-frame timing globals; there is no
 *   event to bind. Non-shipping only; ~4 adds and a compare per frame.
 */
UCLASS()
class THESIGNAL_API UZP_FrameStatsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }

private:
	double WindowStart = -1.0;
	int32 Frames = 0;
	float SumFrameMs = 0.f;
	float SumGameMs = 0.f;
	float SumRenderMs = 0.f;
	float SumRHIMs = 0.f;
	float SumGPUMs = 0.f;
	int32 Hitches = 0;
	float WorstFrameMs = 0.f;
	// Wall-clock time of the previous Tick — hitch counting uses real elapsed time because
	// the engine clamps DeltaTime (real 2.4s stalls reported as "worst=400ms" before 2026-08-04).
	double LastTickWall = -1.0;

	void ResetWindow(double Now);
};
