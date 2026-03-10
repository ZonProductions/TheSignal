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
			"UMG", // ZP_HUDWidget (UUserWidget, UImage, UTextBlock)
			"Niagara", // ZP_GrenadeProjectile explosion VFX
			"RenderCore", // ZP_LipSyncComponent morph target GPU buffer rebuild
			"RHI", // ZP_LipSyncComponent GMaxRHIShaderPlatform
			"AudioMixer" // ZP_LipSyncComponent submix buffer listener for OVRLipSync PCM capture
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

		// NOTE: Do NOT add SlateCore/Slate here (transitive deps of Engine).
		// NOTE: Do NOT add plugin modules here (loaded via .uproject).

		// Editor-only deps for widget tree manipulation (ZP_EditorWidgetUtils)
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UMGEditor", "UnrealEd" });
		}
	}
}
