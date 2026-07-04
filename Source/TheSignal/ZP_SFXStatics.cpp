// Copyright The Signal. All Rights Reserved.

#include "ZP_SFXStatics.h"
#include "ZP_SFXPropagationSubsystem.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/ReverbEffect.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

USoundAttenuation* UZP_SFXStatics::GetCarryAttenuation(EZP_SFXCarry Carry)
{
	// One transient USoundAttenuation per profile, built from the header constants on first use and
	// rooted for the process lifetime. Deliberately NOT a content asset — carry distances live in
	// C++ so they can't silently drift in a .uasset (that drift is how SA_EnemyVoice ended up
	// falling silent at 40 m).
	static USoundAttenuation* Cache[3] = { nullptr, nullptr, nullptr };
	const int32 Idx = FMath::Clamp((int32)Carry, 0, 2);
	if (Cache[Idx])
	{
		return Cache[Idx];
	}

	float Inner = AZP_RoomInnerRadius, Falloff = AZP_RoomFalloff;
	const TCHAR* Name = TEXT("ZP_SFXCarry_Room");
	switch (Carry)
	{
	case EZP_SFXCarry::Close: Inner = AZP_CloseInnerRadius; Falloff = AZP_CloseFalloff; Name = TEXT("ZP_SFXCarry_Close"); break;
	case EZP_SFXCarry::Room:  break;
	case EZP_SFXCarry::Far:   Inner = AZP_FarInnerRadius;   Falloff = AZP_FarFalloff;   Name = TEXT("ZP_SFXCarry_Far");   break;
	}

	USoundAttenuation* Attn = NewObject<USoundAttenuation>(GetTransientPackage(), FName(Name));
	FSoundAttenuationSettings& S = Attn->Attenuation;
	S.bAttenuate = true;
	S.bSpatialize = true;
	// TRUE inverse-square rolloff via a custom curve: amplitude = Inner / distance, i.e. -6 dB per
	// distance doubling — the actual physical law. The engine evaluates this continuously against
	// the listener every frame, so walking toward/away from a source produces the real-life
	// differential at EVERY step. (The previous NaturalSound + big inner radius was near-flat
	// across the 0-20 m band where all combat happens — "max range sounds the same as melee".)
	S.DistanceAlgorithm = EAttenuationDistanceModel::Custom;
	{
		FRichCurve* RC = S.CustomAttenuationCurve.GetRichCurve();
		RC->Reset();
		// X = distance normalized 0..1 across FalloffDistance (past the inner radius).
		// 1/r amplitude law, floor-rescaled so the curve hits EXACTLY 0 at X=1: the engine only
		// re-enables distance culling / max-range rejection when the final key is <= 1e-3
		// (FBaseAttenuationSettings::GetMaxFalloffDistance) — a raw 1/r curve bottoms at ~0.02,
		// which makes every sound engine-audible MAP-WIDE at a constant floor. Rescale shifts
		// near-field values by <4% and keeps the physical shape.
		const float FloorVal = Inner / (Inner + Falloff);
		static const float Keys[] = { 0.f, 0.005f, 0.01f, 0.02f, 0.04f, 0.08f, 0.16f, 0.32f, 0.5f, 0.75f, 1.f };
		for (float X : Keys)
		{
			const float Dist = Inner + X * Falloff;
			RC->AddKey(X, ((Inner / Dist) - FloorVal) / (1.f - FloorVal));
		}
	}
	S.AttenuationShape = EAttenuationShape::Sphere;
	S.AttenuationShapeExtents = FVector(Inner, 0.f, 0.f);
	S.FalloffDistance = Falloff;
	// Air absorption: a GENTLE top-end shelf at real distance (real physics loses ~10-15 dB at
	// 10 kHz over 80 m — nothing like a muffle). Distance character = quieter + wetter, not duller;
	// heavy low-pass is reserved for the Transmitted (through-wall) propagation tier.
	const float MaxRange = Inner + Falloff;
	S.bAttenuateWithLPF = true;
	S.LPFRadiusMin = MaxRange * AZP_LPFStartFraction;
	S.LPFRadiusMax = MaxRange;
	S.LPFFrequencyAtMin = 20000.f;
	S.LPFFrequencyAtMax = AZP_LPFFrequencyAtFar;
	// Reverb send scales with distance: near = mostly dry, far = mostly hallway tail. Renders
	// through the facility reverb bed EnsureWorldReverb activates (or any AudioVolume reverb).
	S.bEnableReverbSend = true;
	S.ReverbSendMethod = EReverbSendMethod::Linear;
	S.ReverbWetLevelMin = AZP_ReverbWetNear;
	S.ReverbWetLevelMax = AZP_ReverbWetFar;
	S.ReverbDistanceMin = Inner;
	S.ReverbDistanceMax = MaxRange;
	// Engine occlusion stays OFF — in this project it mutes sounds outright (confirmed dead end).
	// The manual listener-trace muffle in Play* covers through-wall dampening instead.
	S.bEnableOcclusion = false;
	Attn->AddToRoot();
	Cache[Idx] = Attn;
	return Attn;
}

void UZP_SFXStatics::EnsureWorldReverb(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject
		? (GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
		: nullptr;
	if (!World || !World->IsGameWorld()) { return; }

	// Once per world. The weak ptr goes stale when the world dies, so the next world re-activates.
	static TWeakObjectPtr<UWorld> ActivatedWorld;
	if (ActivatedWorld.Get() == World) { return; }
	ActivatedWorld = World;

	// The facility's default acoustic space: a concrete corridor. Transient + rooted, values from
	// the header constants — same C++-ownership rule as the carry profiles. Per-room AudioVolumes
	// (higher priority) can override this locally later.
	static UReverbEffect* FacilityReverb = nullptr;
	if (!FacilityReverb)
	{
		FacilityReverb = NewObject<UReverbEffect>(GetTransientPackage(), FName(TEXT("ZP_FacilityReverb")));
		FacilityReverb->DecayTime        = AZP_ReverbDecayTime;
		FacilityReverb->DecayHFRatio     = AZP_ReverbDecayHFRatio;
		FacilityReverb->Gain             = AZP_ReverbGain;
		FacilityReverb->GainHF           = AZP_ReverbGainHF;
		FacilityReverb->ReflectionsGain  = AZP_ReverbReflectionsGain;
		FacilityReverb->ReflectionsDelay = AZP_ReverbReflectionsDelay;
		FacilityReverb->LateGain         = AZP_ReverbLateGain;
		FacilityReverb->LateDelay        = AZP_ReverbLateDelay;
		FacilityReverb->Diffusion        = AZP_ReverbDiffusion;
		FacilityReverb->Density          = AZP_ReverbDensity;
		FacilityReverb->AddToRoot();
	}

	UGameplayStatics::ActivateReverbEffect(World, FacilityReverb, FName(TEXT("ZP_FacilityReverb")),
		/*Priority=*/1.f, /*Volume=*/AZP_ReverbMasterVolume, /*FadeTime=*/2.f);
	UE_LOG(LogTemp, Log, TEXT("[SFX] facility reverb bed activated on %s"), *World->GetName());
}

bool UZP_SFXStatics::IsOccludedFromListener(UWorld* World, const FVector& SourceLoc, const AActor* IgnoreActor)
{
	if (!World) { return false; }
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) { return false; }

	FVector CamLoc; FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	FHitResult Hit;
	FCollisionQueryParams P(FName(TEXT("ZPSFXOcclusion")), /*bTraceComplex=*/false);
	if (IgnoreActor) { P.AddIgnoredActor(IgnoreActor); }
	if (APawn* Pawn = PC->GetPawn()) { P.AddIgnoredActor(Pawn); }
	if (World->LineTraceSingleByChannel(Hit, CamLoc, SourceLoc, ECC_WorldStatic, P))
	{
		// Only a REAL between-room wall counts. A blocker hugging the source (its own perch/contact
		// geometry — e.g. a crawler clinging to a wall) is not occlusion.
		const float DistToSource = FVector::Dist(CamLoc, SourceLoc);
		return Hit.Distance < DistToSource - AZP_OcclusionSelfSkin;
	}
	return false;
}

void UZP_SFXStatics::ComputePropagation(UWorld* World, const FVector& SourceLoc, const AActor* IgnoreActor,
	float& OutVolumeMul, float& OutLowPassHz, bool* bOutTransmitConfident)
{
	OutVolumeMul = 1.f;
	OutLowPassHz = 0.f;
	if (bOutTransmitConfident) { *bOutTransmitConfident = false; }
	if (!World) { return; }

	// DIRECT — clear sightline: nothing to do. Also skip all work beyond every profile's audible
	// range (the trace + pathfind would be paid for a sound the listener can't hear anyway).
	APlayerController* PC = World->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (Pawn && FVector::Dist(Pawn->GetActorLocation(), SourceLoc) > AZP_FarInnerRadius + AZP_FarFalloff + 500.f) { return; }
	if (!IsOccludedFromListener(World, SourceLoc, IgnoreActor)) { return; }

	// Sightline is blocked. Sound doesn't stop at a corner — it bends down open routes. Approximate
	// "is there an open route" with a navmesh path from the player's ground position to the source:
	// this map's acoustics ARE its hallways, and the navmesh is exactly the map of its open air.
	if (Pawn)
	{
		const FVector PawnLoc = Pawn->GetActorLocation();
		const float StraightDist = FVector::Dist(PawnLoc, SourceLoc);
		if (StraightDist > 1.f)
		{
			if (UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, PawnLoc, SourceLoc))
			{
				if (Path->IsValid() && !Path->IsPartial())
				{
					const float Ratio = Path->GetPathLength() / StraightDist;
					if (Ratio <= AZP_MaxDiffractionDetour)
					{
						// DIFFRACTED — around the corner / down the connecting hall. Level drops
						// with the detour, top end mostly survives. NOT a muffle.
						const float T = FMath::Clamp((Ratio - 1.f) / (AZP_MaxDiffractionDetour - 1.f), 0.f, 1.f);
						OutVolumeMul = FMath::Lerp(AZP_DiffractedVolumeMin, AZP_DiffractedVolumeMax, T);
						OutLowPassHz = FMath::Lerp(AZP_DiffractedLPFMinHz, AZP_DiffractedLPFMaxHz, T);
						return;
					}
					// Valid full path with a huge detour: CONFIDENTLY the long way around a wall.
					if (bOutTransmitConfident) { *bOutTransmitConfident = true; }
				}
				// Failed/partial path = UNKNOWN route (pawn off-navmesh, unbuilt region) — still
				// muffle the audio (safe default) but do NOT report confidence: gameplay callers
				// (sight-aggro veto) must not blind an enemy over a nav gap.
			}
		}
	}

	// TRANSMITTED — no believable open route: genuinely through a wall.
	OutVolumeMul = AZP_TransmittedVolumeScale;
	OutLowPassHz = AZP_TransmittedLowPassHz;
}

UAudioComponent* UZP_SFXStatics::PlaySFXAttached(USoundBase* Sound, USceneComponent* AttachTo,
	EZP_SFXCarry Carry, float Volume, float Pitch, bool bPropagate, float ForceLowPassHz)
{
	if (!Sound || !AttachTo) { return nullptr; }
	UWorld* World = AttachTo->GetWorld();
	EnsureWorldReverb(World);

	UAudioComponent* AC = UGameplayStatics::SpawnSoundAttached(
		Sound, AttachTo, NAME_None,
		FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget,
		/*bStopWhenAttachedToDestroyed=*/true, Volume, Pitch, 0.f,
		GetCarryAttenuation(Carry), nullptr, /*bAutoDestroy=*/true);

	// Hand the sound to the propagation subsystem: it sets the correct starting state immediately
	// and keeps re-evaluating (occlusion/diffraction) for the sound's whole life — fully dynamic.
	if (AC && World)
	{
		UZP_SFXPropagationSubsystem* Prop = bPropagate ? World->GetSubsystem<UZP_SFXPropagationSubsystem>() : nullptr;
		if (Prop)
		{
			Prop->Track(AC, AttachTo->GetOwner(), Volume, ForceLowPassHz);
		}
		else if (ForceLowPassHz > 0.f)
		{
			// Caller-forced low-pass must apply even without the subsystem.
			AC->SetLowPassFilterEnabled(true);
			AC->SetLowPassFilterFrequency(ForceLowPassHz);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[SFX] '%s' attached to %s vol=%.2f carry=%d dynamic=%d"),
		*Sound->GetName(), AttachTo->GetOwner() ? *AttachTo->GetOwner()->GetName() : TEXT("?"),
		Volume, (int32)Carry, bPropagate ? 1 : 0);
	return AC;
}

UAudioComponent* UZP_SFXStatics::PlaySFXAtLocation(const UObject* WorldContextObject, USoundBase* Sound,
	FVector Location, EZP_SFXCarry Carry, float Volume, float Pitch, bool bPropagate)
{
	if (!Sound || !WorldContextObject) { return nullptr; }
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World) { return nullptr; }
	EnsureWorldReverb(World);

	UAudioComponent* AC = UGameplayStatics::SpawnSoundAtLocation(
		World, Sound, Location, FRotator::ZeroRotator, Volume, Pitch, 0.f,
		GetCarryAttenuation(Carry), nullptr, /*bAutoDestroy=*/true);

	if (AC && bPropagate)
	{
		if (UZP_SFXPropagationSubsystem* Prop = World->GetSubsystem<UZP_SFXPropagationSubsystem>())
		{
			Prop->Track(AC, Cast<AActor>(WorldContextObject), Volume);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[SFX] '%s' at location vol=%.2f carry=%d dynamic=%d"),
		*Sound->GetName(), Volume, (int32)Carry, bPropagate ? 1 : 0);
	return AC;
}
