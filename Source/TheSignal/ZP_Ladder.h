// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_Ladder
 *
 * Purpose: Climbable ladder actor. Player interacts (E) to mount, then
 *          W/S to climb up/down, Space to dismount.
 *
 * Owner Subsystem: Gameplay
 *
 * Blueprint Extension Points:
 *   - LadderStyle: choose visual style 1-5 from the climbing pack
 *   - LadderHeight: total climbable height in UU
 *   - ClimbSpeed: tune via DataAsset or instance edit
 *   - BottomAttach / TopAttach: scene components marking mount/dismount points
 *
 * Dependencies:
 *   - IZP_Interactable (player interaction)
 *   - AZP_GraceCharacter (SetCurrentInteractable/ClearCurrentInteractable)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_Ladder.generated.h"

class UBoxComponent;
class UArrowComponent;
class UInstancedStaticMeshComponent;

UENUM(BlueprintType)
enum class EZP_LadderStyle : uint8
{
	Style1 = 0 UMETA(DisplayName = "Ladder Style 1"),
	Style2 UMETA(DisplayName = "Ladder Style 2"),
	Style3 UMETA(DisplayName = "Ladder Style 3"),
	Style4 UMETA(DisplayName = "Ladder Style 4"),
	Style5 UMETA(DisplayName = "Ladder Style 5")
};

UCLASS()
class THESIGNAL_API AZP_Ladder : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_Ladder();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Hidden bounds/collision proxy — GraceCharacter reads Bounds for camera perpendicular. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<UStaticMeshComponent> LadderMesh;

	/** Where the player stands when mounting from the bottom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<USceneComponent> BottomAttachPoint;

	/** Where the player stands after climbing to the top. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<USceneComponent> TopExitPoint;

	/** Direction player faces while climbing (arrow points INTO the ladder). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<UArrowComponent> ClimbDirection;

	/** Trigger volume around the ladder for interaction detection. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<UBoxComponent> InteractionVolume;

	// --- Modular visual components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Modular")
	TObjectPtr<UInstancedStaticMeshComponent> FootBarISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Modular")
	TObjectPtr<UStaticMeshComponent> BottomLeftRail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Modular")
	TObjectPtr<UStaticMeshComponent> BottomRightRail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Modular")
	TObjectPtr<UInstancedStaticMeshComponent> MidLeftISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Modular")
	TObjectPtr<UInstancedStaticMeshComponent> MidRightISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Modular")
	TObjectPtr<UStaticMeshComponent> TopLeftCap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder|Modular")
	TObjectPtr<UStaticMeshComponent> TopRightCap;

	// --- Config ---

	/** Visual style (1-5) from the climbing animation pack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Config")
	EZP_LadderStyle LadderStyle = EZP_LadderStyle::Style1;

	/** Total ladder height in UU. TopExitPoint and InteractionVolume auto-adjust. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Config", meta = (ClampMin = "50.0"))
	float LadderHeight = 585.f;

	/** Vertical climb speed in cm/s. ~61.4 = 2x anim-synced speed (2 rungs per 1.533s cycle × 2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Config")
	float ClimbSpeed = 61.4f;

	float GetBottomZ() const;
	float GetTopZ() const;
	FRotator GetClimbFacingRotation() const;
	FVector GetBottomAttachLocation() const;
	FVector GetTopExitLocation() const;

	/** World-space center of the ladder (XY midpoint between rails). */
	FVector GetLadderCenter() const;

	/** World-space surface normal (perpendicular to ladder face, horizontal). */
	FVector GetLadderSurfaceNormal() const;

	// --- IZP_Interactable ---
	FText GetInteractionPrompt_Implementation() override;
	void OnInteract_Implementation(ACharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void BuildLadderAssembly();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Mesh path helpers
	static FString GetMeshPath(EZP_LadderStyle Style, const FString& PieceName);
	static bool StyleHasTopCaps(EZP_LadderStyle Style);
};
