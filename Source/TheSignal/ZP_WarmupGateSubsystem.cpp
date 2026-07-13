// ZP_WarmupGateSubsystem.cpp — see header for purpose/ownership.

#include "ZP_WarmupGateSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "PipelineStateCache.h"
#include "ShaderCompiler.h"
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

	if (LoadsDrainedAt == 0.0 && AreLoadsDrained())     { LoadsDrainedAt = Elapsed; }
	if (ShadersDrainedAt == 0.0 && AreShadersDrained()) { ShadersDrainedAt = Elapsed; }
	if (PSOsDrainedAt == 0.0 && ArePSOsDrained())       { PSOsDrainedAt = Elapsed; }

	const bool bAllDrained = LoadsDrainedAt > 0.0 && ShadersDrainedAt > 0.0 && PSOsDrainedAt > 0.0;
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

	UE_LOG(LogTemp, Warning, TEXT("[WarmupGate] HOLDING loading screen: preloading %d assets, draining loaders/shaders/PSOs (timeout %.0fs)."),
		UE_ARRAY_COUNT(GWarmupManifest), CVarWarmupGateTimeoutSec.GetValueOnGameThread());
}

void UZP_WarmupGateSubsystem::ReleaseGate(bool bTimedOut)
{
	UWorld* World = GetWorld();
	const double Elapsed = FPlatformTime::Seconds() - GateStartTime;

	// Unpause BEFORE the hide call — the EGUI hide graph runs a world-latent Delay node
	// that never fires in a paused world.
	if (bPausedByGate && World)
	{
		UGameplayStatics::SetGamePaused(World, false);
		bPausedByGate = false;
	}

	CallStartStopLoadingScreen(/*bStop=*/true, /*bFade=*/true);
	State = EGateState::Released;

	if (bTimedOut)
	{
		UE_LOG(LogTemp, Error, TEXT("[WarmupGate] TIMED OUT after %.1fs — released anyway. Drained: loads=%s(%.1fs) shaders=%s(%.1fs) psos=%s(%.1fs)"),
			Elapsed,
			LoadsDrainedAt > 0.0 ? TEXT("yes") : TEXT("NO"), LoadsDrainedAt,
			ShadersDrainedAt > 0.0 ? TEXT("yes") : TEXT("NO"), ShadersDrainedAt,
			PSOsDrainedAt > 0.0 ? TEXT("yes") : TEXT("NO"), PSOsDrainedAt);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WarmupGate] RELEASED after %.1fs (loads drained at %.1fs, shaders %.1fs, PSOs %.1fs)."),
			Elapsed, LoadsDrainedAt, ShadersDrainedAt, PSOsDrainedAt);
	}
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
