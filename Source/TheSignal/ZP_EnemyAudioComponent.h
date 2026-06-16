// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_EnemyAudioComponent
 *
 * Purpose: Reusable spatialized voice for any enemy. Intermittent "lurking" growls (one-shots
 *          at a low, jittered volume) while unnoticed, plus alert + attack one-shots, all routed
 *          through a shared SoundAttenuation asset (room-scale falloff + occlusion). Mono sources
 *          required to spatialize. Add to any enemy actor; drive from its behavior.
 *
 * Owner Subsystem: EnemyAI / Audio
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_EnemyAudioComponent.generated.h"

class USoundBase;
class USoundAttenuation;

UCLASS(ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_EnemyAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_EnemyAudioComponent();

	/** Lurking growl — played intermittently at low volume while the enemy is unnoticed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyAudio")
	TObjectPtr<USoundBase> LurkingLoop;

	/** One-shot when the enemy notices the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyAudio")
	TObjectPtr<USoundBase> AlertSound;

	/** One-shot when the enemy is hit / takes damage (but survives). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyAudio")
	TObjectPtr<USoundBase> HitSound;

	/** Attack one-shots: [0] = normal strike (Attack1), last = lunge / surprise (Attack2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyAudio")
	TArray<TObjectPtr<USoundBase>> AttackSounds;

	/** Shared room-scale attenuation (SA_EnemyVoice) with occlusion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyAudio")
	TObjectPtr<USoundAttenuation> Attenuation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyAudio")
	float VolumeMultiplier = 1.f;

	/** Base volume for lurking growls (quieter than alert/attack, but must be clearly audible). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyAudio")
	float LurkVolume = 0.8f;

	/** Per-growl random volume swing, in dB (+/-). Keeps the lurking organic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyAudio")
	float LurkVolumeJitterDb = 2.f;

	/** Intermittent lurking growl (low, jittered volume). Call on an interval while unnoticed. */
	UFUNCTION(BlueprintCallable, Category = "EnemyAudio")
	void PlayLurk();

	/** LowPassHz > 0 drops an unconditional low-pass at that frequency on the played instance
	 *  (e.g. 5000 for a muffled gaze-provoke hiss). 0 = clear. */
	UFUNCTION(BlueprintCallable, Category = "EnemyAudio")
	void PlayAlert(float LowPassHz = 0.f);

	/** Hit reaction one-shot. Call when the enemy takes damage but is still alive. */
	UFUNCTION(BlueprintCallable, Category = "EnemyAudio")
	void PlayHit();

	/** Attack vocalization. bLunge -> the lunge strike (Attack2, last); otherwise Attack1 ([0]). */
	UFUNCTION(BlueprintCallable, Category = "EnemyAudio")
	void PlayAttack(bool bLunge = false);

private:
	/** bAllowMuffle: if true, a wall between the listener and the enemy lowpass+halves the sound
	 *  (used for the ambient lurk). Combat events (alert/attack/hit) pass false — always clear. */
	void PlayOneShot(USoundBase* Sound, float Vol, bool bAllowMuffle, float ForceLowPassHz = 0.f);
};
