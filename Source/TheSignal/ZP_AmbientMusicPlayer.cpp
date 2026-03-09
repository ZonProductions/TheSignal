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

	if (!SoundToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("AZP_AmbientMusicPlayer: No SoundToPlay assigned!"));
		return;
	}

	AudioComp->SetSound(SoundToPlay);
	AudioComp->SetVolumeMultiplier(Volume);
	AudioComp->bIsUISound = true;

	// Start first play after a short delay
	const float FirstDelay = FMath::RandRange(1.0f, 5.0f);
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
	if (!SoundToPlay || !AudioComp)
	{
		return;
	}

	// Fade in
	AudioComp->FadeIn(FadeInTime, Volume);

	// Schedule fade out before the sound ends
	const float Duration = GetSoundDuration();
	if (Duration > FadeOutTime + FadeInTime)
	{
		const float FadeOutStart = Duration - FadeOutTime;
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
		AudioComp->FadeOut(FadeOutTime, 0.0f);
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
	const float Delay = FMath::RandRange(MinInterval, MaxInterval);
	UE_LOG(LogTemp, Log, TEXT("AZP_AmbientMusicPlayer: Next play in %.1f seconds"), Delay);
	GetWorldTimerManager().SetTimer(PlayTimerHandle, this, &AZP_AmbientMusicPlayer::StartPlay, Delay, false);
}

float AZP_AmbientMusicPlayer::GetSoundDuration() const
{
	if (SoundToPlay)
	{
		return SoundToPlay->GetDuration();
	}
	return 0.0f;
}
