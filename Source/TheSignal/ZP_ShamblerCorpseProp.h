// AZP_ShamblerCorpseProp
//
// Purpose:   Placeable SHAMBLER BODY PROP (dev 2026-08-05) — the visual of a prone/slumped
//            shambler with ZERO AI. Scatter among real PlayingDead/Slumped shamblers.
// Owned by:  Shambler enemy system (visual decoy; no behavior, no health, no save hooks).
// Rendering: a UPoseableMeshComponent draws the body with DIRECTLY-SET bone matrices — the
//            editor never evaluates animations reliably for level actors, so anything that
//            depends on anim eval renders the necromorph's microscopic ref pose (= invisible).
//            A hidden skeletal component evaluates the pose CPU-side; the poseable copies it.
// BP hooks:  AZP preset dropdown + fine-tune knobs, EditAnywhere per placed instance.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_ShamblerCorpseProp.generated.h"

class UPoseableMeshComponent;
class USkeletalMeshComponent;
class UAnimSequence;

/** The prop's pose presets — mirrors the live shambler presets it impersonates. */
UENUM(BlueprintType)
enum class EShamblerPropPose : uint8
{
	ProneCrawl UMETA(DisplayName = "Prone (lifeless flat — matches Playing Dead)"),
	Slumped    UMETA(DisplayName = "Slumped (against wall — matches Slumped)"),
	Custom     UMETA(DisplayName = "Custom (uses AZP_PoseAnim)"),
};

UCLASS()
class THESIGNAL_API AZP_ShamblerCorpseProp : public AActor
{
	GENERATED_BODY()

public:
	AZP_ShamblerCorpseProp();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** The VISIBLE body — poseable, bone matrices set directly (no anim eval anywhere). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AZP|Preset")
	TObjectPtr<UPoseableMeshComponent> Body;

	/** Hidden pose source — evaluates the chosen clip CPU-side; never rendered. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AZP|Preset")
	TObjectPtr<USkeletalMeshComponent> PoseSource;

	/** THE POSE PRESET — Prone matches the live Crawler/PlayingDead shamblers; Slumped matches
	 *  the Slumped preset; Custom uses AZP_PoseAnim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AZP|Preset")
	EShamblerPropPose AZP_Pose = EShamblerPropPose::ProneCrawl;

	/** Seconds into the pose clip for the held frame — vary per placed body so a pile of them
	 *  doesn't read as copies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AZP|Preset")
	float AZP_PoseTime = 0.f;

	/** Play rate (GAME only). 0 = frozen corpse; a whisper (0.05) = the barely-breathing tell
	 *  the real PlayingDead shamblers show. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AZP|Preset")
	float AZP_PlayRate = 0.f;

	/** Mesh Z lift (UU) so the pose's contact plane sits on the floor. Prone default 14. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AZP|Preset")
	float AZP_MeshZ = 14.f;

	/** Custom pose clip (only used when AZP_Pose = Custom). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AZP|Preset")
	TObjectPtr<UAnimSequence> AZP_PoseAnim;

private:
	void ApplyPose();
	UAnimSequence* ResolveClip() const;
};
