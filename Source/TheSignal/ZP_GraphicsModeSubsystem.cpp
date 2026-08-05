// Copyright The Signal. All Rights Reserved.
#include "ZP_GraphicsModeSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Scalability.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* GModeSection = TEXT("TheSignal.GraphicsMode");
	const TCHAR* GModeKey = TEXT("bPerformanceMode");

	// Both modes pin the FULL Epic tier — the horror lighting is calibrated against Epic Lumen
	// (Medium GI = "beyond broken and unrecognizable", DEAD ENDS 2026-08-04). The ONLY difference
	// between modes is the internal render resolution; TSR upscales to the display either way.

	FAutoConsoleCommandWithWorldAndArgs GModeCommand(
		TEXT("zp.Mode"),
		TEXT("Switch graphics mode: 'zp.Mode Performance' (Epic look, 50%% internal res, ~3x GPU headroom) or 'zp.Mode Quality' (Epic look, native res). Persists."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (Args.Num() < 1 || !World)
				{
					UE_LOG(LogTemp, Display, TEXT("[GraphicsMode] usage: zp.Mode Performance|Quality"));
					return;
				}
				if (UZP_GraphicsModeSubsystem* Sys = UZP_GraphicsModeSubsystem::Get(World))
				{
					const bool bPerf = Args[0].StartsWith(TEXT("P"), ESearchCase::IgnoreCase);
					Sys->ApplyMode(bPerf, /*bPersist=*/true);
				}
			}));
}

UZP_GraphicsModeSubsystem* UZP_GraphicsModeSubsystem::Get(const UObject* WorldContext)
{
	if (const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<UZP_GraphicsModeSubsystem>();
		}
	}
	return nullptr;
}

void UZP_GraphicsModeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// 2026-08-04 FINAL CONTRACT (dev: "copy what I have in my easy ui settings and make sure
	// that's what's going to load each time"): map load APPLIES the saved GameUserSettings —
	// it NEVER writes them. The dev's EGUI menu tuning is the truth; the Quality/Performance
	// presets in ApplyMode fire ONLY on explicit zp.Mode / SetGraphicsMode calls.
	EnforceSavedSettings();
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UZP_GraphicsModeSubsystem::OnPostLoadMap);
}

void UZP_GraphicsModeSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	Super::Deinitialize();
}

void UZP_GraphicsModeSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	EnforceSavedSettings();
}

void UZP_GraphicsModeSubsystem::EnforceSavedSettings()
{
	UWorld* World = GetWorld();
	// Editor plumbing so PIE honors game-side settings at all (root-caused 2026-08-04).
#if WITH_EDITOR
	GEngine->Exec(World, TEXT("r.Editor.Viewport.OverridePIEScreenPercentage 0"));
#endif
	GEngine->Exec(World, TEXT("r.ScreenPercentage.Mode 0"));

	// DEFAULT = LOW, EVERY START, NO EXCEPTIONS (dev directive 2026-08-04). Applied immediately
	// AND re-asserted 0.75s after map load so nothing that runs at startup (EGUI init, engine
	// scalability sinks, anything) can land after us and flip it back to ultra.
	ApplyLowSeed();
	if (UWorld* World2 = GetWorld())
	{
		World2->GetTimerManager().SetTimer(ReapplyHandle,
			FTimerDelegate::CreateUObject(this, &UZP_GraphicsModeSubsystem::ApplyLowSeed),
			0.75f, false);
	}
}

void UZP_GraphicsModeSubsystem::ApplyLowSeed()
{
	UGameUserSettings* GUS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!GUS)
	{
		return;
	}
	Scalability::FQualityLevels Q;
	Q.SetFromSingleQualityLevel(0);   // EVERY graphics group LOW
	Q.ResolutionQuality = 100.0f;     // native scale (no upscaler stress)
	GUS->ScalabilityQuality = Q;
	GUS->ApplyNonResolutionSettings(); // Low tiers + the menu's vsync choice
	UE_LOG(LogTemp, Warning, TEXT("[GraphicsMode] LOW enforced (all scalability groups 0, res scale 100, vsync=%s). Menu changes apply on top for the session."),
		GUS->IsVSyncEnabled() ? TEXT("ON") : TEXT("OFF"));
}

bool UZP_GraphicsModeSubsystem::LoadSavedModeIsPerformance() const
{
	bool bPerf = false;
	GConfig->GetBool(GModeSection, GModeKey, bPerf, GGameUserSettingsIni);
	return bPerf;
}

void UZP_GraphicsModeSubsystem::ApplyMode(const bool bPerformance, const bool bPersist)
{
	UWorld* World = GetWorld();

	// SINGLE OWNER = UGameUserSettings — the SAME store the EasyGameUI settings menu edits
	// (2026-08-04: cvar-level enforcement and the menu's ApplySettings kept stomping each other;
	// the dev's menu vsync toggle + this mode system must never fight). We write the mode into
	// GameUserSettings and apply THROUGH it; the menu's own settings (vsync, window resolution,
	// fullscreen) are the dev's/player's and are applied untouched alongside.
	UGameUserSettings* GUS = GEngine->GetGameUserSettings();
	if (GUS)
	{
		Scalability::FQualityLevels Q = GUS->ScalabilityQuality;
		Q.SetFromSingleQualityLevel(3); // Epic everywhere — the calibrated look, both modes
		Q.ResolutionQuality = bPerformance ? 50.0f : 100.0f;
		GUS->ScalabilityQuality = Q;
		GUS->ApplyNonResolutionSettings(); // applies scalability + the MENU's vsync choice
	}

	// Editor-side plumbing so PIE honors the game's resolution at all (root-caused 2026-08-04):
	// the editor pref r.Editor.Viewport.OverridePIEScreenPercentage makes PIE use the EDITOR
	// VIEWPORT's screen percentage and ignore the game entirely; Mode 0 = Manual control.
#if WITH_EDITOR
	GEngine->Exec(World, TEXT("r.Editor.Viewport.OverridePIEScreenPercentage 0"));
#endif
	GEngine->Exec(World, TEXT("r.ScreenPercentage.Mode 0"));

	// AA method per mode (2026-08-04, dev: vertical edges — door frames, wall corners — "break
	// horizontally" whenever the camera moves). That is TEMPORAL (TSR) reprojection breakup at
	// low fps, NOT vsync tearing — which is why every vsync fix changed nothing. QUALITY runs
	// native res, needs no upscaler: FXAA = single-frame AA, zero motion artifacts, edges stay
	// solid during turns. PERFORMANCE keeps TSR (it IS the 50%→display upscale); its artifacts
	// shrink as fps rises. AA method is not a GameUserSettings field — no fight with the menu.
	GEngine->Exec(World, bPerformance ? TEXT("r.AntiAliasingMethod 4") : TEXT("r.AntiAliasingMethod 1"));

	if (bPersist)
	{
		GConfig->SetBool(GModeSection, GModeKey, bPerformance, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
		if (GUS)
		{
			GUS->SaveSettings(); // safe now: ScalabilityQuality holds fully valid values
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GraphicsMode] %s applied via GameUserSettings (Epic tier, ResolutionQuality %.0f, AA=%s, menu vsync=%s)%s"),
		bPerformance ? TEXT("PERFORMANCE") : TEXT("QUALITY"),
		bPerformance ? 50.0f : 100.0f,
		bPerformance ? TEXT("TSR") : TEXT("FXAA"),
		GUS && GUS->IsVSyncEnabled() ? TEXT("ON") : TEXT("OFF"),
		bPersist ? TEXT(" [saved]") : TEXT(""));
}
