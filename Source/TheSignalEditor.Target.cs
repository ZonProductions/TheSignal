// Copyright The Signal. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TheSignalEditorTarget : TargetRules
{
	public TheSignalEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("TheSignal");
	}
}
