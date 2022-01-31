// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FlowControllerRuntime : ModuleRules
{
	public FlowControllerRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
