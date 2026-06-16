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
			// FLAGGED FOR REVIEW (UE 5.7 upgrade): FReply is in our PUBLIC header
			// (ZP_InventoryTabWidget::NativeOnKeyDown). 5.7 made FReply(bool) an
			// out-of-line SLATECORE_API ctor and Engine pulls SlateCore privately,
			// so the symbol no longer links transitively (link error LNK2019).
			// This is the GAME module, NOT a plugin — the NightShadow "no Slate in
			// Build.cs" rule targets PLUGIN Build.cs (packaged crash 777006) and
			// does not apply here.
			"SlateCore"
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
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UMGEditor", "UnrealEd" });
		}
	}
}
