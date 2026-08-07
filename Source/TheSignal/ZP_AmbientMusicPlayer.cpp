// Copyright The Signal. All Rights Reserved.

#include "ZP_AmbientMusicPlayer.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

AZP_AmbientMusicPlayer::AZP_AmbientMusicPlayer()
{
	PrimaryActorTick.bCanEverTick = false;

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));
	AudioComp->bAutoActivate = false;
	AudioComp->bIsUISound = true; // Non-spatialized (2D)
	RootComponent = AudioComp;
}

void AZP_AmbientMusicPlayer::BeginPlay()
{
	Super::BeginPlay();

	if (!AZP_SoundToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("AZP_AmbientMusicPlayer: No AZP_SoundToPlay assigned!"));
		return;
	}

	AudioComp->SetSound(AZP_SoundToPlay);
	// AZP_Volume lives ONLY on the component multiplier. The old code ALSO used it as the FadeIn
	// target, so the audible level was AZP_Volume * AZP_Volume (0.4 played at 0.16) — every fade
	// below targets 1.0 instead.
	AudioComp->SetVolumeMultiplier(AZP_Volume);
	AudioComp->bIsUISound = true;

	// The natural end of playback drives the cycle — no duration guesswork, no padding.
	AudioComp->OnAudioFinished.AddDynamic(this, &AZP_AmbientMusicPlayer::HandleAudioFinished);

	// Start first play after the configured delay (0/0 = immediately).
	const float FirstDelay = FMath::RandRange(AZP_FirstPlayDelayMin, AZP_FirstPlayDelayMax);
	if (FirstDelay <= 0.0f)
	{
		StartPlay();
	}
	else
	{
		GetWorldTimerManager().SetTimer(PlayTimerHandle, this, &AZP_AmbientMusicPlayer::StartPlay, FirstDelay, false);
	}
}

void AZP_AmbientMusicPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bShuttingDown = true; // OnAudioFinished fires synchronously from Stop() below
	GetWorldTimerManager().ClearTimer(PlayTimerHandle);
	GetWorldTimerManager().ClearTimer(FadeOutTimerHandle);

	if (AudioComp && AudioComp->IsPlaying())
	{
		AudioComp->Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void AZP_AmbientMusicPlayer::StartPlay()
{
	if (!AZP_SoundToPlay || !AudioComp || bShuttingDown)
	{
		return;
	}

	const float Duration = GetSoundDuration();
	const bool bGapless = IsGapless();

	if (bFirstCycle || !bGapless)
	{
		// FadeIn TARGET is 1.0 — AZP_Volume is already on the component multiplier.
		AudioComp->FadeIn(AZP_FadeInTime, 1.0f);
	}
	else
	{
		// Gapless repeat: back-to-back music, no re-fade dip at the loop seam.
		AudioComp->Play();
	}
	bFirstCycle = false;

	// Fade-out only when SILENCE follows this play. In gapless mode the old unconditional
	// fade-out + the Duration+0.5s restart timer produced ~3s of fade to nothing and half a
	// second of dead air per cycle — none of it configured (dev 2026-08-07: "doesn't play
	// consistent to what is setup ... some hard coded delays").
	if (!bGapless && Duration > AZP_FadeOutTime + AZP_FadeInTime)
	{
		const float FadeOutStart = Duration - AZP_FadeOutTime;
		GetWorldTimerManager().SetTimer(FadeOutTimerHandle, this, &AZP_AmbientMusicPlayer::BeginFadeOut, FadeOutStart, false);
	}

	UE_LOG(LogTemp, Log, TEXT("AZP_AmbientMusicPlayer: play %.1fs%s (vol=%.2f, fadeIn=%.1f%s)"),
		Duration, bGapless ? TEXT(" GAPLESS") : TEXT(""), AZP_Volume, AZP_FadeInTime,
		bGapless ? TEXT("") : *FString::Printf(TEXT(", fadeOut@%.1f"), Duration - AZP_FadeOutTime));
}

void AZP_AmbientMusicPlayer::BeginFadeOut()
{
	if (AudioComp && AudioComp->IsPlaying())
	{
		AudioComp->FadeOut(AZP_FadeOutTime, 0.0f);
	}
}

void AZP_AmbientMusicPlayer::HandleAudioFinished()
{
	// Natural end of the track (a FadeOut to silent does NOT stop playback — this still fires at
	// the real end). Also fires from Stop() during teardown, hence the guard.
	if (bShuttingDown)
	{
		return;
	}
	ScheduleNextPlay();
}

void AZP_AmbientMusicPlayer::ScheduleNextPlay()
{
	if (IsGapless())
	{
		// Back-to-back restart, zero configured silence = zero delivered silence.
		StartPlay();
		return;
	}
	const float Delay = FMath::RandRange(AZP_MinInterval, AZP_MaxInterval);
	UE_LOG(LogTemp, Log, TEXT("AZP_AmbientMusicPlayer: Next play in %.1f seconds"), Delay);
	GetWorldTimerManager().SetTimer(PlayTimerHandle, this, &AZP_AmbientMusicPlayer::StartPlay, Delay, false);
}

float AZP_AmbientMusicPlayer::GetSoundDuration() const
{
	if (AZP_SoundToPlay)
	{
		return AZP_SoundToPlay->GetDuration();
	}
	return 0.0f;
}
