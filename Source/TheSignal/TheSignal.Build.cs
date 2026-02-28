// Copyright The Signal. All Rights Reserved.

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
			"EnhancedInput"
		});

		// NOTE: Do NOT add SlateCore/Slate here (transitive deps of Engine).
		// NOTE: Do NOT add plugin modules here (loaded via .uproject).
	}
}
