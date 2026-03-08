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
