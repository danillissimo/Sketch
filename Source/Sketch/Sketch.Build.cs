// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Sketch : ModuleRules
{
	public Sketch(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		if (Target.Configuration == UnrealTargetConfiguration.Debug ||
		    Target.Configuration == UnrealTargetConfiguration.DebugGame)
		{
			bUseUnity = false;
			IWYUSupport = IWYUSupport.Full;
		}

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"SlateCore",
			"Slate",
			"ToolMenus",
			"Projects",

			"BlueprintGraph",
			"KismetCompiler"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AppFramework",
			"ApplicationCore", "GoogleTest",
        });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd", // Used for source code navigation, can be potentially replaced with direct access to SourceCodeAccess module
				"WorkspaceMenuStructure", // Used to add submenu to the "Tools" menu
				"StatusBar",
            });
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}

	// Not working yet
	// public static void SetupFor(ModuleRules Module, ReadOnlyTargetRules Target)
	// {
	// 	System.Console.WriteLine($"Sketch: {Target.bBuildEditor}");
	// 	if (Target.bBuildEditor && Target.Configuration != UnrealTargetConfiguration.Shipping)
	// 	{
	// 		Module.PrivateIncludePathModuleNames.Add("Sketch");
	// 		Module.ForceIncludeFiles.Add("Sketch/Public/Sketch.h");
	// 	}
	// }
}