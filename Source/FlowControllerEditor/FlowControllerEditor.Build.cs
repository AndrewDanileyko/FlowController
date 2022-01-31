// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FlowControllerEditor : ModuleRules
{
	public FlowControllerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "BlueprintGraph", "FlowControllerRuntime" });

		PrivateDependencyModuleNames.AddRange(new string[] { 
			"KismetCompiler", 
			"Slate",
			"SlateCore",
			"PropertyEditor",
			"UnrealEd",
			"EditorStyle",
			"GraphEditor"});

		PrivateIncludePathModuleNames.AddRange(new string[]	{ "Settings", "IntroTutorials", "AssetTools", "LevelEditor"	});

		DynamicallyLoadedModuleNames.AddRange(new string[] { "AssetTools" });
	}
}
