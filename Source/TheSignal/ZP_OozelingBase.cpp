// Copyright The Signal. All Rights Reserved.

#include "ZP_OozelingBase.h"
#include "ZP_ScytheerClimbPath.h"
#include "ZP_HealthComponent.h"
#include "ZP_BloodFXComponent.h"
#include "ZP_SFXStatics.h"
#include "Materials/MaterialInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

AZP_OozelingBase::AZP_OozelingBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule: SKM_BigBlob is a ~120x120x120 UU body with its pivot at the feet. Capsule fits the
	// visible mass and BLOCKS the player's ECC_Visibility hitscan so OnTakePointDamage fires
	// (same shootability pattern as Scytheer/Shambler).
	UCapsuleComponent* Cap = GetCapsuleComponent();
	if (Cap)
	{
		Cap->SetCapsuleHalfHeight(60.f);
		Cap->SetCapsuleRadius(55.f);
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		// A placed pawn must NEVER carve the static navmesh — its capsule punches a hole at its
		// own spawn spot during the editor nav build and maroons it off-mesh (chase can't even
		// start a path). Found live on the placed Oozeling 2026-07-13.
		Cap->SetCanEverAffectNavigation(false);
	}

	// Mesh: pivot-at-feet mesh drops by the capsule half-height; standard pack import axes need
	// only the Yaw=-90 to face the Character's +X forward (verified on BP_TPS_BigBlob: Yaw=-90,
	// scale 1.0). Capsule eats the bullet, mesh ignores Visibility so damage never double-fires.
	// SingleNode mode so PlayClip() owns playback — no AnimBP.
	USkeletalMeshComponent* SM = GetMesh();
	if (SM)
	{
		SM->SetRelativeLocation(FVector(0.f, 0.f, -60.f));
		SM->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		SM->SetRelativeScale3D(FVector(1.f));
		SM->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		SM->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	}

	// Possess with a stock AIController so MoveToLocation/MoveToActor work out of the box.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (CM)
	{
		CM->bOrientRotationToMovement = true;
		CM->bUseControllerDesiredRotation = false;
		CM->MaxWalkSpeed = AZP_WanderSpeed;
		CM->RotationRate = FRotator(0.f, 360.f, 0.f);
		// RVO so the body steers around props/other AI instead of punching into them.
		CM->bUseRVOAvoidance = true;
		CM->AvoidanceConsiderationRadius = 200.f;
		CM->AvoidanceWeight = 0.5f;
		// Grounded body never walks itself off a ledge (spline patrol runs in MOVE_None,
		// navmesh chase/wander already avoid ledges — belt-and-braces, Scytheer default).
		CM->bCanWalkOffLedges = false;
	}
	bUseControllerRotationYaw = false;

	// Color-variant material defaults — SOFT paths only (no load happens at CDO construction;
	// they resolve in ApplyOozeColor at OnConstruction/BeginPlay).
	AZP_GreenMaterial  = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/BigBlob/Materials/Material_Update1/Material_Instances/MI_Blob_UPD17.MI_Blob_UPD17")));
	AZP_PurpleMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/BigBlob/Materials/Material_Update1/Material_Instances/MI_Blob_UPD4.MI_Blob_UPD4")));

	// NOTE: anim/SFX defaults load lazily in BeginPlay (LoadAssetDefaults), NOT here —
	// ConstructorHelpers /Game loads during CDO construction crash the editor (Shambler lesson).
}

void AZP_OozelingBase::ApplyOozeColor()
{
	const bool bPurple = (AZP_OozeColor == EOozelingColor::Purple);

	// Body: variant MI onto slot 0 (SKM_BigBlob's only real slot — slot 1 is an empty stub).
	const TSoftObjectPtr<UMaterialInterface>& Pick = bPurple ? AZP_PurpleMaterial : AZP_GreenMaterial;
	if (UMaterialInterface* Mat = Pick.LoadSynchronous())
	{
		if (USkeletalMeshComponent* M = GetMesh())
		{
			M->SetMaterial(0, Mat);
		}
	}

	// Blood follows the body (dev 2026-08-07: green oozes bled the enemy-default purple).
	// Smoke/decal/wound derive from the main tint with the same darkening ratios the purple
	// defaults use (0.45 / 0.37 / 0.44), so both variants keep the calibrated read.
	if (bAZP_BloodFollowsColor)
	{
		if (UZP_BloodFXComponent* Blood = FindComponentByClass<UZP_BloodFXComponent>())
		{
			const FLinearColor T = bPurple ? AZP_PurpleBloodTint : AZP_GreenBloodTint;
			Blood->AZP_BloodColor = T;
			Blood->AZP_SmokeColor = FLinearColor(T.R * 0.45f, T.G * 0.45f, T.B * 0.45f, 1.f);
			Blood->AZP_DecalColor = FLinearColor(T.R * 0.37f, T.G * 0.37f, T.B * 0.37f, 1.f);
			Blood->AZP_WoundColor = FLinearColor(T.R * 0.44f, T.G * 0.44f, T.B * 0.44f, 1.f);
		}
	}
}

void AZP_OozelingBase::LoadAssetDefaults()
{
	auto FillAnim = [](TObjectPtr<UAnimSequence>& Slot, const TCHAR* P)
	{
		if (!Slot) { Slot = LoadObject<UAnimSequence>(nullptr, P); }
	};
	// BigBlob pack clips (all on SK_BigBlob). Loaded here, at spawn, so nothing hard-references
	// them from the BP/level package (anim-asset hard refs on an enemy BP = editor-open crash
	// surface, DEAD ENDS 2026-06-29). BP-set slots win via the if (!Slot) guards.
	FillAnim(AZP_WalkAnim, TEXT("/Game/BigBlob/Animations/Walk_Roll/AS_BigBlob_Walk_FW.AS_BigBlob_Walk_FW"));
	FillAnim(AZP_RunAnim,  TEXT("/Game/BigBlob/Animations/Run_Roll/AS_BigBlob_Run_FW.AS_BigBlob_Run_FW"));
	FillAnim(AZP_IdleAnim, TEXT("/Game/BigBlob/Animations/idle/AS_BigBlob_Idle_Breath.AS_BigBlob_Idle_Breath"));
	FillAnim(AZP_FallAnim, TEXT("/Game/BigBlob/Animations/Jump/AS_BigBlob_Jump_Loop.AS_BigBlob_Jump_Loop"));
	FillAnim(AZP_EruptAnim, TEXT("/Game/BigBlob/Animations/Update1_Animations/AS_BigBlob_Hug_Attack.AS_BigBlob_Hug_Attack"));
	FillAnim(AZP_HitAnim,  TEXT("/Game/BigBlob/Animations/Get_hit/AS_BigBlob_GetHit.AS_BigBlob_GetHit"));
	FillAnim(AZP_DieAnim,  TEXT("/Game/BigBlob/Animations/Update1_Animations/AS_BigBlob_Death2.AS_BigBlob_Death2"));

	auto FillSnd = [](TObjectPtr<USoundBase>& Slot, const TCHAR* P)
	{
		// LOAD_NoWarn|LOAD_Quiet: these slots are deliberately optional — the assets don't exist
		// yet, and a bare LoadObject on a missing package logs an engine Warning. The slot still
		// auto-fills the session after the dev drops audio at the canonical path.
		if (!Slot) { Slot = LoadObject<USoundBase>(nullptr, P, nullptr, LOAD_NoWarn | LOAD_Quiet); }
	};
	// Oozeling-owned SFX slots. These assets don't exist yet — LoadObject returns null and the
	// Oozeling is simply silent until the dev drops real audio at these paths (or fills the BP slots).
	FillSnd(AZP_AlertSound,    TEXT("/Game/Audio/Oozeling/SFX_Oozeling_Alert.SFX_Oozeling_Alert"));
	FillSnd(AZP_EruptSound,    TEXT("/Game/Audio/Oozeling/SFX_Oozeling_Erupt.SFX_Oozeling_Erupt"));
	FillSnd(AZP_HitSound,      TEXT("/Game/Audio/Oozeling/SFX_Oozeling_Hit.SFX_Oozeling_Hit"));
	FillSnd(AZP_DeathSound,    TEXT("/Game/Audio/Oozeling/SFX_Oozeling_Death.SFX_Oozeling_Death"));
	FillSnd(AZP_BurstSound,    TEXT("/Game/Audio/Oozeling/SFX_Oozeling_Burst.SFX_Oozeling_Burst"));
	FillSnd(AZP_FootstepSound, TEXT("/Game/Audio/Oozeling/SFX_Oozeling_Squelch.SFX_Oozeling_Squelch"));
}

void AZP_OozelingBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Color variant previews live in the editor viewport (flip the Details dropdown, see it).
	ApplyOozeColor();
#if WITH_EDITOR
	// EDITOR AUTHORING PREVIEW (dev 2026-08-05): the placed Oozeling showed only its ref pose
	// in the viewport (single-node mode with no clip assigned until BeginPlay). Pose the idle
	// breath clip and evaluate it in-editor. PIE untouched — BeginPlay's PlayClip overrides.
	UWorld* W = GetWorld();
	if (W && (W->WorldType == EWorldType::Editor || W->WorldType == EWorldType::EditorPreview))
	{
		if (USkeletalMeshComponent* M = GetMesh())
		{
			M->SetUpdateAnimationInEditor(true);
			UAnimSequence* Preview = AZP_IdleAnim ? AZP_IdleAnim.Get() : LoadObject<UAnimSequence>(nullptr,
				TEXT("/Game/BigBlob/Animations/idle/AS_BigBlob_Idle_Breath.AS_BigBlob_Idle_Breath"));
			if (Preview)
			{
				// Serialized play data — the editor instance initializes from AnimationData
				// (SetAnimation() only reaches a live runtime instance).
				M->AnimationData.AnimToPlay = Preview;
				M->AnimationData.SavedPosition = 0.f;
				M->AnimationData.SavedPlayRate = 0.f;
				M->AnimationData.bSavedLooping = true;
				M->AnimationData.bSavedPlaying = false;
				M->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				M->InitAnim(true);
				M->SetPosition(0.f, false);
				M->SetPlayRate(0.f);
				// Push the pose to the renderer (editor doesn't tick anims).
				M->TickAnimation(0.f, false);
				M->RefreshBoneTransforms();
				M->MarkRenderStateDirty();
				// Ground the preview to the capsule bottom (cosmetic; BeginPlay restores the
				// authored mesh Z from the archetype).
				if (GetCapsuleComponent() && M->GetNumBones() > 0)
				{
					float MinZ = TNumericLimits<float>::Max();
					for (int32 i = 0; i < M->GetNumBones(); ++i)
					{
						MinZ = FMath::Min(MinZ, (float)M->GetBoneTransform(i).GetLocation().Z);
					}
					if (MinZ < TNumericLimits<float>::Max())
					{
						const float CapBottom = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
						FVector RL = M->GetRelativeLocation();
						RL.Z -= (MinZ - (CapBottom + 2.f));
						M->SetRelativeLocation(RL);
					}
				}
			}
		}
	}
#endif
}

void AZP_OozelingBase::BeginPlay()
{
	Super::BeginPlay();

	// Variant material + matching blood tints (BP-added BloodFX exists by now).
	ApplyOozeColor();

	// The editor preview grounds the mesh cosmetically — restore the AUTHORED mesh Z from the
	// archetype so the preview offset never leaks into runtime placement.
	if (USkeletalMeshComponent* M0 = GetMesh())
	{
		if (USceneComponent* MeshArch = Cast<USceneComponent>(M0->GetArchetype()))
		{
			FVector RL0 = M0->GetRelativeLocation();
			RL0.Z = MeshArch->GetRelativeLocation().Z;
			M0->SetRelativeLocation(RL0);
		}
	}

	LoadAssetDefaults();

	// Anchor the squelch distance accumulator at spawn so the first tick's delta is ~0.
	LastFootstepLoc = GetActorLocation();

	// Force-spawn the AIController if AutoPossessAI didn't fire (timing race, simulate mode, ...).
	if (!GetController())
	{
		SpawnDefaultController();
	}

	// ACharacter constructs in MOVE_None and only reaches Walking via a Falling->Landed event.
	// Placed ON the floor = no fall = stuck in NONE forever (capsule won't translate,
	// MoveToLocation no-ops). Self-heal.
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		if (CM->MovementMode == MOVE_None)
		{
			CM->SetMovementMode(MOVE_Walking);
		}
	}
	AICon = Cast<AAIController>(GetController());

	// Health — auto-attach; OnDied -> death fall / corpse. (Scytheer pattern: C++-owned, not SCS.)
	Health = FindComponentByClass<UZP_HealthComponent>();
	if (!Health)
	{
		Health = NewObject<UZP_HealthComponent>(this, TEXT("OozelingHealth"));
		Health->RegisterComponent();
	}
	Health->AZP_MaxHealth = AZP_MaxHealth;
	Health->ResetHealth();
	Health->OnDied.AddDynamic(this, &AZP_OozelingBase::OnOwnerDied);
	OnTakePointDamage.AddDynamic(this, &AZP_OozelingBase::OnPointDamage);

	// Capsule/mesh re-assert (post-BP-load) — anything the editor flipped goes back to shootable.
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	if (USkeletalMeshComponent* M = GetMesh())
	{
		M->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	// SELF-HEAL: C++ rebuild reinstancing is known to wipe per-instance object refs (the
	// Scytheer's PatrolPath has been eaten this way since June, KB 20260617b). A rebuild must
	// never leave a placed Oozeling standing dumb — if the slot is empty, bind the nearest
	// climb path in range. An explicitly assigned path always wins (this only fills NULL).
	if (!AZP_PatrolPath && bAZP_AutoBindPath)
	{
		AZP_ScytheerClimbPath* Best = nullptr;
		float BestDistSq = FMath::Square(FMath::Max(AZP_AutoBindPathRadius, 1.f));
		for (TActorIterator<AZP_ScytheerClimbPath> It(GetWorld()); It; ++It)
		{
			const float DistSq = FVector::DistSquared(It->GetActorLocation(), GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = *It;
			}
		}
		if (Best)
		{
			AZP_PatrolPath = Best;
			UE_LOG(LogTemp, Log, TEXT("[Oozeling] %s: AZP_PatrolPath was empty — auto-bound nearest climb path '%s' (%.0f UU)."),
				*GetName(), *Best->GetName(), FMath::Sqrt(BestDistSq));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Oozeling] BeginPlay %s AICon=%s path=%s splineLen=%.0f walkAnim=%s"),
		*GetName(),
		AICon ? *AICon->GetName() : TEXT("NULL"),
		AZP_PatrolPath ? *AZP_PatrolPath->GetName() : TEXT("NULL"),
		AZP_PatrolPath ? AZP_PatrolPath->GetSplineLength() : 0.f,
		AZP_WalkAnim ? *AZP_WalkAnim->GetName() : TEXT("NULL"));

	// AUTHORING VALIDATION: adjacent path normals that are near-anti-parallel (floor point
	// directly next to a ceiling point, no wall point between) make the lerped normal collapse
	// through zero mid-segment — the body somersaults there no matter what the orientation code
	// does. Fail loud so the dev inserts a wall point + re-snaps.
	if (AZP_PatrolPath)
	{
		const TArray<FVector>& Normals = AZP_PatrolPath->AZP_PerPointWallNormals;
		for (int32 i = 0; i + 1 < Normals.Num(); ++i)
		{
			if (FVector::DotProduct(Normals[i].GetSafeNormal(), Normals[i + 1].GetSafeNormal()) < -0.7f)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Oozeling] %s: patrol path '%s' points %d and %d have near-OPPOSITE wall normals — the body will somersault between them. Insert a wall point with an out-of-wall normal between them and re-run 'Snap Wall Normals To Geometry'."),
					*GetName(), *AZP_PatrolPath->GetName(), i, i + 1);
			}
		}
	}

	EnterState(EOozelingState::Wander);
}

void AZP_OozelingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDead)
	{
		if (State == EOozelingState::DieFalling)
		{
			// Death fall: right the body while gravity brings it down. Landed() finalizes the
			// corpse; the timeout covers a body that never lands (pit / hole in collision).
			if (TickFallingUpright(DeltaTime))
			{
				FinalizeCorpse();
			}
		}
		TickAnim(); // keeps the corpse pose pinned on the death clip's final frame
		return;
	}

	// Lazy AIController cache — AutoPossessAI can possess AFTER BeginPlay, so the BeginPlay
	// cache misses. Re-Cast every tick until we have one, then never again.
	if (!AICon)
	{
		if (!GetController())
		{
			SpawnDefaultController();
		}
		AICon = Cast<AAIController>(GetController());
	}

	APawn* Player = GetPlayer();
	const float Dist = Player ? FVector::Dist(GetActorLocation(), Player->GetActorLocation()) : TNumericLimits<float>::Max();

	// perf 2026-08-04: LOS trace + navmesh reachability at AZP_DetectEvalInterval cadence — these
	// ran EVERY FRAME per Oozeling (the reachability sync-pathfind fired per tick whenever the
	// player was merely visible anywhere).
	DetectEvalAccum += DeltaTime;
	if (DetectEvalAccum >= FMath::Max(0.02f, AZP_DetectEvalInterval))
	{
		DetectEvalAccum = 0.f;
		bCachedSee = Player ? HasLOS(Player) : false;
		if (!bAggro)
		{
			UCharacterMovementComponent* CMDetect = GetCharacterMovement();
			const bool bClingingDetect = CMDetect && CMDetect->MovementMode == MOVE_None && AZP_PatrolPath;
			// Also probe reachability inside the proximity radius WITHOUT LOS — the proximity
			// floor needs it as its through-wall guard (dev 2026-08-05: aggro through walls).
			bCachedReachable = (Player && (bCachedSee || Dist <= AZP_ProximityAggroRange))
				? (bClingingDetect ? true : IsPlayerReachable(Player))
				: false;
		}
	}
	const bool bSee = bCachedSee;

	// perf 2026-08-04: off-screen un-aggroed bodies stop evaluating anim — a moving skeletal mesh
	// invalidates the cached VSM shadow pages of every light containing it, every frame.
	if (bAZP_AnimOnlyWhenRendered && GetMesh())
	{
		GetMesh()->VisibilityBasedAnimTickOption = (bAggro || bDead)
			? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones
			: EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}

	// Detection / aggro management. Grounded aggro = range + LOS + navmesh reachability (closed
	// doors block it). Wall/ceiling aggro = range + LOS only — the clinging body isn't on the
	// navmesh, so a reachability query from up there would fail and mute aggro entirely.
	if (!bAggro)
	{
		UCharacterMovementComponent* CM = GetCharacterMovement();
		const bool bSplineClinging = CM && CM->MovementMode == MOVE_None && AZP_PatrolPath;
		const bool bReachable = bCachedReachable; // probed at AZP_DetectEvalInterval cadence above
		// Proximity floor (dev 2026-08-05, all NPC enemies): inside AZP_ProximityAggroRange,
		// aggro is UNAVOIDABLE — no LOS/reachability gate. Stealth still works outside it.
		// Proximity floor is gated (bSee || bReachable) — unavoidable in the SAME space (seen,
		// or hiding behind furniture = still navmesh-reachable), but a wall/closed door blocks
		// both and must block the aggro too (dev 2026-08-05: "aggroing through the wall").
		if (Player && ((Dist <= AZP_DetectionRange && bSee && bReachable)
			|| (Dist <= AZP_ProximityAggroRange && (bSee || bReachable))))
		{
			bAggro = true;
			LostSightTimer = 0.f;
			UZP_SFXStatics::PlaySFXAttached(AZP_AlertSound, GetRootComponent(), EZP_SFXCarry::Far);
			UE_LOG(LogTemp, Log, TEXT("[Oozeling] %s AGGRO dist=%.0f clinging=%d"), *GetName(), Dist, bSplineClinging ? 1 : 0);
			// Route from BOTH quiet states — leaving ReturnToPatrol out latches bAggro without a
			// state change and the enemy then patrols forever ignoring a visible player (the
			// de-aggro branch keeps LostSightTimer at 0 while seen, so it can never re-fire).
			if (State == EOozelingState::Wander || State == EOozelingState::ReturnToPatrol)
			{
				EnterChaseRouted(); // ceiling -> Drop, wall -> WallDescent, ground -> Chase
			}
			// State==Hit: the flinch exit (OnClipComplete / stagger timer) routes via bAggro.
		}
	}
	else
	{
		// Proximity also pins the lose-sight timer — a proximity-aggroed body must not shrug
		// after AZP_LoseSightTime while the player is still standing next to it without LOS.
		LostSightTimer = (bSee || Dist <= AZP_ProximityAggroRange) ? 0.f : (LostSightTimer + DeltaTime);
		if (LostSightTimer >= AZP_LoseSightTime || Dist > AZP_GiveUpRange)
		{
			bAggro = false;
			LostSightTimer = 0.f;
			if (State == EOozelingState::Chase)
			{
				// Patrol path set -> walk back to it (no teleport). No path -> straight to Wander.
				EnterState(AZP_PatrolPath ? EOozelingState::ReturnToPatrol : EOozelingState::Wander);
			}
			// WallDescent/Drop keep going — they end in Chase, whose own de-aggro handles it.
		}
	}

	switch (State)
	{
	case EOozelingState::Wander:
	{
		if (AZP_PatrolPath && AZP_PatrolPath->GetSplineLength() > KINDA_SMALL_NUMBER)
		{
			// SPLINE-DIRECT wall patrol (the Scytheer technique): drive position + rotation straight
			// from the spline, ping-ponging end to end at AZP_WanderSpeed. Per-point wall normals feed
			// the body's Up so it crawls up walls / across ceilings. CharacterMovement is in MOVE_None
			// (set in EnterState) so gravity doesn't pull the body off the wall.
			const float Total = AZP_PatrolPath->GetSplineLength();
			PatrolDistance += (float)PatrolDir * AZP_WanderSpeed * DeltaTime;
			if (PatrolDistance >= Total)      { PatrolDistance = Total; PatrolDir = -1; }
			else if (PatrolDistance <= 0.f)   { PatrolDistance = 0.f;   PatrolDir =  1; }

			const FVector Loc = AZP_PatrolPath->GetLocationAtDistance(PatrolDistance);
			const FVector Up  = AZP_PatrolPath->GetWallNormalAtDistance(PatrolDistance);
			FVector Fwd       = AZP_PatrolPath->GetForwardAtDistance(PatrolDistance);
			if (PatrolDir < 0) { Fwd = -Fwd; }

			// Cap angular speed so the endpoint 180° reversal turns over ~0.5s instead of snapping.
			// Continuity reference = the PREVIOUS DESIRED frame's up (path-derived), never the
			// body's physical up — see LastPatrolUp.
			const FRotator DesiredRot = MakeOrientation(Fwd, Up, LastPatrolUp);
			LastPatrolUp = DesiredRot.Quaternion().GetUpVector();
			const FRotator NewRot     = FMath::RInterpConstantTo(GetActorRotation(), DesiredRot, DeltaTime, AZP_PatrolTurnRate);
			// No sweep (world geometry must not block the authored path) + TeleportPhysics (physics
			// must not infer huge velocities from the per-tick position set).
			SetActorLocationAndRotation(Loc, NewRot, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

			PlayClip(AZP_WalkAnim, true, AZP_WalkPlayRate);
		}
		else
		{
			// Fallback: random navmesh wander when no path is wired.
			if (bWanderMoving)
			{
				// Arrival = distance OR the move request going idle — the AIController can complete
				// or abort short of the goal (accept radius, partial path), and a distance-only check
				// then leaves bWanderMoving stuck true forever (KB lesson, 20260617b).
				const bool bArrived = FVector::Dist2D(GetActorLocation(), WanderDest) < AZP_WanderArriveRadius;
				const bool bMoveIdle = AICon && AICon->GetMoveStatus() == EPathFollowingStatus::Idle;
				if (bArrived || bMoveIdle)
				{
					bWanderMoving = false;
					PauseTimer = FMath::FRandRange(AZP_PauseMin, AZP_PauseMax);
					if (AICon) { AICon->StopMovement(); }
				}
				PlayClip(AZP_WalkAnim, true, AZP_WalkPlayRate);
			}
			else
			{
				PauseTimer -= DeltaTime;
				if (PauseTimer <= 0.f)
				{
					PickNewWanderPoint();
					bWanderMoving = true;
				}
				PlayClip(AZP_IdleAnim, true);
			}
		}
		break;
	}

	case EOozelingState::WallDescent:
	{
		// Sprint along the spline toward its GROUND END (the lower endpoint — found automatically,
		// so it works whichever direction the dev authored the path) using the same spline-direct
		// drive as patrol — CharacterMovement stays MOVE_None so gravity can't peel the body off
		// mid-descent. Leaves the spline the moment it reaches floor height so a long floor tail
		// on the path never makes it run AWAY from the player before chasing.
		if (AZP_PatrolPath && AZP_PatrolPath->GetSplineLength() > KINDA_SMALL_NUMBER)
		{
			const float GroundD = GetGroundEndDistance();
			const float StepDir = (GroundD > PatrolDistance) ? 1.f : -1.f;
			PatrolDistance += StepDir * AZP_ChaseSpeed * DeltaTime;
			const bool bReachedEnd = (StepDir > 0.f) ? (PatrolDistance >= GroundD) : (PatrolDistance <= GroundD);
			if (bReachedEnd) { PatrolDistance = GroundD; }

			const FVector Loc = AZP_PatrolPath->GetLocationAtDistance(PatrolDistance);
			const FVector Up  = AZP_PatrolPath->GetWallNormalAtDistance(PatrolDistance);
			const FVector Fwd = AZP_PatrolPath->GetForwardAtDistance(PatrolDistance) * StepDir;

			const FRotator Desired = MakeOrientation(Fwd, Up, LastPatrolUp);
			LastPatrolUp = Desired.Quaternion().GetUpVector();
			const FRotator NewRot  = FMath::RInterpConstantTo(GetActorRotation(), Desired, DeltaTime, AZP_PatrolTurnRate);
			SetActorLocationAndRotation(Loc, NewRot, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

			// "Reached the floor" = within half the on-wall band of the ground end's height. Don't
			// bail higher than that — a body left mid-wall gravity-drops when Chase restores
			// walking (Scytheer lesson); don't wait for the endpoint either (floor-tail overshoot).
			const float GroundZ = AZP_PatrolPath->GetLocationAtDistance(GroundD).Z;
			if ((GetActorLocation().Z - GroundZ) <= AZP_OnWallZThreshold * 0.5f)
			{
				// Level out before walking — the descent frame can still be nose-down here, and
				// CMC RotationRate only steers yaw (0 deg/s pitch/roll), so an un-leveled body
				// would sprint the whole chase face-planted (adversarial-review finding).
				SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
				EnterState(EOozelingState::Chase);
			}
			else if (bReachedEnd)
			{
				// Ran out of spline still high (both endpoints elevated) — drop the rest.
				EnterState(EOozelingState::Drop);
			}
		}
		else
		{
			SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f)); // level (see above)
			EnterState(EOozelingState::Chase); // path vanished — EnterState restores Walking
		}
		break;
	}

	case EOozelingState::Drop:
	{
		// Alive ceiling drop: gravity owns the descent, we just right the body so it lands feet-
		// first. Landed() flips to Chase; the timeout covers a fall that never lands.
		if (TickFallingUpright(DeltaTime))
		{
			EnterState(EOozelingState::Chase);
		}
		break;
	}

	case EOozelingState::Chase:
	{
		if (!Player) { EnterState(EOozelingState::Wander); break; }
		// Aggro can lapse while WallDescent/Drop was still in flight (they don't de-aggro
		// mid-transition) — if it's gone by the time we're chasing, stand down.
		if (!bAggro)
		{
			EnterState(AZP_PatrolPath ? EOozelingState::ReturnToPatrol : EOozelingState::Wander);
			break;
		}

		// TOUCH -> ERUPTION FUSE. Contact latches the blob on and lights the fuse (Erupt state);
		// the burst itself lands AZP_TouchFuseTime later in Detonate() — that window is the
		// melee counterplay (kill = defused). Checked before anything else so a lunge through
		// the contact shell can't be outrun by the move logic. bSee gate: raw distance would
		// let it latch through a thin wall/doorframe post.
		if (Dist <= AZP_TouchRange && bSee)
		{
			EnterState(EOozelingState::Erupt);
			break;
		}

		// CROWDING (jammed at the player without contact — cornered geometry, tuning where
		// TouchRange < acceptance): track the player and breathe. A planted body that neither
		// turns nor changes clip reads as a frozen statue (live-trace lesson 2026-07-13).
		const bool bCrowding = Dist <= AZP_ChaseAcceptanceRadius * 1.2f;
		if (bCrowding)
		{
			FaceTargetSmooth(DeltaTime);
		}
		// Anim honesty: run clip only while actually translating; idle-breath while planted.
		{
			const bool bMovingNow = GetVelocity().SizeSquared() > 100.f;
			PlayClip(bMovingNow ? AZP_RunAnim.Get() : AZP_IdleAnim.Get(), true,
				bMovingNow ? AZP_RunPlayRate : 1.f);
		}

		// Stuck watchdog (Scytheer pattern): body not moving -> at AZP_ChaseStuckRepathTime retry
		// with a wider acceptance radius (path may be pinched against geometry); at
		// AZP_ChaseStuckGiveUpTime give up and return to patrol — door-jams were the dev's repeat
		// complaint on the Scytheer. CROWDING EXEMPTION: a body jammed right against the player
		// (blocked contact, cornered geometry) must not count as "stuck" and walk away — the
		// touch check above resolves genuine contact. A real jam is stationary AND far away.
		if (!bCrowding
			&& FVector::DistSquared(GetActorLocation(), LastChaseStuckLoc) < AZP_ChaseStuckMoveThreshold * AZP_ChaseStuckMoveThreshold)
		{
			ChaseStuckTimer += DeltaTime;
		}
		else
		{
			ChaseStuckTimer = 0.f;
			LastChaseStuckLoc = GetActorLocation();
		}
		if (ChaseStuckTimer >= AZP_ChaseStuckGiveUpTime)
		{
			UE_LOG(LogTemp, Log, TEXT("[Oozeling] %s CHASE stuck %.1fs -> de-aggro"), *GetName(), AZP_ChaseStuckGiveUpTime);
			bAggro = false;
			LostSightTimer = 0.f;
			ChaseStuckTimer = 0.f;
			EnterState(AZP_PatrolPath ? EOozelingState::ReturnToPatrol : EOozelingState::Wander);
			break;
		}
		if (ChaseStuckTimer >= AZP_ChaseStuckRepathTime && AICon)
		{
			// Wider acceptance — lets path-following declare success so the next frame's
			// MoveToActor recomputes a fresh path.
			AICon->StopMovement();
			AICon->MoveToActor(Player, FMath::Max(AZP_ChaseAcceptanceRadius * 2.f, 200.f));
			ChaseStuckTimer = AZP_ChaseStuckRepathTime + 0.5f; // hold above the threshold, don't re-issue every frame
			break;
		}

		// Keep closing — re-path each frame so the chase tracks the moving player, driving all
		// the way into contact (the touch check above ends the chase).
		//
		// NAV-HOLE FALLBACK: real floors have patchy navmesh (this level's ooze room is riddled
		// with clutter-carved gaps — live-mapped 2026-07-13; a pawn standing in a gap can't even
		// project a path START, so MoveToActor fails instantly and silently, forever). When the
		// nav request FAILS and we can see the player, direct-drive at them instead — the same
		// manual AddMovementInput the Scytheer's lunge uses, ledge-safe via the constructor's
		// bCanWalkOffLedges=false. Navmesh pathing (corners, doors) whenever it works; straight
		// visible pursuit when it doesn't.
		if (!bCrowding)
		{
			EPathFollowingRequestResult::Type MoveRes = EPathFollowingRequestResult::Failed;
			if (AICon)
			{
				// Explicit move request WITHOUT the engine's reach padding: plain MoveToActor
				// adds GoalRadius + AgentRadius*1.1 on top of the acceptance radius (UE 5.8
				// PathFollowingComponent HasReachedInternal), which completes the move at
				// 90 + 55 + 55*1.1 = ~205 UU — OUTSIDE AZP_TouchRange, so the blob would brake
				// two meters short of the player and never burst. Stripped, the body keeps
				// pushing until the touch check fires mid-approach.
				FAIMoveRequest Req(Player);
				Req.SetAcceptanceRadius(AZP_ChaseAcceptanceRadius);
				Req.SetReachTestIncludesAgentRadius(false);
				Req.SetReachTestIncludesGoalRadius(false);
				MoveRes = AICon->MoveTo(Req).Code;
			}
			if (MoveRes == EPathFollowingRequestResult::Failed && bSee)
			{
				FVector Dir = Player->GetActorLocation() - GetActorLocation();
				Dir.Z = 0.f;
				if (Dir.SizeSquared() > 1.f)
				{
					AddMovementInput(Dir.GetSafeNormal(), 1.f);
				}
			}
		}
		break;
	}

	case EOozelingState::ReturnToPatrol:
	{
		// Walk back to the closest spline point via CharacterMovement (navmesh). When close enough,
		// hand off to Wander — which parks MOVE_None and resumes spline patrol from right here, so
		// the handoff never reads as a teleport.
		if (!AZP_PatrolPath)
		{
			EnterState(EOozelingState::Wander);
			break;
		}
		const float ClosestD = AZP_PatrolPath->GetClosestDistanceToPoint(GetActorLocation());
		FVector Goal = AZP_PatrolPath->GetLocationAtDistance(ClosestD);
		// An ELEVATED closest point (mid-wall / ceiling straight overhead — the common case after
		// a ceiling Drop) is unreachable by navmesh walk: MoveToLocation partial-paths to the wall
		// base and the 3D arrival check can never pass — infinite grind. Head for the path's
		// GROUND end instead (the lower endpoint, direction-agnostic); Wander entry then rejoins
		// at the closest distance from there and climbs back up the spline naturally.
		if ((Goal.Z - GetActorLocation().Z) > AZP_OnWallZThreshold)
		{
			Goal = AZP_PatrolPath->GetLocationAtDistance(GetGroundEndDistance());
		}
		if (FVector::Dist(GetActorLocation(), Goal) < AZP_MoveAcceptanceRadius)
		{
			EnterState(EOozelingState::Wander);
			break;
		}
		// No-progress watchdog (KB 20260617b: never trust a MoveTo to reach a 3D goal): if the
		// body grinds in place — blocked goal, elevated point 0, pinched path — snap-rejoin the
		// spline via Wander after the give-up window instead of jamming here forever.
		if (FVector::DistSquared(GetActorLocation(), LastChaseStuckLoc) < AZP_ChaseStuckMoveThreshold * AZP_ChaseStuckMoveThreshold)
		{
			ChaseStuckTimer += DeltaTime;
			if (ChaseStuckTimer >= AZP_ChaseStuckGiveUpTime)
			{
				UE_LOG(LogTemp, Log, TEXT("[Oozeling] %s return-to-patrol blocked %.1fs — rejoining the spline from here."),
					*GetName(), AZP_ChaseStuckGiveUpTime);
				EnterState(EOozelingState::Wander); // rejoins at the closest spline point
				break;
			}
		}
		else
		{
			ChaseStuckTimer = 0.f;
			LastChaseStuckLoc = GetActorLocation();
		}
		// Re-issue MoveTo each tick in case the chosen point shifted or got blocked.
		if (AICon) { AICon->MoveToLocation(Goal, AZP_MoveAcceptanceRadius); }
		break;
	}

	case EOozelingState::Erupt:
	{
		// Fuse burning: planted, tracking the player, erupting. Committed — only death exits
		// early (bDead short-circuits at the top of Tick). Detonate() decides whether the DoT
		// lands (player still inside AZP_BurstRadius with LOS) — dodging out defuses the damage.
		FaceTargetSmooth(DeltaTime);

		// ANIM CUTOFF: freeze the erupt clip at AZP_EruptAnimCutoff and hold that pose for the
		// rest of the fuse — keeps the clip's opening (the vertical rise) and cuts its tail
		// (the flubber thrash). 0 = play the whole clip. Nothing else calls PlayClip during
		// Erupt, so the freeze holds until death/detonation swaps the clip.
		if (AZP_EruptAnimCutoff > 0.f)
		{
			if (USkeletalMeshComponent* SM = GetMesh())
			{
				if (UAnimSingleNodeInstance* SNI = SM->GetSingleNodeInstance())
				{
					// Trigger on ELAPSED fuse time, not the sampled clip time — the looping clip
					// WRAPS its time, so a low-FPS frame can jump clean over a near-end cutoff
					// window and the tail plays anyway (adversarial-review finding). Elapsed
					// world time is monotonic; the clip starts with the fuse at rate 1, so the
					// two clocks match until the freeze. Min-clamp = a cutoff past the clip end
					// freezes on the final pose instead of silently never freezing.
					if (SNI->IsPlaying()
						&& (GetWorld()->GetTimeSeconds() - EruptStartTime) >= AZP_EruptAnimCutoff)
					{
						SNI->SetPosition(FMath::Min(AZP_EruptAnimCutoff, CurrentClipLen), /*bFireNotifies=*/false);
						SNI->SetPlaying(false);
					}
				}
			}
		}

		if (GetWorld()->GetTimeSeconds() - EruptStartTime >= FMath::Max(AZP_TouchFuseTime, 0.f))
		{
			Detonate();
		}
		break;
	}

	case EOozelingState::Hit:
		// Hold position — during wall patrol the body is in MOVE_None, so it stays glued to the
		// wall through the flinch. OnClipComplete (or the stagger timer) routes onward via bAggro.
		break;

	default:
		break;
	}

	// DISTANCE-BASED SQUELCH: one SFX per AZP_FootstepStride of 2D travel, measured from the body's
	// own position delta (not GetVelocity()) so it works for BOTH the navmesh gaits AND the
	// teleport-driven spline patrol (which reports zero velocity). The upper guard skips implausibly
	// large single-frame jumps (respawn / spline rejoin) so they don't dump a burst of squelches.
	{
		const FVector CurLoc = GetActorLocation();
		const float StepDelta = FVector::Dist2D(CurLoc, LastFootstepLoc);
		LastFootstepLoc = CurLoc;
		const float Stride = FMath::Max(20.f, AZP_FootstepStride);
		if (StepDelta > 0.1f && StepDelta < Stride * 4.f)
		{
			StepDistanceAccum += StepDelta;
			if (StepDistanceAccum >= Stride)
			{
				StepDistanceAccum -= Stride;
				PlayFootstep();
			}
		}
	}

	TickAnim();
}

void AZP_OozelingBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Level out — the body may still be part-rotated from the wall/ceiling frame if the fall was
	// shorter than the upright interp needed.
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));

	if (bDead && State == EOozelingState::DieFalling)
	{
		FinalizeCorpse(); // hit the ground — NOW play the death clip
		return;
	}
	if (State == EOozelingState::Drop)
	{
		EnterState(EOozelingState::Chase); // ceiling drop complete — go get them
	}
}

// ───────────────────────── states ─────────────────────────

void AZP_OozelingBase::EnterState(EOozelingState NewState)
{
	State = NewState;

	// Chase + ReturnToPatrol need real CharacterMovement (walking + gravity + navmesh); restore
	// it if the spline patrol parked MOVE_None. Wander/WallDescent manage their own MOVE_None,
	// Drop/DieFalling manage MOVE_Falling, Hit deliberately keeps whatever mode is active.
	if (NewState == EOozelingState::Chase || NewState == EOozelingState::ReturnToPatrol)
	{
		if (UCharacterMovementComponent* CM = GetCharacterMovement())
		{
			if (CM->MovementMode == MOVE_None)
			{
				CM->SetMovementMode(MOVE_Walking);
			}
		}
	}

	switch (NewState)
	{
	case EOozelingState::Wander:
		SetMaxWalkSpeed(AZP_WanderSpeed);
		bWanderMoving = false;
		PauseTimer = 0.f; // pick a leg on the next tick
		// Guard must MATCH the Tick patrol branch (length > epsilon, not just non-null): a
		// degenerate path (points deleted to 0/1) would otherwise park MOVE_None here while
		// Tick falls into the navmesh fallback, which can't move a MOVE_None pawn — permafreeze.
		if (AZP_PatrolPath && AZP_PatrolPath->GetSplineLength() <= KINDA_SMALL_NUMBER && !bWarnedDegeneratePath)
		{
			bWarnedDegeneratePath = true;
			UE_LOG(LogTemp, Warning, TEXT("[Oozeling] %s: AZP_PatrolPath '%s' has zero spline length — falling back to navmesh wander. Shape the path (2+ points)."),
				*GetName(), *AZP_PatrolPath->GetName());
		}
		if (AZP_PatrolPath && AZP_PatrolPath->GetSplineLength() > KINDA_SMALL_NUMBER)
		{
			// Rejoin the nearest spot on the path so the resume point isn't always 0, then park
			// CharacterMovement so gravity / collision pushes don't fight the spline drive.
			PatrolDistance = AZP_PatrolPath->GetClosestDistanceToPoint(GetActorLocation());
			// Seed the continuity reference from the AUTHORED frame at the rejoin point — the
			// body itself may be upright mid-wall here (spawn/revive/return), which poisons
			// the anti-flip continuity on vertical segments.
			LastPatrolUp = AZP_PatrolPath->GetWallNormalAtDistance(PatrolDistance);
			if (UCharacterMovementComponent* CM = GetCharacterMovement())
			{
				CM->StopMovementImmediately();
				CM->SetMovementMode(MOVE_None);
			}
			PlayClip(AZP_WalkAnim, true, AZP_WalkPlayRate);
		}
		else
		{
			// Navmesh wander needs real CharacterMovement (walking + gravity).
			if (UCharacterMovementComponent* CM = GetCharacterMovement())
			{
				if (CM->MovementMode == MOVE_None)
				{
					CM->SetMovementMode(MOVE_Walking);
				}
			}
			PlayClip(AZP_IdleAnim, true);
		}
		break;

	case EOozelingState::WallDescent:
		// Spline-direct sprint down the wall. Ensure MOVE_None (aggro can arrive from a Hit exit
		// where the mode wasn't re-parked) and rejoin the spline where the body actually is.
		if (AICon) { AICon->StopMovement(); }
		SetMaxWalkSpeed(0.f); // movement is spline-driven, not CMC
		if (AZP_PatrolPath)
		{
			PatrolDistance = AZP_PatrolPath->GetClosestDistanceToPoint(GetActorLocation());
			LastPatrolUp = AZP_PatrolPath->GetWallNormalAtDistance(PatrolDistance); // authored seed (see Wander)
		}
		if (UCharacterMovementComponent* CM = GetCharacterMovement())
		{
			CM->StopMovementImmediately();
			CM->SetMovementMode(MOVE_None);
		}
		PlayClip(AZP_RunAnim, true, AZP_RunPlayRate);
		break;

	case EOozelingState::Drop:
		// Peel off the surface and free-fall. StopMovementImmediately zeroes velocity so the body
		// drops straight down; Tick rights it toward upright and Landed() continues the chase.
		if (AICon) { AICon->StopMovement(); }
		SetMaxWalkSpeed(0.f);
		FallStateStartTime = GetWorld()->GetTimeSeconds();
		if (UCharacterMovementComponent* CM = GetCharacterMovement())
		{
			CM->StopMovementImmediately();
			CM->SetMovementMode(MOVE_Falling);
		}
		PlayClip(AZP_FallAnim ? AZP_FallAnim.Get() : AZP_WalkAnim.Get(), true);
		break;

	case EOozelingState::Chase:
		SetMaxWalkSpeed(AZP_ChaseSpeed);
		ChaseStuckTimer = 0.f;
		LastChaseStuckLoc = GetActorLocation();
		// No clip here — the Chase tick picks run-vs-idle from actual velocity every frame.
		break;

	case EOozelingState::Erupt:
		// Latched on: plant and start the fuse. The vibrating telegraph clip + erupt SFX are the
		// player's "kill it NOW" signal.
		if (AICon) { AICon->StopMovement(); }
		SetMaxWalkSpeed(0.f);
		EruptStartTime = GetWorld()->GetTimeSeconds();
		PlayClip(AZP_EruptAnim ? AZP_EruptAnim.Get() : AZP_IdleAnim.Get(), true);
		UZP_SFXStatics::PlaySFXAttached(AZP_EruptSound, GetRootComponent(), EZP_SFXCarry::Far);
		break;

	case EOozelingState::ReturnToPatrol:
		SetMaxWalkSpeed(AZP_WanderSpeed);
		// The return walk reuses the chase's no-progress watchdog fields — reset them.
		ChaseStuckTimer = 0.f;
		LastChaseStuckLoc = GetActorLocation();
		PlayClip(AZP_WalkAnim, true, AZP_WalkPlayRate);
		break;

	case EOozelingState::Hit:
		// Deliberately does NOT touch MovementMode: a wall-clinging body must stay in MOVE_None
		// through the flinch or gravity yanks it off the wall the moment the state changes.
		if (AICon) { AICon->StopMovement(); }
		bWanderMoving = false;
		PlayClip(AZP_HitAnim, false, 1.f, /*bForceRestart=*/true);
		UZP_SFXStatics::PlaySFXAttached(AZP_HitSound, GetRootComponent(), EZP_SFXCarry::Far);
		break;

	case EOozelingState::DieFalling:
		// Dead but not on the ground: freeze the pose it died in and let gravity bring it down.
		// The death clip plays on LANDING (FinalizeCorpse), per dev direction.
		if (AICon) { AICon->StopMovement(); }
		SetMaxWalkSpeed(0.f);
		FallStateStartTime = GetWorld()->GetTimeSeconds();
		if (USkeletalMeshComponent* SM = GetMesh())
		{
			if (UAnimSingleNodeInstance* SNI = SM->GetSingleNodeInstance())
			{
				SNI->SetPlaying(false);
			}
		}
		if (UCharacterMovementComponent* CM = GetCharacterMovement())
		{
			CM->StopMovementImmediately();
			CM->SetMovementMode(MOVE_Falling);
		}
		break;

	case EOozelingState::Die:
		if (AICon) { AICon->StopMovement(); }
		if (AZP_DieAnim)
		{
			PlayClip(AZP_DieAnim, false, 1.f, /*bForceRestart=*/true);
		}
		else if (USkeletalMeshComponent* SM = GetMesh())
		{
			// No death clip — freeze whatever pose was playing as the corpse.
			if (UAnimSingleNodeInstance* SNI = SM->GetSingleNodeInstance())
			{
				SNI->SetPlaying(false);
			}
		}
		// Death SFX plays in OnOwnerDied (the kill moment) — NOT here, so the corpse landing
		// and a load-restore never replay the cry.
		break;
	}
}

void AZP_OozelingBase::EnterChaseRouted()
{
	// Route by current surface situation. Mid-air (already falling for any reason): stay falling,
	// Landed() will continue into Chase.
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (CM && CM->MovementMode == MOVE_Falling)
	{
		EnterState(EOozelingState::Drop);
		return;
	}

	const bool bSplineClinging = CM && CM->MovementMode == MOVE_None
		&& AZP_PatrolPath && AZP_PatrolPath->GetSplineLength() > KINDA_SMALL_NUMBER;
	if (bSplineClinging)
	{
		const FVector PathUp = AZP_PatrolPath->GetWallNormalAtDistance(PatrolDistance);
		// Elevation is measured against the path's GROUND end, not point 0 — paths may be
		// authored top-first.
		const bool bElevated =
			(GetActorLocation().Z - AZP_PatrolPath->GetLocationAtDistance(GetGroundEndDistance()).Z) > AZP_OnWallZThreshold;
		if (PathUp.Z <= AZP_CeilingNormalZ)
		{
			EnterState(EOozelingState::Drop); // on the ceiling: peel off and fall
			return;
		}
		if (bElevated)
		{
			EnterState(EOozelingState::WallDescent); // on a wall: sprint down the spline first
			return;
		}
	}
	EnterState(EOozelingState::Chase);
}

float AZP_OozelingBase::GetGroundEndDistance() const
{
	if (!AZP_PatrolPath) { return 0.f; }
	const float Total = AZP_PatrolPath->GetSplineLength();
	const float ZStart = AZP_PatrolPath->GetLocationAtDistance(0.f).Z;
	const float ZEnd   = AZP_PatrolPath->GetLocationAtDistance(Total).Z;
	return (ZEnd < ZStart) ? Total : 0.f;
}

bool AZP_OozelingBase::TickFallingUpright(float DeltaTime)
{
	// Right the body toward upright (yaw preserved) — a wall/ceiling frame can start the fall
	// sideways or fully inverted.
	const FRotator Cur = GetActorRotation();
	const FRotator Upright(0.f, Cur.Yaw, 0.f);
	SetActorRotation(FMath::RInterpConstantTo(Cur, Upright, DeltaTime, AZP_FallUprightRate));

	return (GetWorld()->GetTimeSeconds() - FallStateStartTime) > FMath::Max(AZP_FallTimeout, 0.5f);
}

void AZP_OozelingBase::FinalizeCorpse()
{
	if (UCapsuleComponent* Cap = GetCapsuleComponent()) { Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (UCharacterMovementComponent* CM = GetCharacterMovement()) { CM->StopMovementImmediately(); CM->DisableMovement(); }
	EnterState(EOozelingState::Die);
}

void AZP_OozelingBase::Detonate()
{
	if (bDead) { return; }

	// DODGE COUNTERPLAY: the DoT lands only if the victim is STILL inside the burst radius with
	// LOS at detonation — backing out during the fuse makes it pop harmlessly. It dies either way.
	APawn* Victim = GetPlayer();
	const float Dist = Victim ? FVector::Dist(GetActorLocation(), Victim->GetActorLocation()) : TNumericLimits<float>::Max();
	if (Victim && Dist <= AZP_BurstRadius && HasLOS(Victim))
	{
		// Arm the DoT BEFORE dying — the timer lives on the actor, and the corpse persists
		// (never destroyed), so ticks keep landing after the burst. Tick count from
		// duration/interval; the first bite lands right now, at the burst.
		TouchDoTVictim = Victim;
		TouchDoTTicksRemaining = FMath::Max(1,
			FMath::RoundToInt(AZP_TouchDamageDuration / FMath::Max(AZP_TouchDamageInterval, 0.05f)));
		UE_LOG(LogTemp, Log, TEXT("[Oozeling] %s BURST on %s (dist=%.0f) — %d DoT ticks of %.1f%%"),
			*GetName(), *Victim->GetName(), Dist, TouchDoTTicksRemaining, AZP_TouchDamagePercentPerSecond);
		TickTouchDoT();
		if (TouchDoTTicksRemaining > 0)
		{
			GetWorld()->GetTimerManager().SetTimer(TouchDoTHandle, this, &AZP_OozelingBase::TickTouchDoT,
				FMath::Max(AZP_TouchDamageInterval, 0.05f), true);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Oozeling] %s BURST whiffed (victim %s, dist=%.0f) — dodged out during the fuse."),
			*GetName(), Victim ? *Victim->GetName() : TEXT("none"), Dist);
	}

	// The burst IS the death: overkill our own health so the normal death flow runs (grounded
	// contact -> Death2 immediately, DeathSave persists the corpse). bBurstDeath routes the
	// death audio to AZP_BurstSound (touch-death) instead of AZP_DeathSound (killed).
	if (Health)
	{
		bBurstDeath = true;
		Health->ApplyDamage(Health->AZP_MaxHealth * 10.f); // OnOwnerDied runs synchronously in here
		bBurstDeath = false;
	}
}

void AZP_OozelingBase::TickTouchDoT()
{
	APawn* Victim = TouchDoTVictim.Get();
	if (!Victim || TouchDoTTicksRemaining <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(TouchDoTHandle);
		return;
	}

	// Percent of the VICTIM'S max health, through the standard damage pipeline — the player's
	// own TakeDamage gates (death, knockdown i-frames) decide whether a tick actually lands.
	float VictimMaxHP = 100.f;
	if (UZP_HealthComponent* VH = Victim->FindComponentByClass<UZP_HealthComponent>())
	{
		VictimMaxHP = VH->AZP_MaxHealth;
	}
	const float TickDamage = VictimMaxHP * (AZP_TouchDamagePercentPerSecond / 100.f);
	UGameplayStatics::ApplyDamage(Victim, TickDamage, GetController(), this, nullptr);

	if (--TouchDoTTicksRemaining <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(TouchDoTHandle);
	}
}

// ───────────────────────── animation (whole-clip SingleNode) ─────────────────────────

void AZP_OozelingBase::PlayClip(UAnimSequence* Clip, bool bLoop, float Rate, bool bForceRestart)
{
	if (!Clip)
	{
		// Slot empty (asset missing/moved) — state logic still runs, body just holds pose.
		// Warn ONCE per instance so a silently pose-frozen enemy is diagnosable from the log.
		if (!bWarnedNullClip)
		{
			bWarnedNullClip = true;
			UE_LOG(LogTemp, Warning, TEXT("[Oozeling] %s: PlayClip called with a NULL clip (state=%d) — anim slot empty, check the BigBlob asset paths / BP anim slots."),
				*GetName(), (int32)State);
		}
		return;
	}
	if (Clip == CurrentClip && bLoop == bClipLoops && !bForceRestart)
	{
		// Already the current clip — but re-assert playback + rate instead of blind-returning:
		// a freeze-pose death (null AZP_DieAnim) leaves the SNI stopped with CurrentClip still the
		// walk loop, so a revived body would otherwise glide along the spline frozen. This also
		// makes AZP_WalkPlayRate/AZP_RunPlayRate live-tunable in PIE.
		if (USkeletalMeshComponent* SMCur = GetMesh())
		{
			if (UAnimSingleNodeInstance* SNI = SMCur->GetSingleNodeInstance())
			{
				if (bLoop && !SNI->IsPlaying()) { SNI->SetPlaying(true); }
				SNI->SetPlayRate(FMath::Max(Rate, KINDA_SMALL_NUMBER));
			}
		}
		return;
	}

	USkeletalMeshComponent* SM = GetMesh();
	if (!SM) { return; }

	SM->PlayAnimation(Clip, bLoop);
	CurrentClip = Clip;
	bClipLoops = bLoop;
	bClipCompleteFired = false;
	CurrentClipLen = Clip->GetPlayLength();

	if (UAnimSingleNodeInstance* SNI = SM->GetSingleNodeInstance())
	{
		SNI->SetPlayRate(FMath::Max(Rate, KINDA_SMALL_NUMBER));
	}
}

void AZP_OozelingBase::TickAnim()
{
	if (bClipLoops || bClipCompleteFired || !CurrentClip) { return; }

	USkeletalMeshComponent* SM = GetMesh();
	UAnimSingleNodeInstance* SNI = SM ? SM->GetSingleNodeInstance() : nullptr;
	if (!SNI) { return; }

	// One-shot finished — pin the final frame, stop playback, fire completion exactly once.
	if (SNI->GetCurrentTime() >= CurrentClipLen - KINDA_SMALL_NUMBER)
	{
		SNI->SetPosition(CurrentClipLen, /*bFireNotifies=*/false);
		SNI->SetPlaying(false);
		bClipCompleteFired = true;
		OnClipComplete();
	}
}

void AZP_OozelingBase::OnClipComplete()
{
	switch (State)
	{
	case EOozelingState::Hit:
		// A forced stagger holds past the flinch clip — the StaggerHandle timer owns the exit.
		if (bStaggerHold) { break; }
		if (bAggro) { EnterChaseRouted(); }         // shot woke it up: descend/drop/chase
		// De-aggroed exit walks back (no teleport): a straight Wander entry would snap the body
		// onto the spline from wherever the flinch happened — the file-wide de-aggro convention.
		else { EnterState(AZP_PatrolPath ? EOozelingState::ReturnToPatrol : EOozelingState::Wander); }
		break;
	case EOozelingState::Die:
		// Stay dead. Final frame held.
		break;
	default:
		break;
	}
}

// ───────────────────────── movement helpers ─────────────────────────

void AZP_OozelingBase::PickNewWanderPoint()
{
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav || !AICon) { return; }
	FNavLocation Result;
	if (Nav->GetRandomReachablePointInRadius(GetActorLocation(), AZP_WanderRadius, Result))
	{
		WanderDest = Result.Location;
		AICon->MoveToLocation(WanderDest, AZP_MoveAcceptanceRadius);
	}
}

void AZP_OozelingBase::SetMaxWalkSpeed(float Speed)
{
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		CM->MaxWalkSpeed = Speed;
	}
}

void AZP_OozelingBase::PlayFootstep()
{
	if (!AZP_FootstepSound || AZP_FootstepVolume <= 0.f) { return; }
	const float Pitch = 1.f + FMath::FRandRange(-AZP_FootstepPitchVar, AZP_FootstepPitchVar);
	UZP_SFXStatics::PlaySFXAtLocation(this, AZP_FootstepSound, GetActorLocation(),
		EZP_SFXCarry::Room, AZP_FootstepVolume, Pitch);
}

void AZP_OozelingBase::FaceTargetSmooth(float DeltaTime)
{
	APawn* P = GetPlayer();
	if (!P) { return; }
	FVector ToP = P->GetActorLocation() - GetActorLocation();
	ToP.Z = 0.f;
	if (ToP.IsNearlyZero()) { return; }
	const FRotator Cur = GetActorRotation();
	const FRotator Desired(0.f, ToP.Rotation().Yaw, 0.f);
	const FRotator NewRot = FMath::RInterpConstantTo(Cur, Desired, DeltaTime, AZP_CombatTurnRate);
	SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
}

APawn* AZP_OozelingBase::GetPlayer() const
{
	return UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}

bool AZP_OozelingBase::HasLOS(const AActor* Target) const
{
	if (!Target || !GetWorld()) { return false; }
	const FVector Start = GetActorLocation() + FVector(0, 0, AZP_LOSEyeZOffset);
	const FVector End = Target->GetActorLocation() + FVector(0, 0, AZP_LOSTargetZOffset);
	FCollisionQueryParams P;
	P.AddIgnoredActor(this);
	P.AddIgnoredActor(Target);
	TArray<AActor*> Att;
	Target->GetAttachedActors(Att);
	for (AActor* A : Att) { P.AddIgnoredActor(A); }
	GetAttachedActors(Att);
	for (AActor* A : Att) { P.AddIgnoredActor(A); }

	// Walk the line, skipping QueryOnly trigger volumes (door interaction boxes, overlap zones).
	// BOTH directions must be clear — complex-as-simple wall collision has no BACKFACES, so a body
	// hugging a wall can trace out through it unblocked; the reverse trace catches the front face
	// (same fix as the Shambler's/Scytheer's through-wall aggro).
	auto DirClear = [this](FVector A, FVector B, FCollisionQueryParams Params) -> bool
	{
		for (int32 Iter = 0; Iter < 8; ++Iter)
		{
			FHitResult Hit;
			if (!GetWorld()->LineTraceSingleByChannel(Hit, A, B, ECC_WorldStatic, Params))
			{
				return true;
			}
			UPrimitiveComponent* Comp = Hit.GetComponent();
			if (Comp && Comp->GetCollisionEnabled() == ECollisionEnabled::QueryOnly)
			{
				Params.AddIgnoredComponent(Comp);
				continue;
			}
			return false;
		}
		return false;
	};
	return DirClear(Start, End, P) && DirClear(End, Start, P);
}

bool AZP_OozelingBase::IsPlayerReachable(const AActor* Target) const
{
	if (!Target || !GetWorld()) { return false; }
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav) { return true; } // no nav system -> degrade to LOS-only (don't lock down aggro)
	const ANavigationData* NavData = Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	if (!NavData) { return true; }

	FPathFindingQuery Query(this, *NavData, GetActorLocation(), Target->GetActorLocation());
	Query.SetAllowPartialPaths(false); // partial paths still mean "blocked somewhere" — reject
	const FPathFindingResult Result = Nav->FindPathSync(Query, EPathFindingMode::Regular);
	if (Result.Result != ENavigationQueryResult::Success || !Result.Path.IsValid())
	{
		return false; // closed door / unreachable region
	}
	const float PathLen = Result.Path->GetLength();
	return PathLen <= AZP_MaxReachablePathLength;
}

FRotator AZP_OozelingBase::MakeOrientation(const FVector& Forward, const FVector& Up, const FVector& ReferenceUp)
{
	const FVector SafeFwd = Forward.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
	const FVector AuthoredUp = Up.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);

	// Project the authored normal off the tangent. The RESIDUAL's magnitude measures how well-
	// conditioned the frame is: ~1 for a proper wall/floor/ceiling normal perpendicular to the
	// tangent, ~0 when the normal runs (nearly) parallel to a steep climb — where its DIRECTION
	// becomes numerically erratic and used to snap the body 180° mid-wall ("bottom vs top
	// confused", dev report 2026-07-14).
	FVector Residual = AuthoredUp - FVector::DotProduct(AuthoredUp, SafeFwd) * SafeFwd;
	const float ResidualSq = Residual.SizeSquared();

	FVector SafeUp;
	if (ResidualSq < 0.04f) // |residual| < 0.2 — ill-conditioned: continuity first
	{
		SafeUp = (ReferenceUp - FVector::DotProduct(ReferenceUp, SafeFwd) * SafeFwd)
			.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ZeroVector);
		// Continuity projection dead too (reference parallel to the tangent) — the WEAK authored
		// residual still knows which SIDE of the wall we're on; prefer it over any world-axis
		// guess (a world-axis fallback picks the wrong side on half of all wall orientations
		// and the hemisphere logic would then lock it in — adversarial-review finding).
		if (SafeUp.IsNearlyZero() && ResidualSq > 4.e-4f)
		{
			SafeUp = Residual.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ZeroVector);
		}
	}
	else
	{
		SafeUp = Residual.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ZeroVector);
		// WEAK authored signal (|residual| < 0.5) disagreeing with the body's current up by
		// more than 90° is the numeric hemisphere flip, not an authored transition — adjacent
		// patrol frames only ever rotate a few degrees. Flip it back. STRONG normals (proper
		// floor/wall/ceiling authoring) are always trusted, so deliberate ceiling crawls that
		// genuinely invert the body still work.
		if (ResidualSq < 0.25f && FVector::DotProduct(SafeUp, ReferenceUp) < 0.f)
		{
			SafeUp = -SafeUp;
		}
	}

	// Everything collapsed (no authored signal AND reference parallel to the tangent) — derive
	// ANY stable perpendicular so the body stays sanely oriented.
	if (SafeUp.IsNearlyZero())
	{
		SafeUp = FVector::CrossProduct(SafeFwd, FVector::RightVector);
		if (SafeUp.IsNearlyZero())
		{
			SafeUp = FVector::CrossProduct(SafeFwd, FVector::UpVector);
		}
		SafeUp = SafeUp.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
	}

	return FRotationMatrix::MakeFromXZ(SafeFwd, SafeUp).Rotator();
}

// ───────────────────────── damage / death ─────────────────────────

void AZP_OozelingBase::OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy,
	FVector HitLocation, UPrimitiveComponent* FHitComp, FName BoneName, FVector ShotDir,
	const UDamageType* DamageType, AActor* DamageCauser)
{
	if (bDead || !Health) { return; }
	// Flat damage — the blob is one uniform body, no headshot zone.
	Health->ApplyDamage(AZP_BodyShotDamage);

	if (!bDead)
	{
		// Being shot is an aggro event even without LOS-driven detection.
		bAggro = true;
		LostSightTimer = 0.f;

		// Cooldown-gated flinch so sustained fire can't perma-stunlock. Only from stable states —
		// a falling body doesn't flinch, and an ERUPTING one is committed (no flinch, no stagger:
		// interruptible eruption would let melee stunlock-defuse without the kill; damage still
		// applies, so killing it during the fuse is the counterplay).
		const double Now = GetWorld()->GetTimeSeconds();
		const bool bFlinchableState =
			State == EOozelingState::Wander || State == EOozelingState::Chase
			|| State == EOozelingState::WallDescent || State == EOozelingState::ReturnToPatrol;
		if (bFlinchableState && AZP_HitAnim && (Now - LastHitReactTime) >= AZP_HitReactCooldown)
		{
			LastHitReactTime = Now;
			EnterState(EOozelingState::Hit);
		}
		else if (State == EOozelingState::Wander)
		{
			// No flinch fired (cooldown / no clip) but we're aggroed now — go hunt.
			EnterChaseRouted();
		}
	}
}

void AZP_OozelingBase::ReceiveStagger_Implementation(float Duration)
{
	if (bDead || State == EOozelingState::Die || State == EOozelingState::DieFalling) { return; }
	// An erupting blob is committed — a melee hit damages it (kill = defuse) but never staggers
	// the fuse, or block/deflect could stunlock-defuse without the kill.
	if (State == EOozelingState::Erupt) { return; }

	bAggro = true; // a landed melee hit/block means the player is right here
	LostSightTimer = 0.f;
	LastHitReactTime = GetWorld()->GetTimeSeconds(); // keep the normal flinch path gated

	bStaggerHold = true;
	EnterState(EOozelingState::Hit);

	// Hold the stagger for Duration, then resume. OnClipComplete won't auto-exit while bStaggerHold.
	GetWorld()->GetTimerManager().ClearTimer(StaggerHandle);
	GetWorld()->GetTimerManager().SetTimer(StaggerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bStaggerHold = false;
		if (!bDead && State == EOozelingState::Hit)
		{
			if (bAggro) { EnterChaseRouted(); }
			// Walk back, never teleport-snap onto the spline (file-wide de-aggro convention).
			else { EnterState(AZP_PatrolPath ? EOozelingState::ReturnToPatrol : EOozelingState::Wander); }
		}
	}), FMath::Max(0.1f, Duration), false);
}

void AZP_OozelingBase::OnOwnerDied()
{
	if (bDead) { return; }
	bDead = true;
	bAggro = false;
	if (AICon) { AICon->StopMovement(); }

	// Death audio at the KILL moment (dev direction: the death CLIP waits for the ground, the
	// sound does not). Touch-death (its own burst) gets its own tunable sound.
	UZP_SFXStatics::PlaySFXAttached(bBurstDeath ? AZP_BurstSound : AZP_DeathSound,
		GetRootComponent(), EZP_SFXCarry::Far);

	// The dying body must stop eating bullets and stop shoving the player, but still LAND on
	// world geometry — so ignore Visibility+Pawn instead of killing collision outright.
	// FinalizeCorpse switches to full NoCollision once it's down.
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (CM && CM->IsMovingOnGround())
	{
		FinalizeCorpse(); // already on the floor — death clip plays immediately
	}
	else
	{
		// On a wall/ceiling (MOVE_None spline cling) or mid-air: fall to the ground first,
		// THEN play the death clip (Landed -> FinalizeCorpse).
		EnterState(EOozelingState::DieFalling);
	}
}

// ───────────────────────── IZP_Revivable (save-state + revival) ─────────────────────────

void AZP_OozelingBase::ApplyDeadStateInstant_Implementation()
{
	// Snap to the corpse pose without replaying the die clip/SFX — used on a LOAD restore.
	// (UZP_DeathSaveComponent marks the HealthComponent dead right after this call.)
	bDead = true;
	bAggro = false;
	if (UCapsuleComponent* Cap = GetCapsuleComponent()) { Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (UCharacterMovementComponent* CM = GetCharacterMovement()) { CM->StopMovementImmediately(); CM->DisableMovement(); }
	EnterState(EOozelingState::Die);
	if (USkeletalMeshComponent* M = GetMesh())
	{
		if (UAnimSingleNodeInstance* SNI = M->GetSingleNodeInstance())
		{
			// Only seek when the die clip is actually current — with a null AZP_DieAnim the
			// freeze-pose branch left the walk loop current, and seeking to a stale length
			// would snap the corpse to a mid-walk frame instead of freezing in place.
			if (AZP_DieAnim && CurrentClip == AZP_DieAnim)
			{
				SNI->SetPosition(CurrentClipLen, /*bFireNotifies=*/false); // hold final frame
			}
			SNI->SetPlaying(false);
			bClipCompleteFired = true;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[Oozeling] dead-state restored on load: %s"), *GetName());
}

void AZP_OozelingBase::ReviveEnemy_Implementation()
{
	// Bring a dead Oozeling back to a fully alive, patrolling state.
	bDead = false;
	bAggro = false;
	LostSightTimer = 0.f;

	// A revived Oozeling gets a fresh burst — stop any still-running ooze DoT from its last one.
	GetWorld()->GetTimerManager().ClearTimer(TouchDoTHandle);
	TouchDoTTicksRemaining = 0;
	TouchDoTVictim = nullptr;

	if (Health) { Health->ResetHealth(); }

	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // re-assert shootability
		Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);       // death path set this to Ignore
	}
	if (UCharacterMovementComponent* CM = GetCharacterMovement())
	{
		CM->SetMovementMode(MOVE_Walking);
	}

	// Force a fresh clip start — a freeze-pose death leaves the SNI stopped with the walk loop
	// still registered as current, which the PlayClip guard would otherwise treat as "playing".
	CurrentClip = nullptr;

	// EnterState(Wander) rejoins the patrol path and parks CharacterMovement appropriately
	// (MOVE_None for spline-direct patrol).
	EnterState(EOozelingState::Wander);
	UE_LOG(LogTemp, Log, TEXT("[Oozeling] REVIVED: %s"), *GetName());
}
