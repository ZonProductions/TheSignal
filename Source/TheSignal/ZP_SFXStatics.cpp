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
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

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

EZP_ImpactSurface UZP_SFXStatics::ClassifySurface(const FHitResult& Hit)
{
	// Gather every name string the hit can tell us about itself. Purchased-pack content has no
	// authored physical materials, but its MATERIAL/MESH names are consistently descriptive
	// (MI_Concrete_5, SM_MetalShelf, M_WoodCrate...) — that's the signal. PhysMaterial name goes
	// in FIRST so an authored PM_Metal wins over a generic mesh name whenever the project starts
	// assigning phys materials (query must set bReturnPhysicalMaterial for it to be present).
	FString Haystack;
	if (const UPhysicalMaterial* Phys = Hit.PhysMaterial.Get())
	{
		Haystack += Phys->GetName() + TEXT(" ");
	}
	if (const UPrimitiveComponent* Comp = Hit.GetComponent())
	{
		for (int32 i = 0; i < Comp->GetNumMaterials(); ++i)
		{
			if (const UMaterialInterface* M = Comp->GetMaterial(i)) { Haystack += M->GetName() + TEXT(" "); }
		}
		if (const UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Comp))
		{
			if (SMC->GetStaticMesh()) { Haystack += SMC->GetStaticMesh()->GetName() + TEXT(" "); }
		}
		else if (const USkeletalMeshComponent* SKC = Cast<USkeletalMeshComponent>(Comp))
		{
			if (SKC->GetSkeletalMeshAsset()) { Haystack += SKC->GetSkeletalMeshAsset()->GetName() + TEXT(" "); }
		}
		Haystack += Comp->GetName() + TEXT(" ");
	}
	if (const AActor* A = Hit.GetActor())
	{
		Haystack += A->GetName();
	}
	Haystack = Haystack.ToLower();

	auto Matches = [&Haystack](std::initializer_list<const TCHAR*> Keys) -> bool
	{
		for (const TCHAR* K : Keys)
		{
			if (Haystack.Contains(K)) { return true; }
		}
		return false;
	};

	// Order = specificity: glass/tile names are unambiguous; wood next; metal has the widest net
	// (facility doors/fixtures are metal); everything else is the concrete family.
	if (Matches({ TEXT("glass"), TEXT("window"), TEXT("tile"), TEXT("ceramic"), TEXT("porcelain"),
	              TEXT("mirror"), TEXT("monitor"), TEXT("screen") }))
	{
		return EZP_ImpactSurface::Glass;
	}
	if (Matches({ TEXT("wood"), TEXT("crate"), TEXT("plank"), TEXT("pallet"), TEXT("timber"),
	              TEXT("plywood"), TEXT("board"), TEXT("desk"), TEXT("table"), TEXT("chair"),
	              TEXT("cardboard"), TEXT("furniture") }))
	{
		return EZP_ImpactSurface::Wood;
	}
	if (Matches({ TEXT("metal"), TEXT("steel"), TEXT("iron"), TEXT("alum"), TEXT("vent"),
	              TEXT("duct"), TEXT("pipe"), TEXT("locker"), TEXT("rail"), TEXT("grate"),
	              TEXT("machine"), TEXT("generator"), TEXT("barrel"), TEXT("tank"), TEXT("door"),
	              TEXT("elevator"), TEXT("fence"), TEXT("server"), TEXT("silo"), TEXT("container") }))
	{
		return EZP_ImpactSurface::Metal;
	}
	return EZP_ImpactSurface::Concrete;
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
	// ANY blocker strictly between the listener and the source = occluded. The old "self-skin"
	// exemption (a blocker within N uu of the source doesn't count) existed for the DEPRECATED
	// wall-clinging crawler and caused three separate full-volume-through-geometry failures as N
	// shrank (150: any enemy within 1.5 m of a thin wall; 40: an enemy pressed against the far
	// side of a closed DOOR — the 2026-08-07 "sounds the same as walking through" report, source
	// ~2 uu behind the leaf). The enemy's own body is excluded via IgnoreActor, so a hit here is
	// real world geometry in the path — occlusion, full stop. Over-muffling edge cases fall to
	// the nav-route stage (diffraction/one-bend), which reopens anything genuinely connected.
	if (World->LineTraceSingleByChannel(Hit, CamLoc, SourceLoc, ECC_WorldStatic, P))
	{
		return true;
	}

	// BIDIRECTIONAL (2026-08-07): pack meshes ship SINGLE-SIDED complex collision — a ray only
	// hits triangles from the side they face, so a forward miss can simply mean the dividing
	// wall's triangles face the OTHER way (measured: player->shambler hit the divider,
	// shambler->player sailed through the same wall). The asset sweep
	// (fix_wall_collision_doublesided.py) fixes ResearchMegaPack, but any other pack/level still
	// gets caught here: on a forward miss, look again from the source's side.
	if (World->LineTraceSingleByChannel(Hit, SourceLoc, CamLoc, ECC_WorldStatic, P))
	{
		return true;
	}
	return false;
}

void UZP_SFXStatics::ComputePropagation(UWorld* World, const FVector& SourceLoc, const AActor* IgnoreActor,
	float& OutVolumeMul, float& OutLowPassHz, bool* bOutTransmitConfident)
{
	// perf 2026-08-04: memoize per (source-cell, listener-cell) for 0.5s — every occluded enemy
	// footstep re-ran the occlusion trace + a SYNC navmesh pathfind from the same 200uu cells.
	struct FPropCacheEntry { double Time; float Vol; float LPF; bool Confident; };
	static TMap<uint64, FPropCacheEntry> Cache;
	uint64 Key = 0;
	bool bHaveKey = false;
	if (World)
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (const APawn* Pawn = PC->GetPawn())
			{
				auto Cell = [](const FVector& V) -> uint64
				{
					return (uint64)(uint16)FMath::FloorToInt(V.X / 200.f)
						| ((uint64)(uint16)FMath::FloorToInt(V.Y / 200.f) << 16)
						| ((uint64)(uint16)FMath::FloorToInt(V.Z / 200.f) << 32);
				};
				Key = Cell(SourceLoc) ^ (Cell(Pawn->GetActorLocation()) * 0x9E3779B97F4A7C15ull);
				bHaveKey = true;
				if (const FPropCacheEntry* Hit = Cache.Find(Key))
				{
					if (FPlatformTime::Seconds() - Hit->Time < 0.5)
					{
						OutVolumeMul = Hit->Vol;
						OutLowPassHz = Hit->LPF;
						if (bOutTransmitConfident) { *bOutTransmitConfident = Hit->Confident; }
						return;
					}
				}
			}
		}
	}
	bool bConfident = false;
	ComputePropagationUncached(World, SourceLoc, IgnoreActor, OutVolumeMul, OutLowPassHz, &bConfident);
	if (bOutTransmitConfident) { *bOutTransmitConfident = bConfident; }
	if (bHaveKey)
	{
		if (Cache.Num() > 256) { Cache.Empty(); }
		Cache.Add(Key, { FPlatformTime::Seconds(), OutVolumeMul, OutLowPassHz, bConfident });
	}
}

void UZP_SFXStatics::ComputePropagationUncached(UWorld* World, const FVector& SourceLoc, const AActor* IgnoreActor,
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
			// Round-15 (dev: "sfx attenuation is broken with open doors"): a source pressed into
			// a door's navmesh CARVE (a crawler hugging the door frame) makes the raw-goal path
			// FAIL, dropping the sound to the full through-wall muffle while the listener stands
			// across an OPEN frame. Project the source onto the navmesh first — the projected
			// point is the open-air spot the sound actually pours from. The trace guard keeps the
			// projection from jumping THROUGH a thin wall (which would un-muffle a sealed room).
			FVector NavGoal = SourceLoc;
			if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(World))
			{
				FNavLocation Projected;
				if (Nav->ProjectPointToNavigation(SourceLoc, Projected, FVector(250.f, 250.f, 400.f)))
				{
					FHitResult ProjHit;
					FCollisionQueryParams ProjParams(FName(TEXT("ZPSFXNavProject")), false);
					if (IgnoreActor) { ProjParams.AddIgnoredActor(IgnoreActor); }
					const FVector ProjTarget = Projected.Location + FVector(0.f, 0.f, 40.f);
					if (!World->LineTraceSingleByChannel(ProjHit, SourceLoc, ProjTarget, ECC_WorldStatic, ProjParams))
					{
						NavGoal = Projected.Location;
					}
				}
			}
			if (UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, PawnLoc, NavGoal))
			{
				// ONE-BEND UPGRADE (dev 2026-08-05: at an OPEN door, shambler alerted in the
				// room — "muffled until I stepped through the door". The doorway route's
				// detour RATIO landed deep in the diffraction band and muffled it like a
				// wall). Geometry beats ratios: if some point on the open-air route sees
				// BOTH the listener and the source, the sound bends exactly ONCE — an open
				// doorway / single corner aperture — and arrives nearly clean no matter how
				// long the detour reads. Multi-corner routes and sealed walls have no such
				// pivot and keep the bands below.
				// Runs on PARTIAL paths too (round-17: the stale STATIC navmesh dead-ended at
				// doorways, every path came back partial, and this test never fired — the sound
				// hard-failed to the wall muffle with the door wide open). A partial path still
				// walks up to the aperture; the pivot rays are what decide open vs sealed — a
				// closed BlockAll door leaf blocks the pivot->source ray, so no false clears.
				if (Path->IsValid())
				{
					const TArray<FVector>& Pts = Path->PathPoints;
					if (Pts.Num() >= 2 && PC)
					{
						FVector CamLoc; FRotator CamRot;
						PC->GetPlayerViewPoint(CamLoc, CamRot);
						FCollisionQueryParams PivotParams(FName(TEXT("ZPSFXOneBend")), false);
						if (IgnoreActor) { PivotParams.AddIgnoredActor(IgnoreActor); }
						if (Pawn) { PivotParams.AddIgnoredActor(Pawn); }
						const FVector SrcEar = SourceLoc + FVector(0.f, 0.f, 40.f);
						const int32 Stride = FMath::Max(1, (Pts.Num() - 2) / 8); // cap ~8 probes
						for (int32 i = 1; i < Pts.Num() - 1; i += Stride)
						{
							const FVector Pivot = Pts[i] + FVector(0.f, 0.f, AZP_OneBendPivotHeight);
							// BOTH directions of BOTH legs must be clear (2026-08-07): single-sided
							// pack collision lets a one-direction ray pass through a wall's back
							// face, faking an open doorway and clearing the aperture through solid
							// geometry. A real opening is clear from every direction.
							FHitResult HitA, HitB;
							if (!World->LineTraceSingleByChannel(HitA, CamLoc, Pivot, ECC_WorldStatic, PivotParams) &&
								!World->LineTraceSingleByChannel(HitA, Pivot, CamLoc, ECC_WorldStatic, PivotParams) &&
								!World->LineTraceSingleByChannel(HitB, Pivot, SrcEar, ECC_WorldStatic, PivotParams) &&
								!World->LineTraceSingleByChannel(HitB, SrcEar, Pivot, ECC_WorldStatic, PivotParams))
							{
								OutVolumeMul = AZP_OneBendVolume;
								OutLowPassHz = AZP_OneBendLPFHz;
								return;
							}
						}
					}
					// PARTIAL paths NEVER enter the ratio math (2026-08-07, the closing bug of the
					// lurk saga): a partial path's length is the distance to WHERE IT GAVE UP (a
					// closed door's nav carve, a navmesh gap), not a route to the source. Measured
					// live: closed door -> partial path len 571 vs straight 685 -> ratio 0.83 ->
					// clamped to the diffraction MINIMUM -> 0.65 vol "practically same room"
					// through a sealed doorway. The one-bend pivots above already ran (they are
					// geometric and legitimately decide open apertures on partial paths); a
					// partial path past them means NO known open route -> through-wall muffle.
					if (!Path->IsPartial())
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
		else if (bPropagate)
		{
			// A propagated sound with NO subsystem plays permanently un-occluded — fail LOUD.
			UE_LOG(LogTemp, Error, TEXT("[SFX] '%s' asked for propagation but UZP_SFXPropagationSubsystem is MISSING — playing raw (full volume through walls)."),
				*Sound->GetName());
			if (ForceLowPassHz > 0.f)
			{
				AC->SetLowPassFilterEnabled(true);
				AC->SetLowPassFilterFrequency(ForceLowPassHz);
			}
		}
		else if (ForceLowPassHz > 0.f)
		{
			// Caller-forced low-pass must apply even without the subsystem.
			AC->SetLowPassFilterEnabled(true);
			AC->SetLowPassFilterFrequency(ForceLowPassHz);
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("[SFX] '%s' attached to %s vol=%.2f carry=%d dynamic=%d"),
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
