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
	AudioComp->SetVolumeMultiplier(AZP_Volume);
	AudioComp->bIsUISound = true;

	// Start first play after a short delay
	const float FirstDelay = FMath::RandRange(AZP_FirstPlayDelayMin, AZP_FirstPlayDelayMax);
	GetWorldTimerManager().SetTimer(PlayTimerHandle, this, &AZP_AmbientMusicPlayer::StartPlay, FirstDelay, false);
}

void AZP_AmbientMusicPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
	if (!AZP_SoundToPlay || !AudioComp)
	{
		return;
	}

	// Fade in
	AudioComp->FadeIn(AZP_FadeInTime, AZP_Volume);

	// Schedule fade out before the sound ends
	const float Duration = GetSoundDuration();
	if (Duration > AZP_FadeOutTime + AZP_FadeInTime)
	{
		const float FadeOutStart = Duration - AZP_FadeOutTime;
		GetWorldTimerManager().SetTimer(FadeOutTimerHandle, this, &AZP_AmbientMusicPlayer::BeginFadeOut, FadeOutStart, false);
	}

	// Schedule next play after this one finishes
	if (Duration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(PlayTimerHandle, this, &AZP_AmbientMusicPlayer::OnPlaybackFinished, Duration + 0.5f, false);
	}
}

void AZP_AmbientMusicPlayer::BeginFadeOut()
{
	if (AudioComp && AudioComp->IsPlaying())
	{
		AudioComp->FadeOut(AZP_FadeOutTime, 0.0f);
	}
}

void AZP_AmbientMusicPlayer::OnPlaybackFinished()
{
	if (AudioComp && AudioComp->IsPlaying())
	{
		AudioComp->Stop();
	}

	ScheduleNextPlay();
}

void AZP_AmbientMusicPlayer::ScheduleNextPlay()
{
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
