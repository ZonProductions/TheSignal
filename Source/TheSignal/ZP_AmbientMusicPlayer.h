// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_AmbientMusicPlayer
 *
 * Purpose: Plays background music at sparse intervals with fade in/out.
 *          Place in level, assign a SoundWave. Auto-plays in PIE.
 *
 * Owner Subsystem: Audio
 *
 * Blueprint Extension Points:
 *   - SoundToPlay, Volume, FadeInTime, FadeOutTime, MinInterval, MaxInterval
 *
 * Dependencies:
 *   - None
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_AmbientMusicPlayer.generated.h"

class UAudioComponent;

UCLASS()
class THESIGNAL_API AZP_AmbientMusicPlayer : public AActor
{
	GENERATED_BODY()

public:
	AZP_AmbientMusicPlayer();

	/** The sound to play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	TObjectPtr<USoundBase> SoundToPlay;

	/** Playback volume (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Volume = 0.4f;

	/** Fade in duration in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float FadeInTime = 3.0f;

	/** Fade out duration in seconds. Starts this many seconds before the sound ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float FadeOutTime = 3.0f;

	/** Minimum silence between plays (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float MinInterval = 10.0f;

	/** Maximum silence between plays (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	float MaxInterval = 30.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> AudioComp;

	FTimerHandle PlayTimerHandle;
	FTimerHandle FadeOutTimerHandle;

	void StartPlay();
	void BeginFadeOut();
	void OnPlaybackFinished();
	void ScheduleNextPlay();

	float GetSoundDuration() const;
};
