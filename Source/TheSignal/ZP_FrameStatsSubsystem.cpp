// Copyright The Signal. All Rights Reserved.
#include "ZP_FrameStatsSubsystem.h"

#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "RHI.h"

#if !UE_BUILD_SHIPPING

static TAutoConsoleVariable<int32> CVarZPFrameStatsEnabled(
	TEXT("zp.FrameStats.Enabled"), 1,
	TEXT("1 = log [FrameStats] thread-split lines during play (non-shipping only)."));

static TAutoConsoleVariable<float> CVarZPFrameStatsInterval(
	TEXT("zp.FrameStats.Interval"), 5.0f,
	TEXT("Seconds per [FrameStats] window."));

static TAutoConsoleVariable<float> CVarZPFrameStatsHitchMs(
	TEXT("zp.FrameStats.HitchMs"), 100.0f,
	TEXT("Frame time (ms) above which a frame counts as a hitch."));

#endif // !UE_BUILD_SHIPPING

bool UZP_FrameStatsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
#endif
}

TStatId UZP_FrameStatsSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZP_FrameStatsSubsystem, STATGROUP_Tickables);
}

void UZP_FrameStatsSubsystem::ResetWindow(const double Now)
{
	WindowStart = Now;
	Frames = 0;
	SumFrameMs = SumGameMs = SumRenderMs = SumRHIMs = SumGPUMs = 0.f;
	Hitches = 0;
	WorstFrameMs = 0.f;
}

void UZP_FrameStatsSubsystem::Tick(const float DeltaTime)
{
#if !UE_BUILD_SHIPPING
	if (!CVarZPFrameStatsEnabled.GetValueOnGameThread())
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	if (WindowStart < 0.0)
	{
		ResetWindow(Now);
		return;
	}

	const float FrameMs = DeltaTime * 1000.f;
	// Hitch magnitude from WALL CLOCK — the engine clamps DeltaTime (MaxTickRate/smoothing), so
	// real multi-second stalls read as exactly 400ms via DeltaTime. Averages keep DeltaTime.
	const float WallMs = (LastTickWall > 0.0) ? static_cast<float>((Now - LastTickWall) * 1000.0) : FrameMs;
	LastTickWall = Now;
	Frames++;
	SumFrameMs += FrameMs;
	SumGameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
	SumRenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
	SumRHIMs += FPlatformTime::ToMilliseconds(GRHIThreadTime);
	SumGPUMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
	if (WallMs > CVarZPFrameStatsHitchMs.GetValueOnGameThread())
	{
		Hitches++;
	}
	WorstFrameMs = FMath::Max(WorstFrameMs, WallMs);

	const float Interval = FMath::Max(1.0f, CVarZPFrameStatsInterval.GetValueOnGameThread());
	if (Now - WindowStart >= Interval && Frames > 0)
	{
		const float N = static_cast<float>(Frames);
		static IConsoleVariable* CVarSP = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
		UE_LOG(LogTemp, Display,
			TEXT("[FrameStats] %.0fs window: fps=%.0f frame=%.1fms game=%.1f render=%.1f rhi=%.1f gpu=%.1f sp=%.0f | hitches>%.0fms: %d worst=%.0fms"),
			Interval, N / Interval, SumFrameMs / N, SumGameMs / N, SumRenderMs / N, SumRHIMs / N, SumGPUMs / N,
			CVarSP ? CVarSP->GetFloat() : -1.f,
			CVarZPFrameStatsHitchMs.GetValueOnGameThread(), Hitches, WorstFrameMs);
		ResetWindow(Now);
	}
#endif
}
