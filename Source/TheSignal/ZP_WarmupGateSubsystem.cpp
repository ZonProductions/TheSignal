// ZP_WarmupGateSubsystem.cpp — see header for purpose/ownership.

#include "ZP_WarmupGateSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/IConsoleManager.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ContentStreaming.h"
#include "Engine/Texture.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PipelineStateCache.h"
#include "ShaderCompiler.h"
#include "UObject/UObjectIterator.h"
#include "ZP_KinemationComponent.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"
#include "ZP_GameMode.h"

static TAutoConsoleVariable<int32> CVarWarmupGateEnabled(
	TEXT("zp.WarmupGate.Enabled"), 1,
	TEXT("1 = hold the EasyGameUI loading screen at world start until assets/shaders/PSOs are warm. 0 = off."));

static TAutoConsoleVariable<float> CVarWarmupGateMinShowSec(
	TEXT("zp.WarmupGate.MinShowSec"), 1.5f,
	TEXT("Minimum seconds the loading screen stays up, even if everything is already warm (anti-strobe)."));

static TAutoConsoleVariable<float> CVarWarmupGateTimeoutSec(
	TEXT("zp.WarmupGate.TimeoutSec"), 45.0f,
	TEXT("Hard cap in seconds — the gate always releases by this point and logs which condition never drained."));

static TAutoConsoleVariable<int32> CVarWarmupGatePause(
	TEXT("zp.WarmupGate.Pause"), 1,
	TEXT("1 = pause the game world while the gate holds (enemies frozen, no damage). Async loading and PSO compilation still progress (engine-level threads)."));

static TAutoConsoleVariable<int32> CVarWarmupGateWaitForShaders(
	TEXT("zp.WarmupGate.WaitForShaders"), 1,
	TEXT("1 = also hold until GShaderCompilingManager reports no pending shader compiles (editor on-demand compiles). Bounded by TimeoutSec."));

static TAutoConsoleVariable<int32> CVarWarmupGateStreamTextures(
	TEXT("zp.WarmupGate.StreamTextures"), 1,
	TEXT("1 = force-stream ALL level textures to full mip residency behind the loading screen so floor/stair crossings never stream textures. 0 = off."));

static TAutoConsoleVariable<float> CVarWarmupGateTextureTimeoutSec(
	TEXT("zp.WarmupGate.TextureTimeoutSec"), 30.0f,
	TEXT("Sub-timeout for the texture pre-stream: past this the gate stops waiting on textures and logs it (streaming pool likely smaller than the level's full texture set)."));

static TAutoConsoleVariable<int32> CVarWarmupGateKeepTexturesResident(
	TEXT("zp.WarmupGate.KeepTexturesResident"), 1,
	TEXT("1 = after the gate releases, keep all pre-streamed textures mip-resident for the session (no re-streaming when crossing floors). If GPU device-hung/VRAM pressure appears, set this to 0 first."));

namespace
{
	const TCHAR* GOpsManagerClassPath =
		TEXT("/Game/EasyGameUI/EasySaveGameUI/Core/BP_EasySaveGameOperationsManager.BP_EasySaveGameOperationsManager_C");

	// Every asset the gameplay code lazily sync-loads mid-play (hitch sources found by
	// profiling 2026-07-12: FlushAsyncLoading on first grab / first kill / first block /
	// first blood hit / first prompt). Loaded here behind the loading screen instead.
	// Source of truth for the paths remains the loaders themselves (ZP_GraceCharacter,
	// ZP_ShamblerBehaviorComponent, ZP_ScytheerBase, ZP_BloodFXComponent, ZP_HUDWidget,
	// ZP_KinemationComponent) — keep this list in sync when adding lazy loads there.
	const TCHAR* GWarmupManifest[] = {
		// EGUI loading screen widget itself (instant CreateWidget on show)
		TEXT("/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_LoadingScreen.WBP_ESGU_LoadingScreen_C"),
		// Marcus grab/knockdown anims (first Shambler grab)
		TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabEntry.A_Marcus_GrabEntry"),
		TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabMunch.A_Marcus_GrabMunch"),
		TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabWrestle.A_Marcus_GrabWrestle"),
		TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabKick.A_Marcus_GrabKick"),
		TEXT("/Game/Marcus/GrabAnims/A_Marcus_GrabPush.A_Marcus_GrabPush"),
		TEXT("/Game/Marcus/GrabAnims/A_Marcus_KnockdownFP.A_Marcus_KnockdownFP"),
		TEXT("/Game/Marcus/GrabAnims/A_Marcus_GetUpBack.A_Marcus_GetUpBack"),
		// Melee block/dodge anims (first block / first dodge)
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockStart.A_MeleePipe_BlockStart"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockLoop.A_MeleePipe_BlockLoop"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockWalk.A_MeleePipe_BlockWalk"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockStop.A_MeleePipe_BlockStop"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockImpact1.A_MeleePipe_BlockImpact1"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockImpact2.A_MeleePipe_BlockImpact2"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_BlockImpact3.A_MeleePipe_BlockImpact3"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle.A_MeleePipe_Idle"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Attack_R.A_MeleePipe_Attack_R"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Attack_L.A_MeleePipe_Attack_L"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Equip.A_MeleePipe_Equip"),
		TEXT("/Game/TheSignal/Animations/Melee/A_MeleePipe_Unequip.A_MeleePipe_Unequip"),
		TEXT("/Game/FPPMeleeAnimset/Animations/SwordnShield/FPP_sns_Dodge.FPP_sns_Dodge"),
		TEXT("/Game/Animations/FPS/AM_FP_GrenadeThrow.AM_FP_GrenadeThrow"),
		// Footsteps config + Shambler death cry (first step / first kill)
		TEXT("/Game/Core/Data/DA_Footsteps.DA_Footsteps"),
		TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_DEATH.SFX_ZOMBIE_DEATH"),
		// Shambler voice + footstep one-shots (spawn-time and combat SFX)
		TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_ALERT.SFX_ZOMBIE_ALERT"),
		TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_HIT.SFX_ZOMBIE_HIT"),
		TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_LURK.SFX_ZOMBIE_LURK"),
		TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_ATTACK.SFX_ZOMBIE_ATTACK"),
		TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_ATTACK2.SFX_ZOMBIE_ATTACK2"),
		TEXT("/Game/Audio/Shambler/SFX_ZOMBIE_IDLE.SFX_ZOMBIE_IDLE"),
		TEXT("/Game/Audio/Shambler/SFX_SHAMBLER_GRAB.SFX_SHAMBLER_GRAB"),
		TEXT("/Game/Audio/Shambler/SFX_GRAB_ALERT.SFX_GRAB_ALERT"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_01.SFX_SHAMBLER_FOOTSTEP_01"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_02.SFX_SHAMBLER_FOOTSTEP_02"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_03.SFX_SHAMBLER_FOOTSTEP_03"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_04.SFX_SHAMBLER_FOOTSTEP_04"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_05.SFX_SHAMBLER_FOOTSTEP_05"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_06.SFX_SHAMBLER_FOOTSTEP_06"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_07.SFX_SHAMBLER_FOOTSTEP_07"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_08.SFX_SHAMBLER_FOOTSTEP_08"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_09.SFX_SHAMBLER_FOOTSTEP_09"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_10.SFX_SHAMBLER_FOOTSTEP_10"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_11.SFX_SHAMBLER_FOOTSTEP_11"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_12.SFX_SHAMBLER_FOOTSTEP_12"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_13.SFX_SHAMBLER_FOOTSTEP_13"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_14.SFX_SHAMBLER_FOOTSTEP_14"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_15.SFX_SHAMBLER_FOOTSTEP_15"),
		TEXT("/Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_16.SFX_SHAMBLER_FOOTSTEP_16"),
		// Shambler anims (spawn-time load for dynamically spawned shamblers)
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Walk.A_Shambler_Walk"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Idle.A_Shambler_Idle"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_L.A_Shambler_Attack_L"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Attack_R.A_Shambler_Attack_R"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_ScreamPinned.A_Shambler_ScreamPinned"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Death_Front.A_Shambler_Death_Front"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Death_Back.A_Shambler_Death_Back"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Hit_Front.A_Shambler_Hit_Front"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_Hit_Back.A_Shambler_Hit_Back"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_RunStiffArmsHead.A_Shambler_RunStiffArmsHead"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabEntry.A_Shambler_GrabEntry"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabMunch.A_Shambler_GrabMunch"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabWrestle.A_Shambler_GrabWrestle"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabKicked.A_Shambler_GrabKicked"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabPushed.A_Shambler_GrabPushed"),
		TEXT("/Game/Enemies/Shambler/Anims/A_Shambler_GrabTakedown.A_Shambler_GrabTakedown"),
		// Scytheer SFX
		TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Alert.SFX_Scytheer_Alert"),
		TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Attack.SFX_Scytheer_Attack"),
		TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Hit.SFX_Scytheer_Hit"),
		TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Death.SFX_Scytheer_Death"),
		TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Lurk.SFX_Scytheer_Lurk"),
		TEXT("/Game/Audio/Scytheer/SFX_Scytheer_Footsteps.SFX_Scytheer_Footsteps"),
		// Blood VFX trios + decal (first hit on component-less actors bypasses prewarm)
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_SplashWithBurst_Hit_01.P_SplashWithBurst_Hit_01"),
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_SplashWithBurst_Hit_02.P_SplashWithBurst_Hit_02"),
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_SplashWithBurst_Hit_03.P_SplashWithBurst_Hit_03"),
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_Splash_HitWithMetal_01.P_Splash_HitWithMetal_01"),
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_Splash_HitWithMetal_02.P_Splash_HitWithMetal_02"),
		TEXT("/Game/Blood_VFX_Pack/Particles/Systems/P_Splash_HitWithMetal_03.P_Splash_HitWithMetal_03"),
		TEXT("/Game/Blood_VFX_Pack/Materials/MI_BloodDecal.MI_BloodDecal"),
		// UI: grab prompt glyphs, vignettes, first-time pickup widget, tab widgets
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Art/Textures/UI/Input/KeyboardMouse/T_IconMouse1.T_IconMouse1"),
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Art/Textures/UI/Input/GamepadXboxOne/T_XB1_RT.T_XB1_RT"),
		TEXT("/Game/InventorySystemPro/Blueprints/UI/SubWidgets/WBP_FirstTimePickupNotificationBase.WBP_FirstTimePickupNotificationBase_C"),
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Art/Pipe/SM_Pipe.SM_Pipe"),
		TEXT("/Game/Materials/UI/M_DamageVignette.M_DamageVignette"),
		TEXT("/Game/Materials/UI/M_HealVignette.M_HealVignette"),
		TEXT("/Game/Materials/UI/M_DamageReductionVignette.M_DamageReductionVignette"),
		TEXT("/Game/Materials/UI/M_InvincibilityVignette.M_InvincibilityVignette"),
		TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_NoteEntry.WBP_NoteEntry_C"),
		// 2026-08-04 first-use hitch hunt (wf_91c5bb35): weapon BP classes sync-load on FIRST
		// equip AND on walking near their pickups (ZP_GraceCharacter.cpp GetWeaponClassFromItem
		// LoadSynchronous) — measured 189-529ms stalls. Load them here instead.
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Equipment/Weapons/BP_Pipe.BP_Pipe_C"),
		TEXT("/Game/KINEMATION/TacticalShooterPack/Blueprints/Weapons/BP_WK-11_Viper.BP_WK-11_Viper_C"),
		TEXT("/Game/KINEMATION/TacticalShooterPack/Blueprints/Weapons/BP_Herrington_11-87_Police.BP_Herrington_11-87_Police_C"),
		TEXT("/Game/KINEMATION/TacticalShooterPack/Blueprints/Weapons/BP_AK105.BP_AK105_C"),
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Equipment/Weapons/BP_ExplosiveGrenade.BP_ExplosiveGrenade_C"),
		TEXT("/Game/InventorySystemPro/ExampleContent/Common/Equipment/Weapons/BP_ThrowableRock.BP_ThrowableRock_C"),
		TEXT("/Game/Core/Meshes/SM_PoliceShotgun.SM_PoliceShotgun"),
		// Item DataAssets — sync-loaded when a locked door/vent SHOWS ITS PROMPT, at card
		// readers, key pickups, and objective HasItem checks (ZP_InteractDoor.cpp:546/626,
		// ZP_VentDoor.cpp:249/262, ZP_CardReaderPanel.cpp:253/281, ZP_KeyPickup.cpp:55,
		// ZP_ObjectiveSubsystem.cpp:436).
		TEXT("/Game/Core/Items/DA_IDCard.DA_IDCard"),
		TEXT("/Game/Core/Items/DA_SecurityOfficeKey.DA_SecurityOfficeKey"),
		TEXT("/Game/Core/Items/DA_Fuse.DA_Fuse"),
		TEXT("/Game/Core/Items/DA_Screwdriver.DA_Screwdriver"),
		TEXT("/Game/Core/Items/DA_Pipe.DA_Pipe"),
		TEXT("/Game/Core/Items/DA_Grace_Pistol.DA_Grace_Pistol"),
		TEXT("/Game/Core/Items/DA_PoliceShotgun.DA_PoliceShotgun"),
		TEXT("/Game/Core/Items/DA_AssaultRifle.DA_AssaultRifle"),
		TEXT("/Game/Core/Items/DA_ExplosiveGrenade.DA_ExplosiveGrenade"),
		TEXT("/Game/Core/Items/DA_ThrowableRock.DA_ThrowableRock"),
		TEXT("/Game/Core/Items/DA_AdrenalineShot.DA_AdrenalineShot"),
		TEXT("/Game/Core/Items/DA_Suppressant.DA_Suppressant"),
		TEXT("/Game/Core/Items/DA_StimVial.DA_StimVial"),
	};
}

void UZP_WarmupGateSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (CVarWarmupGateEnabled.GetValueOnGameThread() == 0)
	{
		return;
	}
	if (!InWorld.IsGameWorld())
	{
		return;
	}
	// Only gameplay maps — the main menu level runs a different GameMode and must not
	// get paused behind a loading screen.
	if (!Cast<AZP_GameMode>(InWorld.GetAuthGameMode()))
	{
		UE_LOG(LogTemp, Log, TEXT("[WarmupGate] Skipped: GameMode is not AZP_GameMode (menu/util map)."));
		return;
	}

	// Defer the actual gate to the first tick so the PC, HUD and pawn have all begun
	// play — the EGUI show path disables input on them and message-calls the GI.
	State = EGateState::PendingStart;
}

void UZP_WarmupGateSubsystem::Deinitialize()
{
	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
		PreloadHandle.Reset();
	}
	Super::Deinitialize();
}

bool UZP_WarmupGateSubsystem::IsTickable() const
{
	return State == EGateState::PendingStart || State == EGateState::Warming;
}

TStatId UZP_WarmupGateSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZP_WarmupGateSubsystem, STATGROUP_Tickables);
}

bool UZP_WarmupGateSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UZP_WarmupGateSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (State == EGateState::PendingStart)
	{
		StartGate();
		return;
	}
	if (State != EGateState::Warming)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	const double Elapsed = Now - GateStartTime;

	// EGUI's own show+hide "flash" (death-respawn arrivals) can tear our screen down a
	// frame or two in — re-assert it while we're still warming.
	if (Now - LastReassertTime > 0.25)
	{
		LastReassertTime = Now;
		if (GetActiveLoadingScreen() == nullptr)
		{
			CallStartStopLoadingScreen(/*bStop=*/false, /*bFade=*/false);
		}
	}

	// --- PSO warm (2026-08-04): PIE cannot precache PSOs, so anything not DRAWN during the
	// gate stalls the render thread on first sight (~uniform 400ms+, the door/stairs hitches).
	// Spin the view 360° with alternating pitch behind the loading screen so every room in
	// range submits draws now. Occlusion queries are disabled for the hold (StartGate).
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (WarmSpinTick < 16 && Pawn)
	{
		if (UCameraComponent* Cam = Pawn->FindComponentByClass<UCameraComponent>())
		{
			if (!bWarmSpinRotCaptured)
			{
				WarmSpinOriginalRot = Cam->GetComponentRotation();
				bWarmSpinRotCaptured = true;
			}
			FRotator R = WarmSpinOriginalRot;
			R.Yaw += 22.5f * WarmSpinTick;
			R.Pitch = (WarmSpinTick % 2 == 0) ? -20.0f : 20.0f;
			Cam->SetWorldRotation(R);
			WarmSpinTick++;
			if (WarmSpinTick == 16)
			{
				Cam->SetWorldRotation(WarmSpinOriginalRot);
			}
		}
		else
		{
			WarmSpinTick = 16; // no camera - skip
		}
	}

	// --- Melee view-model warm (2026-08-04): render-only. The hidden Kubold view meshes are
	// never drawn until first pipe equip (measured 189-529ms stall). Show them for ~0.3s
	// behind the loading screen so their PSOs compile; no Kinemation state is touched.
	if (WarmMeleePhase == 0 && LoadsDrainedAt > 0.0 && Pawn)
	{
		WarmMeleePhase = 2;
		TArray<USkeletalMeshComponent*> SkelComps;
		Pawn->GetComponents<USkeletalMeshComponent>(SkelComps);
		for (USkeletalMeshComponent* C : SkelComps)
		{
			const FString N = C->GetName();
			if ((N == TEXT("MeleeViewMesh") || N == TEXT("MeleeViewOveralls") || N == TEXT("MeleeHands"))
				&& !C->IsVisible() && C->GetSkeletalMeshAsset())
			{
				C->SetVisibility(true);
				WarmMeleePhase = 1;
				WarmMeleeActivatedAt = Now;
			}
		}
	}
	else if (WarmMeleePhase == 1 && Now - WarmMeleeActivatedAt >= 0.3 && Pawn)
	{
		TArray<USkeletalMeshComponent*> SkelComps;
		Pawn->GetComponents<USkeletalMeshComponent>(SkelComps);
		for (USkeletalMeshComponent* C : SkelComps)
		{
			const FString N = C->GetName();
			if (N == TEXT("MeleeViewMesh") || N == TEXT("MeleeViewOveralls") || N == TEXT("MeleeHands"))
			{
				C->SetVisibility(false);
			}
		}
		WarmMeleePhase = 2;
	}

	if (LoadsDrainedAt == 0.0 && AreLoadsDrained())     { LoadsDrainedAt = Elapsed; }
	if (ShadersDrainedAt == 0.0 && AreShadersDrained()) { ShadersDrainedAt = Elapsed; }
	if (PSOsDrainedAt == 0.0 && ArePSOsDrained())       { PSOsDrainedAt = Elapsed; }
	if (TexturesDrainedAt == 0.0 && AreTexturesDrained(Elapsed)) { TexturesDrainedAt = Elapsed; }

	const bool bAllDrained = LoadsDrainedAt > 0.0 && ShadersDrainedAt > 0.0 && PSOsDrainedAt > 0.0
		&& TexturesDrainedAt > 0.0 && WarmSpinTick >= 16 && WarmMeleePhase == 2;
	const float MinShow = CVarWarmupGateMinShowSec.GetValueOnGameThread();
	const float Timeout = CVarWarmupGateTimeoutSec.GetValueOnGameThread();

	if (Elapsed >= Timeout)
	{
		ReleaseGate(/*bTimedOut=*/true);
	}
	else if (bAllDrained && Elapsed >= MinShow)
	{
		ReleaseGate(/*bTimedOut=*/false);
	}
}

void UZP_WarmupGateSubsystem::StartGate()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		State = EGateState::Released;
		return;
	}

	State = EGateState::Warming;
	GateStartTime = FPlatformTime::Seconds();
	LastReassertTime = GateStartTime;

	// 1) Loading screen up (also disables player input via the EGUI graph).
	CallStartStopLoadingScreen(/*bStop=*/false, /*bFade=*/false);

	// 2) Freeze the world so nothing stalks the player while the screen is up. Async
	//    loading + PSO compilation run on engine-level threads and keep progressing.
	if (CVarWarmupGatePause.GetValueOnGameThread() != 0)
	{
		bPausedByGate = UGameplayStatics::SetGamePaused(World, true);
	}

	// 3) Kick the preload manifest — every known lazy sync-load target, warmed now.
	TArray<FSoftObjectPath> Paths;
	Paths.Reserve(UE_ARRAY_COUNT(GWarmupManifest));
	for (const TCHAR* Path : GWarmupManifest)
	{
		Paths.Emplace(FSoftObjectPath(Path));
	}
	PreloadHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		MoveTemp(Paths), FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority,
		/*bManageActiveHandle=*/false, /*bStartStalled=*/false, TEXT("ZP_WarmupGate"));

	// 4) Ask the PSO precache pool to burn through its backlog first (cooked builds;
	//    precaching is disabled in-editor so this is a no-op in PIE).
	PipelineStateCache::PrecachePSOsBoostToHighestPriority(true);

	// 4b) Disable occlusion queries for the hold so occluded rooms still submit their draws
	//     during the warm spin (PSOs compile for geometry the spawn view can't see).
	//     Restored in ReleaseGate.
	if (IConsoleVariable* Occ = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AllowOcclusionQueries")))
	{
		OcclusionQueriesPrior = Occ->GetInt();
		Occ->Set(0, ECVF_SetByCode);
		bOcclusionDisabled = true;
	}

	// 5) Force-stream every level texture to full mip residency while the screen is up —
	//    the "massive loading after each floor" (2026-08-04) is first-sight texture
	//    streaming; front-load all of it here. Level packages hard-ref their materials
	//    and materials hard-ref textures, so every floor's textures are already LOADED
	//    as objects — only their mips stream. Marking them force-resident makes the
	//    streamer pull everything now instead of on first sight. (In PIE this also
	//    catches editor-loaded /Game textures — an acceptable superset.)
	if (CVarWarmupGateStreamTextures.GetValueOnGameThread() != 0)
	{
		// Cover the warm phase generously; the keep-resident re-mark happens at release.
		NumTexturesForced = ForceAllTexturesResident(CVarWarmupGateTimeoutSec.GetValueOnGameThread() + 30.0f);
		UE_LOG(LogTemp, Warning, TEXT("[WarmupGate] texture pre-stream: %d /Game textures forced to full residency."), NumTexturesForced);
	}

	UE_LOG(LogTemp, Warning, TEXT("[WarmupGate] HOLDING loading screen: preloading %d assets, draining loaders/shaders/PSOs (timeout %.0fs)."),
		UE_ARRAY_COUNT(GWarmupManifest), CVarWarmupGateTimeoutSec.GetValueOnGameThread());
}

void UZP_WarmupGateSubsystem::ReleaseGate(bool bTimedOut)
{
	UWorld* World = GetWorld();
	const double Elapsed = FPlatformTime::Seconds() - GateStartTime;

	// Undo the PSO-warm scaffolding (occlusion queries, camera spin, melee warm visibility) —
	// mandatory on the timeout path where the warm cycle may still be mid-flight.
	if (bOcclusionDisabled)
	{
		if (IConsoleVariable* Occ = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AllowOcclusionQueries")))
		{
			Occ->Set(OcclusionQueriesPrior, ECVF_SetByCode);
		}
		bOcclusionDisabled = false;
	}
	APawn* Pawn = nullptr;
	if (World)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			Pawn = PC->GetPawn();
		}
	}
	if (Pawn)
	{
		if (bWarmSpinRotCaptured)
		{
			if (UCameraComponent* Cam = Pawn->FindComponentByClass<UCameraComponent>())
			{
				Cam->SetWorldRotation(WarmSpinOriginalRot);
			}
		}
		if (WarmMeleePhase == 1)
		{
			TArray<USkeletalMeshComponent*> SkelComps;
			Pawn->GetComponents<USkeletalMeshComponent>(SkelComps);
			for (USkeletalMeshComponent* C : SkelComps)
			{
				const FString N = C->GetName();
				if (N == TEXT("MeleeViewMesh") || N == TEXT("MeleeViewOveralls") || N == TEXT("MeleeHands"))
				{
					C->SetVisibility(false);
				}
			}
			WarmMeleePhase = 2;
		}
	}

	// Unpause BEFORE the hide call — the EGUI hide graph runs a world-latent Delay node
	// that never fires in a paused world.
	if (bPausedByGate && World)
	{
		UGameplayStatics::SetGamePaused(World, false);
		bPausedByGate = false;
	}

	CallStartStopLoadingScreen(/*bStop=*/true, /*bFade=*/true);
	State = EGateState::Released;

	// Pin the pre-streamed mips for the whole session so floor/stair crossings never
	// re-stream textures (8h duration ≈ session lifetime; runtime-only flag, not saved).
	if (CVarWarmupGateStreamTextures.GetValueOnGameThread() != 0 &&
		CVarWarmupGateKeepTexturesResident.GetValueOnGameThread() != 0)
	{
		ForceAllTexturesResident(8.0f * 3600.0f);
	}

	if (bTimedOut)
	{
		UE_LOG(LogTemp, Error, TEXT("[WarmupGate] TIMED OUT after %.1fs — released anyway. Drained: loads=%s(%.1fs) shaders=%s(%.1fs) psos=%s(%.1fs) textures=%s(%.1fs)"),
			Elapsed,
			LoadsDrainedAt > 0.0 ? TEXT("yes") : TEXT("NO"), LoadsDrainedAt,
			ShadersDrainedAt > 0.0 ? TEXT("yes") : TEXT("NO"), ShadersDrainedAt,
			PSOsDrainedAt > 0.0 ? TEXT("yes") : TEXT("NO"), PSOsDrainedAt,
			TexturesDrainedAt > 0.0 ? TEXT("yes") : TEXT("NO"), TexturesDrainedAt);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WarmupGate] RELEASED after %.1fs (loads drained at %.1fs, shaders %.1fs, PSOs %.1fs, textures %.1fs [%d forced, resident=%s])."),
			Elapsed, LoadsDrainedAt, ShadersDrainedAt, PSOsDrainedAt, TexturesDrainedAt,
			NumTexturesForced,
			CVarWarmupGateKeepTexturesResident.GetValueOnGameThread() != 0 ? TEXT("session") : TEXT("temp"));
	}
}

int32 UZP_WarmupGateSubsystem::ForceAllTexturesResident(const float DurationSec) const
{
	int32 Count = 0;
	for (TObjectIterator<UTexture> It; It; ++It)
	{
		UTexture* Tex = *It;
		if (!IsValid(Tex) || Tex->IsTemplate())
		{
			continue;
		}
		// Scope to project content — engine/editor UI textures are not the hitch source.
		const UPackage* Pkg = Tex->GetOutermost();
		if (!Pkg || !Pkg->GetName().StartsWith(TEXT("/Game")))
		{
			continue;
		}
		Tex->SetForceMipLevelsToBeResident(DurationSec);
		Count++;
	}
	return Count;
}

bool UZP_WarmupGateSubsystem::AreTexturesDrained(const double Elapsed)
{
	if (CVarWarmupGateStreamTextures.GetValueOnGameThread() == 0)
	{
		return true;
	}
	if (Elapsed >= CVarWarmupGateTextureTimeoutSec.GetValueOnGameThread())
	{
		if (!bTextureTimeoutLogged)
		{
			bTextureTimeoutLogged = true;
			UE_LOG(LogTemp, Error, TEXT("[WarmupGate] texture pre-stream sub-timeout (%.0fs) — releasing without full residency. Streaming pool may be smaller than the level's texture set (check 'stat streaming'); crossings can still stream."),
				CVarWarmupGateTextureTimeoutSec.GetValueOnGameThread());
		}
		return true;
	}
	// Drained = the streamer has nothing left to load AND (in editor) the texture compiler has
	// no DDC encodes left. The compiles are the killer: first PIE after a pack import runs
	// hundreds of 2K/4K encodes and each lands as a render stall mid-play (151 measured
	// 2026-08-04). Require 2s of STABLE zero — both pipelines kick new work in waves.
	int32 Pending = IStreamingManager::Get().GetTextureStreamingManager().GetNumWantingResources();
#if WITH_EDITOR
	Pending += FTextureCompilingManager::Get().GetNumRemainingTextures();
#endif
	if (Pending == 0)
	{
		if (TexZeroSince == 0.0)
		{
			TexZeroSince = Elapsed;
		}
		return (Elapsed - TexZeroSince) >= 2.0;
	}
	TexZeroSince = 0.0;
	return false;
}

AActor* UZP_WarmupGateSubsystem::FindOrSpawnOpsManager()
{
	if (OpsManager.IsValid())
	{
		return OpsManager.Get();
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	UClass* MgrClass = LoadClass<AActor>(nullptr, GOpsManagerClassPath);
	if (!MgrClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[WarmupGate] BP_EasySaveGameOperationsManager class not found at %s — cannot drive the EGUI loading screen."), GOpsManagerClassPath);
		return nullptr;
	}
	AActor* Mgr = UGameplayStatics::GetActorOfClass(World, MgrClass);
	if (!Mgr)
	{
		Mgr = World->SpawnActor<AActor>(MgrClass);
	}
	OpsManager = Mgr;
	return Mgr;
}

void UZP_WarmupGateSubsystem::CallStartStopLoadingScreen(bool bStop, bool bFade)
{
	AActor* Mgr = FindOrSpawnOpsManager();
	if (!Mgr)
	{
		return;
	}
	UFunction* Fn = Mgr->FindFunction(FName(TEXT("StartStopLoadingScreen")));
	if (!Fn)
	{
		UE_LOG(LogTemp, Error, TEXT("[WarmupGate] StartStopLoadingScreen not found on %s — EGUI pack changed?"), *Mgr->GetName());
		return;
	}

	FStructOnScope Params(Fn);
	uint8* Mem = Params.GetStructMemory();
	for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		// BP param names carry literal '?' suffixes (StopLoadingScreen?, PlayFadeAnimation?)
		// — match by prefix so either spelling resolves.
		const FString N = It->GetName();
		if (FBoolProperty* BoolProp = CastField<FBoolProperty>(*It))
		{
			if (N.StartsWith(TEXT("StopLoadingScreen")))
			{
				BoolProp->SetPropertyValue_InContainer(Mem, bStop);
			}
			else if (N.StartsWith(TEXT("PlayFadeAnimation")))
			{
				BoolProp->SetPropertyValue_InContainer(Mem, bFade);
			}
		}
		else if (N.StartsWith(TEXT("OperationType")))
		{
			// E_SaveGameOperationType: 0=Save, 1=Load. Load honors the hide-delay path.
			if (FByteProperty* ByteProp = CastField<FByteProperty>(*It))
			{
				ByteProp->SetPropertyValue_InContainer(Mem, 1);
			}
			else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(*It))
			{
				EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Mem), (int64)1);
			}
		}
	}
	Mgr->ProcessEvent(Fn, Mem);
}

UUserWidget* UZP_WarmupGateSubsystem::GetActiveLoadingScreen() const
{
	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	if (!GI)
	{
		return nullptr;
	}
	UFunction* Fn = GI->FindFunction(FName(TEXT("GetCurrentActiveLoadingScreen")));
	if (!Fn)
	{
		return nullptr;
	}
	FStructOnScope Params(Fn);
	uint8* Mem = Params.GetStructMemory();
	GI->ProcessEvent(Fn, Mem);
	for (TFieldIterator<FProperty> It(Fn); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
		{
			if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(*It))
			{
				return Cast<UUserWidget>(ObjProp->GetObjectPropertyValue_InContainer(Mem));
			}
		}
	}
	return nullptr;
}

bool UZP_WarmupGateSubsystem::AreLoadsDrained() const
{
	if (PreloadHandle.IsValid() && !PreloadHandle->HasLoadCompleted())
	{
		return false;
	}
	return !IsAsyncLoading() && GetNumAsyncPackages() == 0;
}

bool UZP_WarmupGateSubsystem::AreShadersDrained() const
{
	if (CVarWarmupGateWaitForShaders.GetValueOnGameThread() == 0)
	{
		return true;
	}
	return GShaderCompilingManager == nullptr || !GShaderCompilingManager->IsCompiling();
}

bool UZP_WarmupGateSubsystem::ArePSOsDrained() const
{
	// Always 0 in PIE — the engine disables PSO precaching under WITH_EDITOR. Real
	// effect lands in -game / packaged builds.
	return PipelineStateCache::NumActivePrecacheRequests() == 0;
}
