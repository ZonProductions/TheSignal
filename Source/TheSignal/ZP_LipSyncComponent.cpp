// Copyright The Signal. All Rights Reserved.

#include "ZP_LipSyncComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundSubmix.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Blueprint/UserWidget.h"
#include "AudioDevice.h"

DEFINE_LOG_CATEGORY_STATIC(LogLipSync, Log, All);

// ============================================================================
// OVRLipSync viseme indices (from OVRLipSync.h enum)
// ============================================================================
// 0=sil, 1=PP, 2=FF, 3=TH, 4=DD, 5=kk, 6=CH, 7=SS,
// 8=nn, 9=RR, 10=aa, 11=E, 12=ih, 13=oh, 14=ou

// Viseme -> jawOpen morph weight
static const float VisemeToJaw[15] = {
	0.00f, // sil
	0.15f, // PP
	0.35f, // FF
	0.45f, // TH
	0.60f, // DD
	0.55f, // kk
	0.50f, // CH
	0.30f, // SS
	0.20f, // nn
	0.50f, // RR
	1.00f, // aa
	0.70f, // E
	0.55f, // ih
	0.80f, // oh
	0.60f, // ou
};

// Viseme -> lower lip raise (bilabials/labiodentals: PP, FF, OU)
static const float VisemeToLipUp[15] = {
	0.00f, // sil
	0.70f, // PP — lips pressed together
	0.50f, // FF — lower lip curls up
	0.10f, // TH
	0.00f, // DD
	0.00f, // kk
	0.00f, // CH
	0.00f, // SS
	0.10f, // nn
	0.00f, // RR
	0.00f, // aa
	0.00f, // E
	0.00f, // ih
	0.00f, // oh
	0.30f, // ou
};

// Viseme -> mouth width (positive = spread, negative = purse)
static const float VisemeToWidth[15] = {
	0.00f, // sil
	-0.40f,// PP — lips together/narrow
	-0.15f,// FF
	0.00f, // TH
	0.00f, // DD
	0.00f, // kk
	0.15f, // CH
	0.25f, // SS — teeth visible, spread
	0.00f, // nn
	0.00f, // RR
	0.50f, // aa — wide open
	0.60f, // E  — spread
	0.50f, // ih — spread
	-0.50f,// oh — rounded
	-0.60f,// ou — pursed
};

// ============================================================================
// Submix proxy
// ============================================================================

void FLipSyncSubmixProxy::OnNewSubmixBuffer(
	const USoundSubmix* OwningSubmix, float* AudioData,
	int32 NumSamples, int32 NumChannels,
	const int32 SampleRate, double AudioClock)
{
	if (UZP_LipSyncComponent* Comp = Owner.Get())
	{
		Comp->ProcessAudioBuffer(AudioData, NumSamples, NumChannels, SampleRate);
	}
}

// ============================================================================
// Lifecycle
// ============================================================================

UZP_LipSyncComponent::UZP_LipSyncComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	VisemesCurrent.SetNumZeroed(ovrLipSyncViseme_Count);
	VisemesGameThread.SetNumZeroed(ovrLipSyncViseme_Count);
}

void UZP_LipSyncComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize OVRLipSync (but don't touch meshes yet — CC presets must stay visible)
	InitOVRLipSync();

	// Register submix buffer listener on main submix
	if (bOVRInitialized)
	{
		if (FAudioDevice* AudioDevice = GetWorld()->GetAudioDeviceRaw())
		{
			USoundSubmix& MainSubmix = AudioDevice->GetMainSubmixObject();
			MainSubmixRef = &MainSubmix;
			SubmixProxy = MakeShared<FLipSyncSubmixProxy, ESPMode::ThreadSafe>(this);
			AudioDevice->RegisterSubmixBufferListener(SubmixProxy.ToSharedRef(), MainSubmix);
			UE_LOG(LogLipSync, Log, TEXT("LipSync: OVR initialized, submix listener registered."));
		}
	}
}

void UZP_LipSyncComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SubmixProxy.IsValid() && MainSubmixRef.IsValid())
	{
		if (FAudioDevice* AudioDevice = GetWorld()->GetAudioDeviceRaw())
		{
			AudioDevice->UnregisterSubmixBufferListener(SubmixProxy.ToSharedRef(), *MainSubmixRef.Get());
		}
	}
	SubmixProxy.Reset();

	// Restore original head visibility
	if (OriginalHeadMesh.IsValid())
	{
		OriginalHeadMesh->SetVisibility(true);
	}
	if (FaceMesh)
	{
		FaceMesh->DestroyComponent();
		FaceMesh = nullptr;
	}

	ShutdownOVRLipSync();

	Super::EndPlay(EndPlayReason);
}

// ============================================================================
// Mesh setup
// ============================================================================

USkeletalMeshComponent* UZP_LipSyncComponent::FindMeshWithJawBone() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	TArray<USkeletalMeshComponent*> Meshes;
	Owner->GetComponents<USkeletalMeshComponent>(Meshes);

	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (!Mesh || !Mesh->GetSkeletalMeshAsset()) continue;
		if (Mesh->GetBoneIndex(FName("jaw")) != INDEX_NONE)
		{
			return Mesh;
		}
	}
	return nullptr;
}

void UZP_LipSyncComponent::CreateFaceMesh(USkeletalMeshComponent* SourceHead)
{
	AActor* Owner = GetOwner();

	FaceMesh = NewObject<UPoseableMeshComponent>(Owner, TEXT("FaceMesh_LipSync"));
	FaceMesh->SetSkinnedAssetAndUpdate(SourceHead->GetSkeletalMeshAsset(), true);

	// Copy materials (preserves CC customization — skin tone, textures, etc.)
	for (int32 i = 0; i < SourceHead->GetNumMaterials(); ++i)
	{
		FaceMesh->SetMaterial(i, SourceHead->GetMaterial(i));
	}

	// Copy morph target weights (preserves CC face shape customization)
	// Both inherit MorphTargetWeights from USkinnedMeshComponent — direct array copy
	if (SourceHead->MorphTargetWeights.Num() > 0
		&& FaceMesh->MorphTargetWeights.Num() == SourceHead->MorphTargetWeights.Num())
	{
		FaceMesh->MorphTargetWeights = SourceHead->MorphTargetWeights;
		UE_LOG(LogLipSync, Log, TEXT("LipSync: Copied %d morph target weights from CC head."),
			FaceMesh->MorphTargetWeights.Num());
	}

	// Attach to same parent with same relative transform
	if (USceneComponent* ParentComp = SourceHead->GetAttachParent())
	{
		FaceMesh->AttachToComponent(ParentComp,
			FAttachmentTransformRules::KeepRelativeTransform,
			SourceHead->GetAttachSocketName());
		FaceMesh->SetRelativeTransform(SourceHead->GetRelativeTransform());
	}
	else
	{
		FaceMesh->SetWorldTransform(SourceHead->GetComponentTransform());
		FaceMesh->AttachToComponent(Owner->GetRootComponent(),
			FAttachmentTransformRules::KeepWorldTransform);
	}

	FaceMesh->RegisterComponent();

	// Hide the original head mesh but keep it ticking for bone updates
	SourceHead->SetVisibility(false);
	SourceHead->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Copy initial pose from original head (still animated via leader pose)
	FaceMesh->CopyPoseFromSkeletalComponent(SourceHead);

	UE_LOG(LogLipSync, Log, TEXT("LipSync: PoseableMeshComponent created with CC state. Original head hidden."));
}

// ============================================================================
// OVRLipSync
// ============================================================================

void UZP_LipSyncComponent::InitOVRLipSync()
{
	FString DllDir = FPaths::Combine(FPaths::ProjectPluginsDir(),
		TEXT("OVRLipSync"), TEXT("ThirdParty"), TEXT("Lib"),
		FPlatformProcess::GetBinariesSubdirectory());

	auto rc = ovrLipSync_InitializeEx(48000, 4096, TCHAR_TO_ANSI(*DllDir));
	if (rc != ovrLipSyncSuccess)
	{
		UE_LOG(LogLipSync, Error, TEXT("LipSync: ovrLipSync_InitializeEx FAILED: %d (DllDir: %s)"),
			rc, *DllDir);
		return;
	}

	rc = ovrLipSync_CreateContextEx(&OVRContext, ovrLipSyncContextProvider_Enhanced, 48000, true);
	if (rc != ovrLipSyncSuccess)
	{
		UE_LOG(LogLipSync, Error, TEXT("LipSync: ovrLipSync_CreateContextEx FAILED: %d"), rc);
		return;
	}

	bOVRInitialized = true;
	UE_LOG(LogLipSync, Log, TEXT("LipSync: OVRLipSync initialized. Context=%u"), OVRContext);
}

void UZP_LipSyncComponent::ShutdownOVRLipSync()
{
	if (bOVRInitialized)
	{
		ovrLipSync_DestroyContext(OVRContext);
		OVRContext = 0;
		bOVRInitialized = false;
	}
}

// Called on AUDIO RENDER THREAD via proxy
void UZP_LipSyncComponent::ProcessAudioBuffer(
	float* AudioData, int32 NumSamples, int32 NumChannels, int32 SampleRate)
{
	if (!bOVRInitialized || OVRContext == 0 || NumSamples == 0)
	{
		return;
	}

	ovrLipSyncAudioDataType DataType =
		(NumChannels >= 2) ? ovrLipSyncAudioDataType_F32_Stereo : ovrLipSyncAudioDataType_F32_Mono;

	TArray<float> LocalVisemes;
	LocalVisemes.SetNumZeroed(ovrLipSyncViseme_Count);

	ovrLipSyncFrame Frame = {};
	Frame.visemes = LocalVisemes.GetData();
	Frame.visemesLength = LocalVisemes.Num();

	int32 SamplesPerChannel = NumSamples / FMath::Max(NumChannels, 1);
	auto rc = ovrLipSync_ProcessFrameEx(OVRContext, AudioData, SamplesPerChannel, DataType, &Frame);

	if (rc == ovrLipSyncSuccess)
	{
		FScopeLock Lock(&VisemeLock);
		VisemesCurrent = LocalVisemes;
		LaughterScoreCurrent = Frame.laughterScore;
	}
}

// ============================================================================
// Dialogue widget tracking
// ============================================================================

void UZP_LipSyncComponent::SetDialogueWidget(UUserWidget* Widget)
{
	DialogueWidget = Widget;
	Sound2DPropCache = nullptr;

	if (Widget)
	{
		Sound2DPropCache = CastField<FObjectProperty>(
			Widget->GetClass()->FindPropertyByName(TEXT("Sound2D")));

		// Lazy-create PoseableMeshComponent on first dialogue
		if (!FaceMesh && bOVRInitialized)
		{
			USkeletalMeshComponent* HeadMesh = FindMeshWithJawBone();
			if (HeadMesh)
			{
				OriginalHeadMesh = HeadMesh;
				CreateFaceMesh(HeadMesh);
			}
		}

		if (FaceMesh)
		{
			SetComponentTickEnabled(true);
			UE_LOG(LogLipSync, Log, TEXT("LipSync: Tick ENABLED — dialogue started."));
		}
	}
}

void UZP_LipSyncComponent::ClearDialogueWidget()
{
	DialogueWidget.Reset();
	Sound2DPropCache = nullptr;
	bAudioPlaying = false;

	// Destroy PoseableMeshComponent and restore original CC head
	if (FaceMesh)
	{
		FaceMesh->DestroyComponent();
		FaceMesh = nullptr;
	}
	if (OriginalHeadMesh.IsValid())
	{
		OriginalHeadMesh->SetVisibility(true);
		UE_LOG(LogLipSync, Log, TEXT("LipSync: Original CC head restored."));
	}

	// Reset smoothed values for next dialogue
	CurrentJawOpen = 0.f;
	SmoothedLipUp = 0.f;
	SmoothedWidth = 0.f;

	SetComponentTickEnabled(false);

	{
		FScopeLock Lock(&VisemeLock);
		for (float& V : VisemesCurrent) V = 0.f;
	}
}

// ============================================================================
// Tick + viseme application
// ============================================================================

void UZP_LipSyncComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!FaceMesh)
	{
		SetComponentTickEnabled(false);
		return;
	}

	bAudioPlaying = false;

	// Check if dialogue audio is playing
	if (DialogueWidget.IsValid() && Sound2DPropCache)
	{
		UObject* AudioObj = Sound2DPropCache->GetObjectPropertyValue(
			Sound2DPropCache->ContainerPtrToValuePtr<void>(DialogueWidget.Get()));
		UAudioComponent* PluginAudioComp = Cast<UAudioComponent>(AudioObj);

		if (PluginAudioComp && PluginAudioComp->IsPlaying())
		{
			bAudioPlaying = true;
		}
	}

	// Sync pose from original head mesh (still animated via leader pose, just hidden)
	if (OriginalHeadMesh.IsValid())
	{
		FaceMesh->CopyPoseFromSkeletalComponent(OriginalHeadMesh.Get());
	}

	// Snapshot visemes from audio thread
	{
		FScopeLock Lock(&VisemeLock);
		VisemesGameThread = VisemesCurrent;
	}

	ApplyVisemes(DeltaTime);
}

void UZP_LipSyncComponent::ApplyVisemes(float DeltaTime)
{
	// Compute target values from viseme weights
	float TargetJaw = 0.f;
	float TargetLipUp = 0.f;
	float TargetWidth = 0.f;

	if (bAudioPlaying)
	{
		for (int32 i = 0; i < FMath::Min(VisemesGameThread.Num(), 15); ++i)
		{
			float V = VisemesGameThread[i];
			TargetJaw += V * VisemeToJaw[i];
			TargetLipUp += V * VisemeToLipUp[i];
			TargetWidth += V * VisemeToWidth[i];
		}
		TargetJaw = FMath::Clamp(TargetJaw, 0.f, 1.f);
		TargetLipUp = FMath::Clamp(TargetLipUp, 0.f, 1.f);
		TargetWidth = FMath::Clamp(TargetWidth, -1.f, 1.f);
	}

	// Smooth interpolation
	float JawSpeed = (TargetJaw > CurrentJawOpen) ? 25.f : 8.f;
	CurrentJawOpen = FMath::FInterpTo(CurrentJawOpen, TargetJaw, DeltaTime, JawSpeed);
	if (CurrentJawOpen < 0.005f) CurrentJawOpen = 0.f;

	SmoothedLipUp = FMath::FInterpTo(SmoothedLipUp, TargetLipUp, DeltaTime, 18.f);
	SmoothedWidth = FMath::FInterpTo(SmoothedWidth, TargetWidth, DeltaTime, 15.f);

	// --- Apply bone rotation DELTAS on top of copied pose ---
	// SetBoneRotationByName REPLACES the rotation. We must get the current
	// rotation from CopyPose and compose our delta via quaternion multiply.

	auto ApplyBoneDelta = [this](FName BoneName, const FRotator& DeltaRot)
	{
		FQuat CurrentQ = FaceMesh->GetBoneRotationByName(BoneName, EBoneSpaces::ComponentSpace).Quaternion();
		FQuat DeltaQ = DeltaRot.Quaternion();
		FQuat NewQ = CurrentQ * DeltaQ;
		FaceMesh->SetBoneRotationByName(BoneName, NewQ.Rotator(), EBoneSpaces::ComponentSpace);
	};

	// Jaw — open
	float JawAngleDeg = CurrentJawOpen * 25.f;
	ApplyBoneDelta(FName("jaw"), FRotator(0.f, JawAngleDeg, 0.f));

	// Lower lips — raise for bilabials (PP, FF)
	float LipAngle = SmoothedLipUp * -12.f;
	ApplyBoneDelta(FName("lip_lower_r"), FRotator(0.f, LipAngle, 0.f));
	ApplyBoneDelta(FName("lip_lower_l"), FRotator(0.f, LipAngle, 0.f));

	// Mouth corners — spread/purse
	float WidthAngle = SmoothedWidth * 8.f;
	ApplyBoneDelta(FName("mouth_r"), FRotator(0.f, 0.f, WidthAngle));
	ApplyBoneDelta(FName("mouth_l"), FRotator(0.f, 0.f, -WidthAngle));

	// Log periodically
	static int32 LogCounter = 0;
	if (bAudioPlaying && ++LogCounter % 60 == 1)
	{
		float MaxV = 0.f;
		int32 MaxIdx = 0;
		for (int32 i = 0; i < VisemesGameThread.Num(); ++i)
		{
			if (VisemesGameThread[i] > MaxV) { MaxV = VisemesGameThread[i]; MaxIdx = i; }
		}
		static const TCHAR* VNames[] = {
			TEXT("sil"), TEXT("PP"), TEXT("FF"), TEXT("TH"), TEXT("DD"),
			TEXT("kk"), TEXT("CH"), TEXT("SS"), TEXT("nn"), TEXT("RR"),
			TEXT("aa"), TEXT("E"), TEXT("ih"), TEXT("oh"), TEXT("ou")
		};
		UE_LOG(LogLipSync, Log, TEXT("LipSync: V=%s(%.2f) Jaw=%.2f LipUp=%.2f Width=%.2f"),
			(MaxIdx < 15) ? VNames[MaxIdx] : TEXT("?"), MaxV,
			CurrentJawOpen, SmoothedLipUp, SmoothedWidth);
	}

	// Tick stays active while FaceMesh exists (for continuous pose sync)
	// ClearDialogueWidget() handles disabling tick and destroying FaceMesh
}
