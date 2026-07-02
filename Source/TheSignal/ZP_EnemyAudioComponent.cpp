// Copyright The Signal. All Rights Reserved.

#include "ZP_EnemyAudioComponent.h"
#include "ZP_SFXStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

UZP_EnemyAudioComponent::UZP_EnemyAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Default to the Crawler set (overridable per-enemy in editor).
	static ConstructorHelpers::FObjectFinder<USoundBase> LurkF(TEXT("/Game/Audio/Crawler/SFX_Crawler_Lurking.SFX_Crawler_Lurking"));
	static ConstructorHelpers::FObjectFinder<USoundBase> AlertF(TEXT("/Game/Audio/Crawler/SFX_Crawler_Alert.SFX_Crawler_Alert"));
	static ConstructorHelpers::FObjectFinder<USoundBase> HitF(TEXT("/Game/Audio/Crawler/SFX_Crawler_Hit.SFX_Crawler_Hit"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Atk1F(TEXT("/Game/Audio/Crawler/SFX_Crawler_Attack.SFX_Crawler_Attack"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Atk2F(TEXT("/Game/Audio/Crawler/SFX_Crawler_Attack2.SFX_Crawler_Attack2"));
	if (LurkF.Succeeded())  { LurkingLoop = LurkF.Object; }
	if (AlertF.Succeeded()) { AlertSound = AlertF.Object; }
	if (HitF.Succeeded())   { HitSound = HitF.Object; }
	if (Atk1F.Succeeded())  { AttackSounds.Add(Atk1F.Object); }   // [0] normal strike (Attack1)
	if (Atk2F.Succeeded())  { AttackSounds.Add(Atk2F.Object); }   // [last] lunge (Attack2)
}

void UZP_EnemyAudioComponent::PlayLurk()
{
	if (!LurkingLoop) { return; }
	const float JitterLin = FMath::Pow(10.f, FMath::FRandRange(-LurkVolumeJitterDb, LurkVolumeJitterDb) / 20.f);
	PlayOneShot(LurkingLoop, LurkVolume * JitterLin, /*bAllowMuffle=*/true);
}

void UZP_EnemyAudioComponent::PlayAlert(float LowPassHz)
{
	PlayOneShot(AlertSound, 1.f, /*bAllowMuffle=*/true, LowPassHz);
}

void UZP_EnemyAudioComponent::PlayHit()
{
	PlayOneShot(HitSound, 1.f, /*bAllowMuffle=*/true);
}

void UZP_EnemyAudioComponent::PlayAttack(bool bLunge)
{
	if (AttackSounds.Num() == 0) { return; }
	USoundBase* Sound = bLunge ? AttackSounds.Last() : AttackSounds[0];
	PlayOneShot(Sound, 1.f, /*bAllowMuffle=*/true);
}

void UZP_EnemyAudioComponent::PlayOneShot(USoundBase* Sound, float Vol, bool bAllowMuffle, float ForceLowPassHz)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Sound) { return; }

	// All spawning, carry attenuation, and the Direct/Diffracted/Transmitted propagation model live
	// in UZP_SFXStatics — the one playback path every world SFX uses. Enemy voices carry Far so an
	// aggro scream is audible down a hallway, bends around corners, and muffles only through walls.
	UZP_SFXStatics::PlaySFXAttached(Sound, Owner->GetRootComponent(), EZP_SFXCarry::Far,
		VolumeMultiplier * Vol, 1.f, /*bPropagate=*/bAllowMuffle, ForceLowPassHz);
}
