// Copyright The Signal. All Rights Reserved.

#include "ZP_ShamblerBehaviorComponent.h"
#include "ZP_EnemyAudioComponent.h"
#include "ZP_HealthComponent.h"
#include "Components/AudioComponent.h"
#include "ZP_MeleeDamageType.h"
#include "ZP_SFXStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// Prone/rise ground-contact bone set — extremities AND the load-bearing core (round-17: the
// army-crawl bears on FOREARMS/chest/pelvis; clamping only hands/feet sank everything below
// that plane). Shared by the runtime clamp and the WITH_EDITOR preview grounding so the placed
// preview and the playing body rest on the SAME plane (round-18: they disagreed by 12uu).
static const FName GShamblerContactBones[] = {
	FName(TEXT("mixamorig_LeftHand_011")),    FName(TEXT("mixamorig_RightHand_019")),
	FName(TEXT("mixamorig_LeftFoot_026")),    FName(TEXT("mixamorig_RightFoot_030")),
	FName(TEXT("mixamorig_LeftToeBase_027")), FName(TEXT("mixamorig_RightToeBase_031")),
	FName(TEXT("mixamorig_LeftLeg_025")),     FName(TEXT("mixamorig_RightLeg_029")),
	FName(TEXT("mixamorig_LeftForeArm_010")), FName(TEXT("mixamorig_RightForeArm_018")),
	FName(TEXT("mixamorig_Hips_01")),         FName(TEXT("mixamorig_Spine_02")),
	FName(TEXT("mixamorig_Spine2_04")),       FName(TEXT("mixamorig_Head_06")) };

UZP_ShamblerBehaviorComponent::UZP_ShamblerBehaviorComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // per-frame combat facing (state logic stays on the eval timer)

	// NOTE: anim defaults are loaded lazily in BeginPlay (LoadAnimDefaults), NOT here.
	// ConstructorHelpers::FObjectFinder loading /Game assets during CDO construction crashed
	// on editor load (EXCEPTION_ACCESS_VIOLATION in FindOrLoadObject). Lazy LoadObject mirrors
	// the sound-loading pattern below and preserves per-instance BP overrides.
}

void UZP_ShamblerBehaviorComponent::LoadAnimDefaults()
{
	// Only fill slots the Blueprint hasn't overridden (serialized overrides are present before BeginPlay).
	// NOTE: these clips must stay free of orphan skeleton curves. The retargeted Hit_Front/Hit_Back shipped
	// with stale GASP distance-matching curves (DistanceToApex/DistanceCurve/blendOrient1) that the necromorph
	// skeleton has no name mapping for; on editor load that tripped UAnimSequence::PostLoad's curve-name
	// verification and hard-crashed (EXCEPTION_ACCESS_VIOLATION) on PIE / -game / level open. The curves were
	// stripped 2026-06-29 (Shambler plays SingleNode clips, never used them). See checkpoint
	// 2026-06-29_shambler_anim_curve_postload_crash. If you re-import these clips, strip stray curves again.
	auto Fill = [](TObjectPtr<UAnimSequence>& Slot, const TCHAR* P)
	{
		if (!Slot) { Slot = LoadObject<UAnimSequence>(nullptr, P); }
	};
	Fill(AZP_WalkAnim,       TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Walk.A_Shambler_Walk"));
	Fill(AZP_IdleAnim,       TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Idle.A_Shambler_Idle"));
	Fill(AZP_AttackLAnim,    TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_L.A_Shambler_Attack_L"));
	Fill(AZP_AttackRAnim,    TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_R.A_Shambler_Attack_R"));
	// ScreamPinned = the scream with the LOWER BODY frozen at the idle stance — the raw clip
	// lifted the feet a few inches off the ground (dev 2026-07-03). Baked by
	// bake_shambler_scream_run_fixes.py; plain A_Shambler_Scream still exists.
	Fill(AZP_ScreamAnim,     TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_ScreamPinned.A_Shambler_ScreamPinned"));
	Fill(AZP_DeathFrontAnim, TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Death_Front.A_Shambler_Death_Front"));
	Fill(AZP_DeathBackAnim,  TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Death_Back.A_Shambler_Death_Back"));
	Fill(AZP_HitFrontAnim,   TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Hit_Front.A_Shambler_Hit_Front"));
	Fill(AZP_HitBackAnim,    TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Hit_Back.A_Shambler_Hit_Back"));
	// RunStiffArmsHead = the NAAT run body with arms AND head/neck FROZEN at the LL_Idle pose
	// (dev 2026-07-03: no arm pump, and "the head swinging around" looked goofy). Baked by
	// bake_shambler_run_arms.py + bake_shambler_scream_run_fixes.py; A_Shambler_RunStiffArms
	// (head animated) and plain A_Shambler_Run both still exist.
	Fill(AZP_RunAnim,        TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_RunStiffArmsHead.A_Shambler_RunStiffArmsHead"));
	// NAAT grab pair, attacker side (retargeted 2026-07-02, curve-audited — see retarget_grab_anims.py).
	Fill(AZP_GrabEntryAnim,    TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabEntry.A_Shambler_GrabEntry"));
	Fill(AZP_GrabMunchAnim,    TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabMunch.A_Shambler_GrabMunch"));
	Fill(AZP_GrabWrestleAnim,  TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabWrestle.A_Shambler_GrabWrestle"));
	Fill(AZP_GrabKickedAnim,   TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabKicked.A_Shambler_GrabKicked"));
	Fill(AZP_GrabPushedAnim,   TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabPushed.A_Shambler_GrabPushed"));
	Fill(AZP_GrabTakedownAnim, TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabTakedown.A_Shambler_GrabTakedown"));
	// Alert-turn stepping clips (SLS Turn_* retargeted via RTG_CC_to_Shambler 2026-07-14,
	// curve-stripped at retarget — see retarget_shambler_turns2.py). Pelvis rotation deliberately
	// does NOT transfer: the C++ rotates the ACTOR while the clip provides the stepping feet.
	Fill(AZP_TurnL90Anim,  TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Turn_L90.A_Shambler_Turn_L90"));
	Fill(AZP_TurnR90Anim,  TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Turn_R90.A_Shambler_Turn_R90"));
	Fill(AZP_TurnL180Anim, TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Turn_L180.A_Shambler_Turn_L180"));
	Fill(AZP_TurnR180Anim, TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Turn_R180.A_Shambler_Turn_R180"));
	// Crawl stance set (NAAT crawl clips retargeted 2026-08-05 via retarget_shambler_crawl.py —
	// curve-stripped + additive/root-lock cleared + size-verified at retarget time).
	Fill(AZP_CrawlIdleAnim,     TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_CrawlIdle.A_Shambler_CrawlIdle"));
	Fill(AZP_CrawlWalkAnim,     TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_CrawlWalk.A_Shambler_CrawlWalk"));
	Fill(AZP_CrawlRunAnim,      TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_CrawlRun.A_Shambler_CrawlRun"));
	Fill(AZP_AttackCrawlAnim,   TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_Crawl.A_Shambler_Attack_Crawl"));
	Fill(AZP_AttackStandLHAnim, TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_StandLH.A_Shambler_Attack_StandLH"));
	Fill(AZP_AttackStandBHAnim, TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_StandBH.A_Shambler_Attack_StandBH"));
	Fill(AZP_SlumpToStandAnim,  TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_SlumpToStand.A_Shambler_SlumpToStand"));
	Fill(AZP_PlayDeadPoseAnim,  TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_ProneToStand.A_Shambler_ProneToStand"));
}

void UZP_ShamblerBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<ACharacter>(GetOwner());
	if (!Owner) { return; }

	// The body must NEVER affect nav generation (the 2026-07-13 Oozeling ctor lesson, never
	// applied here — moot under STATIC nav, live since the 2026-08-05 DYNAMIC flip): a
	// nav-relevant pawn carves a moving hole under ITSELF, the tiles rebuild every step, and
	// its own path invalidates in a stall-restart loop (the round-19 one-sided-crawl trigger).
	Owner->SetCanAffectNavigationGeneration(false);
	if (UCapsuleComponent* NavCap = Owner->GetCapsuleComponent()) { NavCap->SetCanEverAffectNavigation(false); }
	if (USkeletalMeshComponent* NavMesh = Owner->GetMesh()) { NavMesh->SetCanEverAffectNavigation(false); }

	LoadAnimDefaults(); // fill any anim slots the BP didn't override (was a crashing ctor load)

	AICon = Cast<AAIController>(Owner->GetController());
	if (AICon)
	{
		// Chain the next wander leg the INSTANT the current move completes (arrived/aborted/failed).
		// Without this, the body sits idle in the gap between MoveTo completion and the next 0.25s
		// Evaluate — which is exactly the visible "pause" the dev still saw after removing pause logic.
		AICon->ReceiveMoveCompleted.AddDynamic(this, &UZP_ShamblerBehaviorComponent::OnMoveCompleted);
	}

	// Spatial voice (lurk/alert/attack) — reuse the enemy audio component, point it at the zombie SFX.
	// Carry/attenuation is C++-owned (UZP_SFXStatics Far profile, ~120 m) — no SA asset to assign.
	Audio = NewObject<UZP_EnemyAudioComponent>(Owner, TEXT("ShamblerAudio"));
	if (Audio)
	{
		Audio->RegisterComponent();
		Audio->AZP_AlertSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_ALERT.SFX_ZOMBIE_ALERT"));
		// Hit vocal for the flinch. Overrides the component's Crawler default (wrong creature); stays
		// null (= silent flinch) until the dev imports a zombie pain grunt to this exact path.
		Audio->AZP_HitSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_HIT.SFX_ZOMBIE_HIT"));
		if (!Audio->AZP_HitSound)
		{
			UE_LOG(LogTemp, Log, TEXT("[Shambler] hit vocal missing — import /Game/Audio/Shambler/SFX_ZOMBIE_HIT to voice the flinch."));
		}
		// The imported lurk growl is SFX_ZOMBIE_LURK (there is no LURK1/LURK2). Import SFX_ZOMBIE_LURK2
		// and restore the two-growl RandBool pick in UpdateLurk if you want variety.
		Audio->AZP_LurkingLoop = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_LURK.SFX_ZOMBIE_LURK"));
		// Two strikes, alternated with the swing side (L=Attack1, R=Attack2). Attack2 is optional —
		// until SFX_ZOMBIE_ATTACK2 is imported, both swings fall back to Attack1.
		Audio->AZP_AttackSounds.Empty();
		if (USoundBase* Atk1 = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_ATTACK.SFX_ZOMBIE_ATTACK")))
		{
			Audio->AZP_AttackSounds.Add(Atk1);
		}
		if (USoundBase* Atk2 = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_ATTACK2.SFX_ZOMBIE_ATTACK2")))
		{
			Audio->AZP_AttackSounds.Add(Atk2);
		}
	}

	// Idle groan — fires exactly when the wander idle ANIMATION starts (see Evaluate WALK→IDLE).
	if (!AZP_IdleSound)
	{
		AZP_IdleSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_IDLE.SFX_ZOMBIE_IDLE"));
	}

	// Footstep one-shots — 16 steps sliced from the dev's SFX_SHAMBLER_FOOTSTEPS reel
	// (slice_footstep_reel.py + import_shambler_footsteps.py). Distance-driven in Tick.
	if (AZP_FootstepSounds.Num() == 0)
	{
		for (int32 i = 1; i <= 16; ++i)
		{
			const FString P = FString::Printf(
				TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_%02d.SFX_SHAMBLER_FOOTSTEP_%02d"), i, i);
			if (USoundBase* Step = LoadObject<USoundBase>(nullptr, *P))
			{
				AZP_FootstepSounds.Add(Step);
			}
		}
		if (AZP_FootstepSounds.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Shambler] no footstep sounds at /Game/Audio/Shambler/Footsteps/ — feet are silent"));
		}
	}

	// Grapple snarl loop — the wave asset is set to Looping; started at latch, hard-cut on release.
	if (!AZP_GrabLoopSound)
	{
		AZP_GrabLoopSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_SHAMBLER_GRAB.SFX_SHAMBLER_GRAB"));
	}
	// Latch alert sting — one-shot at the moment the grab lands (dev 2026-07-03).
	if (!AZP_GrabAlertSound)
	{
		AZP_GrabAlertSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_GRAB_ALERT.SFX_GRAB_ALERT"));
	}

	// Health — create one if the BP doesn't already have it, so the Shambler can take damage + die.
	Health = Owner->FindComponentByClass<UZP_HealthComponent>();
	if (!Health)
	{
		Health = NewObject<UZP_HealthComponent>(Owner, TEXT("ShamblerHealth"));
		Health->AZP_MaxHealth = AZP_MaxHealth;
		Health->RegisterComponent();
	}
	Health->AZP_MaxHealth = AZP_MaxHealth;
	Health->ResetHealth();
	Health->OnDied.AddDynamic(this, &UZP_ShamblerBehaviorComponent::OnOwnerDied);

	// Take bullets: the weapon fires ApplyPointDamage (carries the hit bone) -> head vs body damage.
	Owner->OnTakePointDamage.AddDynamic(this, &UZP_ShamblerBehaviorComponent::OnPointDamage);
	Owner->OnTakeAnyDamage.AddDynamic(this, &UZP_ShamblerBehaviorComponent::OnAnyDamage); // PROBE

	if (USkeletalMeshComponent* M0 = Owner->GetMesh())
	{
		// The EDITOR PREVIEW grounds the mesh visually (preview-only cosmetic write) — restore
		// the AUTHORED base Z from the archetype before capturing, or the preview offset would
		// poison every runtime state offset. Mesh Z is never hand-authored per instance in this
		// project (all Z tuning is knob-driven).
		if (USceneComponent* MeshArch = Cast<USceneComponent>(M0->GetArchetype()))
		{
			FVector RL0 = M0->GetRelativeLocation();
			RL0.Z = MeshArch->GetRelativeLocation().Z;
			M0->SetRelativeLocation(RL0);
		}
		MeshBaseRelZ = M0->GetRelativeLocation().Z;
		MeshBaseRelXY = FVector2D(M0->GetRelativeLocation().X, M0->GetRelativeLocation().Y);
		M0->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); // capsule takes the bullet instead

	}

	// Tether: every wander pick is anchored around the spawn point so the Shambler doesn't
	// drift the length of the building over time. AZP_WanderRadius now reads as a true leash radius.
	SpawnLocation = Owner->GetActorLocation();
	// Shootable exactly like the Crawler: the CAPSULE blocks the hitscan's Visibility trace (the per-bone
	// mesh trace doesn't register on this rig). The Pawn profile ignores Visibility, so force it here.
	if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent())
	{
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] shootable via capsule: VisResp=%d"),
			(int32)Cap->GetCollisionResponseToChannel(ECC_Visibility));
	}

	// CRAWLER CAPSULE (dev 2026-08-05: "it swims... should cling to the ground"): shrink the
	// standing capsule so the body rides at floor height, with mesh-base + actor-Z compensation
	// (ApplyCrawlCapsule). Standing values are captured FIRST — the one-way stand-up restores
	// them, and revive re-shrinks. Also clamps the CMC yaw rate (wild-rotation fix).
	if (UCapsuleComponent* CapBP = Owner->GetCapsuleComponent())
	{
		StandingCapsuleHH = CapBP->GetUnscaledCapsuleHalfHeight();
	}
	if (UCharacterMovementComponent* CMBP = Owner->GetCharacterMovement())
	{
		StandingRotationRateYaw = CMBP->RotationRate.Yaw;
	}
	bCrawlStoodUp = false;
	// Legacy bAZP_Crawling (pre-preset) maps onto the preset dropdown.
	if (bAZP_Crawling && AZP_Preset == EShamblerPreset::Standing)
	{
		AZP_Preset = EShamblerPreset::Crawler;
	}
	if (IsPronePreset())
	{
		ApplyCrawlCapsule(true);
	}

	SetState(EShamblerState::Wander);

	GetWorld()->GetTimerManager().SetTimer(EvalTimer, this, &UZP_ShamblerBehaviorComponent::Evaluate, AZP_EvalInterval, true);
	GetWorld()->GetTimerManager().SetTimer(ProbeTimer, this, &UZP_ShamblerBehaviorComponent::ProbeShootable, 2.0f, true); // PROBE
}

void UZP_ShamblerBehaviorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(EvalTimer);
		W->GetTimerManager().ClearTimer(AttackHitTimer);
		W->GetTimerManager().ClearTimer(WindupReleaseTimer);
		W->GetTimerManager().ClearTimer(HitchRestoreTimer);
		W->GetTimerManager().ClearTimer(ProbeTimer);
		W->GetTimerManager().ClearTimer(IdleLockTimer);
		W->GetTimerManager().ClearTimer(GrabIdleFillTimer);
		W->GetTimerManager().ClearTimer(VictimUpWaitTimer);
		W->GetTimerManager().ClearTimer(CollisionRestoreTimer);
		W->GetTimerManager().ClearTimer(GrabSlideProbeTimer);
		W->GetTimerManager().ClearTimer(LatchWindowProbeTimer);
		W->GetTimerManager().ClearTimer(EscapePushbackDelayTimer);
		W->GetTimerManager().ClearTimer(CrawlStandUpTimer);
		W->GetTimerManager().ClearTimer(CrawlClampReleaseTimer);
	}
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void UZP_ShamblerBehaviorComponent::OnRegister()
{
	Super::OnRegister();
	// EDITOR AUTHORING PREVIEW (dev 2026-08-05: "I can't author... without seeing it in editor").
	// The necromorph ref pose renders microscopic, so placed shamblers showed NOTHING in the
	// viewport. Evaluate a real pose in-editor: prone/slumped presets preview their dormant
	// clip (single-node, frame 0); Standing evaluates the ABP idle. Re-runs when the dev edits
	// the preset dropdown (components re-register on property edits). PIE is untouched —
	// BeginPlay -> SetState(Wander) -> EnsureLocomotion restores the ABP mode at runtime.
	UWorld* W = GetWorld();
	if (!W || (W->WorldType != EWorldType::Editor && W->WorldType != EWorldType::EditorPreview))
	{
		return;
	}
	ACharacter* Ch = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* M = Ch ? Ch->GetMesh() : nullptr;
	if (!M) { return; }
	M->SetUpdateAnimationInEditor(true);

	const bool bLegacyCrawl = bAZP_Crawling && AZP_Preset == EShamblerPreset::Standing;
	const EShamblerPreset Preview = bLegacyCrawl ? EShamblerPreset::Crawler : AZP_Preset;
	UAnimSequence* PreviewClip = nullptr;
	if (Preview == EShamblerPreset::PlayingDead)
	{
		// Lifeless flat pose — matches the dormant in-game hold.
		PreviewClip = AZP_PlayDeadPoseAnim ? AZP_PlayDeadPoseAnim.Get() : LoadObject<UAnimSequence>(nullptr,
			TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_ProneToStand.A_Shambler_ProneToStand"));
	}
	else if (Preview == EShamblerPreset::Crawler)
	{
		PreviewClip = AZP_CrawlIdleAnim ? AZP_CrawlIdleAnim.Get() : LoadObject<UAnimSequence>(nullptr,
			TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_CrawlIdle.A_Shambler_CrawlIdle"));
	}
	else if (Preview == EShamblerPreset::Slumped)
	{
		PreviewClip = AZP_SlumpToStandAnim ? AZP_SlumpToStandAnim.Get() : LoadObject<UAnimSequence>(nullptr,
			TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_SlumpToStand.A_Shambler_SlumpToStand"));
	}
	if (PreviewClip)
	{
		if (M->GetAnimationMode() != EAnimationMode::AnimationSingleNode
			|| M->AnimationData.AnimToPlay != PreviewClip)
		{
			// Serialized play data, not SetAnimation() — the editor instance initializes from
			// AnimationData (a runtime-only SetAnimation call leaves the preview empty).
			M->AnimationData.AnimToPlay = PreviewClip;
			M->AnimationData.SavedPosition = 0.f;
			M->AnimationData.SavedPlayRate = 0.f;
			M->AnimationData.bSavedLooping = true;
			M->AnimationData.bSavedPlaying = false;
			M->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			M->InitAnim(true);
		}
	}
	else if (M->GetAnimationMode() != EAnimationMode::AnimationBlueprint && M->GetAnimClass())
	{
		M->SetAnimationMode(EAnimationMode::AnimationBlueprint); // Standing: back to the ABP idle
	}
	// PUSH the pose to the RENDERER — the editor doesn't tick anims, so without this the GPU
	// keeps REF-POSE matrices and the necromorph renders microscopic (invisible).
	M->TickAnimation(0.f, false);
	M->RefreshBoneTransforms();
	M->MarkRenderStateDirty();

	// GROUND THE PREVIEW (dev round-10: "the preview should touch the bottom of the capsule,
	// not float") — SAME contact-bone set and SAME AZP_CrawlBoneLift as the runtime ground
	// clamp, so the placed preview and the playing body rest on an identical plane (round-18:
	// all-bones-min@+2 here vs contact-min@-10 in game = editor floats, game buries — one dial
	// now drives both). Cosmetic, editor-only: BeginPlay restores the authored Z.
	if (PreviewClip && Ch->GetCapsuleComponent() && M->GetNumBones() > 0)
	{
		float MinZ = TNumericLimits<float>::Max();
		for (const FName& B : GShamblerContactBones)
		{
			if (M->GetBoneIndex(B) != INDEX_NONE)
			{
				MinZ = FMath::Min(MinZ, (float)M->GetBoneLocation(B).Z);
			}
		}
		if (MinZ < TNumericLimits<float>::Max())
		{
			const float CapBottom = Ch->GetActorLocation().Z - Ch->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const float Err = MinZ - (CapBottom + AZP_CrawlBoneLift);
			FVector RL = M->GetRelativeLocation();
			RL.Z -= Err;
			M->SetRelativeLocation(RL);
		}
	}
}
#endif

void UZP_ShamblerBehaviorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Owner) { return; }

	// perf 2026-08-04: distant wandering shamblers tick at interval, and wandering bodies stop
	// evaluating anim while off-screen (each moving mesh invalidates VSM shadow pages of every
	// light containing it). Combat/death states always run full-rate — swing chaining reads
	// anim positions and must never stall. The dev-built BP wander is untouched.
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		const float DistToPlayer = PlayerPawn
			? FVector::Dist(Owner->GetActorLocation(), PlayerPawn->GetActorLocation()) : 1.e9f;
		const bool bFarIdle = State == EShamblerState::Wander && !bDead
			&& DistToPlayer > AZP_FarWanderDistance;
		SetComponentTickInterval(bFarIdle ? AZP_FarWanderTickInterval : 0.f);
		if (bAZP_AnimOnlyWhenRendered)
		{
			if (USkeletalMeshComponent* GateM = Owner->GetMesh())
			{
				// Prone bodies NEVER skip pose eval (round-17): the ground clamp reads
				// GetBoneLocation every tick — with the pose frozen off-screen it clamps against
				// stale/ref-pose bone Z and the body surfaces buried until FInterpTo recovers.
				// A dormant crawler's held pose is a frozen frame anyway — eval is cheap.
				GateM->VisibilityBasedAnimTickOption =
					(State == EShamblerState::Wander && !bDead && !IsCrawlingNow())
					? EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered
					: EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			}
		}
	}

	if (bDead)
	{
		// Lower the mesh over the death clip to fake the missing root-motion drop -> corpse grounds.
		if (bDropping)
		{
			if (USkeletalMeshComponent* M = Owner->GetMesh())
			{
				const float DropDur = FMath::Max(DeathAnimLen - AZP_DeathDropLead, 0.1f);
				float Alpha = FMath::Clamp((float)(GetWorld()->GetTimeSeconds() - DeathStartTime) / DropDur, 0.f, 1.f);
				Alpha = 1.f - (1.f - Alpha) * (1.f - Alpha); // ease-out: drops quick, settles
				FVector RL = M->GetRelativeLocation();
				RL.Z = MeshBaseRelZ - AZP_DeathDropZ * Alpha;
				M->SetRelativeLocation(RL);
				if (Alpha >= 1.f) { bDropping = false; }
			}
		}
		return;
	}
	// DISTANCE-BASED FOOTSTEPS: one step SFX per AZP_FootstepStride units of 2D travel — the only
	// scheme that cadence-matches EVERY gait (wander/chase/run-burst/stumble) with zero per-clip
	// work: velocity drives it, so faster gaits step proportionally faster and a stationary or
	// grabbed body is silent. Accumulator carries across short pauses (resumes mid-stride).
	{
		const float Speed2D = Owner->GetVelocity().Size2D();
		if (Speed2D > 20.f)
		{
			StepDistanceAccum += Speed2D * DeltaTime;
			if (StepDistanceAccum >= FMath::Max(20.f, AZP_FootstepStride))
			{
				StepDistanceAccum = 0.f;
				PlayFootstep();
			}
		}
	}

	// [FloatProbe] (dev 2026-07-14: "always seems roughly a small amount floating" while moving/
	// attacking). INSTRUMENTATION ONLY — measures, fixes nothing. Throttled; see bAZP_FloatProbe.
	if (bAZP_FloatProbe && !bDead && Owner)
	{
		const double NowFP = GetWorld()->GetTimeSeconds();
		if ((NowFP - LastFloatProbeTime) >= AZP_FloatProbeInterval)
		{
			LastFloatProbeTime = NowFP;
			RunFloatProbe(NowFP);
		}
	}

	// Hit-jolt spring-back: the punch set in OnPointDamage decays to rest here every frame. XY only —
	// the state logic owns the mesh Z (idle/scream offsets). Every landed hit visibly shoves the body.
	if (!MeshPunch.IsNearlyZero(0.05f))
	{
		MeshPunch = FMath::Vector2DInterpTo(MeshPunch, FVector2D::ZeroVector, DeltaTime, AZP_HitPunchRecovery);
		if (MeshPunch.IsNearlyZero(0.05f)) { MeshPunch = FVector2D::ZeroVector; }
		if (USkeletalMeshComponent* M = Owner->GetMesh())
		{
			FVector RL = M->GetRelativeLocation();
			RL.X = MeshBaseRelXY.X + MeshPunch.X;
			RL.Y = MeshBaseRelXY.Y + MeshPunch.Y;
			M->SetRelativeLocation(RL);
		}
	}

	// Latch lunge-in: ease-out slide into AZP_GrabPairDistance (replaces the one-frame teleport when
	// grabbing from max reach). Pair collision is mutually ignored and movement is MOVE_None, so
	// a plain unswept lerp is safe; a grab that ends mid-lunge just drops the slide.
	if (bGrabSnapIn)
	{
		if (State != EShamblerState::Grab || bDead)
		{
			bGrabSnapIn = false;
		}
		else
		{
			const float Dur = FMath::Max(0.01f, AZP_GrabSnapInDuration);
			const float A = FMath::Clamp((float)((GetWorld()->GetTimeSeconds() - GrabSnapStart) / Dur), 0.f, 1.f);
			const float E = 1.f - FMath::Square(1.f - A); // ease-out: lunges hard, lands soft
			Owner->SetActorLocation(FMath::Lerp(GrabSnapFrom, GrabSnapTo, E), /*bSweep*/false);
			if (A >= 1.f) { bGrabSnapIn = false; }
		}
	}

	// Escape pushback: short eased slide-to-stop (replaces the old LaunchCharacter glide — see
	// OnVictimGrabPhase EscapeKick/Push). Absolute lerp between the captured from/to points; the
	// victim is still collision-ignored, and the sweep lets walls stop it naturally. KNOWN LIMIT:
	// the raw sweep has no step-up/slide-along — a stair nosing or threshold lip behind the body
	// dead-stops the shove early (log shows the truncated end distance). Accepted; if a "kick did
	// nothing on stairs" report lands, this is why — not the wander or depenetration.
	if (bEscapePushback)
	{
		const float Dur = FMath::Max(0.05f, AZP_EscapePushbackDuration);
		const float Alpha = FMath::Clamp((float)(GetWorld()->GetTimeSeconds() - EscapePushbackStart) / Dur, 0.f, 1.f);
		const float Eased = 1.f - FMath::Square(1.f - Alpha); // ease-out: hits hard, settles
		FVector NewLoc = FMath::Lerp(EscapePushbackFrom, EscapePushbackTo, Eased);
		NewLoc.Z = Owner->GetActorLocation().Z; // Z stays the CMC's (floor snap / steps)
		Owner->SetActorLocation(NewLoc, /*bSweep*/true);
		if (Alpha >= 1.f)
		{
			bEscapePushback = false;
			UE_LOG(LogTemp, Warning, TEXT("[GrabProbe] pushback END distToPlayer=%.0f vel=%.0f"),
				Target ? FVector::Dist2D(Owner->GetActorLocation(), Target->GetActorLocation()) : -1.f,
				Owner->GetVelocity().Size());
		}
	}

	// CRAWL STANCE — a crawler keeps a crawl loop seated on the DefaultSlot at ALL times (the
	// BS_Shambler BlendSpace underneath is standing-only; ANY slot gap shows a standing pose).
	// The maintainer owns every gap the standing regimes below would otherwise handle, wander
	// AND combat, so those regimes are skipped for crawlers.
	if (!bDead)
	{
		// Ground clamp runs through the WHOLE slump-rise window (rise + settle) AND while a
		// Slumped preset sits dormant — the pose plants itself wherever it is in its curve.
		if (bCrawlStandWindow || IsDormantSlumped())
		{
			GroundClampCrawlRise(DeltaTime);
		}
		if (IsCrawlingNow())
		{
			MaintainCrawlLocomotion(DeltaTime);
		}
		else if (CrawlLoopMontage.IsValid() && Owner && Owner->GetMesh())
		{
			// No longer prone — release any leftover crawl loop so the BlendSpace shows.
			if (UAnimInstance* StAI = Owner->GetMesh()->GetAnimInstance())
			{
				if (StAI->Montage_IsPlaying(CrawlLoopMontage.Get())) { StopSlotLoop(); }
			}
			CrawlLoopMontage = nullptr;
		}
		// SLUMPED dormant pose: SlumpToStand frame 0 held on the slot (near-zero rate loop),
		// re-seated if anything displaces it. Cleared by StartSlumpRise.
		if (IsDormantSlumped() && !bStandingUpNow && AZP_SlumpToStandAnim && Owner && Owner->GetMesh())
		{
			if (UAnimInstance* SlAI = Owner->GetMesh()->GetAnimInstance())
			{
				if (!SlumpHoldMontage.IsValid() || !SlAI->Montage_IsPlaying(SlumpHoldMontage.Get()))
				{
					SlumpHoldMontage = SlAI->PlaySlotAnimationAsDynamicMontage(
						AZP_SlumpToStandAnim, FName(TEXT("DefaultSlot")), 0.25f, 0.25f, 0.001f,
						/*LoopCount=*/INT32_MAX);
				}
			}
		}
	}

	// SLOT GAP COVER — a stationary, alive, non-grabbing body must never show naked
	// BS_Shambler@speed-0 (wander-pause marching, post-takedown swim, post-flinch T-freeze).
	// TWO regimes (dev 2026-07-03: "in combat, idle shouldn't happen at all"):
	//  - WANDER: vacuum-fill the empty slot with the docile idle loop (expected, dev-approved).
	//  - COMBAT (Scream/Chase/Attack): the idle is BANNED. Instead, HOLD the final pose of
	//    whatever combat clip is on the slot: pause the montage just before its blend-out.
	//    A swing ends -> the body holds the end-of-swing stance for the <=0.25s until the next
	//    swing/decision crossfades over it (same-slot montages auto-interrupt with blend-in).
	//    Movement resumes -> the hold is released and locomotion takes back over. No new
	//    animation content anywhere — the held pose IS the clip the dev already authored/uses.
	if (!bDead && State != EShamblerState::Grab && !IsCrawlingNow())
	{
		if (USkeletalMeshComponent* M = Owner->GetMesh())
		{
			if (UAnimInstance* AI = M->GetAnimInstance())
			{
				const bool bStationary = Owner->GetVelocity().SizeSquared2D() < 400.f; // < 20 uu/s
				const bool bCombat = (State == EShamblerState::Scream)
					|| (State == EShamblerState::Chase)
					|| (State == EShamblerState::Attack);

				if (!bCombat)
				{
					// WANDER: original vacuum-fill, unchanged.
					if (AZP_IdleAnim)
					{
						if (bStationary && !AI->Montage_IsActive(nullptr))
						{
							SlotVacuumFill = PlaySlotLoop(AZP_IdleAnim);
						}
						else if (!bStationary && SlotVacuumFill.IsValid()
							&& AI->Montage_IsPlaying(SlotVacuumFill.Get()))
						{
							StopSlotLoop();
							SlotVacuumFill = nullptr;
						}
					}
				}
				else if (bStationary && !bStaggered)
				{
					// COMBAT + stationary (NOT staggered): freeze the active clip at its last usable frame to
					// bridge a gap between swings. A STAGGER is EXCLUDED (dev 2026-07-05c: the residual ~0.5s
					// freeze after a block was this pose-hold PAUSING the flinch mid-stun at ~0.45/0.80). During
					// a stagger the flinch must play SMOOTHLY; the stun-release active-resume (ReceiveStaggerHit)
					// crossfades the next swing/chase over the still-playing flinch — no paused beat, and no BS@0
					// because the resume always seats a live clip on the slot (the old T-pose risk that made us
					// hold the flinch here is gone now that the release drives the resume).
					// Window: pause BEFORE the auto blend-out point (len - 0.15) or the montage
					// terminates on its own; looping montages have a huge composite length so the
					// remaining-time test never triggers on them (run loop, loom idle are safe).
					UAnimMontage* Cur = AI->GetCurrentActiveMontage();
					if (Cur && AI->Montage_IsPlaying(Cur))
					{
						const float Len = Cur->GetPlayLength();
						const float Pos = AI->Montage_GetPosition(Cur);
						if (Len > 0.6f && (Len - Pos) <= 0.35f && (Len - Pos) > 0.16f)
						{
							AI->Montage_Pause(Cur);
							CombatPoseHold = Cur;
						}
					}
				}
				else
				{
					// Moving again — release any held pose back into locomotion.
					ReleaseCombatPoseHold();
				}
			}
		}
	}

	// GUARD for the BP EventGraph wander (dev-built: BeginPlay → 3s looping timer → move to a
	// random reachable point). It stays fully alive in normal play — but in the windows where
	// this component deliberately freezes movement (the grab hold, grab-reaction staggers, the
	// loom over a downed player) any path it starts must be stopped, or it drives the body
	// mid-reaction (the [GrabSlideProbe] moveStatus=Moving orbit/slide, root-caused 2026-07-02).
	// NOTE round-21: this guard MUST keep running before the NEXT frame's crawl maintainer —
	// the maintainer samples velocity earlier in this same tick, so the one-tick spike of a
	// just-started BP move is additionally armored against in the maintainer itself
	// (sustained-motion requirement) — ordering alone can't close the same-tick window.
	const bool bMovementFrozen = (State == EShamblerState::Grab) || bStaggered
		|| bStandingUpNow
		// Dormant presets are MOTIONLESS (dev 2026-08-05: "the zombie should be motionless") —
		// the dev-built BP wander must not drag a prone/slumped body around before aggro.
		|| (State == EShamblerState::Wander && (IsPronePreset() || IsDormantSlumped()))
		|| GetWorld()->GetTimerManager().IsTimerActive(VictimUpWaitTimer)
		// the escape pushback (pending or live) is also a frozen window — if the knobs are ever
		// dialed past the stun length, the lerp must not fight a live AI path (review finding)
		|| bEscapePushback
		|| GetWorld()->GetTimerManager().IsTimerActive(EscapePushbackDelayTimer);
	if (bMovementFrozen && AICon && AICon->GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		AICon->StopMovement();
		// StopMovement kills the REQUEST but the CMC braking ramp lets the velocity tail decay
		// over ~0.5s. Under STATIC nav the BP wander's moves failed instantly (no velocity, so
		// this never showed); DYNAMIC nav gave its 3s timer real paths again, and the decaying
		// blip crossed the crawl anim threshold on every fire — the round-19 one-arm metronome
		// ([CrawlProbe]: vel 6-23 for ~0.5s every ~3s in Wander). A frozen body stops NOW.
		if (UCharacterMovementComponent* FrozenCM = Owner->GetCharacterMovement())
		{
			FrozenCM->StopMovementImmediately();
		}
		UE_LOG(LogTemp, Verbose, TEXT("[GrabProbe] stopped external AI move during frozen window (state=%d staggered=%d)"),
			(int32)State, bStaggered ? 1 : 0);
	}

	// Only hand-face the player while mid-ATTACK (tracks a circling player through the swing).
	// Scream does NOT track: it snap-faces ONCE at entry and then HOLDS — continuous tracking read
	// as a "little twist" when the player strafed mid-scream (dev report). Chase faces movement —
	// EXCEPT while standing ready inside AZP_AttackRange (no movement to orient to).
	if (State == EShamblerState::Attack || (State == EShamblerState::Chase && bChaseHoldingInRange))
	{
		FaceTargetSmooth(DeltaTime);
	}

	// ALERT TURN drive (dev 2026-07-14): while the stepping turn clip plays, rotate the root
	// toward the target's LIVE position at the precomputed rate — the clip's feet cover the
	// rotation. Hands off to the scream when aligned or when the clip runs out. (This is a
	// pre-scream phase, not mid-scream tracking — the scream itself still holds its facing.)
	if (State == EShamblerState::Scream && bAlertTurning && !bStaggered && !bDead && Owner)
	{
		float RemainingYaw = 0.f;
		AActor* T = Target ? Target.Get() : Cast<AActor>(GetPlayer());
		if (T)
		{
			FVector To = T->GetActorLocation() - Owner->GetActorLocation();
			To.Z = 0.f;
			if (!To.IsNearlyZero())
			{
				const float DesiredYaw = To.Rotation().Yaw;
				const FRotator Cur = Owner->GetActorRotation();
				RemainingYaw = FMath::FindDeltaAngleDegrees(Cur.Yaw, DesiredYaw);
				const FRotator NewRot = FMath::RInterpConstantTo(Cur, FRotator(0.f, DesiredYaw, 0.f),
					DeltaTime, FMath::Max(AlertTurnYawRate, 30.f));
				Owner->SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
			}
		}
		if (FMath::Abs(RemainingYaw) <= 8.f
			|| (GetWorld()->GetTimeSeconds() - AlertTurnStartedAt) >= AlertTurnDuration)
		{
			StartScreamPhase();
		}
	}

	// Mid-leg stumble end: as soon as the stumble timer runs out, re-issue the move toward the same
	// wander target. Done here (per-frame) rather than in Evaluate (0.25s eval) so the resume isn't
	// gated on the next eval tick — keeps the stumble feeling like a hesitation, not a freeze.
	if (bStumbling && State == EShamblerState::Wander && GetWorld()->GetTimeSeconds() >= StumbleEndTime)
	{
		bStumbling = false;
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] STUMBLE END (bWanderMoving=%d)"), bWanderMoving ? 1 : 0);
		if (bWanderMoving && AICon)
		{
			AICon->MoveToLocation(WanderDest, AZP_WanderAcceptRadius);
		}
	}

}

void UZP_ShamblerBehaviorComponent::FaceTargetSmooth(float DeltaTime)
{
	AActor* T = Target ? Target.Get() : Cast<AActor>(GetPlayer());
	if (!T || !Owner) { return; }
	FVector ToT = T->GetActorLocation() - Owner->GetActorLocation();
	ToT.Z = 0.f;
	if (ToT.IsNearlyZero()) { return; }
	const FRotator Cur = Owner->GetActorRotation();
	const FRotator Desired(0.f, ToT.Rotation().Yaw, 0.f);
	const FRotator NewRot = FMath::RInterpConstantTo(Cur, Desired, DeltaTime, AZP_CombatTurnRate);
	Owner->SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
}

// ───────────────────────── core loop ─────────────────────────

void UZP_ShamblerBehaviorComponent::Evaluate()
{
	if (bDead || !Owner) { return; }
	if (bStaggered) { return; }
	StateTimer += AZP_EvalInterval;

	APawn* Player = GetPlayer();
	const float DistToPlayer = Player ? FVector::Dist(Owner->GetActorLocation(), Player->GetActorLocation()) : TNumericLimits<float>::Max();

	switch (State)
	{
	case EShamblerState::Wander:
	{
		UpdateLurk(DistToPlayer);

		// SLUMPED preset, dormant: motionless against its wall. 3-TIER WAKE (dev 2026-08-05:
		// room entry was "immediate" — the dread should BUILD): tier 3 = within
		// AZP_SlumpProximityRange (instant); tier 1 = AZP_SlumpWakePresenceTime continuous
		// seconds inside the room envelope with LOS; tier 2 = AZP_SlumpWakeStareTime seconds
		// of the player's view centered on the body. Damage always wakes (OnPointDamage).
		if (IsDormantSlumped())
		{
			const TCHAR* WakeTier = nullptr;
			if (Player)
			{
				if (DistToPlayer <= AZP_SlumpProximityRange && IsProximityAggroValid(Player))
				{
					WakeTier = TEXT("PROXIMITY");
				}
				else if (DistToPlayer <= AZP_SlumpWakeRange && HasLOS(Player))
				{
					SlumpPresenceTimer += AZP_EvalInterval;
					bool bStaring = false;
					if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
					{
						FVector CamLoc;
						FRotator CamRot;
						PC->GetPlayerViewPoint(CamLoc, CamRot);
						const FVector To = (Owner->GetActorLocation() - CamLoc).GetSafeNormal();
						bStaring = FVector::DotProduct(CamRot.Vector(), To) >= 0.95f; // ~18 deg cone
					}
					SlumpStareTimer = bStaring ? SlumpStareTimer + AZP_EvalInterval : 0.f;
					if (SlumpStareTimer >= AZP_SlumpWakeStareTime)       { WakeTier = TEXT("STARE"); }
					else if (SlumpPresenceTimer >= AZP_SlumpWakePresenceTime) { WakeTier = TEXT("PRESENCE"); }
				}
				else
				{
					// Left the room / lost LOS — the clocks reset; the corpse goes back to scenery.
					SlumpPresenceTimer = 0.f;
					SlumpStareTimer = 0.f;
				}
			}
			if (WakeTier)
			{
				Target = Player;
				if (Audio) { Audio->PlayAlert(); }
				UE_LOG(LogTemp, Warning, TEXT("[Shambler] SLUMPED WAKE (%s): dist=%.0f presence=%.1fs stare=%.1fs"),
					WakeTier, DistToPlayer, SlumpPresenceTimer, SlumpStareTimer);
				StartSlumpRise();
			}
			break; // dormant: never wanders
		}

		// PLAYING DEAD preset: corpse-still — sight NEVER wakes it; only the proximity floor
		// (or damage). "These shamblers look dead, but get too close..." (dev 2026-08-05).
		if (AZP_Preset == EShamblerPreset::PlayingDead)
		{
			if (Player && DistToPlayer <= AZP_ProximityAggroRange && IsProximityAggroValid(Player))
			{
				Target = Player;
				PlayCrawlAlert();
				UE_LOG(LogTemp, Warning, TEXT("[Shambler] PLAYING-DEAD WAKE: dist=%.0f"), DistToPlayer);
				SetState(EShamblerState::Chase);
			}
			break; // corpse-still: never wanders
		}

		// PROXIMITY AGGRO — UNAVOIDABLE, EVERY STANCE (dev 2026-08-05: "aggro is impossible to
		// avoid within proximity" — and it applies to ALL NPC enemies, not a crawler thing).
		// Inside this radius the hunt starts regardless of facing, LOS or noise. Stealth still
		// exists at range: the sight detection below is unchanged (some stealth, not full TLOU).
		if (Player && DistToPlayer <= AZP_ProximityAggroRange && IsProximityAggroValid(Player))
		{
			Target = Player;
			UE_LOG(LogTemp, Warning, TEXT("[Shambler] PROXIMITY AGGRO: dist=%.0f (radius %.0f)"),
				DistToPlayer, AZP_ProximityAggroRange);
			if (IsCrawlingNow())
			{
				PlayCrawlAlert();
				SetState(EShamblerState::Chase);
			}
			else
			{
				bPendingAlertTurn = true;
				SetState(EShamblerState::Scream);
			}
			break;
		}

		// Spotted? Needs range + roughly facing the player + a clear sightline (same room) —
		// the STEALTH-ABLE layer, all stances (a sighted crawler stays prone unless the 20m
		// stand rule fires).
		if (Player && DistToPlayer <= AZP_DetectionRange)
		{
			const FVector ToP = (Player->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
			const float Facing = FVector::DotProduct(Owner->GetActorForwardVector(), ToP);
			const bool bLOS = HasLOS(Player);
			const bool bClose = DistToPlayer <= AZP_CloseSenseRange; // right next to it -> senses you regardless of facing
			if (bLOS && (bClose || Facing >= AZP_FacingThreshold))
			{
				// VETO: cross-check with the audio propagation model. If the acoustic path CONFIDENTLY
				// says "through a wall" (a valid nav route exists and it's a huge detour), HasLOS
				// slipped through geometry it shouldn't have — seeing through a wall is nonsense, and
				// the muffled through-wall alert is exactly the reported bug. Never vetoes point-blank
				// (bClose), and never vetoes on mere route-UNKNOWN (player off-navmesh, nav gaps) —
				// a nav hiccup must not blind the enemy in open air.
				bool bTransmitConfident = false;
				float PropVol = 1.f, PropLPF = 0.f;
				UZP_SFXStatics::ComputePropagation(GetWorld(), Owner->GetActorLocation(), Owner, PropVol, PropLPF, &bTransmitConfident);
				if (!bClose && bTransmitConfident)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Shambler] sight-aggro VETOED (confident through-wall): dist=%.0f facing=%.2f — HasLOS passed but the only route is a huge detour"),
						DistToPlayer, Facing);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[Shambler] SIGHT AGGRO: dist=%.0f facing=%.2f close=%d propVol=%.2f"),
						DistToPlayer, Facing, bClose ? 1 : 0, PropVol);
					Target = Player;
					if (IsCrawlingNow())
					{
						// Crawler sighted the player: bark and hunt prone. Never stands.
						PlayCrawlAlert();
						SetState(EShamblerState::Chase);
						break;
					}
					// SIGHT aggro is the ONLY path that earns the stepping alert turn — hurt/
					// stagger/takedown reuse SetState(Scream) verbatim and keep the instant snap.
					bPendingAlertTurn = true;
					SetState(EShamblerState::Scream);
					break;
				}
			}
		}

		// CRAWLER preset: MOTIONLESS until aggro (dev 2026-08-05: "the zombie should be
		// motionless, the idle crawl") — no wander legs; the maintainer holds the crawl idle.
		if (IsCrawlingNow())
		{
			break;
		}

		// Two-phase wander: WALK for WalkDuration (3-6s) then IDLE for IdleDuration (6-9s).
		// During WALK, legs chain seamlessly via OnMoveCompleted so velocity never drops to zero
		// (no idle frame leaking through from the BlendSpace). During IDLE, movement is fully
		// stopped and the zombie idle anim plays via the slot.
		// STANDING LURKER preset never enters the idle phase at all (dev 2026-08-05: "ensure
		// they are never pausing fully") — legs chain forever; stumbles stay the only beat.
		if (bWanderMoving)
		{
			// Walk timer up -> drop into idle.
			if (StateTimer >= WalkDuration && AZP_Preset != EShamblerPreset::Standing)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Shambler] WALK→IDLE (walked %.2fs of %.2fs)"), StateTimer, WalkDuration);
				bWanderMoving = false;
				StateTimer = 0.f;
				IdleDuration = FMath::FRandRange(AZP_IdleDurationMin, AZP_IdleDurationMax);
				// Smooth deceleration: AICon->StopMovement aborts the path-following request, but we
				// do NOT call StopMovementImmediately or set MOVE_None here — the CMC's braking
				// deceleration ramps velocity from walk speed → 0 over a fraction of a second,
				// which the BlendSpace shows as a natural walk→idle transition under the slot
				// blend. After AZP_IdleLockDelay, LockIdleMovement() snaps MOVE_None to freeze anything
				// residual (RVO, accel, etc.).
				if (AICon) { AICon->StopMovement(); }
				if (UAnimSequence* IdleClip = PickIdleAnim())
				{
					UAnimMontage* IdleLoop = PlaySlotLoop(IdleClip);
					if (IsCrawlingNow()) { CrawlLoopMontage = IdleLoop; CrawlLoopClip = IdleClip; }
					// Idle groan — tied verbatim to the idle ANIMATION starting (one-shot per idle phase).
					UZP_SFXStatics::PlaySFXAttached(AZP_IdleSound, Owner->GetRootComponent(), EZP_SFXCarry::Far);
					if (!IsCrawlingNow()) // effective crawlers: the Tick maintainer owns mesh Z
					{
						if (USkeletalMeshComponent* SM = Owner->GetMesh())
						{
							FVector RL = SM->GetRelativeLocation();
							RL.Z = MeshBaseRelZ + AZP_IdleMeshZOffset;
							SM->SetRelativeLocation(RL);
						}
					}
				}
				GetWorld()->GetTimerManager().SetTimer(IdleLockTimer, this,
					&UZP_ShamblerBehaviorComponent::LockIdleMovement, AZP_IdleLockDelay, false);
			}
			else if (!bStumbling)
			{
				// Mid-leg stumble (rare). Interrupts movement for a beat then resumes the same leg.
				if (FMath::FRand() < AZP_StumbleChancePerSec * AZP_EvalInterval)
				{
					bStumbling = true;
					StumbleEndTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(AZP_StumbleMin, AZP_StumbleMax);
					UE_LOG(LogTemp, Warning, TEXT("[Shambler] STUMBLE START (will end at %.2fs)"), StumbleEndTime);
					if (AICon) { AICon->StopMovement(); }
					if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
					{
						CM->StopMovementImmediately();
					}
				}
			}
		}
		else
		{
			// Idle phase: wait IdleDuration, then resume walking.
			if (StateTimer >= IdleDuration)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Shambler] IDLE→WALK (idled %.2fs of %.2fs)"), StateTimer, IdleDuration);
				StopSlotLoop();
				if (!IsCrawlingNow()) // effective crawlers: the Tick maintainer owns mesh Z
				{
					if (USkeletalMeshComponent* SM = Owner->GetMesh())
					{
						FVector RL = SM->GetRelativeLocation();
						RL.Z = MeshBaseRelZ + AZP_LocoMeshZOffset; // walk pose floated +5.97 without it ([FloatProbe])
						SM->SetRelativeLocation(RL);
					}
				}
				// Cancel a pending idle lock (shouldn't be pending this late but defensive),
				// and re-enable walking before issuing the next leg.
				GetWorld()->GetTimerManager().ClearTimer(IdleLockTimer);
				if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
				{
					CM->SetMovementMode(MOVE_Walking);
				}
				if (PickNewWanderPoint())
				{
					StartWanderLeg();
					bWanderMoving = true;
					StateTimer = 0.f;
					WalkDuration = FMath::FRandRange(AZP_WalkDurationMin, AZP_WalkDurationMax);
				}
				else
				{
					// Couldn't find a navmesh point — extend the idle by a short retry beat.
					StateTimer = IdleDuration - FMath::FRandRange(0.2f, 0.5f);
				}
			}
		}
		break;
	}

	case EShamblerState::Scream:
	{
		// MELEE-RANGE INTERRUPT (dev 2026-07-14): the player closing to attack range cuts the
		// alert (turn or scream) short — SetState's exit hook blends the clip out, and Chase's
		// very next eval runs the grab-first/attack logic with full gating.
		if (Target
			&& FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation()) <= AZP_AttackRange
			&& HasLOS(Target))
		{
			UE_LOG(LogTemp, Log, TEXT("[Shambler] alert interrupted — player in melee range"));
			SetState(EShamblerState::Chase);
			break;
		}
		// Hold the scream, then break into the fast walk. CurrentScreamHold is per-aggro: the full
		// cinematic AZP_ScreamHoldTime on sight, the short AZP_HurtScreamHoldTime when damage caused
		// the scream. The hold clock starts at the SCREAM phase (StartScreamPhase resets StateTimer),
		// so the turn never eats the wail.
		if (!bAlertTurning && StateTimer >= CurrentScreamHold)
		{
			SetState(EShamblerState::Chase);
		}
		break;
	}

	case EShamblerState::Chase:
	{
		if (!Target) { SetState(EShamblerState::Wander); break; }

		// Mid-slump-rise: hold position until FinishSlumpRise releases the hunt.
		if (bStandingUpNow) { break; }

		const bool bSee = HasLOS(Target);
		// Proximity pins the lose-sight timer too — a proximity-aggroed shambler must not give
		// up while the player is still inside the unavoidable radius without LOS.
		LostSightTimer = (bSee || DistToPlayer <= AZP_ProximityAggroRange) ? 0.f : (LostSightTimer + AZP_EvalInterval);

		// Give up: lost sight too long, or leashed out.
		if (LostSightTimer >= AZP_LoseSightTime || DistToPlayer > AZP_GiveUpRange)
		{
			Target = nullptr;
			SetState(EShamblerState::Wander);
			PickNewWanderPoint();
			break;
		}

		// Close enough to LATCH? THE GRAB IS THE OPENER — the first thing it does in reach
		// (jump-scare by design, dev direction 2026-07-02). Melee is the fallback while the
		// grab is on cooldown or was deflected/evaded.
		const double Now = GetWorld()->GetTimeSeconds();
		// HORIZONTAL reach (dev 2026-07-14: "you can avoid shambler grapple 100% by just staying in
		// crouch"). DistToPlayer is a 3D distance and crouching drops Grace's capsule centre 44uu
		// (halfH 88 -> AZP_CrouchedHalfHeight 44), so the Z leg ATE the grab radius: at the live
		// AZP_GrabRange=125 the effective horizontal reach fell 125 -> 117 while the capsule radii
		// (34 + 55) already floor the separation at 89 — the window shrank 36uu -> 28uu. Melee barely
		// noticed the same Z (230 -> 225.7), which is why ONLY the grab read as crouch-proof. A grab is
		// a horizontal lunge; the victim's height must not gate it. Measured, not guessed — see
		// checkpoint 2026-07-14_shambler_grab_crouch_and_float.
		const float GrabDist2D = Player
			? FVector::Dist2D(Owner->GetActorLocation(), Player->GetActorLocation())
			: TNumericLimits<float>::Max();
		if (!IsCrawlingNow() && GrabDist2D <= AZP_GrabRange && bSee
			&& (Now - LastGrabTime) >= AZP_GrabCooldown)
		{
			TryStartGrab();
			if (State == EShamblerState::Grab) { break; }
			// A deflected grab staggers us from INSIDE this eval (TryStartGrab -> ReceiveStaggerHit), so
			// bStaggered can flip true AFTER the entry guard (~line 518) already passed. Bail now — the
			// fall-through swing logic below would otherwise BeginAttack and swing THROUGH the stun,
			// instantly overriding the flinch (confirmed [BlockFreeze] stagger #1, 2026-07-05c).
			if (bStaggered) { break; }
			// Deflected/unavailable — fall through to the normal swing logic below.
		}
		else if (bAZP_GrabGateProbe && Player && GrabDist2D <= AZP_GrabRange + 120.f)
		{
			// [GrabGate] The player was NEAR but no latch fired — name the blocker. dz>0 = the player's
			// centre is BELOW ours (crouching). If this reads "d2D IN | see ok | COOLING" the cooldown
			// owns it; if it reads "TOO FAR" while dz~44, the reach is the problem; if "BLOCKED", LOS is.
			UE_LOG(LogTemp, Warning, TEXT("[GrabGate] t=%.2f dtState=%.2f NO LATCH: d2D=%.1f vs range=%.1f (%s) | ")
				TEXT("d3D=%.1f dz=%.1f%s | see=%d (%s) | cdLeft=%.1f (%s)"),
				Now, (float)(Now - LastStateChangeTime),
				GrabDist2D, AZP_GrabRange, (GrabDist2D <= AZP_GrabRange) ? TEXT("IN") : TEXT("TOO FAR"),
				DistToPlayer, Owner->GetActorLocation().Z - Player->GetActorLocation().Z,
				(Owner->GetActorLocation().Z - Player->GetActorLocation().Z) > 25.f ? TEXT(" CROUCHED?") : TEXT(""),
				bSee ? 1 : 0, bSee ? TEXT("ok") : TEXT("BLOCKED"),
				FMath::Max(0.f, AZP_GrabCooldown - (float)(Now - LastGrabTime)),
				((Now - LastGrabTime) >= AZP_GrabCooldown) ? TEXT("ready") : TEXT("COOLING"));
		}

		// ENGAGE RANGE (dev 2026-07-14: "The shambler should just get closer in this scenario").
		// THE GRAB IS THE OPENER by design — but the swing gate fired at AZP_AttackRange (230) while
		// the live AZP_GrabRange is 125, so the Shambler PARKED at swing range (the hold below calls
		// StopMovement) and flurried forever. The latch could then only fire if the PLAYER walked
		// inside 125 — i.e. a player who keeps their distance was grab-immune BY CONSTRUCTION, at any
		// stance. Crouching just made it worse (the old 3D gate cost another 8uu of reach), which is
		// why it read as "avoid the grapple 100% by staying in crouch".
		// While the latch is READY, hold fire and keep walking until we're actually inside grab range.
		// Once it's spent (landed -> AZP_GrabCooldown, failed -> AZP_GrabFailCooldown) the swing gate
		// returns to AZP_AttackRange and it melees at range exactly as before — which is the documented
		// intent: "This is the window in which it melees instead".
		// Crawlers never grab (standing paired clips) — bGrabReady must be false or the engage
		// range would park them at AZP_GrabRange with a latch that can never fire.
		const bool bGrabReady = !IsCrawlingNow() && (Now - LastGrabTime) >= AZP_GrabCooldown;
		// Prone claws are short/low — crawlers engage at their OWN range (round-24: "it swings
		// at a distance where it cannot hit me" at the standing 230).
		const float EngageRange = IsCrawlingNow() ? AZP_CrawlAttackRange
			: (bGrabReady ? AZP_GrabRange : AZP_AttackRange);

		// HORIZONTAL for the engage decision too (dev: "crouching requires me to walk closer to melee
		// for the shambler to react"). DistToPlayer is 3D, so a crouched player's 44uu centre drop
		// inflated it and pushed the Shambler's hold/swing threshold further out. Height must not
		// decide whether it engages.
		const float EngageDist = (GrabDist2D < TNumericLimits<float>::Max()) ? GrabDist2D : DistToPlayer;

		// In range + off cooldown -> swipe.
		if (EngageDist <= EngageRange && bSee && (Now - LastAttackTime) >= AZP_AttackCooldown)
		{
			BeginAttack();
			break;
		}

		// Keep closing — re-path to the (possibly moving) player each eval so it tracks instead of
		// running a stale path past you. Stop WELL inside AZP_AttackRange (not at its edge) so it reliably
		// crosses the attack threshold and swings instead of parking just out of reach.
		// ALREADY in striking distance but the swing isn't available this eval (cooldown/LOS
		// blip)? STAND READY instead of re-pathing — the per-eval MoveToActor at point-blank
		// walks orbits around the target ("full walking circle", dev report 2026-07-02).
		// Face-tracking runs per-frame in TickComponent while holding.
		// Hold at ENGAGE range, not attack range — while the latch is ready that is AZP_GrabRange, so
		// it keeps closing instead of parking out of latch reach (see the EngageRange note above).
		if (EngageDist <= EngageRange && bSee)
		{
			StopRunChase(); // arrived on top of the player — sprint over, attack flow owns it
			if (!bChaseHoldingInRange)
			{
				bChaseHoldingInRange = true;
				if (AICon) { AICon->StopMovement(); }
				UE_LOG(LogTemp, Warning, TEXT("[GrabProbe] chase HOLD in range (d2D=%.0f engage=%.0f grabReady=%d) — standing ready"),
					EngageDist, EngageRange, bGrabReady ? 1 : 0);
			}
		}
		else
		{
			if (bChaseHoldingInRange)
			{
				bChaseHoldingInRange = false;
				UE_LOG(LogTemp, Warning, TEXT("[GrabProbe] chase hold RELEASED (d2D=%.0f engage=%.0f see=%d) — pathing"),
					EngageDist, EngageRange, bSee ? 1 : 0);
			}

			// SPRINT-CHASE: outside AZP_RunTriggerDistance (and visible) the shamble breaks into the
			// run — the walk gave the player the whole room for free ("I don't even need to
			// dodge", dev 2026-07-03). Losing sight drops back to the walk; arriving inside
			// AZP_AttackRange exits via the hold branch above. Sight/leash logic above unchanged.
			// INTERSPERSED (dev 2026-07-03): the sprint is not one steady run — it cycles
			// RUN BURST (AZP_RunBurstDuration @ AZP_RunSpeed, run clip) -> FAST WALK (AZP_RunWalkDuration @
			// AZP_ChaseSpeed, walk BlendSpace) -> burst -> ... for as long as the band holds.
			// Always OPENS with a burst. AZP_RunWalkDuration 0 = continuous run (old behavior).
			// PRONE GAIT is DISTANCE-based (dev 2026-08-05: "crawl slow if closer and crawl
			// fast if far") — no sprint bursts; the maintainer picks the matching clip.
			if (IsCrawlingNow())
			{
				SetSpeed(DistToPlayer > AZP_CrawlFastDistance ? AZP_CrawlRunSpeed : AZP_CrawlChaseSpeed);
			}
			if (!IsCrawlingNow() && !bRunningChase && bSee && DistToPlayer > AZP_RunTriggerDistance)
			{
				bRunningChase = true;
				bRunBurstNow = true; // open with the burst — that's the scare
				RunPhaseStart = Now;
				SetSpeed(AZP_RunSpeed);
				UE_LOG(LogTemp, Warning, TEXT("[Shambler] RUN start (dist=%.0f > %.0f) burst=%.1fs walk=%.1fs"),
					DistToPlayer, AZP_RunTriggerDistance, AZP_RunBurstDuration, AZP_RunWalkDuration);
			}
			else if (bRunningChase && !bSee)
			{
				StopRunChase();
			}
			if (bRunningChase && AZP_RunWalkDuration > 0.f)
			{
				const float PhaseLen = bRunBurstNow ? FMath::Max(0.25f, AZP_RunBurstDuration) : AZP_RunWalkDuration;
				if ((Now - RunPhaseStart) >= PhaseLen)
				{
					bRunBurstNow = !bRunBurstNow;
					RunPhaseStart = Now;
					SetSpeed(bRunBurstNow ? AZP_RunSpeed : AZP_ChaseSpeed); // sprint = standing-only
					if (!bRunBurstNow)
					{
						// Burst over — release the run clip so the walk BlendSpace takes back
						// over as the CMC decelerates to AZP_ChaseSpeed (same only-if-ours release
						// as StopRunChase; never cut a flinch that displaced the loop).
						UAnimInstance* RunAIw = (Owner && Owner->GetMesh()) ? Owner->GetMesh()->GetAnimInstance() : nullptr;
						if (RunLoopMontage.IsValid() && RunAIw && RunAIw->Montage_IsPlaying(RunLoopMontage.Get()))
						{
							StopSlotLoop();
						}
						RunLoopMontage = nullptr;
					}
					UE_LOG(LogTemp, Warning, TEXT("[Shambler] RUN cycle -> %s (dist=%.0f)"),
						bRunBurstNow ? TEXT("BURST") : TEXT("fast walk"), DistToPlayer);
				}
			}
			if (bRunningChase && bRunBurstNow && AZP_RunAnim && !IsCrawlingNow())
			{
				// (Re-)assert the run loop — a flinch crossfading over it mid-sprint ends on the
				// slot, so after each interruption the loop must be re-seated. Stride-matched:
				// play rate = AZP_RunSpeed / AZP_RunAnimRefSpeed. (Crawlers: the crawl maintainer
				// in Tick owns the slot and picks the fast-crawl clip by speed instead.)
				UAnimInstance* RunAI = (Owner && Owner->GetMesh()) ? Owner->GetMesh()->GetAnimInstance() : nullptr;
				if (RunAI && (!RunLoopMontage.IsValid() || !RunAI->Montage_IsPlaying(RunLoopMontage.Get())))
				{
					RunLoopMontage = PlaySlotLoop(AZP_RunAnim,
						FMath::Max(0.1f, AZP_RunSpeed / FMath::Max(AZP_RunAnimRefSpeed, 1.f)));
				}
			}
			// Acceptance must sit INSIDE the engage range or the path stops short of it and the hold
			// never triggers. With the latch ready that means closing on AZP_GrabRange (125), not
			// AZP_AttackRange (230) — the old max(230-70,40)=160 parked it 35uu outside grab reach,
			// so the opener could never fire (dev 2026-07-14 crouch-immunity report).
			// Re-issue ONLY when the goal actually moved (round-19 "shuffles 1 arm constantly":
			// under DYNAMIC nav every MoveToActor abort+re-path costs a beat of velocity, and the
			// 0.25s blind re-issue turned chase speed into a sawtooth that reset the crawl gait
			// each dip; under STATIC pathfinds resolved instantly so this never showed). MoveToActor
			// tracks a moving goal by itself — the re-issue is only needed after big goal jumps.
			if (AICon && Target)
			{
				const FVector GoalNow = Target->GetActorLocation();
				UPathFollowingComponent* PFC = AICon->GetPathFollowingComponent();
				const bool bMoveActive = AICon->GetMoveStatus() == EPathFollowingStatus::Moving;
				// MOVE OWNERSHIP by REQUEST ID (round-22: the 400uu destination heuristic let a
				// BP-wander random point NEAR the player pass as "ours" — "crawled/approached me
				// and then turned around then staggered around"). The dev-built BP wander seizing
				// path-following gets a NEW request id; if the live move's id isn't the one WE
				// issued, it isn't ours — retake immediately. Exact, no distance guesswork.
				const bool bOurs = bMoveActive && PFC && PFC->GetCurrentRequestId() == LastChaseMoveId;
				if (!bOurs || FVector::Dist2D(GoalNow, LastChaseGoal) > 150.f)
				{
					AICon->MoveToActor(Target, FMath::Max(EngageRange - AZP_ChaseAcceptanceInset, 40.f));
					LastChaseGoal = GoalNow;
					if (PFC) { LastChaseMoveId = PFC->GetCurrentRequestId(); }
				}
			}
		}
		break;
	}

	case EShamblerState::Attack:
	{
		// Swing length is computed per-swing in BeginAttack (clip length remapped through the
		// wind-up/strike play rates); AZP_AttackDuration is only the fallback for a missing clip.
		const float SwingLen = (CurrentSwingTotalTime > 0.f) ? CurrentSwingTotalTime : AZP_AttackDuration;
		// The pose-hold engaging on the ACTIVE swing means the clip already reached its last usable
		// frame — the VISUAL end of the swing. Waiting out the residual rate-math tail past that
		// point left the body a statue for ~1s at every swing end (dev 2026-07-03 evening: "freezes
		// at the start and end of each animation"). Chain as soon as the damage sweep has fired AND
		// the hold engaged; StateTimer >= SwingLen remains the fallback for swings that never enter
		// the hold window (e.g. body still drifting, hold requires stationary).
		const bool bHeldAtEnd = CombatPoseHold.IsValid() && ActiveSwingMontage.IsValid()
			&& CombatPoseHold.Get() == ActiveSwingMontage.Get()
			&& StateTimer >= (CurrentSwingHitTime + 0.05f);
		if (StateTimer >= SwingLen || bHeldAtEnd)
		{
			if (bHeldAtEnd && StateTimer < SwingLen)
			{
			}
			// GRAB FIRST between swings too. The Attack state chains L/R/L/R without ever
			// returning to Chase, so without this re-check a point-blank flurry would melee the
			// player to death with the grab never firing again (exactly the reported bug).
			const double NowAtk = GetWorld()->GetTimeSeconds();
			// Same horizontal-reach rule as the Chase gate — a crouching player must be exactly as
			// grabbable as a standing one (dev 2026-07-14 crouch-immunity report).
			const float GrabDist2DAtk = Target
				? FVector::Dist2D(Owner->GetActorLocation(), Target->GetActorLocation())
				: TNumericLimits<float>::Max();
			if (!IsCrawlingNow() && Target && GrabDist2DAtk <= AZP_GrabRange && HasLOS(Target)
				&& (NowAtk - LastGrabTime) >= AZP_GrabCooldown)
			{
				TryStartGrab();
				if (State == EShamblerState::Grab) { break; }
			}
			// A deflected grab here staggers us mid-eval too — bail before the flurry BeginAttack below
			// swings through the stun (same guard as the Chase case).
			if (bStaggered) { break; }

			// Still on top of the player when the swing ends -> immediately swing again (other side,
			// other strike sound). No cooldown gap: it flurries L/R/L/R until you die or break away.
			if (Target && DistToPlayer <= (IsCrawlingNow() ? AZP_CrawlAttackRange : AZP_AttackRange)
				&& HasLOS(Target))
			{
				BeginAttack();
			}
			else
			{
				// Leaving the flurry with the swing still paused/active on the slot would ride the
				// frozen statue pose through the whole chase — blend it off before handing over.
				ReleaseCombatPoseHold();
				if (ActiveSwingMontage.IsValid() && Owner && Owner->GetMesh())
				{
					if (UAnimInstance* SlotAI = Owner->GetMesh()->GetAnimInstance())
					{
						if (SlotAI->Montage_IsActive(ActiveSwingMontage.Get()))
						{
							SlotAI->Montage_Stop(0.25f, ActiveSwingMontage.Get());
						}
					}
				}
				SetState(Target ? EShamblerState::Chase : EShamblerState::Wander);
			}
		}
		break;
	}

	case EShamblerState::Grab:
	{
		// Victim-driven: the player's phase machine calls OnVictimGrabPhase at every transition.
		// Nothing to evaluate — movement is MOVE_None and the paired clips own the mesh.
		break;
	}
	}
}

void UZP_ShamblerBehaviorComponent::SetState(EShamblerState NewState)
{
	// Leaving the alert: blend any still-playing turn/scream clip out instead of letting its
	// tail wail over the moving body (KB bug 2026-07-14b — the hurt-shortened scream kept
	// playing frozen-legged into the chase). Covers every exit: timer, damage interrupt,
	// melee-range interrupt, stagger resume.
	if (State == EShamblerState::Scream && NewState != EShamblerState::Scream)
	{
		StopAlertMontage(0.3f);
	}

	State = NewState;
	StateTimer = 0.f;
	if (GetWorld()) { LastStateChangeTime = GetWorld()->GetTimeSeconds(); } // [FloatProbe] dt anchor
	bChaseHoldingInRange = false; // every state entry re-decides hold-vs-path
	bRunningChase = false;        // sprint is Chase-internal; re-triggers on the next Chase eval
	RunLoopMontage = nullptr;     // the new state's clips crossfade over the loop (Wander stops it)

	// Wander faces its movement direction; combat states are hand-faced toward the player (no shimmy,
	// fast pivots even while stopped to attack).
	if (Owner)
	{
		if (UCharacterMovementComponent* M = Owner->GetCharacterMovement())
		{
			M->bOrientRotationToMovement = (NewState == EShamblerState::Wander || NewState == EShamblerState::Chase);
			// Idle phase parks CMC in MOVE_None; any state transition must restore Walking so the
			// AI can move again (Chase/Wander-walk). Idle re-applies MOVE_None inside Evaluate.
			if (M->MovementMode == MOVE_None)
			{
				M->SetMovementMode(MOVE_Walking);
			}
		}
		// Ground the floaty scream pose / height-align the grab pair by nudging the mesh Z.
		if (USkeletalMeshComponent* SM = Owner->GetMesh())
		{
			// Chase/Attack/Wander fall through to AZP_LocoMeshZOffset — they used to apply NOTHING,
			// which is the visible floor gap the dev reported (measured +7.31 Chase / +8.22 Attack
			// above the capsule bottom). Scream/Grab keep their own authored nudges.
			float StateZ = AZP_LocoMeshZOffset;
			// The scream nudge grounds ScreamPinned's frozen idle legs — but the alert-TURN phase
			// really steps, so it starts at 0; StartScreamPhase applies the nudge when the scream
			// clip actually begins.
			if (NewState == EShamblerState::Scream) { StateZ = bPendingAlertTurn ? 0.f : AZP_ScreamMeshZOffset; }
			else if (NewState == EShamblerState::Grab) { StateZ = AZP_GrabPairZOffset; }
			// Effective crawlers: the Tick maintainer owns mesh Z; during the stand window the
			// ground clamp owns it (through rise AND settle — state-entry snaps here would pop
			// the plant). A fully stood-up crawler takes the normal standing write.
			if (!IsCrawlingNow() && !bCrawlStandWindow)
			{
				FVector RL = SM->GetRelativeLocation();
				RL.Z = MeshBaseRelZ + StateZ;
				SM->SetRelativeLocation(RL);
			}
		}
	}

	switch (NewState)
	{
	case EShamblerState::Wander:
		EnsureLocomotion();
		SetSpeed(IsCrawlingNow() ? AZP_CrawlWanderSpeed : AZP_WanderSpeed);
		bWanderMoving = false;
		bStumbling = false;
		IdleDuration = 0.f; // pick a first leg on the very next eval — no startup pause
		StopSlotLoop();
		break;

	case EShamblerState::Scream:
	{
		SetSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		CurrentScreamHold = AZP_ScreamHoldTime; // sight-aggro default; damage paths shorten it AFTER (contract)

		// ALERT TURN (dev 2026-07-14): on sight aggro with the player well off-axis, turn with a
		// real stepping clip while the root yaws in sync (TickComponent drives it) — replaces the
		// old feet-planted snap that read as skating. Only the sight-aggro call site sets
		// bPendingAlertTurn; hurt/stagger/takedown keep the instant snap+scream below.
		float DeltaYaw = 0.f;
		if (Owner)
		{
			AActor* FaceTarget = Target ? Target.Get() : Cast<AActor>(GetPlayer());
			if (FaceTarget)
			{
				FVector To = FaceTarget->GetActorLocation() - Owner->GetActorLocation();
				To.Z = 0.f;
				if (!To.IsNearlyZero())
				{
					DeltaYaw = FMath::FindDeltaAngleDegrees(Owner->GetActorRotation().Yaw, To.Rotation().Yaw);
				}
			}
		}
		UAnimSequence* TurnClip = nullptr;
		if (bPendingAlertTurn && FMath::Abs(DeltaYaw) >= AZP_AlertTurnMinAngle)
		{
			const bool bLeft = DeltaYaw < 0.f;
			TurnClip = (FMath::Abs(DeltaYaw) >= 135.f)
				? (bLeft ? AZP_TurnL180Anim.Get() : AZP_TurnR180Anim.Get())
				: (bLeft ? AZP_TurnL90Anim.Get() : AZP_TurnR90Anim.Get());
		}
		bPendingAlertTurn = false;

		if (TurnClip)
		{
			bAlertTurning = true;
			const float Rate = FMath::Max(AZP_AlertTurnPlayRate, 0.1f);
			AlertTurnDuration = TurnClip->GetPlayLength() / Rate;
			// Root yaw completes at ~70% of the clip so the plant lands with the final step.
			AlertTurnYawRate = FMath::Abs(DeltaYaw) / FMath::Max(AlertTurnDuration * 0.7f, 0.05f);
			AlertTurnStartedAt = GetWorld()->GetTimeSeconds();
			AlertMontage = PlayOneShot(TurnClip, Rate);
			// No snap-face — the tick turn owns the yaw. Alert audio + the scream clip + the
			// mesh-Z nudge land in StartScreamPhase() when the turn finishes.
		}
		else
		{
			StartScreamPhase(); // snap-face + audio + pinned scream clip (pre-2026-07-14 behavior)
		}
		break;
	}

	case EShamblerState::Chase:
		EnsureLocomotion();
		SetSpeed(IsCrawlingNow() ? AZP_CrawlChaseSpeed : AZP_ChaseSpeed);
		break;

	case EShamblerState::Attack:
		SetSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		break;

	case EShamblerState::Grab:
		SetSpeed(0.f);
		if (AICon) { AICon->StopMovement(); }
		break;
	}
}

// ───────────────────────── attack ─────────────────────────

void UZP_ShamblerBehaviorComponent::BeginAttack()
{
	bAttackIsLeft = !bAttackIsLeft;
	LastAttackTime = GetWorld()->GetTimeSeconds();
	SetState(EShamblerState::Attack);
	// Alternate the strike sound with the swing side: Left -> Attack1, Right -> Attack2.
	if (Audio) { Audio->PlayAttack(/*bLunge=*/!bAttackIsLeft); }

	// Two-phase swing: the clip's rear-back plays SLOWED (AZP_WindupPlayRate) up to AZP_WindupEndTime — the
	// readable "block now" telegraph — then ReleaseSwing snaps it to AZP_StrikePlayRate so the strike
	// lands fast. All timers below are in REAL seconds, remapped through the two rates.
	UAnimSequence* Swing = bAttackIsLeft ? AZP_AttackLAnim : AZP_AttackRAnim;
	float HitTimeClipKnob    = AZP_AttackHitTime;
	float WindupEndClipKnob  = AZP_WindupEndTime;
	if (IsCrawlingNow())
	{
		// Prone presets swing the crawl swipe only (presets rework 2026-08-05). The stand
		// attacks stay wired as slots for future presets; a risen SLUMPED shambler is a normal
		// standing shambler and uses the plain L/R swipes above. The L/R alternator still
		// drives the strike-sound alternation regardless of which clip plays.
		// NEVER a standing swipe on a prone body (round-23 "flys in the air like switching to
		// standing"): a missing crawl clip means NO swing visual — the crawl loop holds and the
		// timers below still deal the damage (null Swing is handled by the fallbacks).
		Swing             = AZP_AttackCrawlAnim.Get();
		HitTimeClipKnob   = AZP_CrawlAttackHitTime;
		WindupEndClipKnob = AZP_CrawlWindupEndTime;
	}
	ActiveSwingMontage = PlayOneShot(Swing, AZP_WindupPlayRate);
	bSwingReleased = false;
	bHitchedThisSwing = false;

	const float ClipLen     = Swing ? Swing->GetPlayLength() : AZP_AttackDuration;
	const float WindupClip  = FMath::Clamp(WindupEndClipKnob, 0.f, ClipLen);
	const float HitClip     = FMath::Clamp(HitTimeClipKnob, WindupClip, ClipLen);
	const float WindupRate  = FMath::Max(AZP_WindupPlayRate, 0.05f);
	const float StrikeRate  = FMath::Max(AZP_StrikePlayRate, 0.05f);
	const float WindupReal  = WindupClip / WindupRate;
	const float HitReal     = WindupReal + (HitClip - WindupClip) / StrikeRate;
	CurrentSwingTotalTime   = WindupReal + (ClipLen - WindupClip) / StrikeRate;
	CurrentSwingHitTime     = HitReal;

	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.SetTimer(WindupReleaseTimer, this, &UZP_ShamblerBehaviorComponent::ReleaseSwing, WindupReal, false);
	TM.SetTimer(AttackHitTimer, this, &UZP_ShamblerBehaviorComponent::ApplyAttackDamage, HitReal, false);
}

bool UZP_ShamblerBehaviorComponent::IsProximityAggroValid(APawn* Player)
{
	// SAME-SPACE POSITIVE CONFIRM (round-15, dev 2026-08-05: "BP_Shambler14 aggrod through
	// walls" WITH the propagation veto in place — log-proven dist=550/571). The acoustic
	// veto's failed-path default is LENIENT ("unknown route: don't blind the enemy"), which
	// is right for the sight veto (LOS already passed) and exactly backwards here (no LOS):
	// a wall or shut door makes the nav path FAIL, the veto never fires, aggro passes.
	// Proximity now requires POSITIVE proof of shared space — LOS, or a valid COMPLETE
	// navmesh route that isn't a wall-detour (closed doors carve the dynamic navmesh, so a
	// wall/shut door = no route = no aggro; crouching behind furniture stays a short route).
	if (!Player || !Owner) { return false; }
	if (HasLOS(Player)) { return true; }
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav) { return true; } // no nav system -> keep the unavoidable floor (test maps)
	const ANavigationData* NavData = Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	if (!NavData) { return true; }
	FPathFindingQuery Query(Owner, *NavData, Owner->GetActorLocation(), Player->GetActorLocation());
	Query.SetAllowPartialPaths(false); // partial still means "blocked somewhere" — reject
	const FPathFindingResult Result = Nav->FindPathSync(Query, EPathFindingMode::Regular);
	if (Result.Result != ENavigationQueryResult::Success || !Result.Path.IsValid())
	{
		return false;
	}
	// Route must be same-space SHORT: around furniture is ~1x the straight line, through the
	// hallway around a wall is 2x+. The +300 floor keeps point-blank distances from making
	// the 2x cap degenerate (straight 100 -> cap 400, still a real desk detour).
	const float Straight = FVector::Dist(Owner->GetActorLocation(), Player->GetActorLocation());
	return Result.Path->GetLength() <= FMath::Max(Straight * 2.f, Straight + 300.f);
}

void UZP_ShamblerBehaviorComponent::PlayCrawlAlert()
{
	// Crawler/PlayingDead aggro bark — separate SFX slot for later replacement (dev
	// 2026-08-05); the standard alert vocal is the fallback.
	if (!Owner) { return; }
	if (AZP_CrawlAlertSound)
	{
		UZP_SFXStatics::PlaySFXAttached(AZP_CrawlAlertSound, Owner->GetRootComponent(), EZP_SFXCarry::Far);
	}
	else if (Audio)
	{
		Audio->PlayAlert();
	}
}

void UZP_ShamblerBehaviorComponent::StartSlumpRise()
{
	// Slumped preset wake: rise IN PLACE through SlumpToStand under the ground clamp, then
	// hunt as a NORMAL standing shambler.
	// Window ON BEFORE SetState — its state-entry mesh-Z write must not snap the still-slumped
	// body up to the standing calibration for one beat (dev round-10: "it jolts up and then
	// the animation starts... like it's readjusting"). With the window up, SetState skips the
	// write and the ground clamp owns Z through the whole rise.
	bStandingUpNow = true;
	bCrawlStandWindow = true;
	SetState(EShamblerState::Chase); // Target is set; the Chase eval idles while bStandingUpNow
	SetSpeed(0.f);
	if (AICon) { AICon->StopMovement(); }

	float Lead = 1.0f;
	const float Rate = FMath::Max(AZP_SlumpRiseRate, 0.1f);
	if (AZP_SlumpToStandAnim && Owner)
	{
		ReleaseCombatPoseHold();
		if (USkeletalMeshComponent* M = Owner->GetMesh())
		{
			if (UAnimInstance* AI = M->GetAnimInstance())
			{
				if (SlumpHoldMontage.IsValid() && AI->Montage_IsPlaying(SlumpHoldMontage.Get()))
				{
					AI->Montage_Stop(0.2f, SlumpHoldMontage.Get());
				}
				SlumpHoldMontage = nullptr;
				// LONG blend-out (0.6s): the risen end pose eases into the standing
				// BlendSpace instead of popping.
				StandUpMontage = AI->PlaySlotAnimationAsDynamicMontage(
					AZP_SlumpToStandAnim, FName(TEXT("DefaultSlot")), 0.25f, 0.6f, Rate);
			}
		}
		Lead = FMath::Max(AZP_SlumpToStandAnim->GetPlayLength() / Rate - 0.3f, 0.2f);
	}
	GetWorld()->GetTimerManager().SetTimer(CrawlStandUpTimer, this,
		&UZP_ShamblerBehaviorComponent::FinishSlumpRise, Lead, false);
}

void UZP_ShamblerBehaviorComponent::FinishSlumpRise()
{
	if (bDead) { return; }
	// THE ONE-WAY WAKE: from here on this shambler IS STANDING — every IsDormantSlumped()
	// gate releases and normal standing behaviors own it. Revive resets to the dormant slump.
	bStandingUpNow = false;
	bCrawlStoodUp = true;
	ApplyCrawlCapsule(false); // no-op on the capsule for slumped (never shrunk); restores rot rate
	SetSpeed(State == EShamblerState::Chase ? AZP_ChaseSpeed : AZP_WanderSpeed);
	// Keep the ground clamp through the montage's 0.6s blend-out so the settle into standing
	// locomotion stays planted, then hand mesh Z back to the state machine at the standing
	// calibration.
	GetWorld()->GetTimerManager().SetTimer(CrawlClampReleaseTimer,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			bCrawlStandWindow = false;
			if (Owner && !bDead)
			{
				if (USkeletalMeshComponent* SM = Owner->GetMesh())
				{
					FVector RL = SM->GetRelativeLocation();
					RL.Z = MeshBaseRelZ + AZP_LocoMeshZOffset;
					SM->SetRelativeLocation(RL);
				}
			}
		}), 0.7f, false);
	UE_LOG(LogTemp, Log, TEXT("[Shambler] SLUMP RISE complete — standing shambler from here"));
}

void UZP_ShamblerBehaviorComponent::ReleaseSwing()
{
	// Wind-up over — the strike launches at full speed. If the swing montage was cut (stagger/death
	// flinch replaced it on the slot), there's nothing to retime.
	bSwingReleased = true; // even if the montage is gone: RestoreSwingRate must pick the strike rate
	if (bDead || !Owner || !ActiveSwingMontage.IsValid()) { return; }
	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		if (UAnimInstance* AI = M->GetAnimInstance())
		{
			if (AI->Montage_IsPlaying(ActiveSwingMontage.Get()))
			{
				AI->Montage_SetPlayRate(ActiveSwingMontage.Get(), AZP_StrikePlayRate);
			}
		}
	}
}

void UZP_ShamblerBehaviorComponent::CancelPendingSwing()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(AttackHitTimer);
		W->GetTimerManager().ClearTimer(WindupReleaseTimer);
		W->GetTimerManager().ClearTimer(HitchRestoreTimer);
	}
	// Stop the montage itself — with the restore timer cleared, a mid-hitch swing would otherwise
	// stay frozen at AZP_SwingHitchRate on the slot forever if no follow-up clip replaces it.
	if (ActiveSwingMontage.IsValid() && Owner)
	{
		if (USkeletalMeshComponent* M = Owner->GetMesh())
		{
			if (UAnimInstance* AI = M->GetAnimInstance())
			{
				if (AI->Montage_IsPlaying(ActiveSwingMontage.Get()))
				{
					AI->Montage_Stop(0.1f, ActiveSwingMontage.Get());
				}
			}
		}
	}
	ActiveSwingMontage = nullptr;
}

void UZP_ShamblerBehaviorComponent::ApplyAttackDamage()
{
	if (bDead || !Owner || !Target) { return; }
	// Must still be close and visible at the moment of impact — back-stepping out of hit range
	// during the wind-up makes the swing whiff (no hitting through walls / after the player
	// ran). Prone claws use their own shorter reach (round-24).
	const float HitRange = IsCrawlingNow() ? AZP_CrawlAttackHitRange : AZP_AttackHitRange;
	if (FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation()) > HitRange) { return; }
	if (!HasLOS(Target)) { return; }

	AController* Inst = Owner->GetController();
	UGameplayStatics::ApplyDamage(Target, AZP_AttackDamage, Inst, Owner, nullptr);
}

// ───────────────────────── grab / struggle (Docs/Plan_GrabStruggle.md) ─────────────────────────

void UZP_ShamblerBehaviorComponent::TryStartGrab()
{
	if (!Owner || !Target) { return; }
	IZP_Grabbable* Victim = Cast<IZP_Grabbable>(Target.Get());
	if (!Victim) { return; }

	const double Now = GetWorld()->GetTimeSeconds();
	// [LatchProbe] shambler-side snapshot at the latch attempt: what was this body doing?
	// dtHit = seconds since it last took a hit reaction ("shambler has been hit" trigger).
	{
		UAnimInstance* ProbeAI = Owner->GetMesh() ? Owner->GetMesh()->GetAnimInstance() : nullptr;
		UAnimMontage* ProbeCur = ProbeAI ? ProbeAI->GetCurrentActiveMontage() : nullptr;
		UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] t=%.2f SHAMBLER TryStartGrab: state=%d staggered=%d slot='%s'%s dtHit=%.2f dist=%.0f hp=%.0f"),
			Now, (int32)State, bStaggered ? 1 : 0,
			ProbeCur ? *ProbeCur->GetName() : TEXT("EMPTY"),
			(ProbeCur && ProbeAI && !ProbeAI->Montage_IsPlaying(ProbeCur)) ? TEXT("(PAUSED)") : TEXT(""),
			(float)(Now - LastHitReactTime),
			FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation()),
			Health ? Health->CurrentHealth : -1.f);
	}
	const EZP_GrabAttemptResult Result = Victim->TryBeginGrab(Owner);

	// FAILED attempts (either flavor) pay only AZP_GrabFailCooldown before the next try — the full
	// AZP_GrabCooldown is reserved for LANDED grabs (dev 2026-07-03). Back-dating LastGrabTime by
	// (AZP_GrabCooldown - fail) makes the standard (Now - LastGrabTime) >= AZP_GrabCooldown gate expire
	// in exactly fail seconds; clamped so a fail can never out-cool a landed grab.
	const double FailCD = FMath::Clamp(AZP_GrabFailCooldown, 0.f, AZP_GrabCooldown);
	if (Result == EZP_GrabAttemptResult::Deflected)
	{
		// The guard turned the grab away: fail cooldown + the block-reward stagger.
		LastGrabTime = Now - AZP_GrabCooldown + FailCD;
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] GRAB DEFLECTED (player blocking) — staggering, next try in %.1fs"), FailCD);
		ReceiveStaggerHit(AZP_DeflectStaggerDuration);
		return;
	}
	if (Result == EZP_GrabAttemptResult::Unavailable)
	{
		// Immunity window / mid-dodge / menus — fail cooldown, keep swinging meanwhile.
		LastGrabTime = Now - AZP_GrabCooldown + FailCD;
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] GRAB EVADED (immunity/dodge/menu) — next try in %.1fs"), FailCD);
		return;
	}

	// GRABBED — the victim froze itself and snapped to face us. Kill any in-flight swing, normalize
	// the authored pair spacing, face the victim, freeze, and play the entry clip. The victim's
	// phase machine drives everything from here via OnVictimGrabPhase.
	LastGrabTime = Now;
	CancelPendingSwing();
	bEscapePushback = false; // a stale pushback lerp would fight the grab hold
	GetWorld()->GetTimerManager().ClearTimer(EscapePushbackDelayTimer);

	FVector ToVictim = Target->GetActorLocation() - Owner->GetActorLocation();
	ToVictim.Z = 0.f;
	const FVector Dir = ToVictim.GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		FVector NewLoc = Target->GetActorLocation() - Dir * AZP_GrabPairDistance;
		NewLoc.Z = Owner->GetActorLocation().Z;
		if (AZP_GrabSnapInDuration > 0.f)
		{
			// LUNGE the last stretch into pair spacing instead of teleporting — grabbing from
			// max reach (AZP_GrabRange) popped the body ~160uu in one frame (dev 2026-07-03:
			// "latches on from the farthest reach... jerks"). Victim froze in TryBeginGrab, so
			// the captured target point is static; TickComponent drives the ease-out slide.
			GrabSnapFrom = Owner->GetActorLocation();
			GrabSnapTo = NewLoc;
			GrabSnapStart = GetWorld()->GetTimeSeconds();
			bGrabSnapIn = true;
		}
		else
		{
			Owner->SetActorLocation(NewLoc, /*bSweep*/false); // knob at 0 = old instant snap
		}
		Owner->SetActorRotation(FRotator(0.f, Dir.Rotation().Yaw, 0.f)); // snap-face ONCE (scream pattern)
	}
	if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent())
	{
		Cap->IgnoreActorWhenMoving(Target.Get(), true); // victim ignores us from its own side
	}

	SetState(EShamblerState::Grab);
	if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
	{
		CM->StopMovementImmediately();
		CM->SetMovementMode(MOVE_None); // AFTER SetState (SetState restores Walking from MOVE_None)
	}
	// Latch alert sting — one-shot, fires exactly as the grab lands (dev 2026-07-03). Far carry:
	// it's an alert-class vocal, audible like the scream.
	if (AZP_GrabAlertSound)
	{
		UZP_SFXStatics::PlaySFXAttached(AZP_GrabAlertSound, Owner->GetMesh(), EZP_SFXCarry::Far);
	}
	// Grapple snarl: looping wave attached to the body for the whole hold — hard-cut on any
	// release path (EndGrabOnShambler / death). Replaces the one-shot attack bark here.
	if (AZP_GrabLoopSound)
	{
		GrabLoopAudio = UZP_SFXStatics::PlaySFXAttached(AZP_GrabLoopSound, Owner->GetMesh(), EZP_SFXCarry::Close);
	}
	// CLEAN SLATE at latch: a pipe hit landing just before the grab leaves its cosmetic layers on
	// the body — the flinch clip blending out UNDER the entry and the mesh hit-punch offset still
	// decaying — and the paired entry then reads with the arm/head knobs off ("only the grabs that
	// happened when not swinging looked correct", dev 2026-07-03). The grab owns the body from
	// here: zero the punch, reset the mesh XY, and stop ALL in-flight montages with the entry's
	// own blend so every latch starts from the same base pose.
	MeshPunch = FVector2D::ZeroVector;
	if (USkeletalMeshComponent* GrabMesh = Owner->GetMesh())
	{
		FVector RL = GrabMesh->GetRelativeLocation();
		RL.X = MeshBaseRelXY.X;
		RL.Y = MeshBaseRelXY.Y;
		GrabMesh->SetRelativeLocation(RL);
		if (UAnimInstance* GrabAI = GrabMesh->GetAnimInstance())
		{
			ReleaseCombatPoseHold(); // a PAUSED hold must resume first or the stop's blend-out may not tick
			GrabAI->Montage_Stop(0.1f); // blend-out matches the entry's 0.1 blend-in — no snap
		}
	}
	PlayOneShot(AZP_GrabEntryAnim);
	LastLatchTime = Now; // [LatchProbe] anchor — damage/stagger probes log their dt vs this
	UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] t=%.2f SHAMBLER LATCHED — entry clip playing"), Now);
	UE_LOG(LogTemp, Warning, TEXT("[Shambler] GRAB latched"));

	// [LatchProbe] SWIN — 2s post-latch window, 0.1s samples: what the SHAMBLER side is doing.
	// 'slot' shows the montage actually on the mesh (entry -> munch expected); zOff shows the
	// AZP_GrabPairZOffset seat; drift/yaw catch any post-snap slide or facing twist.
	LatchWindowOrigin = Owner->GetActorLocation();
	GetWorld()->GetTimerManager().ClearTimer(LatchWindowProbeTimer);
	GetWorld()->GetTimerManager().SetTimer(LatchWindowProbeTimer,
		FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (!Owner)
		{
			GetWorld()->GetTimerManager().ClearTimer(LatchWindowProbeTimer);
			return;
		}
		const double Dt = GetWorld()->GetTimeSeconds() - LastLatchTime;
		if (Dt > 2.0)
		{
			GetWorld()->GetTimerManager().ClearTimer(LatchWindowProbeTimer);
			return;
		}
		USkeletalMeshComponent* M = Owner->GetMesh();
		UAnimInstance* AI = M ? M->GetAnimInstance() : nullptr;
		UAnimMontage* Cur = AI ? AI->GetCurrentActiveMontage() : nullptr;
		const UCharacterMovementComponent* CM = Owner->GetCharacterMovement();
		UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] SWIN dt=%.1f state=%d drift=%.0f dist=%.0f vel=%.0f mode=%d slot='%s'%s zOff=%.1f yaw=%.0f"),
			Dt, (int32)State,
			FVector::Dist2D(Owner->GetActorLocation(), LatchWindowOrigin),
			Target ? FVector::Dist2D(Owner->GetActorLocation(), Target->GetActorLocation()) : -1.f,
			Owner->GetVelocity().Size(),
			CM ? (int32)CM->MovementMode.GetValue() : -1,
			Cur ? *Cur->GetName() : TEXT("EMPTY"),
			(Cur && AI && !AI->Montage_IsPlaying(Cur)) ? TEXT("(PAUSED)") : TEXT(""),
			M ? (M->GetRelativeLocation().Z - MeshBaseRelZ) : 0.f,
			Owner->GetActorRotation().Yaw);
	}), 0.1f, /*bLoop*/true);
}

void UZP_ShamblerBehaviorComponent::OnVictimGrabPhase(EZP_GrabPhase NewPhase)
{
	if (!Owner) { return; }
	UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] t=%.2f SHAMBLER got victim phase %d (state=%d dead=%d dtLatch=%.2f)"),
		GetWorld()->GetTimeSeconds(), (int32)NewPhase, (int32)State, bDead ? 1 : 0,
		(float)(GetWorld()->GetTimeSeconds() - LastLatchTime));

	// Dead grabber (grenade etc. mid-grab): the death path owns the body — only drop the pair
	// collision ignore and cut the snarl loop, never touch movement/state.
	if (bDead)
	{
		if (GrabLoopAudio) { GrabLoopAudio->Stop(); GrabLoopAudio = nullptr; }
		if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent())
		{
			if (Target) { Cap->IgnoreActorWhenMoving(Target.Get(), false); }
		}
		return;
	}

	switch (NewPhase)
	{
	case EZP_GrabPhase::Munch:
		PlaySlotLoop(AZP_GrabMunchAnim);
		break;

	case EZP_GrabPhase::Wrestle:
		PlaySlotLoop(AZP_GrabWrestleAnim);
		break;

	case EZP_GrabPhase::EscapeKick:
	case EZP_GrabPhase::EscapePush:
	{
		// Kicked/pushed off: paired reaction clip + programmatic knockback (no root motion in the
		// pack) + the escape-reward stun window. Punish window = max(stun, clip) so the zombie
		// never resumes acting mid-reaction.
		StopSlotLoop();
		// Contact knob: re-snap the pair spacing the instant the escape begins so the push/kick
		// arms actually reach the body (dev report: "too far away from arms to physically push").
		// Runs while collision is still mutually ignored — safe to slide the capsule in.
		if (AZP_GrabEscapeSnapDistance > 0.f && Target)
		{
			FVector ToVictim = Target->GetActorLocation() - Owner->GetActorLocation();
			ToVictim.Z = 0.f;
			const FVector SnapDir = ToVictim.GetSafeNormal();
			if (!SnapDir.IsNearlyZero())
			{
				FVector SnapLoc = Target->GetActorLocation() - SnapDir * AZP_GrabEscapeSnapDistance;
				SnapLoc.Z = Owner->GetActorLocation().Z;
				Owner->SetActorLocation(SnapLoc, /*bSweep*/false);
			}
		}
		EndGrabOnShambler(/*bResumeChase*/true);
		// SetState(Chase) just re-enabled bOrientRotationToMovement — with the backward launch
		// the CMC then ROTATES the body to face its slide direction (the "rotating/sliding
		// around", dev report). Keep facing locked through the reaction; the stagger-end
		// release restores per-state orientation.
		if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
		{
			CM->bOrientRotationToMovement = false;
		}
		UAnimSequence* Reaction = (NewPhase == EZP_GrabPhase::EscapeKick) ? AZP_GrabKickedAnim : AZP_GrabPushedAnim;
		PlayOneShot(Reaction);
		// Knockback REBUILT (dev report: "the push removes the offset placed for the grab, so the
		// shambler slides unnaturally away"). The old LaunchCharacter(450) glide started the same
		// frame as the reaction clip — the pair spacing broke before the contact even read, then
		// the body coasted on CMC friction. Now: zero residual velocity, HOLD the grapple spacing
		// through the contact beat (AZP_EscapePushbackDelay), then a short eased slide-to-stop to
		// AZP_EscapePushbackDistance (TickComponent drives it) — deterministic, ends planted.
		if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
		{
			CM->StopMovementImmediately();
		}
		GetWorld()->GetTimerManager().ClearTimer(EscapePushbackDelayTimer);
		GetWorld()->GetTimerManager().SetTimer(EscapePushbackDelayTimer,
			FTimerDelegate::CreateWeakLambda(this, [this]() { StartEscapePushback(); }),
			FMath::Max(0.01f, AZP_EscapePushbackDelay), false);
		UE_LOG(LogTemp, Warning, TEXT("[GrabProbe] escape (%s): holding pair spacing %.2fs, then pushback to %.0fuu over %.2fs (distToVictim now %.0f)"),
			NewPhase == EZP_GrabPhase::EscapeKick ? TEXT("kick") : TEXT("push"),
			AZP_EscapePushbackDelay, AZP_EscapePushbackDistance, AZP_EscapePushbackDuration,
			Target ? FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation()) : -1.f);
		const float ClipLen = Reaction ? Reaction->GetPlayLength() : 2.3f;
		// NO idle fill here anymore (dev 2026-07-03: no idle in combat, ever) — the Tick pose-hold
		// freezes the reaction clip's final frame through the remainder of the stun instead.
		PauseAIWithoutFlinch(FMath::Max(AZP_EscapeStunDuration, ClipLen));
		break;
	}

	case EZP_GrabPhase::FailKnockdown:
	{
		// The victim failed the struggle — the zombie stands over the downed player and does the
		// FULL aggro alert (scream SFX + scream clip), dev spec 2026-07-03. SetState(Scream) IS
		// that combo verbatim: snap-face the (downed) target, PlayAlert, scream clip, scream
		// mesh-Z nudge, speed 0 + StopMovement, orientation locked (non-Chase state). It replaces
		// the earlier swipe read. (The pack's paired Zombie_Grab_To_TakeDown stays unused — its
		// dive carries no pelvis translation through the necromorph retarget and floats in air,
		// see DEAD ENDS 2026-07-02.) Visual only — no damage timers.
		StopSlotLoop();
		EndGrabOnShambler(/*bResumeChase*/true);
		SetState(EShamblerState::Scream);
		// Post-loom the scream state must not hold an EXTRA AZP_ScreamHoldTime: Evaluate is frozen
		// through the whole loom (bStaggered), so StateTimer only starts once the player is back
		// up — exit to Chase on the first eval after release.
		CurrentScreamHold = 0.1f;
		const float ScreamLen = AZP_ScreamAnim ? AZP_ScreamAnim->GetPlayLength() : 2.f;
		ScheduleGrabIdleFill(ScreamLen);
		// It knocked the victim down — it LOOMS: idle in place until they're back on their
		// feet (no pathing circles around the downed body), then the chase resumes.
		BeginWaitForVictimUp(ScreamLen);
		break;
	}

	case EZP_GrabPhase::None:
	default:
		// Aborted (victim died, or the grab was broken by damage): clean release, no outcome anims.
		StopSlotLoop();
		EndGrabOnShambler(/*bResumeChase*/true);
		break;
	}
}

void UZP_ShamblerBehaviorComponent::EndGrabOnShambler(bool bResumeChase)
{
	if (!Owner) { return; }
	bGrabSnapIn = false; // a grab ending mid-lunge must not keep sliding the body
	// Hard-cut the grapple snarl the instant the grab ends (dev spec: no fade, no tail).
	if (GrabLoopAudio) { GrabLoopAudio->Stop(); GrabLoopAudio = nullptr; }
	// Collision restore is DEFERRED until the capsules are clear of each other — restoring while
	// interpenetrated (AZP_GrabPairDistance < sum of radii) makes the engine depenetrate: the random
	// shove/slide/orbit at release (dev report 2026-07-02).
	DeferCollisionRestore();
	if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
	{
		if (CM->MovementMode == MOVE_None) { CM->SetMovementMode(MOVE_Walking); }
	}
	// PROBE: geometry at the moment of release — overlap depth is the depenetration budget.
	if (Target)
	{
		const float Dist = FVector::Dist2D(Owner->GetActorLocation(), Target->GetActorLocation());
		float SumR = 0.f;
		if (UCapsuleComponent* MyCap = Owner->GetCapsuleComponent()) { SumR += MyCap->GetScaledCapsuleRadius(); }
		if (const ACharacter* TC = Cast<ACharacter>(Target.Get()))
		{
			if (const UCapsuleComponent* VC = TC->GetCapsuleComponent()) { SumR += VC->GetScaledCapsuleRadius(); }
		}
		UE_LOG(LogTemp, Warning, TEXT("[GrabProbe] release: dist2D=%.0f sumRadii=%.0f overlap=%.0f vel=%.0f"),
			Dist, SumR, FMath::Max(0.f, SumR - Dist), Owner->GetVelocity().Size());
	}
	StartGrabSlideProbe();
	SetState((bResumeChase && Target) ? EShamblerState::Chase : EShamblerState::Wander);
}

void UZP_ShamblerBehaviorComponent::StartGrabSlideProbe()
{
	// CONTINUOUS post-release motion trace (dev request): every 0.1s for 6s, log how far the
	// body has moved from the release spot and everything that could be moving it. Any slide
	// shows as growing drift with the culprit named on the same line.
	SlideProbeOrigin = Owner->GetActorLocation();
	SlideProbeStart = GetWorld()->GetTimeSeconds();
	if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GrabSlideProbe] START origin=(%.0f,%.0f) RVO=%d orient=%d"),
			SlideProbeOrigin.X, SlideProbeOrigin.Y,
			CM->bUseRVOAvoidance ? 1 : 0, CM->bOrientRotationToMovement ? 1 : 0);
	}
	GetWorld()->GetTimerManager().ClearTimer(GrabSlideProbeTimer);
	GetWorld()->GetTimerManager().SetTimer(GrabSlideProbeTimer,
		FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (!Owner || bDead)
		{
			GetWorld()->GetTimerManager().ClearTimer(GrabSlideProbeTimer);
			return;
		}
		const double Elapsed = GetWorld()->GetTimeSeconds() - SlideProbeStart;
		if (Elapsed > 6.0)
		{
			GetWorld()->GetTimerManager().ClearTimer(GrabSlideProbeTimer);
			UE_LOG(LogTemp, Warning, TEXT("[GrabSlideProbe] END total drift=%.0fuu"),
				FVector::Dist2D(Owner->GetActorLocation(), SlideProbeOrigin));
			return;
		}
		UCharacterMovementComponent* CM = Owner->GetCharacterMovement();
		UCapsuleComponent* Cap = Owner->GetCapsuleComponent();
		const bool bIgnoring = (Cap && Target)
			? Cap->GetMoveIgnoreActors().Contains(Target.Get()) : false;
		UE_LOG(LogTemp, Warning,
			TEXT("[GrabSlideProbe] t=%.1f drift=%.0f vel=%.0f mode=%d yaw=%.0f staggered=%d moveStatus=%d distToPlayer=%.0f ignoringPlayer=%d"),
			Elapsed,
			FVector::Dist2D(Owner->GetActorLocation(), SlideProbeOrigin),
			Owner->GetVelocity().Size(),
			CM ? (int32)CM->MovementMode.GetValue() : -1,
			Owner->GetActorRotation().Yaw,
			bStaggered ? 1 : 0,
			AICon ? (int32)AICon->GetMoveStatus() : -1,
			Target ? FVector::Dist2D(Owner->GetActorLocation(), Target->GetActorLocation()) : -1.f,
			bIgnoring ? 1 : 0);
	}), 0.1f, /*bLoop*/true);
}

void UZP_ShamblerBehaviorComponent::StartEscapePushback()
{
	if (!Owner || bDead) { return; }
	EscapePushbackFrom = Owner->GetActorLocation();
	// Push direction: straight away from the victim (facing is locked on them through the
	// reaction, so this is the reaction clip's backward axis). Falls back to actor-back if the
	// victim is gone mid-escape.
	FVector Away = -Owner->GetActorForwardVector();
	if (Target)
	{
		FVector FromVictim = EscapePushbackFrom - Target->GetActorLocation();
		FromVictim.Z = 0.f;
		if (!FromVictim.IsNearlyZero()) { Away = FromVictim.GetSafeNormal(); }
	}
	Away.Z = 0.f;
	Away = Away.GetSafeNormal();
	// PURE DISPLACEMENT, relative to our own position — always backward, never anchored on the
	// player (an end-spacing target could pull the body TOWARD a retreating player, and a large
	// travel is exactly the "slides across the room" the dev rejected). ~45uu = the stumble step.
	EscapePushbackTo = EscapePushbackFrom + Away * FMath::Max(0.f, AZP_EscapePushbackDistance);
	EscapePushbackTo.Z = EscapePushbackFrom.Z;
	EscapePushbackStart = GetWorld()->GetTimeSeconds();
	bEscapePushback = true;
	UE_LOG(LogTemp, Warning, TEXT("[GrabProbe] pushback START from=(%.0f,%.0f) to=(%.0f,%.0f) dist=%.0f"),
		EscapePushbackFrom.X, EscapePushbackFrom.Y, EscapePushbackTo.X, EscapePushbackTo.Y,
		FVector::Dist2D(EscapePushbackFrom, EscapePushbackTo));
}

void UZP_ShamblerBehaviorComponent::DeferCollisionRestore()
{
	TWeakObjectPtr<AActor> Victim = Target;
	if (!Victim.IsValid())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(CollisionRestoreTimer);
	const double StartTime = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(CollisionRestoreTimer,
		FTimerDelegate::CreateWeakLambda(this, [this, Victim, StartTime]()
	{
		AActor* V = Victim.Get();
		UCapsuleComponent* MyCap = Owner ? Owner->GetCapsuleComponent() : nullptr;
		if (!V || !MyCap)
		{
			GetWorld()->GetTimerManager().ClearTimer(CollisionRestoreTimer);
			return;
		}
		float SumR = MyCap->GetScaledCapsuleRadius();
		if (const ACharacter* TC = Cast<ACharacter>(V))
		{
			if (const UCapsuleComponent* VC = TC->GetCapsuleComponent()) { SumR += VC->GetScaledCapsuleRadius(); }
		}
		const float Dist = FVector::Dist2D(Owner->GetActorLocation(), V->GetActorLocation());
		const double Elapsed = GetWorld()->GetTimeSeconds() - StartTime;
		// Restore ONLY once actually clear. The old 4s failsafe fired MID-LOOM (the wait over a
		// downed player — ~3.9s since the GetUp phase was cut) and restored while overlapped — the engine then
		// depenetrated the pair: the very slide this deferral exists to prevent. 15s = abandon
		// point for genuinely wedged cases; logged loudly so it shows next to the slide probe.
		const bool bClear = Dist > SumR + 10.f;
		if (bClear || Elapsed > 15.0)
		{
			MyCap->IgnoreActorWhenMoving(V, false);
			GetWorld()->GetTimerManager().ClearTimer(CollisionRestoreTimer);
			if (bClear)
			{
				UE_LOG(LogTemp, Warning, TEXT("[GrabProbe] collision restored CLEAR after %.2fs (dist2D=%.0f sumRadii=%.0f)"),
					Elapsed, Dist, SumR);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[GrabProbe] collision FORCE-restored still-overlapped at 15s (dist2D=%.0f sumRadii=%.0f) — expect a depenetration shove"),
					Dist, SumR);
			}
		}
	}), 0.2f, /*bLoop*/true);
}

void UZP_ShamblerBehaviorComponent::BreakGrabFromDamage()
{
	if (State != EShamblerState::Grab || !Target) { return; }
	UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] t=%.2f GRAB BROKEN BY DAMAGE — dtLatch=%.2fs (the causer is the DAMAGE line directly above)"),
		GetWorld()->GetTimeSeconds(), (float)(GetWorld()->GetTimeSeconds() - LastLatchTime));
	UE_LOG(LogTemp, Warning, TEXT("[Shambler] GRAB broken by damage"));
	if (IZP_Grabbable* Victim = Cast<IZP_Grabbable>(Target.Get()))
	{
		// The victim's AbortGrab calls back OnVictimGrabPhase(None), which restores our state.
		Victim->AbortGrab();
	}
	else
	{
		EndGrabOnShambler(true);
	}
}

void UZP_ShamblerBehaviorComponent::PauseAIWithoutFlinch(float Duration)
{
	bStaggered = true; // freezes Evaluate exactly like a stagger, but the slot keeps its clip
	GetWorld()->GetTimerManager().ClearTimer(StaggerHandle);
	GetWorld()->GetTimerManager().SetTimer(StaggerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bStaggered = false;
		StopSlotLoop(); // release the grab idle-fill (if queued) so locomotion takes back over
		if (!bDead)
		{
			EnsureLocomotion();
			// Restore per-state orientation (the grab-reaction windows lock it off so the
			// backward launch / depenetration can't rotate the body while it slides).
			if (Owner)
			{
				if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
				{
					CM->bOrientRotationToMovement = (State == EShamblerState::Wander || State == EShamblerState::Chase);
				}
			}
		}
	}), FMath::Max(0.1f, Duration), false);
}

void UZP_ShamblerBehaviorComponent::ScheduleGrabIdleFill(float ReactionClipLen)
{
	GetWorld()->GetTimerManager().ClearTimer(GrabIdleFillTimer);
	GetWorld()->GetTimerManager().SetTimer(GrabIdleFillTimer,
		FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (!bDead && bStaggered && AZP_IdleAnim) { PlaySlotLoop(AZP_IdleAnim); }
	}), FMath::Max(0.1f, ReactionClipLen - 0.15f), false);
}

void UZP_ShamblerBehaviorComponent::BeginWaitForVictimUp(float MinWait)
{
	bStaggered = true; // freezes Evaluate — the wait owns the release, not the fixed stagger timer
	GetWorld()->GetTimerManager().ClearTimer(StaggerHandle);
	GetWorld()->GetTimerManager().ClearTimer(VictimUpWaitTimer);
	const double StartTime = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(VictimUpWaitTimer,
		FTimerDelegate::CreateWeakLambda(this, [this, StartTime, MinWait]()
	{
		if (bDead)
		{
			GetWorld()->GetTimerManager().ClearTimer(VictimUpWaitTimer);
			return;
		}
		const double Elapsed = GetWorld()->GetTimeSeconds() - StartTime;
		if (Elapsed < MinWait) { return; } // let the swipe reaction play out first

		bool bVictimDown = false;
		if (IZP_Grabbable* Victim = Cast<IZP_Grabbable>(Target.Get()))
		{
			bVictimDown = Victim->IsGrabRecovering();
		}
		// Release when they're up — or after a hard 12s failsafe (never wait forever on a
		// stale/dead target).
		if (!bVictimDown || Elapsed > 12.0)
		{
			GetWorld()->GetTimerManager().ClearTimer(VictimUpWaitTimer);
			bStaggered = false;
			StopSlotLoop();
			EnsureLocomotion();
			// Restore per-state orientation (locked off through the loom).
			if (Owner)
			{
				if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
				{
					CM->bOrientRotationToMovement = (State == EShamblerState::Wander || State == EShamblerState::Chase);
				}
			}
		}
	}), 0.25f, /*bLoop*/true);
}

// ───────────────────────── helpers ─────────────────────────

APawn* UZP_ShamblerBehaviorComponent::GetPlayer() const
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		return PC->GetPawn();
	}
	return nullptr;
}

bool UZP_ShamblerBehaviorComponent::HasLOS(const AActor* InTarget) const
{
	if (!InTarget || !Owner || !GetWorld()) { return false; }
	const FVector Start = Owner->GetActorLocation() + FVector(0, 0, 60.f);
	const FVector End = InTarget->GetActorLocation() + FVector(0, 0, 40.f);
	FCollisionQueryParams P;
	P.AddIgnoredActor(Owner);
	P.AddIgnoredActor(InTarget);
	// Ignore both actors' attached actors (weapon meshes, child actors) so they can't false-block.
	TArray<AActor*> Att;
	InTarget->GetAttachedActors(Att);
	for (AActor* A : Att) { P.AddIgnoredActor(A); }
	Owner->GetAttachedActors(Att);
	for (AActor* A : Att) { P.AddIgnoredActor(A); }

	// Walk the sightline, IGNORING pure trigger volumes (door interaction boxes, overlap zones) and
	// re-tracing past each one until we hit SOLID geometry. A single multi-trace can't do this — it
	// stops dead at the first blocker, and a door's InteractionVolume BLOCKS WorldStatic right where
	// the zombie stands (@0), so the trace would die on the trigger and never reach the wall behind it.
	auto DirClear = [this](FVector A, FVector B, FCollisionQueryParams Params) -> bool
	{
		for (int32 Iter = 0; Iter < 8; ++Iter)
		{
			FHitResult Hit;
			if (!GetWorld()->LineTraceSingleByChannel(Hit, A, B, ECC_WorldStatic, Params))
			{
				return true; // nothing solid left on the line -> clear this direction
			}
			UPrimitiveComponent* Comp = Hit.GetComponent();
			// Only SKIP genuine trigger/interaction volumes — query-only SHAPE components. A wall
			// or (query-only) DOOR MESH is a UStaticMeshComponent, so it correctly BLOCKS.
			if (Comp && Comp->GetCollisionEnabled() == ECollisionEnabled::QueryOnly && Comp->IsA<UShapeComponent>())
			{
				Params.AddIgnoredComponent(Comp); // trigger volume -> see through it, re-trace
				continue;
			}
			return false;
		}
		return false;
	};

	// BOTH directions must be clear. Complex-as-simple meshes (this project's wall collision) do
	// NOT hit BACKFACES: a body embedded near/inside a wall traces outward through the wall's back
	// faces without a hit — "sees" and ATTACKS through the wall (dev report: BP_Shambler3 spawned
	// against a wall). The reverse trace hits the wall's front face and correctly blocks.
	return DirClear(Start, End, P) && DirClear(End, Start, P);
}

bool UZP_ShamblerBehaviorComponent::PickNewWanderPoint()
{
	if (!Owner) { return false; }
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav) { return false; }
	// Target-dot + spawn-tether picker. Picks the candidate whose direction from the body is CLOSEST
	// to AZP_WanderTargetDot relative to forward — default 0.5 (~60° off forward) gives curving paths
	// instead of the highest-dot picker which produced straight forward lines / circles. Anchored
	// around SpawnLocation so the body stays in its leash.
	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const FVector Anchor = SpawnLocation.IsNearlyZero() ? Origin : SpawnLocation;

	FVector BestLoc = FVector::ZeroVector;
	float BestErr = TNumericLimits<float>::Max();
	const float MinLegSq = AZP_WanderMinLegDistance * AZP_WanderMinLegDistance;

	for (int32 i = 0; i < 12; ++i)
	{
		FNavLocation R;
		if (!Nav->GetRandomReachablePointInRadius(Anchor, AZP_WanderRadius, R)) { continue; }
		FVector ToCandidate = R.Location - Origin;
		ToCandidate.Z = 0.f;
		if (ToCandidate.SizeSquared() < MinLegSq) { continue; } // too close, would end instantly
		const FVector Dir = ToCandidate.GetSafeNormal();
		const float Dot = FVector::DotProduct(Forward, Dir);
		const float Err = FMath::Abs(Dot - AZP_WanderTargetDot);
		if (Err < BestErr)
		{
			BestErr = Err;
			BestLoc = R.Location;
		}
	}
	if (BestErr < TNumericLimits<float>::Max())
	{
		WanderDest = BestLoc;
		return true;
	}
	return false;
}

void UZP_ShamblerBehaviorComponent::StartWanderLeg()
{
	if (AICon)
	{
		AICon->MoveToLocation(WanderDest, AZP_WanderAcceptRadius); // accept "close enough" so it doesn't orbit the exact point
	}
}

void UZP_ShamblerBehaviorComponent::SetSpeed(float Speed)
{
	if (Owner)
	{
		if (UCharacterMovementComponent* M = Owner->GetCharacterMovement())
		{
			M->MaxWalkSpeed = Speed;
		}
	}
}

void UZP_ShamblerBehaviorComponent::StartScreamPhase()
{
	bAlertTurning = false;
	if (Owner)
	{
		// Face the player ONCE, instantly, and hold — per-frame tracking during the scream reads
		// as a twist when the player strafes (dev report 2026-07-02). After an alert turn the
		// residual is a few degrees, so this snap is invisible.
		AActor* FaceTarget = Target ? Target.Get() : Cast<AActor>(GetPlayer());
		if (FaceTarget)
		{
			FVector To = FaceTarget->GetActorLocation() - Owner->GetActorLocation();
			To.Z = 0.f;
			if (!To.IsNearlyZero())
			{
				Owner->SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
			}
		}
		// ScreamPinned's legs are the frozen idle stance — apply its ground nudge now (the turn
		// phase ran at StateZ 0 because its legs really step).
		if (USkeletalMeshComponent* SM = Owner->GetMesh())
		{
			FVector RL = SM->GetRelativeLocation();
			RL.Z = MeshBaseRelZ + AZP_ScreamMeshZOffset;
			SM->SetRelativeLocation(RL);
		}
	}
	if (Audio) { Audio->PlayAlert(); }
	AlertMontage = PlayOneShot(AZP_ScreamAnim);
	StateTimer = 0.f; // the scream hold counts from HERE — the turn never eats the wail
}

void UZP_ShamblerBehaviorComponent::StopAlertMontage(float BlendOut)
{
	bAlertTurning = false;
	if (Owner && AlertMontage)
	{
		if (USkeletalMeshComponent* M = Owner->GetMesh())
		{
			if (UAnimInstance* AI = M->GetAnimInstance())
			{
				if (AI->Montage_IsActive(AlertMontage))
				{
					// A pose-held (paused) montage may refuse to blend out — the release resumes
					// it so the blend-out ticks (KB pose-hold rule, 2026-07-03/05c).
					ReleaseCombatPoseHold();
					AI->Montage_Stop(BlendOut, AlertMontage);
				}
			}
		}
	}
	AlertMontage = nullptr;
}

UAnimMontage* UZP_ShamblerBehaviorComponent::PlayOneShot(UAnimSequence* Anim, float PlayRate)
{
	if (!Owner || !Anim) { return nullptr; }
	USkeletalMeshComponent* M = Owner->GetMesh();
	if (!M) { return nullptr; }
	// Slot-based playback: ABP_Shambler's AnimGraph now wires BS_Shambler -> Slot 'DefaultSlot' ->
	// Output Pose. PlaySlotAnimationAsDynamicMontage layers Anim on top of the locomotion via the
	// slot, with a short blend-in/blend-out. No mesh mode swap, no pose snap — the visible "jiggle"
	// between Wander/Chase and Scream/Attack/Hit is gone.
	if (UAnimInstance* AI = M->GetAnimInstance())
	{
		// A paused pose-hold must be resumed before the new clip lands so its blend-out ticks
		// under the new clip's blend-in (crossfade, not a pop).
		ReleaseCombatPoseHold();
		// NO hard StopSlotAnimation(0) first — montages in the same slot group auto-interrupt with
		// the NEW montage's blend-in, a proper clip-to-clip crossfade. The old hard-cut zeroed the
		// slot for the blend-in window, so every handoff SNAPPED through naked BS_Shambler@speed-0
		// ("the arms shift, every time" on grab Entry->Munch, dev 2026-07-03).
		return AI->PlaySlotAnimationAsDynamicMontage(Anim, FName(TEXT("DefaultSlot")),
			/*BlendIn=*/0.1f, /*BlendOut=*/0.15f, PlayRate);
	}
	return nullptr;
}

UAnimMontage* UZP_ShamblerBehaviorComponent::PlaySlotLoop(UAnimSequence* Anim, float PlayRate)
{
	if (!Owner || !Anim) { return nullptr; }
	USkeletalMeshComponent* M = Owner->GetMesh();
	if (!M) { return nullptr; }
	if (UAnimInstance* AI = M->GetAnimInstance())
	{
		// Same pose-hold release as PlayOneShot — the new loop crossfades over the resumed tail.
		ReleaseCombatPoseHold();
		// AZP_IdleBlendInTime covers the CMC's natural braking ramp — long enough that velocity has
		// dropped to ~0 by the time the slot is fully visible, so no walk pose leaks through.
		// No hard-cut of the prior montage: same-group montages crossfade via this blend-in
		// (the old StopSlotAnimation(0) snapped every handoff through BS@0 — the grab arm shift).
		return AI->PlaySlotAnimationAsDynamicMontage(Anim, FName(TEXT("DefaultSlot")),
			/*BlendInTime=*/AZP_IdleBlendInTime, /*BlendOutTime=*/AZP_IdleBlendOutTime, PlayRate,
			/*LoopCount=*/INT32_MAX, /*BlendOutTriggerTime=*/-1.f, /*InTimeToStartMontageAt=*/0.f);
	}
	return nullptr;
}

void UZP_ShamblerBehaviorComponent::StopRunChase()
{
	if (!bRunningChase) { return; }
	bRunningChase = false;
	bRunBurstNow = false; // next sprint entry re-opens with a fresh burst
	SetSpeed(IsCrawlingNow() ? AZP_CrawlChaseSpeed : AZP_ChaseSpeed);
	// Release the run loop back to the walk BlendSpace — but only if the run loop is what's
	// actually on the slot (a flinch may have displaced it; never cut someone else's clip).
	if (RunLoopMontage.IsValid() && Owner)
	{
		if (USkeletalMeshComponent* M = Owner->GetMesh())
		{
			if (UAnimInstance* AI = M->GetAnimInstance())
			{
				if (AI->Montage_IsPlaying(RunLoopMontage.Get())) { StopSlotLoop(); }
			}
		}
	}
	RunLoopMontage = nullptr;
	UE_LOG(LogTemp, Warning, TEXT("[Shambler] RUN end -> walk"));
}

void UZP_ShamblerBehaviorComponent::StopSlotLoop()
{
	if (!Owner) { return; }
	// A pose-hold is a PAUSED montage — resume it first so its blend-out actually ticks
	// (blend progression on a paused instance is not guaranteed across engine versions).
	ReleaseCombatPoseHold();
	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		if (UAnimInstance* AI = M->GetAnimInstance())
		{
			AI->StopSlotAnimation(AZP_IdleBlendOutTime, FName(TEXT("DefaultSlot")));
		}
	}
}

UAnimSequence* UZP_ShamblerBehaviorComponent::PickIdleAnim() const
{
	return (IsCrawlingNow() && AZP_CrawlIdleAnim) ? AZP_CrawlIdleAnim.Get() : AZP_IdleAnim.Get();
}

void UZP_ShamblerBehaviorComponent::GroundClampCrawlRise(float DeltaTime)
{
	// REAR-UP GROUND CLAMP (round-4 "floats in mid air", round-5 "feet were in the ground"):
	// the stand-up pose sweeps prone -> half-risen -> standing on ITS own curve, so no blind
	// lerp can track it — pin the LOWEST contact bone to capsule bottom + AZP_CrawlBoneLift
	// every tick instead (the lift ~= limb radius: bones are joint centres, flesh hangs below
	// — round-18 "shins and feet and arms partly buried" was the old -10 sink). Runs through
	// the whole stand window (rise AND the blend-out settle after the stance flips).
	if (bDead || !Owner) { return; }
	USkeletalMeshComponent* M = Owner->GetMesh();
	if (!M) { return; }
	float MinBoneZ = TNumericLimits<float>::Max();
	for (const FName& B : GShamblerContactBones)
	{
		if (M->GetBoneIndex(B) != INDEX_NONE)
		{
			MinBoneZ = FMath::Min(MinBoneZ, (float)M->GetBoneLocation(B).Z);
		}
	}
	if (MinBoneZ == TNumericLimits<float>::Max()) { return; }
	float CapBottomZ = Owner->GetActorLocation().Z - 40.f;
	if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent())
	{
		CapBottomZ = Owner->GetActorLocation().Z - Cap->GetScaledCapsuleHalfHeight();
	}
	const float Err = MinBoneZ - (CapBottomZ + AZP_CrawlBoneLift);
	// UPPER bound: standing-plane headroom exists ONLY for the slump-rise window. A prone body
	// stays pinned near the crawl plane — the crawl ATTACK rears the contact bones up mid-pounce
	// and chasing that pose launched the whole mesh toward the standing plane (round-23:
	// "when the crawler attacks sometimes it flys in the air like it's switching to standing").
	// The attack clip's rear-up is AUTHORED body motion; the origin must not compensate for it.
	const float UpperZ = bCrawlStandWindow
		? MeshBaseRelZ + AZP_LocoMeshZOffset + 30.f
		: MeshBaseRelZ + AZP_CrawlMeshZOffset + 15.f;
	FVector RL = M->GetRelativeLocation();
	RL.Z = FMath::Clamp(FMath::FInterpTo(RL.Z, RL.Z - Err, DeltaTime, 12.f),
		MeshBaseRelZ + AZP_CrawlMeshZOffset - 110.f, UpperZ);
	M->SetRelativeLocation(RL);
}

void UZP_ShamblerBehaviorComponent::ApplyCrawlCapsule(bool bCrawl)
{
	if (!Owner) { return; }
	UCapsuleComponent* Cap = Owner->GetCapsuleComponent();
	if (Cap)
	{
		const float CurHH  = Cap->GetUnscaledCapsuleHalfHeight();
		const float WantHH = bCrawl
			? FMath::Clamp(AZP_CrawlCapsuleHalfHeight, Cap->GetUnscaledCapsuleRadius(), StandingCapsuleHH)
			: StandingCapsuleHH;
		if (!FMath::IsNearlyEqual(CurHH, WantHH, 0.5f))
		{
			// The capsule centre settles by exactly the half-height delta — mirror it into the
			// mesh base and the actor Z so the body's WORLD plane never pops.
			const float Delta = CurHH - WantHH; // + when shrinking, - when restoring
			Cap->SetCapsuleHalfHeight(WantHH);
			MeshBaseRelZ += Delta;
			Owner->AddActorWorldOffset(FVector(0.f, 0.f, -Delta), false, nullptr, ETeleportType::TeleportPhysics);
			if (!bCrawl)
			{
				// Restoring mid-pose: shift the mesh rel-Z by the same delta so its WORLD
				// position is unchanged — the ground clamp (still live through the settle)
				// keeps owning the plant; the clamp-release timer hands Z to the state machine.
				if (USkeletalMeshComponent* SM = Owner->GetMesh())
				{
					FVector RL = SM->GetRelativeLocation();
					RL.Z += Delta;
					SM->SetRelativeLocation(RL);
				}
			}
			UE_LOG(LogTemp, Log, TEXT("[Shambler] capsule %.0f -> %.0f (%s; mesh base rel-Z %+.0f)"),
				CurHH, WantHH, bCrawl ? TEXT("crawl") : TEXT("STAND"), Delta);
		}
	}
	if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
	{
		CM->RotationRate.Yaw = bCrawl ? FMath::Max(AZP_CrawlRotationRate, 10.f) : StandingRotationRateYaw;
	}
}

void UZP_ShamblerBehaviorComponent::MaintainCrawlLocomotion(float DeltaTime)
{
	// Keeps a crawl loop on the DefaultSlot whenever no one-shot owns it. Runs every Tick for
	// crawlers, all living states — the BS_Shambler BlendSpace underneath is standing-only, so
	// any slot gap (stagger window, post-swing beat, post-revive, chase hold) would flash a
	// standing pose. Clip pick: idle when stationary, slow crawl when moving, fast crawl in the
	// sprint band; stride-matched via the crawl ref-speed knobs.
	if (bDead || !Owner || State == EShamblerState::Grab) { return; }
	USkeletalMeshComponent* M = Owner->GetMesh();
	UAnimInstance* AI = M ? M->GetAnimInstance() : nullptr;
	if (!AI) { return; }

	// MESH-Z OWNER (crawlers only — SetState/wander skip their snaps for us). During the
	// rear-up window the GroundClampCrawlRise tick (called from TickComponent) owns Z instead.
	if (!bCrawlStandWindow)
	{
		// Bone-measured plant every tick, not the fixed -87 plane (round-15, dev: "feet were
		// breaking through the ground" on a crawler holding at a door — the constant plane is
		// only right for the pose it was measured on; the clamp pins the lowest contact bone
		// to the capsule bottom through every crawl/idle/flat pose, same as the rise window).
		GroundClampCrawlRise(DeltaTime);
	}

	// One-shots own the slot while they play (the crawl swing, the rear-up; the alert montage
	// never fires for crawlers but the guard is cheap and future-proof).
	// CRAWL SWING TAIL (round-25: "periodically stands after attacking prone but its waist is
	// going through the floor"): waiting for the swing's Montage_IsPlaying to go false let its
	// BLEND-OUT hand slot weight back to the standing BlendSpace underneath — a standing pose
	// flashing at the crawl mesh plane (-87) = waist buried. Fall through once the swing is
	// inside its blend-out window so the crawl loop's blend-in crossfades OVER the tail
	// (same-slot montages auto-interrupt) and the slot never exposes the BS.
	if (ActiveSwingMontage.IsValid() && AI->Montage_IsPlaying(ActiveSwingMontage.Get()))
	{
		UAnimMontage* Sw = ActiveSwingMontage.Get();
		const float SwRate = FMath::Max(AI->Montage_GetPlayRate(Sw), 0.05f);
		const float SwRemaining = (Sw->GetPlayLength() - AI->Montage_GetPosition(Sw)) / SwRate;
		if (SwRemaining > 0.25f) { return; }
	}
	if (StandUpMontage.IsValid() && AI->Montage_IsPlaying(StandUpMontage.Get())) { return; }
	if (AlertMontage && AI->Montage_IsPlaying(AlertMontage)) { return; }

	// Clip pick + stride from the state's BASE speed (not live velocity — the limb-pulse below
	// makes velocity oscillate on purpose and must not flicker the clip choice or the rate).
	// Chase base is DISTANCE-picked (slow crawl near / fast crawl far, dev 2026-08-05).
	float BaseSpeed = AZP_CrawlWanderSpeed;
	if (State == EShamblerState::Chase && Target)
	{
		const float DistT = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
		BaseSpeed = (DistT > AZP_CrawlFastDistance) ? AZP_CrawlRunSpeed : AZP_CrawlChaseSpeed;
	}
	const float Speed2D = Owner->GetVelocity().Size2D();
	UAnimSequence* WantClip = nullptr;
	float WantRate = 1.f;
	// Stationary threshold sits BELOW the limb-pulse's speed troughs (base * (1-PulseAmount)) —
	// at 20 the wander pulse dipped under it every cycle, flipping walk->idle->walk and
	// restarting the crawl clip at its FIRST (right-arm) pull forever (round-5: "only moves one
	// side... right side extending and returning constantly").
	// PLAYING DEAD, dormant: LIFELESS flat pose — ProneToStand frame 0 held frozen, no crawl
	// idle lift at all (dev 2026-08-05). On wake (Target set) this branch stops matching and
	// the crawl loops crossfade in — the body visibly comes to life.
	if (AZP_Preset == EShamblerPreset::PlayingDead && State == EShamblerState::Wander && !Target)
	{
		if (AZP_PlayDeadPoseAnim
			&& (!CrawlLoopMontage.IsValid() || !AI->Montage_IsPlaying(CrawlLoopMontage.Get())
				|| CrawlLoopClip.Get() != AZP_PlayDeadPoseAnim.Get()))
		{
			CrawlLoopMontage = PlaySlotLoop(AZP_PlayDeadPoseAnim.Get(), 0.001f);
			CrawlLoopClip = AZP_PlayDeadPoseAnim.Get();
		}
		return;
	}

	// BOTH clip edges are debounced (round-21, [CrawlProbe]-proven): the walk clip needs
	// SUSTAINED MOTION and the idle pose needs SUSTAINED STILLNESS. The BP wander's 3s timer
	// produces a ONE-TICK velocity spike (10-23uu/s) before the frozen-body guard — which runs
	// AFTER this function in the same tick — kills it; the instant walk-engage turned that
	// single tick into one arm pull toward a random heading every 3s ("still not in a straight
	// line"). Between the two thresholds the CURRENT clip keeps playing untouched.
	if (Speed2D < 6.f) { CrawlIdleStillTime += DeltaTime; CrawlMoveSustain = 0.f; }
	else { CrawlMoveSustain += DeltaTime; CrawlIdleStillTime = 0.f; }
	if (CrawlIdleStillTime >= FMath::Max(AZP_CrawlIdleDebounce, 0.f))
	{
		WantClip = PickIdleAnim();
	}
	else if (CrawlMoveSustain < 0.15f)
	{
		// Neither edge sustained — hold whatever is on the slot (a live loop keeps cycling; a
		// one-tick blip on a resting body changes NOTHING).
		return;
	}
	else if (AZP_CrawlRunAnim && BaseSpeed >= AZP_CrawlRunClipMinSpeed)
	{
		// The slow->fast clip handoff sits on its own knob (dev 2026-08-05: "crawl speed should
		// be measured differently even for crawl slow to fast").
		WantClip = AZP_CrawlRunAnim.Get();
		WantRate = FMath::Max(0.1f, BaseSpeed / FMath::Max(AZP_CrawlRunAnimRefSpeed, 1.f));
	}
	else if (AZP_CrawlWalkAnim)
	{
		WantClip = AZP_CrawlWalkAnim.Get();
		WantRate = FMath::Max(0.1f, BaseSpeed / FMath::Max(AZP_CrawlWalkAnimRefSpeed, 1.f));
	}
	if (!WantClip) { return; }

	// LIMB-PHASE MEMORY: remember the live loop's normalized position every tick, and start
	// any replacement moving-loop AT that phase — a restart resumes mid-stride wherever the
	// limbs were, never snapping back to the clip's first (right-arm) pull. Idle poses start
	// clean (they're a rest, not a stride).
	const bool bLoopLive = CrawlLoopMontage.IsValid() && AI->Montage_IsPlaying(CrawlLoopMontage.Get());
	if (bLoopLive && CrawlLoopClip.IsValid())
	{
		const float LiveLen = FMath::Max(CrawlLoopClip->GetPlayLength(), 0.01f);
		CrawlLoopPhase = FMath::Fmod(AI->Montage_GetPosition(CrawlLoopMontage.Get()), LiveLen) / LiveLen;
	}
	if (!bLoopLive || CrawlLoopClip.Get() != WantClip)
	{
		// [CrawlProbe] round-19 instrumentation: every restart names its reason — if the gait
		// still resets in PIE, this line identifies the displacing path beyond argument.
		UE_LOG(LogTemp, Warning, TEXT("[CrawlProbe] RESTART loopLive=%d prev='%s' want='%s' phase=%.2f vel=%.0f state=%d t=%.2f"),
			bLoopLive ? 1 : 0, CrawlLoopClip.IsValid() ? *CrawlLoopClip->GetName() : TEXT("none"),
			*WantClip->GetName(), CrawlLoopPhase, Speed2D, (int32)State, GetWorld()->GetTimeSeconds());
		CrawlLoopMontage = PlaySlotLoop(WantClip, WantRate);
		CrawlLoopClip = WantClip;
		if (WantClip != PickIdleAnim() && CrawlLoopMontage.IsValid())
		{
			AI->Montage_SetPosition(CrawlLoopMontage.Get(), CrawlLoopPhase * WantClip->GetPlayLength());
		}
	}
	else
	{
		AI->Montage_SetPlayRate(CrawlLoopMontage.Get(), WantRate);
	}

	// BONE-DRIVEN PROPULSION (round-22, dev: "movements don't feel driven by bone movement...
	// the animation is there, but the movement amount/timing is off" — replaces the hand-tuned
	// sine pulse). The crawl clips are IN-PLACE, so a planted limb sweeps BACKWARD in actor
	// space at exactly the drag speed the animator authored for the body. Read that sweep off
	// the drive limbs every tick and let the fastest one BE the walk speed: the body surges
	// precisely when — and exactly as hard as — the arms pull, and settles to the floor speed
	// while they recover. Timing AND amount come from the playing animation itself; the clip's
	// play rate (stride-matched to BaseSpeed above) scales the sweeps automatically.
	if (WantClip != PickIdleAnim())
	{
		static const FName DriveBones[] = {
			FName(TEXT("mixamorig_LeftHand_011")),    FName(TEXT("mixamorig_RightHand_019")),
			FName(TEXT("mixamorig_LeftForeArm_010")), FName(TEXT("mixamorig_RightForeArm_018")),
			FName(TEXT("mixamorig_LeftFoot_026")),    FName(TEXT("mixamorig_RightFoot_030")) };
		constexpr int32 NumDrive = UE_ARRAY_COUNT(DriveBones);
		if (CrawlDrivePrevX.Num() != NumDrive)
		{
			CrawlDrivePrevX.Init(TNumericLimits<float>::Max(), NumDrive);
		}
		const FTransform ActorTM = Owner->GetActorTransform();
		float Drive = 0.f;
		for (int32 i = 0; i < NumDrive; ++i)
		{
			if (M->GetBoneIndex(DriveBones[i]) == INDEX_NONE) { continue; }
			const float RelX = (float)ActorTM.InverseTransformPosition(M->GetBoneLocation(DriveBones[i])).X;
			const float Prev = CrawlDrivePrevX[i];
			CrawlDrivePrevX[i] = RelX;
			if (Prev == TNumericLimits<float>::Max() || DeltaTime <= KINDA_SMALL_NUMBER) { continue; }
			// + = the limb sweeping BACKWARD through stance = propulsion.
			Drive = FMath::Max(Drive, (Prev - RelX) / DeltaTime);
		}
		if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
		{
			const float Cap = FMath::Max(BaseSpeed * FMath::Max(AZP_CrawlDriveCap, 0.1f), 10.f);
			CM->MaxWalkSpeed = FMath::Clamp(Drive * AZP_CrawlDriveScale, FMath::Max(AZP_CrawlDriveFloor, 1.f), Cap);
		}
	}
}

void UZP_ShamblerBehaviorComponent::ReleaseCombatPoseHold()
{
	if (!CombatPoseHold.IsValid() || !Owner) { return; }
	UAnimMontage* Held = CombatPoseHold.Get();
	CombatPoseHold = nullptr;
	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		if (UAnimInstance* AI = M->GetAnimInstance())
		{
			if (AI->Montage_IsActive(Held) && !AI->Montage_IsPlaying(Held))
			{
				// Resume: it is ~0.2s from its end, so it plays out and blends off on its own —
				// under the next clip's blend-in or into resumed locomotion.
				AI->Montage_Resume(Held);
			}
		}
	}
}

void UZP_ShamblerBehaviorComponent::LockIdleMovement()
{
	// Only lock if we actually arrived in the idle phase — a Scream/Chase could have interrupted
	// the AZP_IdleLockDelay window, and we must not freeze the body during combat.
	if (bDead || State != EShamblerState::Wander || bWanderMoving) { return; }
	if (UCharacterMovementComponent* CM = Owner ? Owner->GetCharacterMovement() : nullptr)
	{
		CM->StopMovementImmediately();
		CM->SetMovementMode(MOVE_None);
	}
}

void UZP_ShamblerBehaviorComponent::EnsureLocomotion()
{
	if (!Owner) { return; }
	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		if (M->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
		{
			M->SetAnimationMode(EAnimationMode::AnimationBlueprint); // re-activate ABP_Shambler locomotion
		}
		// Slot-based one-shots blend out on their own; nothing to clear here. The legacy mode-swap
		// path used to leave the mesh stuck in SingleNode if a one-shot was still mid-play, which is
		// why EnsureLocomotion existed.
	}
}

void UZP_ShamblerBehaviorComponent::PlayFootstep()
{
	if (!Owner || AZP_FootstepSounds.Num() == 0 || AZP_FootstepVolume <= 0.f) { return; }
	USoundBase* Step = AZP_FootstepSounds[FMath::RandRange(0, AZP_FootstepSounds.Num() - 1)];
	if (!Step) { return; }
	// Room carry (~60 m — the profile documented for "footfalls of others"); slight random
	// pitch so the 16 slices never read as a mechanical loop. AZP_FootstepVolume = THE dev knob.
	const float Pitch = 1.f + FMath::FRandRange(-AZP_FootstepPitchVar, AZP_FootstepPitchVar);
	UZP_SFXStatics::PlaySFXAtLocation(this, Step, Owner->GetActorLocation(),
		EZP_SFXCarry::Room, AZP_FootstepVolume, Pitch);
}

void UZP_ShamblerBehaviorComponent::UpdateLurk(float DistToPlayer)
{
	const bool bInRange = DistToPlayer <= AZP_LurkRange;
	if (!bLurkInit)
	{
		bLurkInit = true;
		bWasInLurkRange = bInRange;
		LurkInterval = FMath::FRandRange(AZP_LurkIntervalMin, AZP_LurkIntervalMax);
		return;
	}
	if (!bInRange)
	{
		bWasInLurkRange = false;
		LurkTimer = 0.f;
		return;
	}
	LurkTimer += AZP_EvalInterval;
	if (!bWasInLurkRange || LurkTimer >= LurkInterval)
	{
		// Play the lurk growl set on the audio comp at BeginPlay (SFX_ZOMBIE_LURK). If you import a
		// second growl (SFX_ZOMBIE_LURK2), restore a RandBool LoadObject pick here for variety.
		if (Audio)
		{
			Audio->PlayLurk();
		}
		LurkTimer = 0.f;
		LurkInterval = FMath::FRandRange(AZP_LurkIntervalMin, AZP_LurkIntervalMax);
	}
	bWasInLurkRange = true;
}

void UZP_ShamblerBehaviorComponent::OnAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("[Shambler] PROBE OnAnyDamage fired: %.0f raw (causer=%s)"),
		Damage, DamageCauser ? *DamageCauser->GetName() : TEXT("?"));
}

void UZP_ShamblerBehaviorComponent::ProbeShootable()
{
	if (bDead || !Owner || !GetWorld()) { return; }
	USkeletalMeshComponent* M = Owner->GetMesh();
	if (!M) { return; }
	// Fire a Visibility ray straight through the mesh's bounds centre. If the mesh's collision is
	// actually hittable, this hits Owner. If it sails through, the physics bodies aren't intercepting.
	const FVector C = M->Bounds.Origin;
	const FVector R = Owner->GetActorRightVector();
	FHitResult H;
	FCollisionQueryParams P;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(H, C + R * 120.f, C - R * 120.f, ECC_Visibility, P);
	UE_LOG(LogTemp, Warning, TEXT("[Shambler] PROBE selfRay hit=%d actor='%s' comp='%s'"),
		bHit ? 1 : 0,
		(bHit && H.GetActor()) ? *H.GetActor()->GetName() : TEXT("none"),
		(bHit && H.GetComponent()) ? *H.GetComponent()->GetName() : TEXT("-"));

	// Trace from the player's eye toward the Shambler — exactly what a shot does. Tells us if a door/wall
	// sits between them or if it's clean (then it's the aim/capsule size).
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		FVector EyeLoc; FRotator EyeRot;
		PC->GetPlayerViewPoint(EyeLoc, EyeRot);
		const FVector Tgt = Owner->GetActorLocation();
		FHitResult EH;
		FCollisionQueryParams EP;
		if (PC->GetPawn()) { EP.AddIgnoredActor(PC->GetPawn()); }
		const bool bEH = GetWorld()->LineTraceSingleByChannel(EH, EyeLoc, Tgt, ECC_Visibility, EP);
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] PROBE eye->shambler distToShambler=%.0f | firstBlock='%s' atDist=%.0f (shambler is the target)"),
			FVector::Dist(EyeLoc, Tgt),
			(bEH && EH.GetActor()) ? *EH.GetActor()->GetName() : TEXT("CLEAR-no block"),
			bEH ? EH.Distance : 0.f);
	}
}

void UZP_ShamblerBehaviorComponent::OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy,
	FVector HitLocation, UPrimitiveComponent* FHitComp, FName BoneName, FVector ShotDir,
	const UDamageType* DamageType, AActor* DamageCauser)
{
	if (bDead || !Health) { return; }
	// Remember which side took the (possibly lethal) hit so the death animation falls the right way:
	// a shot travelling AGAINST the zombie's forward came at its FRONT.
	bLastHitFront = FVector::DotProduct(ShotDir.GetSafeNormal(), Owner->GetActorForwardVector()) < 0.f;
	// MELEE (pipe) never headshots — it's a blunt body strike, tagged with UZP_MeleeDamageType, and
	// deals its own passed-in damage. Only ranged fire is judged head-vs-body by hit HEIGHT (capsule
	// hits carry no bone, so headshots go by how high up the body the shot landed).
	const bool bMelee = DamageType && DamageType->IsA(UZP_MeleeDamageType::StaticClass());
	const float HitZAboveCentre = HitLocation.Z - Owner->GetActorLocation().Z;
	// Crawler head sits near/below the capsule centre — the standing 55uu threshold would make
	// headshots impossible while prone.
	const float HeadMinZ = IsCrawlingNow() ? AZP_CrawlHeadshotMinZ : AZP_HeadshotMinZ;
	const bool bHead = !bMelee && (HitZAboveCentre >= HeadMinZ);
	const float Dmg = bMelee ? Damage : (bHead ? AZP_HeadShotDamage : AZP_BodyShotDamage);
	UE_LOG(LogTemp, Warning, TEXT("[Shambler] HIT z+%.0f -> %s, %.0f dmg (was %.0f HP)"),
		HitZAboveCentre, bMelee ? TEXT("melee/body") : (bHead ? TEXT("HEADSHOT") : TEXT("body")), Dmg, Health->CurrentHealth);
	// [LatchProbe] every damage event stamped against the last latch. dtLatch small + state=Grab(4)
	// = the damage-broke-the-grab path — the line right below will be "GRAB broken by damage".
	UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] t=%.2f SHAMBLER DAMAGE %s from %s: state=%d dtLatch=%.2f"),
		GetWorld()->GetTimeSeconds(), bMelee ? TEXT("MELEE") : TEXT("ranged"),
		DamageCauser ? *DamageCauser->GetName() : TEXT("?"),
		(int32)State, (float)(GetWorld()->GetTimeSeconds() - LastLatchTime));
	Health->ApplyDamage(Dmg);

	// FIGHT BACK: taking damage aggros it regardless of facing/LOS (it got HIT — it knows), with the
	// SHORT hurt-scream so a point-blank melee spammer gets a swing back inside their combo instead
	// of farming a 2 s stationary wail. Damage mid-scream also cuts the wail short. Previously it
	// died to 4 spam hits before its sight-aggro -> scream -> chase -> swing chain ever landed.
	if (!Health->bIsDead)
	{
		if (State == EShamblerState::Wander)
		{
			Target = GetPlayer();
			if (IsDormantSlumped())
			{
				// Damage always wakes the slumped body.
				if (Audio) { Audio->PlayAlert(); }
				StartSlumpRise();
			}
			else if (IsCrawlingNow())
			{
				// Prone: no standing scream pose — bark and hunt. Never stands.
				PlayCrawlAlert();
				SetState(EShamblerState::Chase);
			}
			else
			{
				SetState(EShamblerState::Scream);
				CurrentScreamHold = AZP_HurtScreamHoldTime;
			}
		}
		else if (State == EShamblerState::Scream)
		{
			// Dev 2026-07-14: damage during the alert (turn OR scream) cancels it OUTRIGHT —
			// straight into the hunt. (Previously only shortened the remaining hold to 0.5s;
			// SetState's exit hook blends the alert clip out.)
			UE_LOG(LogTemp, Log, TEXT("[Shambler] alert interrupted — took damage"));
			SetState(EShamblerState::Chase);
		}
	}

	// Shot/hit mid-grab: the grab BREAKS — the second out besides mashing (a cooked grenade,
	// a future companion). Release FIRST so the cosmetic flinch below plays over Chase, not
	// over the paired grab clip.
	if (!Health->bIsDead && State == EShamblerState::Grab)
	{
		BreakGrabFromDamage();
	}

	// Visible hit feedback — three always-cosmetic layers (never pauses the AI, never cancels a
	// swing; gameplay stagger stays block-only):
	//  1. Mesh punch: EVERY hit, EVERY state, ungated — the body visibly shoves along the hit
	//     direction and springs back (decay in TickComponent). This is the guarantee that no
	//     landed hit ever reads as ignored.
	//  2. Mid-swing: absorb-hitch — near-freeze hit-stop on the swing, then it powers through.
	//  3. Otherwise (not screaming): directional flinch clip + hit vocal (once imported).
	if (!Health->bIsDead)
	{
		// (1) punch — horizontal shove along the shot direction, in actor-local space.
		FVector PunchDir = ShotDir;
		PunchDir.Z = 0.f;
		if (PunchDir.Normalize() && AZP_HitPunchStrength > 0.f)
		{
			const FVector Local = Owner->GetActorTransform().InverseTransformVectorNoScale(PunchDir);
			MeshPunch += FVector2D(Local.X, Local.Y) * AZP_HitPunchStrength;
			MeshPunch = MeshPunch.GetSafeNormal() * FMath::Min(MeshPunch.Size(), AZP_HitPunchStrength * 2.f);
		}

		const double Now = GetWorld()->GetTimeSeconds();
		if ((Now - LastHitReactTime) >= AZP_FlinchCooldown)
		{
			// Only CONSUME the cooldown when a reaction actually fires — a no-op hitch (already
			// hitched this swing) or a missing clip must not eat the window and starve the next
			// real reaction (that starvation was "I still don't see it flinch at all").
			if (State == EShamblerState::Attack)
			{
				if (DoSwingHitch()) { LastHitReactTime = Now; } // (2)
			}
			else if (IsCrawlingNow())
			{
				// Crawler: the flinch clips are STANDING poses — keep the hit vocal (the mesh
				// punch above already sells the impact) and skip the clip + movement gate.
				LastHitReactTime = Now;
				if (Audio) { Audio->PlayHit(); }
			}
			else if (State != EShamblerState::Scream)
			{
				UAnimSequence* HitAnim = bLastHitFront ? AZP_HitFrontAnim : AZP_HitBackAnim;
				if (HitAnim)
				{
					LastHitReactTime = Now;
					if (Audio) { Audio->PlayHit(); } // silent until SFX_ZOMBIE_HIT is imported
					if (bAZP_HitReactGatesMovement)
					{
						// (3) GATE MOVEMENT ON THE FLINCH (dev 2026-07-14: "shamblers should move during
						// hit animation"). The flinch is a FULL-BODY slot montage over BS_Shambler, so it
						// masks the walk cycle — but nothing ever stopped the capsule, so a chasing
						// Shambler kept pathing and SLID across the floor in a frozen hit pose. Freeze the
						// AI for the clip: plant -> flinch -> resume, legs and body agreeing.
						// ReceiveStaggerHit already owns exactly this (StopMovement + bStaggered + release
						// at clip-length − 0.15 + the ACTIVE resume into swing/chase), so route into it
						// rather than duplicate the release. It also sets bStaggered, which the combat
						// pose-hold excludes — so the flinch plays smoothly instead of latching paused
						// ([BlockFreeze] lineage, 2026-07-05c). AZP_FlinchCooldown (checked above) is the
						// anti-stunlock rate limit.
						ReceiveStaggerHit(HitAnim->GetPlayLength(), HitAnim);
					}
					else
					{
						PlayOneShot(HitAnim); // (3) legacy: cosmetic-only flinch, AI keeps pathing
					}
				}
			}
		}
	}
}

// ───────────────────────────── [FloatProbe] — measurement only ─────────────────────────────
// Splits "it looks like it's floating" into the three independent gaps, because each has a
// DIFFERENT fix and they are routinely confused:
//   capGap  = capsule bottom − traced floor       -> the CAPSULE is off the ground (CMC/collision)
//   footGap = lowest foot/toe bone − traced floor -> the FEET are off the ground (mesh Z / clip)
//   meshOff = mesh relative Z − MeshBaseRelZ      -> which state nudge is applied right now
// If capGap ~0 and footGap > 0, the capsule is grounded and the MESH sits too high — that is a mesh
// Z / clip-authoring fix, NOT a collision one (cf. AZP_DeathDropZ=90, the death clips' known
// hip-height float, and the AZP_ScreamMeshZOffset / AZP_IdleMeshZOffset −8 nudges already in place
// for exactly this. Chase/Attack apply NO nudge — StateZ=0 — which is where the dev reports it).
void UZP_ShamblerBehaviorComponent::RunFloatProbe(double Now)
{
	const float dtState = (float)(Now - LastStateChangeTime);
	const int32 S = (int32)State;

	ACharacter* Ch = Cast<ACharacter>(Owner);
	if (!Ch)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FloatProbe] t=%.2f dtState=%.2f state=%d NO_CHARACTER (owner is not ACharacter)"), Now, dtState, S);
		return;
	}
	USkeletalMeshComponent* M = Ch->GetMesh();
	UCapsuleComponent* Cap = Ch->GetCapsuleComponent();
	UCharacterMovementComponent* CMC = Ch->GetCharacterMovement();

	const FVector ActorLoc = Ch->GetActorLocation();
	const float HalfH = Cap ? Cap->GetScaledCapsuleHalfHeight() : 0.f;
	const float CapBottomZ = ActorLoc.Z - HalfH;

	const TCHAR* Mode = TEXT("?");
	float CMCFloorDist = -1.f;
	int32 bWalkable = -1;
	if (CMC)
	{
		switch (CMC->MovementMode)
		{
		case MOVE_None:    Mode = TEXT("None");    break;
		case MOVE_Walking: Mode = TEXT("Walking"); break;
		case MOVE_Falling: Mode = TEXT("Falling"); break;
		case MOVE_Flying:  Mode = TEXT("Flying");  break;
		case MOVE_Custom:  Mode = TEXT("Custom");  break;
		default:           Mode = TEXT("other");   break;
		}
		// CMC's OWN idea of the floor — if this disagrees with the trace below, the capsule is being
		// held off the ground by the movement component itself, not by the mesh.
		CMCFloorDist = CMC->CurrentFloor.bBlockingHit ? CMC->CurrentFloor.FloorDist : -1.f;
		bWalkable = CMC->CurrentFloor.bWalkableFloor ? 1 : 0;
	}

	// Floor under the actor. Ignore self so the capsule/mesh can't be its own floor.
	FHitResult Floor;
	FCollisionQueryParams QP(SCENE_QUERY_STAT(ShamblerFloatProbe), /*bTraceComplex=*/true, Owner);
	const bool bHitFloor = GetWorld()->LineTraceSingleByChannel(Floor, ActorLoc,
		ActorLoc - FVector(0.f, 0.f, HalfH + 500.f), ECC_Visibility, QP);
	if (!bHitFloor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FloatProbe] t=%.2f dtState=%.2f state=%d mode=%s NO_FLOOR_TRACE ")
			TEXT("(nothing within %.0fuu below actorZ=%.1f) capBottomZ=%.1f cmcFloorDist=%.2f"),
			Now, dtState, S, Mode, HalfH + 500.f, ActorLoc.Z, CapBottomZ, CMCFloorDist);
		return;
	}
	const float FloorZ = Floor.ImpactPoint.Z;
	const float CapGap = CapBottomZ - FloorZ;

	if (!M)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FloatProbe] t=%.2f dtState=%.2f state=%d mode=%s NO_MESH capGap=%.2f"),
			Now, dtState, S, Mode, CapGap);
		return;
	}
	const float MeshRelZ = M->GetRelativeLocation().Z;
	const float MeshOff = MeshRelZ - MeshBaseRelZ;

	// Lowest foot/toe bone. Any unresolved bone is NAMED rather than silently read as 0 — a wrong
	// name would otherwise fake a huge float.
	const FName Bones[4] = { AZP_FloatProbeFootBoneL, AZP_FloatProbeFootBoneR,
	                         AZP_FloatProbeToeBoneL,  AZP_FloatProbeToeBoneR };
	float Z[4] = { 0.f, 0.f, 0.f, 0.f };
	float LowBone = TNumericLimits<float>::Max();
	FString Missing;
	for (int32 i = 0; i < 4; ++i)
	{
		if (Bones[i] != NAME_None && M->DoesSocketExist(Bones[i]))
		{
			Z[i] = M->GetSocketLocation(Bones[i]).Z;
			LowBone = FMath::Min(LowBone, Z[i]);
		}
		else
		{
			Missing += Bones[i].ToString() + TEXT(" ");
		}
	}
	if (!Missing.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FloatProbe] t=%.2f dtState=%.2f state=%d BONES_MISSING: %s")
			TEXT("(fix the AZP_FloatProbeBone knobs — remaining numbers below exclude them)"),
			Now, dtState, S, *Missing);
	}
	const float FootGap = (LowBone < TNumericLimits<float>::Max()) ? (LowBone - FloorZ) : -999.f;

	UE_LOG(LogTemp, Warning, TEXT("[FloatProbe] t=%.2f dtState=%.2f state=%d mode=%s vel2D=%.0f | ")
		TEXT("FLOOR z=%.2f (%s) | CAPSULE actorZ=%.2f halfH=%.2f bottomZ=%.2f capGap=%.2f cmcFloorDist=%.2f walkable=%d | ")
		TEXT("MESH relZ=%.2f baseRelZ=%.2f meshOff=%.2f worldZ=%.2f | ")
		TEXT("FEET Lfoot=%.2f Rfoot=%.2f Ltoe=%.2f Rtoe=%.2f low=%.2f FOOTGAP=%.2f"),
		Now, dtState, S, Mode, Ch->GetVelocity().Size2D(),
		FloorZ, *GetNameSafe(Floor.GetActor()),
		ActorLoc.Z, HalfH, CapBottomZ, CapGap, CMCFloorDist, bWalkable,
		MeshRelZ, MeshBaseRelZ, MeshOff, M->GetComponentLocation().Z,
		Z[0], Z[1], Z[2], Z[3], (LowBone < TNumericLimits<float>::Max() ? LowBone : 0.f), FootGap);
}

bool UZP_ShamblerBehaviorComponent::DoSwingHitch()
{
	if (bHitchedThisSwing || !Owner || !ActiveSwingMontage.IsValid()) { return false; }
	USkeletalMeshComponent* M = Owner->GetMesh();
	UAnimInstance* AI = M ? M->GetAnimInstance() : nullptr;
	if (!AI || !AI->Montage_IsPlaying(ActiveSwingMontage.Get())) { return false; }

	bHitchedThisSwing = true; // one per swing — repeated hitches would visibly desync the contact frame
	AI->Montage_SetPlayRate(ActiveSwingMontage.Get(), AZP_SwingHitchRate);
	GetWorld()->GetTimerManager().SetTimer(HitchRestoreTimer, this,
		&UZP_ShamblerBehaviorComponent::RestoreSwingRate, AZP_SwingHitchTime, false);
	return true;
}

void UZP_ShamblerBehaviorComponent::RestoreSwingRate()
{
	if (bDead || !Owner || !ActiveSwingMontage.IsValid()) { return; }
	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		if (UAnimInstance* AI = M->GetAnimInstance())
		{
			if (AI->Montage_IsPlaying(ActiveSwingMontage.Get()))
			{
				AI->Montage_SetPlayRate(ActiveSwingMontage.Get(), bSwingReleased ? AZP_StrikePlayRate : AZP_WindupPlayRate);
			}
		}
	}
}

void UZP_ShamblerBehaviorComponent::ReceiveStaggerHit(float Duration, UAnimSequence* OverrideAnim)
{
	if (bDead || !Owner) { return; }
	UE_LOG(LogTemp, Warning, TEXT("[LatchProbe] t=%.2f SHAMBLER STAGGER dur=%.2f state=%d dtLatch=%.2f"),
		GetWorld()->GetTimeSeconds(), Duration, (int32)State,
		(float)(GetWorld()->GetTimeSeconds() - LastLatchTime));

	// NO internal cooldown here — this is the BLOCK reward and it must ALWAYS read (design: spam
	// hits never stagger — the player passes Duration 0 for those — so this only fires when one of
	// our own swings lands on a blocking player, which its swing cadence already rate-limits, plus
	// the player-side AZP_BlockStaggerCooldown). Refresh the flinch gate so the cosmetic OnPointDamage
	// twitch doesn't double-play on top of this full stagger.
	const double Now = GetWorld()->GetTimeSeconds();
	LastHitReactTime = Now;

	// A stagger is contact — aggro even if it hadn't noticed the player yet (short hurt-scream,
	// same as damage-aggro; the stagger pause below overlays it).
	if (State == EShamblerState::Wander)
	{
		Target = GetPlayer();
		if (IsDormantSlumped())
		{
			// Melee contact wakes the slumped body (stagger pause overlays the rise).
			if (Audio) { Audio->PlayAlert(); }
			StartSlumpRise();
		}
		else if (IsCrawlingNow())
		{
			// Prone: no standing scream pose — bark and hunt (stagger pause overlays Chase).
			PlayCrawlAlert();
			SetState(EShamblerState::Chase);
		}
		else
		{
			SetState(EShamblerState::Scream);
			CurrentScreamHold = AZP_HurtScreamHoldTime;
		}
	}

	// Staggered mid-grab (defensive — the victim can't block while grabbed, but an external
	// stagger must never leave the player locked in the pair): break the grab first.
	if (State == EShamblerState::Grab)
	{
		BreakGrabFromDamage();
	}

	// Staggered mid-swing (a landed block/melee hit during the wind-up or strike): the flinch clip
	// replaces the swing on the slot, so the queued damage/release timers MUST die with it — a
	// staggered swing invisibly landing its hit would break the block flow the stagger rewards.
	if (State == EShamblerState::Attack)
	{
		CancelPendingSwing();
	}

	// Play the flinch, then release the AI as it finishes so it comes straight back (no frozen tail). The
	// stun releases at clip-length − 0.15 (PlayOneShot's blend-out); on release we ACTIVELY resume the flurry
	// (swing again / chase — see the timer lambda) so the flinch can NEVER sit pose-held waiting on the
	// deadlocked Attack-state chain. Capped at Duration so a stun already shorter than the flinch is never EXTENDED.
	float ResumeDelay = FMath::Max(0.1f, Duration);
	// OverrideAnim = the damage path's DIRECTIONAL flinch (back clip for a shot from behind). The block
	// path passes nullptr and keeps its original front-first pick.
	UAnimSequence* DefaultFlinch = AZP_HitFrontAnim ? AZP_HitFrontAnim : AZP_HitBackAnim;
	// Crawlers have no crawl flinch clips — the stagger degrades to a clip-less AI pause for
	// Duration (the crawl Tick maintainer keeps the crawl idle seated over the pause).
	UAnimSequence* Anim = IsCrawlingNow() ? nullptr : (OverrideAnim ? OverrideAnim : DefaultFlinch);
	if (Anim)
	{
		PlayOneShot(Anim);
		ResumeDelay = FMath::Clamp(Anim->GetPlayLength() - 0.15f, 0.1f, FMath::Max(0.1f, Duration));
	}

	bStaggered = true;

	// Cancel any in-flight AI move so the swipe truly pauses.
	if (APawn* P = Cast<APawn>(Owner))
	{
		if (AAIController* AI = Cast<AAIController>(P->GetController()))
		{
			AI->StopMovement();
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(StaggerHandle);
	GetWorld()->GetTimerManager().SetTimer(StaggerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bStaggered = false;
		if (bDead || !Owner) { return; }
		EnsureLocomotion();
		// ACTIVELY resume — never leave the flinch pose-held waiting on the Attack-state chain. That chain
		// only chains off a held SWING (bHeldAtEnd needs CombatPoseHold==ActiveSwingMontage) and CancelPendingSwing
		// nulled ActiveSwingMontage on the stagger, so a held FLINCH is never recognized — it froze at ~0.47/0.80
		// for SECONDS until the player re-hit the body ([BlockFreeze] confirmed 2026-07-05c). Resume the flurry the
		// stagger interrupted: swing again if still in reach (BeginAttack's PlayOneShot releases the pose-hold and
		// crossfades the swing over the flinch), else chase (movement releases the flinch into locomotion), else wander.
		AActor* Tgt = Target;
		const float Dist = Tgt ? FVector::Dist(Owner->GetActorLocation(), Tgt->GetActorLocation()) : TNumericLimits<float>::Max();
		if (Tgt && Dist <= AZP_AttackRange && HasLOS(Tgt))
		{
			BeginAttack();
		}
		else if (Tgt)
		{
			ReleaseCombatPoseHold();
			SetState(EShamblerState::Chase);
		}
		else
		{
			ReleaseCombatPoseHold();
			SetState(EShamblerState::Wander);
		}
	}), ResumeDelay, false);
}

void UZP_ShamblerBehaviorComponent::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	// Chain the next leg only DURING the walk phase. In idle phase the body must stay stopped, so
	// re-issuing MoveTo would kick velocity back up and the idle pose would never show.
	// Stumble is an intentional break — let StumbleEnd in Tick re-issue the move.
	if (bDead || State != EShamblerState::Wander || bStumbling) { return; }
	if (!bWanderMoving) { return; } // in idle phase — do NOT chain
	if (PickNewWanderPoint())
	{
		StartWanderLeg();
	}
	// Note: do NOT reset StateTimer — the walk-phase timer must keep ticking across legs so the
	// transition to idle still fires at WalkDuration.
}

void UZP_ShamblerBehaviorComponent::OnOwnerDied()
{
	bDead = true;

	// Died mid-grab (grenade, etc.): release the victim FIRST — their AbortGrab restores their
	// camera/input/movement (and calls back OnVictimGrabPhase(None), which with bDead set only
	// drops the pair collision ignore). The death path owns this body from here.
	if (State == EShamblerState::Grab && Target)
	{
		if (IZP_Grabbable* Victim = Cast<IZP_Grabbable>(Target.Get()))
		{
			Victim->AbortGrab();
		}
	}

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(EvalTimer);
		W->GetTimerManager().ClearTimer(AttackHitTimer);
		W->GetTimerManager().ClearTimer(WindupReleaseTimer);
		W->GetTimerManager().ClearTimer(HitchRestoreTimer);
		W->GetTimerManager().ClearTimer(EscapePushbackDelayTimer);
	}
	// Killed mid-escape: the pushback state must die with the body — Tick's bDead branch never
	// reaches the alpha>=1 clear, and a stale To would sweep-teleport an objective-revived corpse.
	bEscapePushback = false;
	if (AICon) { AICon->StopMovement(); }
	SetSpeed(0.f);

	// Death cry — reuses the alert path with the (dev-swappable) death SFX.
	if (Audio)
	{
		USoundBase* DeathSfx = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_DEATH.SFX_ZOMBIE_DEATH"));
		if (DeathSfx)
		{
			Audio->AZP_AlertSound = DeathSfx;
			Audio->PlayAlert();
		}
		else
		{
			// Asset not imported yet -> no death cry. Import a death growl to this exact path.
			UE_LOG(LogTemp, Warning, TEXT("[Shambler] death SFX missing — import /Game/Audio/Shambler/SFX_ZOMBIE_DEATH to hear the death cry."));
		}
	}

	// Play the directional death animation and HOLD the final (corpse) frame. NOT ragdoll — physics
	// produces NaN at this mesh's 0.01 scale (the body vanished). Front-hit -> Death_Front, back-hit -> Death_Back.
	if (Owner)
	{
		if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent()) { Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
		if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement()) { CM->StopMovementImmediately(); CM->DisableMovement(); }

		// A slumped body KILLED MID-RISE is a different animal (dev 2026-08-06: "a slump to stand
		// shambler dies in t-pose not facedown"): IsDormantSlumped() stays true until
		// FinishSlumpRise sets bCrawlStoodUp, so a shot landed during the ~2.3s rise used to take
		// the died-in-place branch below — which assumes the frozen slump hold still owns the
		// slot. But StartSlumpRise already STOPPED that hold; nothing owned the pose, the rise
		// montage ran out on the corpse and the mesh fell back to REF POSE (the T-pose). A body
		// killed mid-rise is upright(ish) — it takes the standing directional fall instead.
		const bool bKilledMidRise = bStandingUpNow;

		// PRONE / DORMANT-SLUMPED DEATH (dev 2026-08-05: "crawling shambler rise and fall
		// after dying" — the standing fall clips STAND the body UP first). A prone body dies
		// IN PLACE: crossfade into the flat corpse pose over the crawl pose (reads as going
		// limp); a dormant slumped body just freezes in its slump. No death-drop Z either —
		// AZP_DeathDropZ is calibrated for a standing hip height and would bury a body that is
		// already on the ground.
		if ((IsCrawlingNow() || IsDormantSlumped()) && !bKilledMidRise)
		{
			if (USkeletalMeshComponent* M = Owner->GetMesh())
			{
				if (IsCrawlingNow() && AZP_PlayDeadPoseAnim)
				{
					PlaySlotLoop(AZP_PlayDeadPoseAnim.Get(), 0.001f); // collapse into the flat corpse
				}
				// Dormant slumped: the frozen slump hold already on the slot IS the corpse.
				MeshPunch = FVector2D::ZeroVector;
				FVector RL = M->GetRelativeLocation();
				RL.X = MeshBaseRelXY.X;
				RL.Y = MeshBaseRelXY.Y;
				M->SetRelativeLocation(RL);
			}
			DeathStartTime = GetWorld()->GetTimeSeconds();
			DeathAnimLen = 0.f;
			bDropping = false;
			UE_LOG(LogTemp, Warning, TEXT("[Shambler] DEATH (prone/slumped — died in place, no fall clip)"));
			return;
		}

		if (bKilledMidRise)
		{
			// Shut the rise machinery down so nothing fights the death clip: the pending
			// FinishSlumpRise would early-return on bDead anyway, but the ground-clamp window
			// must release NOW so the dead-tick drop owns mesh Z instead of the rise clamp.
			bStandingUpNow = false;
			bCrawlStandWindow = false;
			// Counts as "stood" from here: save-restore then rebuilds this corpse through the
			// STANDING corpse path (death clip end pose + full drop), matching how it fell —
			// otherwise a reload would re-seat it as a slumped corpse.
			bCrawlStoodUp = true;
			GetWorld()->GetTimerManager().ClearTimer(CrawlStandUpTimer);
			// The rise slot montage dies with the animation-mode switch in PlayAnimation below.
			UE_LOG(LogTemp, Warning, TEXT("[Shambler] DEATH mid slump-rise — taking the standing fall clip"));
		}

		UAnimSequence* DeathAnim = bLastHitFront ? AZP_DeathFrontAnim : AZP_DeathBackAnim;
		if (USkeletalMeshComponent* M = Owner->GetMesh())
		{
			if (DeathAnim)
			{
				M->PlayAnimation(DeathAnim, /*bLooping=*/false); // single clip, holds last frame = corpse
			}
			// Clear any in-flight hit punch — the dead branch of Tick never decays it, so a punch
			// from the killing blow would leave the corpse laterally offset from the capsule.
			MeshPunch = FVector2D::ZeroVector;
			FVector RL = M->GetRelativeLocation();
			RL.X = MeshBaseRelXY.X;
			RL.Y = MeshBaseRelXY.Y;
			M->SetRelativeLocation(RL);
		}
		DeathStartTime = GetWorld()->GetTimeSeconds();
		DeathAnimLen = DeathAnim ? DeathAnim->GetPlayLength() : 0.f;
		bDropping = (DeathAnim != nullptr && AZP_DeathDropZ != 0.f);
		UE_LOG(LogTemp, Warning, TEXT("[Shambler] DEATH anim=%s hitFront=%d"),
			DeathAnim ? *DeathAnim->GetName() : TEXT("none-yet (retarget pending)"), bLastHitFront ? 1 : 0);
	}
}

// ───────────────────────── IZP_Revivable (save-state + revival) ─────────────────────────

void UZP_ShamblerBehaviorComponent::ApplyDeadStateInstant_Implementation()
{
	// Snap to the corpse pose without replaying the fall/SFX — used when a LOAD restores a dead Shambler.
	if (!Owner) { return; }
	bDead = true;
	bDropping = false; // no easing on restore — we jump straight to grounded
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(EvalTimer);
		W->GetTimerManager().ClearTimer(AttackHitTimer);
		W->GetTimerManager().ClearTimer(WindupReleaseTimer);
		W->GetTimerManager().ClearTimer(HitchRestoreTimer);
		W->GetTimerManager().ClearTimer(EscapePushbackDelayTimer);
	}
	bEscapePushback = false; // same reasoning as OnOwnerDied — no stale escape state on a corpse
	if (AICon) { AICon->StopMovement(); }
	SetSpeed(0.f);

	if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent()) { Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement()) { CM->StopMovementImmediately(); CM->DisableMovement(); }

	if (USkeletalMeshComponent* M = Owner->GetMesh())
	{
		if (IsCrawlingNow() || IsDormantSlumped())
		{
			// Prone/dormant-slumped corpse: the flat (or slump) pose frame 0, at the pose's own
			// ground plane — no standing fall clip, no death drop (see OnOwnerDied).
			UAnimSequence* CorpsePose = IsCrawlingNow()
				? (AZP_PlayDeadPoseAnim ? AZP_PlayDeadPoseAnim.Get() : nullptr)
				: (AZP_SlumpToStandAnim ? AZP_SlumpToStandAnim.Get() : nullptr);
			if (CorpsePose)
			{
				M->PlayAnimation(CorpsePose, /*bLooping=*/false);
				M->SetPosition(0.f, /*bFireNotifies=*/false);
				M->Stop();
			}
			FVector RL = M->GetRelativeLocation();
			RL.X = MeshBaseRelXY.X;
			RL.Y = MeshBaseRelXY.Y;
			RL.Z = MeshBaseRelZ + (IsCrawlingNow() ? AZP_CrawlMeshZOffset : AZP_LocoMeshZOffset);
			M->SetRelativeLocation(RL);
		}
		else
		{
			UAnimSequence* DeathAnim = AZP_DeathFrontAnim ? AZP_DeathFrontAnim : AZP_DeathBackAnim;
			if (DeathAnim)
			{
				M->PlayAnimation(DeathAnim, /*bLooping=*/false);
				M->SetPosition(DeathAnim->GetPlayLength(), /*bFireNotifies=*/false); // hold the final (corpse) frame
			}
			FVector RL = M->GetRelativeLocation();
			RL.X = MeshBaseRelXY.X; // also clear any stale hit-punch offset
			RL.Y = MeshBaseRelXY.Y;
			RL.Z = MeshBaseRelZ - AZP_DeathDropZ; // apply the full grounding drop instantly
			M->SetRelativeLocation(RL);
		}
	}
	MeshPunch = FVector2D::ZeroVector;
	UE_LOG(LogTemp, Log, TEXT("[Shambler] dead-state restored on load: %s"), *Owner->GetName());
}

void UZP_ShamblerBehaviorComponent::ReviveEnemy_Implementation()
{
	// Bring a dead Shambler back to a fully alive, patrolling state.
	if (!Owner) { return; }
	bDead = false;
	bDropping = false;
	bStaggered = false;
	bEscapePushback = false; // belt-and-braces — a stale escape lerp must never move a revived body
	MeshPunch = FVector2D::ZeroVector; // no stale death-frame jolt on the first alive frames
	Target = nullptr;
	LostSightTimer = 0.f;

	if (Health) { Health->ResetHealth(); } // CurrentHealth=Max, bIsDead=false

	if (UCapsuleComponent* Cap = Owner->GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // re-assert shootability
	}
	if (UCharacterMovementComponent* CM = Owner->GetCharacterMovement())
	{
		CM->SetMovementMode(MOVE_Walking);
	}

	// Revive restores the SPAWN preset: prone presets back to the crawl (shrunken capsule,
	// crawl rotation rate; maintainer re-seats the loops next tick), a risen Slumped back to
	// its dormant slump (the Tick hold re-seats the pose).
	bStandingUpNow = false;
	bCrawlStandWindow = false;
	StandUpMontage = nullptr;
	SlumpHoldMontage = nullptr;
	bCrawlStoodUp = false;
	SlumpPresenceTimer = 0.f;
	SlumpStareTimer = 0.f;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(CrawlStandUpTimer);
		W->GetTimerManager().ClearTimer(CrawlClampReleaseTimer);
	}
	if (IsPronePreset())
	{
		ApplyCrawlCapsule(true); // no-op if it never stood (capsule persisted through death)
	}

	// SetState(Wander) re-links the locomotion AnimBP (EnsureLocomotion), restores walk speed and the
	// mesh's base Z, and clears the idle/stumble bookkeeping.
	SetState(EShamblerState::Wander);

	// Restart the AI evaluation loop (cleared on death / dead-state restore).
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(EvalTimer, this, &UZP_ShamblerBehaviorComponent::Evaluate, AZP_EvalInterval, true);
	}
	UE_LOG(LogTemp, Log, TEXT("[Shambler] REVIVED: %s"), *Owner->GetName());
}
