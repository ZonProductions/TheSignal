// Copyright The Signal. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TheSignalTarget : TargetRules
{
	public TheSignalTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// Latest (not explicit V7 / Unreal5_8): the resync tool builds Target.cs with
		// an older UBT that lacks the Unreal5_8 enum. Latest resolves per-engine and
		// never triggers the BuildSettings upgrade nag.
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		// UE 5.8 defaults these warning levels to Error, but the installed engine
		// binaries were built with them Off. Override so our shared-env target can build.
		bOverrideBuildEnvironment = true;
		ExtraModuleNames.Add("TheSignal");
	}
}
