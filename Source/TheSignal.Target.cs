// Copyright The Signal. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TheSignalTarget : TargetRules
{
	public TheSignalTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("TheSignal");
	}
}
