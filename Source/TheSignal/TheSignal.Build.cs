// Copyright The Signal. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class TheSignal : ModuleRules
{
	public TheSignal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule", // ZP_PatrolComponent auto-binds UPawnSensingComponent
			"NavigationSystem", // crawler path-following to walls (UNavigationSystemV1) — engine module, not a plugin
			"UMG", // ZP_HUDWidget (UUserWidget, UImage, UTextBlock)
			"Niagara", // ZP_GrenadeProjectile explosion VFX
			"RenderCore", // ZP_LipSyncComponent morph target GPU buffer rebuild
			"RHI", // ZP_LipSyncComponent GMaxRHIShaderPlatform
			"AudioMixer", // ZP_LipSyncComponent submix buffer listener for OVRLipSync PCM capture
			"GameplayTags", // Notes bridge — FGameplayTag::RequestGameplayTag for Item.Note tag
			"Json",          // ZP_ObjectiveSubsystem (M2) — objective defs loaded from Content/Data/Objectives.json
			"JsonUtilities", // ZP_ObjectiveSubsystem (M2) — FJsonObjectConverter
			// FLAGGED FOR REVIEW (2026-07-02): PhysicsCore = ENGINE runtime module (not a plugin),
			// needed for UPhysicalMaterial/SurfaceType in the footstep surface system
			// (ZP_FootstepData). The NightShadow "no plugin modules" rule does not apply.
			"PhysicsCore",   // ZP_FootstepData — floor UPhysicalMaterial->SurfaceType resolution
			// FLAGGED FOR REVIEW (2026-07-12): ProceduralMeshComponent = ENGINE plugin module
			// (Engine/Plugins/Runtime/ProceduralMeshComponent, EnabledByDefault). Added to the
			// GAME module for ASM_Surface (procedural slab with real cut-out holes). The
			// NightShadow "no plugin modules in game deps" incident concerned plugins loaded
			// via .uproject that were ALSO compile deps of a plugin Build.cs; engine-default
			// runtime plugins linked from the game module are the documented usage.
			"ProceduralMeshComponent", // ASM_Surface — UProceduralMeshComponent slab
			// FLAGGED FOR REVIEW (UE 5.7 upgrade): FReply is in our PUBLIC header
			// (ZP_InventoryTabWidget::NativeOnKeyDown). 5.7 made FReply(bool) an
			// out-of-line SLATECORE_API ctor and Engine pulls SlateCore privately,
			// so the symbol no longer links transitively (link error LNK2019).
			// This is the GAME module, NOT a plugin — the NightShadow "no Slate in
			// Build.cs" rule targets PLUGIN Build.cs (packaged crash 777006) and
			// does not apply here.
			"SlateCore",
			// FLAGGED FOR REVIEW (2026-08-05): "Slate" added for IInputProcessor /
			// FSlateApplication (ZP_GlyphDeviceSubsystem — hardware-level input-device
			// detection; PlayerController polling is blind to menu-consumed gamepad keys).
			// GAME module, not a plugin Build.cs — the 777006 crash rule does not apply.
			"Slate"
		});

		// OVRLipSync C API — ThirdParty lib, NOT a UE plugin module dependency.
		// We call the C DLL directly via shim lib; the UE plugin handles DLL staging.
		string OVRLipSyncThirdParty = Path.Combine(ModuleDirectory, "..", "..", "Plugins", "OVRLipSync", "ThirdParty");
		PublicIncludePaths.Add(Path.Combine(OVRLipSyncThirdParty, "Include"));
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string OVRLibDir = Path.Combine(OVRLipSyncThirdParty, "Lib", "Win64");
			PublicAdditionalLibraries.Add(Path.Combine(OVRLibDir, "OVRLipSyncShim.lib"));
			PublicDelayLoadDLLs.Add("OVRLipSync.dll");
		}

		// NOTE: SlateCore IS required here as of UE 5.7 (see FReply note above).
		//       Do NOT add full "Slate" unless a link error demands it.
		// NOTE: Do NOT add plugin modules here (loaded via .uproject).

		// Editor-only deps for widget tree manipulation (ZP_EditorWidgetUtils)
		// and the skeletal->static mesh bake tool (ZP_MeshBakeUtils).
		// FLAGGED FOR REVIEW: MeshMergeUtilities is an engine DEVELOPER module
		// gated by bBuildEditor (never compiled into runtime/packaged builds),
		// so the NightShadow "no extra modules" rule (which targets PLUGIN
		// Build.cs / runtime deps) does not apply. It backs ZP_MeshBakeUtils,
		// the C++ "Convert to Static Mesh" (MergeComponentsToStaticMesh) that
		// UE5.7 Python does not expose.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UMGEditor", "UnrealEd", "MeshMergeUtilities" });
		}
	}
}
