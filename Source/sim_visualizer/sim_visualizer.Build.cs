// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class sim_visualizer : ModuleRules
{
	public sim_visualizer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "RHI", "RenderCore",  "Networking", "Sockets" });


		
		if (Target.bBuildEditor)
		{
		    PrivateDependencyModuleNames.AddRange(
		        new string[] {
		            "UnrealEd",
		            "EditorFramework",
		            "EditorSubsystem",
		            "EditorStyle"
		        }
		    );
		}
		



		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
