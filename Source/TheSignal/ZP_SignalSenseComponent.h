// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_SignalSenseComponent
 *
 * Purpose: "The Phone" — modern Silent Hill radio. Proximity warning, escalating through
 *          stages, driving synced channels: layered audio, HUD waveform amplitude,
 *          controller vibration, stage-changed event.
 *
 * Owner Subsystem: PlayerCharacter (lives on AZP_GraceCharacter).
 *
 * Behaviour:
 *   - INTERFERENCE: being inside a voidzone. Its own dedicated, always-looping voice whose
 *     VOLUME ramps with distance from the void centre (0 dB centre -> InterferenceEdgeDb at
 *     the edge) and fades to silence outside. Never stops, so leaving + re-entering resumes
 *     the same loop with no cut-off and no cooldown. Void uses HORIZONTAL distance.
 *   - ENEMY / MELEE: enemy proximity (Ring near, Alarm melee), crossfaded ping-pong voices.
 *   - NOTIFY: one-shot, fired only when you LEAVE combat (Enemy/Melee -> non-combat).
 *   - CLEAR sting: one-shot, AZP_ClearDelay seconds after everything is fully quiet (None).
 *
 * Detection: tag-based on a low-frequency timer.
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_SignalSenseComponent.generated.h"

class USoundBase;
class UAudioComponent;
class UForceFeedbackEffect;
class UTexture2D;

UENUM(BlueprintType)
enum class EZP_SignalStage : uint8
{
	None         UMETA(DisplayName = "None (Clear)"),
	Interference UMETA(DisplayName = "Interference (in void)"),
	Enemy        UMETA(DisplayName = "Enemy (Call/Ring)"),
	Melee        UMETA(DisplayName = "Melee (Alarm)")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSignalStageChanged, EZP_SignalStage, NewStage);

UCLASS(ClassGroup = (Signal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_SignalSenseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_SignalSenseComponent();

	// --- Detection ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Detection")
	FName AZP_VoidmoteTag = TEXT("Voidmote");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Detection")
	FName AZP_EnemyTag = TEXT("Enemy");

	/** Fallback box half-size used only if a tagged void actor reports no usable bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Detection")
	float AZP_VoidZoneRadius = 500.f;

	/** Enemy "in range" radius -> Ring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Detection")
	float AZP_EnemyRadius = 1400.f;

	/** Enemy "on you" radius -> Alarm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Detection")
	float AZP_MeleeRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Detection")
	float AZP_EvaluateInterval = 0.25f;

	// --- Audio (matched 10.38s loop stems, drone baked in) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	TObjectPtr<USoundBase> AZP_InterferenceLoop;   // MS_Signal_InterferenceBed (in-void drone)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	TObjectPtr<USoundBase> AZP_RingLoop;           // SFX_Phone_Ring (enemy near)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	TObjectPtr<USoundBase> AZP_AlarmLoop;          // SFX_Signal_Alarm (enemy melee)

	/** One-shot "all clear" sting, AZP_ClearDelay seconds after fully quiet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	TObjectPtr<USoundBase> AZP_ClearSting;         // SFX_Phone_Silence

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	float AZP_CrossfadeTime = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	float AZP_ClearDelay = 4.0f;

	/** Crescendo knee: proximity-to-centre (0 = box surface, 1 = centre) where the gentle outer
	 *  region ends and the ramp to full begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	float AZP_InterferenceKneeProximity = 0.6f;

	/** Volume at the knee. Outer region rises gently to this; inner region ramps linearly from
	 *  here to full at the centre. (Dev spec: 30% up to 60% proximity, then linear.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	float AZP_InterferenceKneeVolume = 0.3f;

	/** Seconds for the curved fade-in/out at the void boundary (entering/leaving ALL voids).
	 *  Only fires at the 0<->1 void transition — never between chained/overlapping voids. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	float AZP_VoidFadeDuration = 3.0f;

	/** How fast the in-void crescendo tracks your position (higher = snappier to centre/edge). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	float AZP_CrescendoSmoothSpeed = 5.0f;

	/** Grace before the fade-out starts after leaving all voids. Matched to AZP_VoidFadeDuration so
	 *  any gap crossed faster than the fade holds the bed seamlessly (no clip, no dip). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Audio")
	float AZP_VoidExitGrace = 3.0f;

	// --- Haptics (gamepad only; null-safe until assets exist) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Haptics")
	TObjectPtr<UForceFeedbackEffect> AZP_RingRumble;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Haptics")
	TObjectPtr<UForceFeedbackEffect> AZP_AlarmRumble;

	// --- Outputs ---
	UPROPERTY(BlueprintAssignable, Category = "SignalSense")
	FOnSignalStageChanged OnSignalStageChanged;

	UPROPERTY(BlueprintReadOnly, Category = "SignalSense")
	EZP_SignalStage CurrentStage = EZP_SignalStage::None;

	UFUNCTION(BlueprintPure, Category = "SignalSense")
	float GetWaveformAmplitude() const { return WaveformAmplitude; }

	UFUNCTION(BlueprintPure, Category = "SignalSense")
	EZP_SignalStage GetCurrentStage() const { return CurrentStage; }

	/** Rolling amplitude-history texture (256x1, G8) for the chronological scope. */
	UFUNCTION(BlueprintPure, Category = "SignalSense")
	UTexture2D* GetAmpHistoryTexture() const { return AmpHistoryTex; }

	/** Normalized write head (0..1) — where "now" is in the ring buffer. */
	UFUNCTION(BlueprintPure, Category = "SignalSense")
	float GetAmpWritePhase() const;

	/** Scope sample rate (samples/sec). Higher = faster scroll, less time on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Waveform")
	float AZP_AmpSampleRate = 60.f;

	/** FInterpTo speed with which the HUD waveform amplitude eases toward its stage target — the feel of how quickly the scope reacts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Waveform")
	float AZP_AmplitudeSmoothSpeed = 6.f;

	/** HUD waveform amplitude target while in the Melee (Alarm) stage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Waveform")
	float AZP_MeleeStageAmplitude = 1.0f;

	/** HUD waveform amplitude target while in the Enemy (Ring) stage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SignalSense|Waveform")
	float AZP_EnemyStageAmplitude = 0.7f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Evaluate();
	void SetStage(EZP_SignalStage NewStage);

	USoundBase* EnemyLoopForStage(EZP_SignalStage S) const;
	UForceFeedbackEffect* RumbleForStage(EZP_SignalStage S) const;

	void CrossfadeEnemyTo(USoundBase* Sound);
	void PlayClearSting();
	void PlayStageRumble(EZP_SignalStage S);
	void StopRumble();

	float NearestTaggedDistance(FName Tag, bool bHorizontalOnly) const;
	/** 0 = centre of a tagged void's box, 1 = its surface, >1 = outside all of them. */
	float BestVoidPenetration() const;
	float InterferenceGainForT(float T) const;
	static bool IsCombat(EZP_SignalStage S) { return S == EZP_SignalStage::Enemy || S == EZP_SignalStage::Melee; }

	FTimerHandle EvalTimer;
	FTimerHandle ClearTimer;

	// Dedicated, always-looping interference voice (volume-driven, never restarted).
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> InterferenceComp;
	// Ping-pong crossfade voices for the enemy stems.
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> CurrentComp;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> SpareComp;

	float WaveformAmplitude = 0.f;
	float AmplitudeTarget = 0.f;
	float InterferenceVolume = 0.f;   // final = CrescendoVolume * boundary envelope
	float CrescendoVolume = 0.f;      // in-void position crescendo (edge -> centre)
	float CrescendoTarget = 0.f;
	float EnvPhase = 0.f;             // 0..1 boundary fade phase (AZP_VoidFadeDuration), smoothstepped
	float TimeOutOfVoid = 0.f;       // seconds since leaving all voids (grace before fade-out)
	bool bInAnyVoid = false;

	// Chronological scope: rolling amplitude history pushed into a texture each frame.
	static constexpr int32 AZP_AmpHistorySize = 256;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> AmpHistoryTex;
	TArray<uint8> AmpBuffer;
	int32 AmpWriteHead = 0;
	float AmpSampleAccum = 0.f;
	void InitAmpHistory();
	void PushAmpHistory(float DeltaTime);

	static const FName RumbleTag;
};
