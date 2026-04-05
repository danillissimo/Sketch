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
			"Projects", // Used by FSketchStyle
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AppFramework", // Used for things like color picker
			"ApplicationCore", // Used for clipboard operations
			"UnrealEd", // Used for source code navigation, can be potentially replaced with direct access to SourceCodeAccess module
			"WorkspaceMenuStructure", // Used to add submenu to the "Tools" menu
			"StatusBar",
		});
	}

	public static void SetupFor(ModuleRules Module, ReadOnlyTargetRules Target)
	{
		if (Target.bBuildEditor && Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			Module.PrivateDependencyModuleNames.Add("Sketch");
			Module.PrivateDefinitions.Add("SketchHeader=\"Sketch.h\"");
			// Works, but not currently supported by resharper
			// Module.ForceIncludeFiles.Add("Sketch.h");
		}
		else
		{
			Module.PrivateIncludePathModuleNames.Add("Sketch");
			Module.PrivateDefinitions.Add("SketchHeader=\"NoSketch.h\"");
		}
	}
}