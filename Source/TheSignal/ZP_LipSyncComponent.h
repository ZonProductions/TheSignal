// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_LipSyncComponent
 *
 * Purpose: OVRLipSync-driven lip sync for NPCs during dialogue.
 *          Listens on the main audio submix for PCM data,
 *          feeds to OVRLipSync C API for real-time viseme analysis.
 *          Swaps CC head SkeletalMeshComponent to PoseableMeshComponent
 *          for direct bone rotation (jaw, lips, mouth corners).
 *
 * Owner Subsystem: Gameplay
 *
 * Blueprint Extension Points:
 *   - Add to any NPC actor using Character Customizer.
 *   - Zero configuration — auto-finds Head mesh by jaw bone presence.
 *
 * Dependencies:
 *   - Character Customizer mesh setup (Head mesh with jaw bone) on owner actor.
 *   - DialoguePlugin widget (Sound2D property) via reflection — NO compile-time dependency.
 *   - OVRLipSync C API (ThirdParty lib, linked via shim — NOT a UE plugin module dep).
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ISubmixBufferListener.h"
#include "OVRLipSync.h"
#include "ZP_LipSyncComponent.generated.h"

class USkeletalMeshComponent;
class UPoseableMeshComponent;
class USkeletalMesh;
class UAudioComponent;
class USoundSubmix;

/** Proxy that forwards submix PCM data to the component (thread-safe shared ptr). */
class FLipSyncSubmixProxy : public ISubmixBufferListener
{
public:
	TWeakObjectPtr<UZP_LipSyncComponent> Owner;
	FLipSyncSubmixProxy(UZP_LipSyncComponent* InOwner) : Owner(InOwner) {}
	virtual void OnNewSubmixBuffer(
		const USoundSubmix* OwningSubmix, float* AudioData,
		int32 NumSamples, int32 NumChannels,
		const int32 SampleRate, double AudioClock) override;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_LipSyncComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_LipSyncComponent();

	/** Set the active dialogue widget to monitor its Sound2D property. */
	void SetDialogueWidget(UUserWidget* Widget);

	/** Clear widget reference (dialogue ended / player walked away). */
	void ClearDialogueWidget();

	/** Process PCM audio buffer (called by proxy on audio render thread). */
	void ProcessAudioBuffer(float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate);

	// --- Viseme mapping (index order: 0=sil, 1=PP, 2=FF, 3=TH, 4=DD, 5=kk, 6=CH, 7=SS, 8=nn, 9=RR, 10=aa, 11=E, 12=ih, 13=oh, 14=ou) ---

	/** Per-viseme jaw-open weight mapping shaping how far the jaw opens for each detected phoneme. */
	UPROPERTY(EditAnywhere, Category = "LipSync|Visemes")
	float AZP_VisemeToJaw[15] = {
		0.00f, 0.15f, 0.35f, 0.45f, 0.60f, 0.55f, 0.50f, 0.30f,
		0.20f, 0.50f, 1.00f, 0.70f, 0.55f, 0.80f, 0.60f };

	/** Per-viseme lower-lip raise weight mapping (bilabials/labiodentals PP, FF, ou dominate). */
	UPROPERTY(EditAnywhere, Category = "LipSync|Visemes")
	float AZP_VisemeToLipUp[15] = {
		0.00f, 0.70f, 0.50f, 0.10f, 0.00f, 0.00f, 0.00f, 0.00f,
		0.10f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.30f };

	/** Per-viseme mouth-width mapping (positive = spread as in E/ih/aa, negative = purse as in oh/ou/PP). */
	UPROPERTY(EditAnywhere, Category = "LipSync|Visemes")
	float AZP_VisemeToWidth[15] = {
		0.00f, -0.40f, -0.15f, 0.00f, 0.00f, 0.00f, 0.15f, 0.25f,
		0.00f, 0.00f, 0.50f, 0.60f, 0.50f, -0.50f, -0.60f };

	// --- Smoothing knobs ---

	/** FInterpTo speed used when the jaw is opening (target above current) - higher snaps the mouth open faster on syllable onsets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Smoothing")
	float AZP_JawOpenInterpSpeed = 25.f;

	/** FInterpTo speed used when the jaw is closing (target below current) - lower gives a softer, lazier mouth close between syllables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Smoothing")
	float AZP_JawCloseInterpSpeed = 8.f;

	/** FInterpTo smoothing speed for the lower-lip raise channel (bilabial lip press response speed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Smoothing")
	float AZP_LipUpInterpSpeed = 18.f;

	/** FInterpTo smoothing speed for the mouth-width (spread/purse) channel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Smoothing")
	float AZP_MouthWidthInterpSpeed = 15.f;

	// --- Bone pose knobs ---

	/** Maximum jaw bone rotation in degrees at full jaw-open weight - the master 'how wide does the mouth open' knob. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|BonePose")
	float AZP_MaxJawOpenAngleDeg = 25.f;

	/** Rotation in degrees applied to lip_lower_l/r bones at full lip-raise weight (negative = curl upward) for PP/FF bilabial shapes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|BonePose")
	float AZP_MaxLipRaiseAngleDeg = -12.f;

	/** Rotation in degrees applied to mouth_l/r corner bones at full width weight (mirrored +/- between sides) for spread vs purse shapes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|BonePose")
	float AZP_MaxMouthWidthAngleDeg = 8.f;

	// --- Bone names ---

	/** Skeleton bone name driven for jaw open; also the bone whose presence auto-identifies the CC head mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Bones")
	FName AZP_JawBoneName = TEXT("jaw");

	/** Skeleton bone name for the right lower-lip bone driven by the lip-raise channel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Bones")
	FName AZP_LipLowerRightBoneName = TEXT("lip_lower_r");

	/** Skeleton bone name for the left lower-lip bone driven by the lip-raise channel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Bones")
	FName AZP_LipLowerLeftBoneName = TEXT("lip_lower_l");

	/** Skeleton bone name for the right mouth-corner bone driven by the width channel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Bones")
	FName AZP_MouthRightBoneName = TEXT("mouth_r");

	/** Skeleton bone name for the left mouth-corner bone driven by the width channel (rotation applied mirrored/negated vs the right corner). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LipSync|Bones")
	FName AZP_MouthLeftBoneName = TEXT("mouth_l");

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	// --- Mesh references ---

	/** The original CC head SkeletalMeshComponent (hidden after swap). */
	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> OriginalHeadMesh;

	/** Our PoseableMeshComponent clone (visible, drives bones). */
	UPROPERTY()
	UPoseableMeshComponent* FaceMesh = nullptr;

	// --- Dialogue tracking ---

	/** The active dialogue widget being monitored. */
	TWeakObjectPtr<UUserWidget> DialogueWidget;

	/** Cached reflection property for Sound2D on the widget. */
	FObjectProperty* Sound2DPropCache = nullptr;

	/** Whether audio is currently detected as playing. */
	bool bAudioPlaying = false;

	// --- Submix listener ---

	/** Submix buffer listener proxy (shared ptr for UE5.4 API). */
	TSharedPtr<FLipSyncSubmixProxy, ESPMode::ThreadSafe> SubmixProxy;

	/** Cached main submix reference for unregister. */
	TWeakObjectPtr<USoundSubmix> MainSubmixRef;

	// --- OVRLipSync ---

	/** OVRLipSync context handle. */
	ovrLipSyncContext OVRContext = 0;
	bool bOVRInitialized = false;

	/** Thread-safe viseme storage (written on audio thread, read on game thread). */
	FCriticalSection VisemeLock;
	TArray<float> VisemesCurrent; // 15 floats (ovrLipSyncViseme_Count)
	float LaughterScoreCurrent = 0.f;

	/** Game-thread copy of visemes (snapshot per tick). */
	TArray<float> VisemesGameThread;

	// --- Smoothed bone values ---

	/** Smoothed jaw openness. */
	float CurrentJawOpen = 0.f;
	float SmoothedLipUp = 0.f;
	float SmoothedWidth = 0.f;

	// --- Setup helpers ---

	/** Find SkeletalMeshComponent that has a bone named "jaw". */
	USkeletalMeshComponent* FindMeshWithJawBone() const;

	/** Create PoseableMeshComponent clone of head mesh. */
	void CreateFaceMesh(USkeletalMeshComponent* SourceHead);

	/** Initialize OVRLipSync C API. */
	void InitOVRLipSync();

	/** Shutdown OVRLipSync. */
	void ShutdownOVRLipSync();

	/** Apply viseme data to face bones via direct bone rotation. */
	void ApplyVisemes(float DeltaTime);
};
