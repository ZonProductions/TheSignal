#include "ZP_ShamblerCorpseProp.h"

#include "Animation/AnimSequence.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"

AZP_ShamblerCorpseProp::AZP_ShamblerCorpseProp()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only ticks in-game when AZP_PlayRate > 0

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Necromorph recipe: authored at 100x, 0.01 scale; +Z lift plants the prone contact plane.
	Body = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetRelativeScale3D(FVector(0.01f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, 14.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PoseSource = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PoseSource"));
	PoseSource->SetupAttachment(Root);
	PoseSource->SetRelativeScale3D(FVector(0.01f));
	PoseSource->SetRelativeLocation(FVector(0.f, 0.f, 14.f));
	PoseSource->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoseSource->SetVisibility(false);
	PoseSource->SetHiddenInGame(true);
	// Hidden mesh that must still evaluate — never OnlyTickPoseWhenRendered (DEAD ENDS 2026-08-04).
	PoseSource->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void AZP_ShamblerCorpseProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPose();
}

void AZP_ShamblerCorpseProp::BeginPlay()
{
	Super::BeginPlay();
	ApplyPose();
	SetActorTickEnabled(AZP_PlayRate > 0.f);
}

void AZP_ShamblerCorpseProp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// GAME-only breathing: the source plays the clip; mirror it onto the visible body.
	if (Body && PoseSource && PoseSource->GetSkeletalMeshAsset())
	{
		Body->CopyPoseFromSkeletalComponent(PoseSource);
	}
}

UAnimSequence* AZP_ShamblerCorpseProp::ResolveClip() const
{
	switch (AZP_Pose)
	{
	case EShamblerPropPose::ProneCrawl:
		// LIFELESS flat prone (dev 2026-08-05: the crawl idle's propped-up torso read as
		// alive) — frame 0 of ProneToStand, same hold as a dormant Playing Dead shambler.
		return LoadObject<UAnimSequence>(nullptr,
			TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_ProneToStand.A_Shambler_ProneToStand"));
	case EShamblerPropPose::Slumped:
		return LoadObject<UAnimSequence>(nullptr,
			TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_SlumpToStand.A_Shambler_SlumpToStand"));
	case EShamblerPropPose::Custom:
	default:
		return AZP_PoseAnim.Get();
	}
}

void AZP_ShamblerCorpseProp::ApplyPose()
{
	if (!Body || !PoseSource) { return; }

	// Lazy asset loads (constructor loading crashed on editor boot — Shambler lesson).
	USkeletalMesh* SM = Cast<USkeletalMesh>(Body->GetSkinnedAsset());
	if (!SM)
	{
		SM = LoadObject<USkeletalMesh>(nullptr,
			TEXT("/Game/Enemies/Shambler/SkeletalMeshes/zombie_monster_slasher_necromorph.zombie_monster_slasher_necromorph"));
		if (!SM) { return; }
		Body->SetSkinnedAssetAndUpdate(SM);
		PoseSource->SetSkeletalMesh(SM);
	}
	if (!PoseSource->GetSkeletalMeshAsset()) { PoseSource->SetSkeletalMesh(SM); }

	FVector RL = Body->GetRelativeLocation();
	RL.Z = AZP_MeshZ;
	Body->SetRelativeLocation(RL);
	PoseSource->SetRelativeLocation(RL);

	UAnimSequence* Clip = ResolveClip();
	if (!Clip) { return; }

	// Evaluate the clip on the HIDDEN source (CPU pose — this works in editor; verified via
	// socket probes), then copy the bone matrices onto the VISIBLE poseable body. The poseable
	// path involves no anim instance at render time, so the editor draws it unconditionally —
	// the necromorph ref pose (microscopic) can never be what's on screen.
	PoseSource->AnimationData.AnimToPlay = Clip;
	PoseSource->AnimationData.SavedPosition = FMath::Clamp(AZP_PoseTime, 0.f, Clip->GetPlayLength());
	PoseSource->AnimationData.SavedPlayRate = FMath::Max(AZP_PlayRate, 0.f);
	PoseSource->AnimationData.bSavedLooping = true;
	PoseSource->AnimationData.bSavedPlaying = (AZP_PlayRate > 0.f);
	PoseSource->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	PoseSource->InitAnim(true);
	PoseSource->SetPosition(PoseSource->AnimationData.SavedPosition, false);
	PoseSource->SetPlayRate(PoseSource->AnimationData.SavedPlayRate);
	PoseSource->TickAnimation(0.f, false);
	PoseSource->RefreshBoneTransforms();

	Body->CopyPoseFromSkeletalComponent(PoseSource);
	Body->RefreshBoneTransforms();
	Body->MarkRenderStateDirty();

	if (UWorld* W = GetWorld(); W && W->IsGameWorld())
	{
		if (AZP_PlayRate > 0.f) { PoseSource->Play(true); }
		else { PoseSource->Stop(); }
	}
}
