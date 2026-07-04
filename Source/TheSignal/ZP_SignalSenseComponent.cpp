// Copyright The Signal. All Rights Reserved.

#include "ZP_SignalSenseComponent.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "GameFramework/ForceFeedbackEffect.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

const FName UZP_SignalSenseComponent::RumbleTag = TEXT("SignalSenseRumble");

UZP_SignalSenseComponent::UZP_SignalSenseComponent()
{
	// Tick drives the smooth waveform amplitude AND the interference volume crescendo.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Default the stems to the imported SignalSense SFX (overridable per-BP in editor).
	static ConstructorHelpers::FObjectFinder<USoundBase> InterferenceFinder(TEXT("/Game/Audio/Signal/MS_Signal_InterferenceBed.MS_Signal_InterferenceBed"));
	static ConstructorHelpers::FObjectFinder<USoundBase> RingFinder(TEXT("/Game/Audio/Signal/SFX_Phone_Ring.SFX_Phone_Ring"));
	static ConstructorHelpers::FObjectFinder<USoundBase> AlarmFinder(TEXT("/Game/Audio/Signal/SFX_Signal_Alarm.SFX_Signal_Alarm"));
	static ConstructorHelpers::FObjectFinder<USoundBase> SilenceFinder(TEXT("/Game/Audio/Signal/SFX_Phone_Silence.SFX_Phone_Silence"));
	if (InterferenceFinder.Succeeded()) { AZP_InterferenceLoop = InterferenceFinder.Object; }
	if (RingFinder.Succeeded())         { AZP_RingLoop = RingFinder.Object; }
	if (AlarmFinder.Succeeded())        { AZP_AlarmLoop = AlarmFinder.Object; }
	if (SilenceFinder.Succeeded())      { AZP_ClearSting = SilenceFinder.Object; }
}

void UZP_SignalSenseComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		// Dedicated interference voice: looping, starts at volume 0, never stops.
		InterferenceComp = NewObject<UAudioComponent>(Owner);
		if (InterferenceComp)
		{
			InterferenceComp->bAutoActivate = false;
			InterferenceComp->SetSound(AZP_InterferenceLoop);
			InterferenceComp->RegisterComponent();
			InterferenceComp->SetVolumeMultiplier(0.f);
			InterferenceComp->Play();   // loops silently until we drive volume up
		}

		// Enemy crossfade ping-pong voices.
		CurrentComp = NewObject<UAudioComponent>(Owner);
		SpareComp = NewObject<UAudioComponent>(Owner);
		for (UAudioComponent* AC : { CurrentComp.Get(), SpareComp.Get() })
		{
			if (AC)
			{
				AC->bAutoActivate = false;
				AC->RegisterComponent();
			}
		}
	}

	InitAmpHistory();

	if (UWorld* World = GetWorld())
	{
		const float Interval = FMath::Max(0.05f, AZP_EvaluateInterval);
		World->GetTimerManager().SetTimer(EvalTimer, this, &UZP_SignalSenseComponent::Evaluate, Interval, true);
	}
}

void UZP_SignalSenseComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvalTimer);
		World->GetTimerManager().ClearTimer(ClearTimer);
	}
	StopRumble();
	Super::EndPlay(Reason);
}

void UZP_SignalSenseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	WaveformAmplitude = FMath::FInterpTo(WaveformAmplitude, AmplitudeTarget, DeltaTime, AZP_AmplitudeSmoothSpeed);

	// Boundary fade: 3s curved attack/release on entering/leaving ALL voids (only at 0<->1).
	// A grace hold across small gaps between motes keeps mote->gap->mote seamless (no clip).
	if (bInAnyVoid) { TimeOutOfVoid = 0.f; }
	else            { TimeOutOfVoid += DeltaTime; }

	const float PhaseRate = DeltaTime / FMath::Max(0.05f, AZP_VoidFadeDuration);
	if (bInAnyVoid)
	{
		EnvPhase = FMath::Min(1.f, EnvPhase + PhaseRate);          // attack
	}
	else if (TimeOutOfVoid > AZP_VoidExitGrace)
	{
		EnvPhase = FMath::Max(0.f, EnvPhase - PhaseRate);          // release (only past grace)
	}
	// else: within grace -> hold EnvPhase, bridging the gap between motes.
	const float BoundaryEnv = EnvPhase * EnvPhase * (3.f - 2.f * EnvPhase); // smoothstep S-curve

	// In-void crescendo tracks position; rides under the boundary envelope so the volumes
	// always match at the transition (no pop between fade and crescendo).
	CrescendoVolume = FMath::FInterpTo(CrescendoVolume, CrescendoTarget, DeltaTime, FMath::Max(0.01f, AZP_CrescendoSmoothSpeed));

	InterferenceVolume = CrescendoVolume * BoundaryEnv;
	if (InterferenceComp)
	{
		InterferenceComp->SetVolumeMultiplier(InterferenceVolume);
	}

	PushAmpHistory(DeltaTime);
}

float UZP_SignalSenseComponent::GetAmpWritePhase() const
{
	return (float)AmpWriteHead / (float)AZP_AmpHistorySize;
}

void UZP_SignalSenseComponent::InitAmpHistory()
{
	AmpBuffer.Init(0, AZP_AmpHistorySize);
	AmpWriteHead = 0;
	AmpSampleAccum = 0.f;

	AmpHistoryTex = UTexture2D::CreateTransient(AZP_AmpHistorySize, 1, PF_G8);
	if (AmpHistoryTex)
	{
		AmpHistoryTex->SRGB = false;
		AmpHistoryTex->Filter = TF_Nearest;          // crisp per-sample bars
		AmpHistoryTex->AddressX = TA_Wrap;           // ring buffer wraps cleanly
		AmpHistoryTex->AddressY = TA_Clamp;
		AmpHistoryTex->NeverStream = true;
		AmpHistoryTex->UpdateResource();
	}
}

void UZP_SignalSenseComponent::PushAmpHistory(float DeltaTime)
{
	if (!AmpHistoryTex || AmpBuffer.Num() != AZP_AmpHistorySize)
	{
		return;
	}

	const float Rate = FMath::Max(1.f, AZP_AmpSampleRate);
	const float Step = 1.f / Rate;
	AmpSampleAccum += DeltaTime;

	bool bChanged = false;
	int32 Guard = 0;
	while (AmpSampleAccum >= Step && Guard++ < AZP_AmpHistorySize)
	{
		AmpSampleAccum -= Step;
		AmpBuffer[AmpWriteHead] = (uint8)(FMath::Clamp(WaveformAmplitude, 0.f, 1.f) * 255.f);
		AmpWriteHead = (AmpWriteHead + 1) % AZP_AmpHistorySize;
		bChanged = true;
	}

	if (bChanged)
	{
		uint8* Copy = (uint8*)FMemory::Malloc(AZP_AmpHistorySize);
		FMemory::Memcpy(Copy, AmpBuffer.GetData(), AZP_AmpHistorySize);
		FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, AZP_AmpHistorySize, 1);
		AmpHistoryTex->UpdateTextureRegions(0, 1, Region, AZP_AmpHistorySize, 1, Copy,
			[](uint8* Data, const FUpdateTextureRegion2D* Reg) { FMemory::Free(Data); delete Reg; });
	}
}

void UZP_SignalSenseComponent::Evaluate()
{
	const float VoidT = BestVoidPenetration();   // 0 centre -> 1 box surface, >1 outside
	const float EnemyDist = NearestTaggedDistance(AZP_EnemyTag, /*bHorizontalOnly=*/false);
	const bool bInVoid = VoidT <= 1.0f;

	EZP_SignalStage NewStage = EZP_SignalStage::None;
	if (EnemyDist <= AZP_MeleeRadius)      NewStage = EZP_SignalStage::Melee;
	else if (EnemyDist <= AZP_EnemyRadius) NewStage = EZP_SignalStage::Enemy;
	else if (bInVoid)                  NewStage = EZP_SignalStage::Interference;

	// In-void crescendo target (clamped, so the edge value persists just outside and the
	// boundary fade-out rides it down). Ducked during combat (combat stems carry the drone).
	bInAnyVoid = bInVoid && !IsCombat(NewStage);
	CrescendoTarget = InterferenceGainForT(VoidT);

	// Waveform amplitude: interference follows its live volume; combat steps up.
	if (NewStage == EZP_SignalStage::Melee)      AmplitudeTarget = AZP_MeleeStageAmplitude;
	else if (NewStage == EZP_SignalStage::Enemy) AmplitudeTarget = AZP_EnemyStageAmplitude;
	else                                         AmplitudeTarget = InterferenceVolume;

	if (NewStage != CurrentStage)
	{
		SetStage(NewStage);
	}

	UE_LOG(LogTemp, Warning, TEXT("[SignalSense] EVAL stage=%d inVoid=%d voidT=%.3f cresT=%.2f cresV=%.2f env=%.3f vol=%.2f tOut=%.2f"),
		(int32)CurrentStage, bInAnyVoid ? 1 : 0, VoidT, CrescendoTarget, CrescendoVolume, EnvPhase, InterferenceVolume, TimeOutOfVoid);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage((uint64)(UPTRINT)this, 0.5f, FColor::Cyan,
			FString::Printf(TEXT("[Signal] stage=%d  voidT=%.2f (in<=1)  enemy=%.0f  interfVol=%.2f"),
				(int32)CurrentStage, VoidT, EnemyDist, InterferenceVolume));
	}
}

float UZP_SignalSenseComponent::BestVoidPenetration() const
{
	const AActor* Owner = GetOwner();
	if (!Owner || AZP_VoidmoteTag.IsNone())
	{
		return TNumericLimits<float>::Max();
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(this, AZP_VoidmoteTag, Found);

	const FVector P = Owner->GetActorLocation();
	float Best = TNumericLimits<float>::Max();
	for (const AActor* A : Found)
	{
		if (!A || A == Owner) { continue; }

		// Use the void actor's own bounding box as the zone (scale the mote to fit the hall;
		// the box bottom at the floor gives "not below me" for free).
		FVector Origin, Extent;
		A->GetActorBounds(/*bOnlyCollidingComponents=*/false, Origin, Extent);
		if (Extent.IsNearlyZero())
		{
			Extent = FVector(AZP_VoidZoneRadius); // fallback if the actor reports no usable bounds
		}

		const FVector D = P - Origin;
		const float Tx = (Extent.X > KINDA_SMALL_NUMBER) ? FMath::Abs(D.X) / Extent.X : TNumericLimits<float>::Max();
		const float Ty = (Extent.Y > KINDA_SMALL_NUMBER) ? FMath::Abs(D.Y) / Extent.Y : TNumericLimits<float>::Max();
		const float Tz = (Extent.Z > KINDA_SMALL_NUMBER) ? FMath::Abs(D.Z) / Extent.Z : TNumericLimits<float>::Max();
		const float T = FMath::Max3(Tx, Ty, Tz); // 0 centre, 1 surface
		UE_LOG(LogTemp, Warning, TEXT("[SignalSense]   mote %s  T=%.3f  ext=(%.0f,%.0f,%.0f)"),
			*A->GetName(), T, Extent.X, Extent.Y, Extent.Z);
		Best = FMath::Min(Best, T);
	}
	return Best;
}

float UZP_SignalSenseComponent::InterferenceGainForT(float T) const
{
	// Two-slope crescendo by proximity-to-centre P (0 = surface, 1 = centre):
	//   outer region [0, KneeP]   -> gentle rise to KneeV (e.g. 30% by 60% proximity)
	//   inner region [KneeP, 1]   -> linear from KneeV up to full at the centre
	const float P = 1.f - FMath::Clamp(T, 0.f, 1.f);
	const float KneeP = FMath::Clamp(AZP_InterferenceKneeProximity, 0.01f, 0.99f);
	const float KneeV = FMath::Clamp(AZP_InterferenceKneeVolume, 0.f, 1.f);

	if (P <= KneeP)
	{
		return KneeV * (P / KneeP);
	}
	return KneeV + (1.f - KneeV) * (P - KneeP) / (1.f - KneeP);
}

float UZP_SignalSenseComponent::NearestTaggedDistance(FName Tag, bool bHorizontalOnly) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || Tag.IsNone())
	{
		return TNumericLimits<float>::Max();
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(this, Tag, Found);

	const FVector P = Owner->GetActorLocation();
	float Best = TNumericLimits<float>::Max();
	for (const AActor* A : Found)
	{
		if (A && A != Owner)
		{
			const FVector D = A->GetActorLocation() - P;
			const float Dist = bHorizontalOnly ? D.Size2D() : D.Size();
			Best = FMath::Min(Best, Dist);
		}
	}
	return Best;
}

void UZP_SignalSenseComponent::SetStage(EZP_SignalStage NewStage)
{
	UE_LOG(LogTemp, Warning, TEXT("[SignalSense] >>> SETSTAGE %d -> %d  (bInAnyVoid=%d  env=%.2f  vol=%.2f)"),
		(int32)CurrentStage, (int32)NewStage, bInAnyVoid ? 1 : 0, EnvPhase, InterferenceVolume);
	CurrentStage = NewStage;

	UWorld* World = GetWorld();

	// Enemy stems crossfade (interference is handled separately by volume).
	if (IsCombat(NewStage))
	{
		if (World) { World->GetTimerManager().ClearTimer(ClearTimer); }
		CrossfadeEnemyTo(EnemyLoopForStage(NewStage));
	}
	else
	{
		CrossfadeEnemyTo(nullptr);   // fade the enemy bed out
	}

	// Clear sting DISABLED (dev request 2026-06-14) — this Silence sting was the sound being
	// chased. Re-enable when we want an all-clear cue.
	if (World)
	{
		World->GetTimerManager().ClearTimer(ClearTimer);
	}

	// Haptics
	StopRumble();
	if (IsCombat(NewStage))
	{
		PlayStageRumble(NewStage);
	}

	OnSignalStageChanged.Broadcast(NewStage);
}

void UZP_SignalSenseComponent::CrossfadeEnemyTo(USoundBase* Sound)
{
	if (!CurrentComp || !SpareComp)
	{
		return;
	}

	if (CurrentComp->IsPlaying())
	{
		CurrentComp->FadeOut(AZP_CrossfadeTime, 0.f);
	}

	if (Sound)
	{
		SpareComp->SetSound(Sound);
		SpareComp->FadeIn(AZP_CrossfadeTime, 1.f);
	}

	Swap(CurrentComp, SpareComp);
}

void UZP_SignalSenseComponent::PlayClearSting()
{
	if (CurrentStage == EZP_SignalStage::None && AZP_ClearSting)
	{
		UGameplayStatics::PlaySound2D(this, AZP_ClearSting);
	}
}

void UZP_SignalSenseComponent::PlayStageRumble(EZP_SignalStage S)
{
	UForceFeedbackEffect* FX = RumbleForStage(S);
	if (!FX)
	{
		return; // null-safe until the FF assets exist
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PC)
	{
		FForceFeedbackParameters Params;
		Params.Tag = RumbleTag;
		Params.bLooping = true;
		PC->ClientPlayForceFeedback(FX, Params);
	}
}

void UZP_SignalSenseComponent::StopRumble()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PC)
	{
		PC->ClientStopForceFeedback(nullptr, RumbleTag);
	}
}

USoundBase* UZP_SignalSenseComponent::EnemyLoopForStage(EZP_SignalStage S) const
{
	switch (S)
	{
	case EZP_SignalStage::Enemy: return AZP_RingLoop;
	case EZP_SignalStage::Melee: return AZP_AlarmLoop;
	default:                     return nullptr;
	}
}

UForceFeedbackEffect* UZP_SignalSenseComponent::RumbleForStage(EZP_SignalStage S) const
{
	switch (S)
	{
	case EZP_SignalStage::Enemy: return AZP_RingRumble;
	case EZP_SignalStage::Melee: return AZP_AlarmRumble;
	default:                     return nullptr;
	}
}
