// Copyright The Signal. All Rights Reserved.

#include "ZP_SFXPropagationSubsystem.h"
#include "ZP_SFXStatics.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

// PIE console: `zp.SFXProp.Debug 1` — per-retarget on-screen + log readout for EVERY tracked
// sound: distance, occlusion verdict, computed tier targets, and the volume/LPF ACTUALLY applied
// to the audio component. Added 2026-08-07 after two blind tuning rounds on the lurk-through-wall
// report ("stop guessing") — this is the measurement that ends the guessing.
static TAutoConsoleVariable<int32> CVarZPSFXPropDebug(
	TEXT("zp.SFXProp.Debug"), 0,
	TEXT("1 = on-screen/log readout of every propagated sound's distance, occlusion, tier targets and applied volume/LPF."));

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
	float StartLPF = (LPF > 0.f) ? LPF : AZP_LPFOpenHz;
	if (E.MaxLowPassHz > 0.f) { StartLPF = FMath::Min(StartLPF, E.MaxLowPassHz); }
	E.CurLPF = E.TargetLPF = StartLPF;

	AC->SetVolumeMultiplier(E.BaseVolume * E.CurVolMul);
	AC->SetLowPassFilterEnabled(true);
	AC->SetLowPassFilterFrequency(E.CurLPF);

	if (CVarZPSFXPropDebug.GetValueOnGameThread())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFXProp] TRACK %-28s base=%.2f startVol=%.2f startLPF=%.0f ignore=%s"),
			AC->Sound ? *AC->Sound->GetName() : TEXT("?"), E.BaseVolume, E.CurVolMul, E.CurLPF,
			IgnoreActor ? *IgnoreActor->GetName() : TEXT("none"));
	}

	Tracked.Add(E);
}

void UZP_SFXPropagationSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UWorld* World = GetWorld();
	if (!World) { return; }

	// Re-target on an interval (trace + nav query per sound); interpolate every tick.
	RetargetAccum += DeltaTime;
	const bool bRetarget = RetargetAccum >= AZP_RetargetInterval;
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
			E.TargetLPF = (LPF > 0.f) ? LPF : AZP_LPFOpenHz;
			if (E.MaxLowPassHz > 0.f) { E.TargetLPF = FMath::Min(E.TargetLPF, E.MaxLowPassHz); }

			if (CVarZPSFXPropDebug.GetValueOnGameThread())
			{
				// The full truth for this sound, this instant: where it is, what the model decided,
				// and what the component is ACTUALLY set to. If a sound is loud with tierVol low,
				// the leak is below the propagation layer (asset/attenuation); if tierVol is high,
				// the model itself cleared it (occlusion/route); if no line prints for a sound you
				// can hear, that sound never entered this pipeline at all.
				float DistM = -1.f;
				if (APlayerController* PC = World->GetFirstPlayerController())
				{
					FVector CamLoc; FRotator CamRot;
					PC->GetPlayerViewPoint(CamLoc, CamRot);
					DistM = FVector::Dist(CamLoc, AC->GetComponentLocation()) / 100.f;
				}
				const bool bOccl = UZP_SFXStatics::IsOccludedFromListener(World, AC->GetComponentLocation(), E.Ignore.Get());
				const TCHAR* Tier = !bOccl ? TEXT("DIRECT")
					: (E.TargetVolMul <= 0.31f ? TEXT("TRANSMIT")
					: (E.TargetVolMul >= 0.59f && E.TargetVolMul <= 0.61f ? TEXT("ONEBEND") : TEXT("DIFFRACT")));
				const USoundAttenuation* Attn = AC->AttenuationSettings;
				const FString Msg = FString::Printf(
					TEXT("[SFXProp] %-28s d=%5.1fm occl=%d %-8s tier(vol=%.2f lpf=%5.0f) applied(vol=%.2f lpf=%5.0f) base=%.2f attn=%s"),
					AC->Sound ? *AC->Sound->GetName() : TEXT("?"), DistM, bOccl ? 1 : 0, Tier,
					E.TargetVolMul, E.TargetLPF, AC->VolumeMultiplier, E.CurLPF, E.BaseVolume,
					Attn ? *Attn->GetName() : TEXT("NONE"));
				UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage((uint64)(9500 + i), 3.0f, FColor::Orange, Msg);
				}
			}
		}

		E.CurVolMul = FMath::FInterpTo(E.CurVolMul, E.TargetVolMul, DeltaTime, AZP_InterpSpeed);
		E.CurLPF = FMath::FInterpTo(E.CurLPF, E.TargetLPF, DeltaTime, AZP_InterpSpeed);
		AC->SetVolumeMultiplier(E.BaseVolume * E.CurVolMul);
		AC->SetLowPassFilterFrequency(E.CurLPF);
	}
}

TStatId UZP_SFXPropagationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZP_SFXPropagationSubsystem, STATGROUP_Tickables);
}
