// Copyright The Signal. All Rights Reserved.

#include "ZP_EnemyAudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

UZP_EnemyAudioComponent::UZP_EnemyAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Default to the Crawler set + shared attenuation (overridable per-enemy in editor).
	static ConstructorHelpers::FObjectFinder<USoundBase> LurkF(TEXT("/Game/Audio/Crawler/SFX_Crawler_Lurking.SFX_Crawler_Lurking"));
	static ConstructorHelpers::FObjectFinder<USoundBase> AlertF(TEXT("/Game/Audio/Crawler/SFX_Crawler_Alert.SFX_Crawler_Alert"));
	static ConstructorHelpers::FObjectFinder<USoundBase> HitF(TEXT("/Game/Audio/Crawler/SFX_Crawler_Hit.SFX_Crawler_Hit"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Atk1F(TEXT("/Game/Audio/Crawler/SFX_Crawler_Attack.SFX_Crawler_Attack"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Atk2F(TEXT("/Game/Audio/Crawler/SFX_Crawler_Attack2.SFX_Crawler_Attack2"));
	static ConstructorHelpers::FObjectFinder<USoundAttenuation> AttnF(TEXT("/Game/Audio/SA_EnemyVoice.SA_EnemyVoice"));
	if (LurkF.Succeeded())  { LurkingLoop = LurkF.Object; }
	if (AlertF.Succeeded()) { AlertSound = AlertF.Object; }
	if (HitF.Succeeded())   { HitSound = HitF.Object; }
	if (Atk1F.Succeeded())  { AttackSounds.Add(Atk1F.Object); }   // [0] normal strike (Attack1)
	if (Atk2F.Succeeded())  { AttackSounds.Add(Atk2F.Object); }   // [last] lunge (Attack2)
	if (AttnF.Succeeded())  { Attenuation = AttnF.Object; }
}

void UZP_EnemyAudioComponent::PlayLurk()
{
	if (!LurkingLoop) { return; }
	const float JitterLin = FMath::Pow(10.f, FMath::FRandRange(-LurkVolumeJitterDb, LurkVolumeJitterDb) / 20.f);
	PlayOneShot(LurkingLoop, LurkVolume * JitterLin, /*bAllowMuffle=*/true);
}

void UZP_EnemyAudioComponent::PlayAlert(float LowPassHz)
{
	PlayOneShot(AlertSound, 1.f, /*bAllowMuffle=*/false, LowPassHz);
}

void UZP_EnemyAudioComponent::PlayHit()
{
	PlayOneShot(HitSound, 1.f, /*bAllowMuffle=*/false);
}

void UZP_EnemyAudioComponent::PlayAttack(bool bLunge)
{
	if (AttackSounds.Num() == 0) { return; }
	USoundBase* Sound = bLunge ? AttackSounds.Last() : AttackSounds[0];
	PlayOneShot(Sound, 1.f, /*bAllowMuffle=*/false);
}

void UZP_EnemyAudioComponent::PlayOneShot(USoundBase* Sound, float Vol, bool bAllowMuffle, float ForceLowPassHz)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Sound) { return; }

	UWorld* World = GetWorld();

	// C++ self-occlusion: the engine's SoundAttenuation occlusion reliably MUTES these sounds in
	// this project (confirmed twice, on both Visibility and WorldStatic channels), so we roll our
	// own. Trace WorldStatic (walls always block it) from the listener to this enemy — if solid
	// geometry is between them, muffle: half volume + low-pass. Clear/full in the open.
	float FinalVol = VolumeMultiplier * Vol;
	float DistToPlayer = -1.f;
	bool bOccluded = false;
	if (World && bAllowMuffle)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FVector CamLoc; FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			const FVector EnemyLoc = Owner->GetActorLocation();
			FHitResult Hit;
			FCollisionQueryParams P;
			P.AddIgnoredActor(Owner);
			if (World->LineTraceSingleByChannel(Hit, CamLoc, EnemyLoc, ECC_WorldStatic, P))
			{
				// Only a REAL between-room wall counts as occlusion. A crawler perched on a wall/
				// ceiling sits in that surface, so ignore any blocker within ~150cm of the enemy —
				// that's its own perch geometry, not a wall between you and it.
				const float DistToEnemy = FVector::Dist(CamLoc, EnemyLoc);
				bOccluded = (Hit.Distance < DistToEnemy - 150.f);
			}
			if (APawn* Pawn = PC->GetPawn())
			{
				DistToPlayer = FVector::Dist(EnemyLoc, Pawn->GetActorLocation());
			}
		}
	}
	if (bOccluded) { FinalVol *= 0.5f; }

	UE_LOG(LogTemp, Warning, TEXT("[ENEMYAUDIO] %s plays '%s' vol=%.2f distToPlayer=%.0fcm occluded=%s"),
		*Owner->GetName(), *Sound->GetName(), FinalVol, DistToPlayer, bOccluded ? TEXT("YES") : TEXT("no"));

	UAudioComponent* AC = UGameplayStatics::SpawnSoundAttached(
		Sound, Owner->GetRootComponent(), NAME_None,
		FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget,
		/*bStopWhenAttachedToDestroyed=*/true, FinalVol, 1.f, 0.f,
		Attenuation, nullptr, /*bAutoDestroy=*/true);

	if (AC && bOccluded)
	{
		AC->SetLowPassFilterEnabled(true);
		AC->SetLowPassFilterFrequency(600.f); // muffled tone through walls
	}
	else if (AC && ForceLowPassHz > 0.f)
	{
		AC->SetLowPassFilterEnabled(true);
		AC->SetLowPassFilterFrequency(ForceLowPassHz); // caller-requested muffle (e.g. gaze hiss @ 5k)
	}
}
