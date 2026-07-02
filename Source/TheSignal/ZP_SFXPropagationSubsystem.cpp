// Copyright The Signal. All Rights Reserved.

#include "ZP_SFXPropagationSubsystem.h"
#include "ZP_SFXStatics.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"

void UZP_SFXPropagationSubsystem::Track(UAudioComponent* AC, const AActor* IgnoreActor, float BaseVolume, float MaxLowPassHz)
{
	if (!AC) { return; }

	FTrackedSFX E;
	E.AC = AC;
	E.Ignore = IgnoreActor;
	E.BaseVolume = BaseVolume;
	E.MaxLowPassHz = MaxLowPassHz;

	// Start AT the current propagation state (no first-frame blast, no fade-in from wrong values).
	float VolMul = 1.f, LPF = 0.f;
	UZP_SFXStatics::ComputePropagation(GetWorld(), AC->GetComponentLocation(), IgnoreActor, VolMul, LPF);
	E.CurVolMul = E.TargetVolMul = VolMul;
	float StartLPF = (LPF > 0.f) ? LPF : LPFOpenHz;
	if (E.MaxLowPassHz > 0.f) { StartLPF = FMath::Min(StartLPF, E.MaxLowPassHz); }
	E.CurLPF = E.TargetLPF = StartLPF;

	AC->SetVolumeMultiplier(E.BaseVolume * E.CurVolMul);
	AC->SetLowPassFilterEnabled(true);
	AC->SetLowPassFilterFrequency(E.CurLPF);

	Tracked.Add(E);
}

void UZP_SFXPropagationSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UWorld* World = GetWorld();
	if (!World) { return; }

	// Re-target on an interval (trace + nav query per sound); interpolate every tick.
	RetargetAccum += DeltaTime;
	const bool bRetarget = RetargetAccum >= RetargetInterval;
	if (bRetarget) { RetargetAccum = 0.f; }

	for (int32 i = Tracked.Num() - 1; i >= 0; --i)
	{
		FTrackedSFX& E = Tracked[i];
		UAudioComponent* AC = E.AC.Get();
		if (!AC || !AC->IsPlaying())
		{
			Tracked.RemoveAtSwap(i);
			continue;
		}

		if (bRetarget)
		{
			float VolMul = 1.f, LPF = 0.f;
			UZP_SFXStatics::ComputePropagation(World, AC->GetComponentLocation(), E.Ignore.Get(), VolMul, LPF);
			E.TargetVolMul = VolMul;
			E.TargetLPF = (LPF > 0.f) ? LPF : LPFOpenHz;
			if (E.MaxLowPassHz > 0.f) { E.TargetLPF = FMath::Min(E.TargetLPF, E.MaxLowPassHz); }
		}

		E.CurVolMul = FMath::FInterpTo(E.CurVolMul, E.TargetVolMul, DeltaTime, InterpSpeed);
		E.CurLPF = FMath::FInterpTo(E.CurLPF, E.TargetLPF, DeltaTime, InterpSpeed);
		AC->SetVolumeMultiplier(E.BaseVolume * E.CurVolMul);
		AC->SetLowPassFilterFrequency(E.CurLPF);
	}
}

TStatId UZP_SFXPropagationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZP_SFXPropagationSubsystem, STATGROUP_Tickables);
}
