// ZP_WarmupGateSubsystem.h
//
// Purpose:    Front-loads hitch pain behind the EasyGameUI loading screen. At world
//             start it shows WBP_ESGU_LoadingScreen (via BP_EasySaveGameOperationsManager),
//             pauses the game, async-preloads every asset the gameplay code otherwise
//             sync-loads mid-combat (grab anims, enemy SFX, melee anims, blood VFX, UI
//             glyphs), and releases only when the async loader, shader compiler and PSO
//             precache queue are all drained (PSO drain is a no-op in PIE — the engine
//             hard-disables PSO precaching under WITH_EDITOR).
// Owned by:   Core / loading & performance.
// Extension:  Not intended for BP extension. Tuning via console variables:
//             zp.WarmupGate.Enabled, zp.WarmupGate.MinShowSec, zp.WarmupGate.TimeoutSec,
//             zp.WarmupGate.Pause, zp.WarmupGate.WaitForShaders, zp.WarmupGate.StreamTextures,
//             zp.WarmupGate.TextureTimeoutSec, zp.WarmupGate.KeepTexturesResident (see .cpp
//             defaults; registered in Docs/AZP_CustomKnobs.md).
//             2026-08-04: gate now also force-streams ALL level textures to full residency
//             behind the loading screen and (by default) keeps them resident for the session —
//             floor/stair crossings no longer stream textures ("massive loading after each
//             floor" dev report). If VRAM device-hung returns, set
//             zp.WarmupGate.KeepTexturesResident 0 first, then zp.WarmupGate.StreamTextures 0.
// Depends on: EasyGameUI (reflection-only — BP_EasySaveGameOperationsManager::
//             StartStopLoadingScreen + GI GetCurrentActiveLoadingScreen; no compile-time
//             coupling), FStreamableManager, RHI PipelineStateCache, ShaderCompiler.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZP_WarmupGateSubsystem.generated.h"

class UUserWidget;
struct FStreamableHandle;

UCLASS()
class THESIGNAL_API UZP_WarmupGateSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	enum class EGateState : uint8 { Idle, PendingStart, Warming, Released };

	void StartGate();
	void ReleaseGate(bool bTimedOut);

	// EasyGameUI bridge (reflection only — BP function/param names, incl. the literal
	// '?' suffixes on bool params, are resolved by name at runtime).
	AActor* FindOrSpawnOpsManager();
	void CallStartStopLoadingScreen(bool bStop, bool bFade);
	UUserWidget* GetActiveLoadingScreen() const;

	bool AreLoadsDrained() const;
	bool AreShadersDrained() const;
	bool ArePSOsDrained() const;
	bool AreTexturesDrained(double Elapsed);

	// Marks every loaded /Game texture's mips force-resident for DurationSec.
	// Returns the number of textures touched.
	int32 ForceAllTexturesResident(float DurationSec) const;

	EGateState State = EGateState::Idle;
	double GateStartTime = 0.0;
	double LastReassertTime = 0.0;
	// First wall-clock time each drain condition was observed satisfied (0 = not yet).
	double LoadsDrainedAt = 0.0;
	double ShadersDrainedAt = 0.0;
	double PSOsDrainedAt = 0.0;
	double TexturesDrainedAt = 0.0;
	int32 NumTexturesForced = 0;
	bool bTextureTimeoutLogged = false;
	// Wall-clock Elapsed when both streaming-wants AND texture DDC compiles first hit zero;
	// reset if either resumes. Textures count as drained only after 2s of stable zero.
	double TexZeroSince = 0.0;
	// PSO warm: occlusion queries disabled + camera spun 360° during the hold so every
	// in-range room submits draws behind the loading screen (PIE has no PSO precache).
	int32 WarmSpinTick = 0;
	FRotator WarmSpinOriginalRot = FRotator::ZeroRotator;
	bool bWarmSpinRotCaptured = false;
	int32 OcclusionQueriesPrior = 1;
	bool bOcclusionDisabled = false;
	// Melee view-model warm cycle: 0=pending, 1=activated (waiting frames), 2=done.
	int32 WarmMeleePhase = 0;
	double WarmMeleeActivatedAt = 0.0;

	bool bPausedByGate = false;

	TWeakObjectPtr<AActor> OpsManager;
	// Held for the world's lifetime so preloaded assets stay resident (GC-rooted by the handle).
	TSharedPtr<FStreamableHandle> PreloadHandle;
};
