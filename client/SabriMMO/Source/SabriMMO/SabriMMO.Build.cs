// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SabriMMO : ModuleRules
{
	public SabriMMO(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"HTTP",
			"Json",
			"JsonUtilities"
		});

		// Exclude NetworkPrediction module due to UE 5.7 compiler bug
		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SabriMMO",
			"SabriMMO/Variant_Platforming",
			"SabriMMO/Variant_Platforming/Animation",
			"SabriMMO/Variant_Combat",
			"SabriMMO/Variant_Combat/AI",
			"SabriMMO/Variant_Combat/Animation",
			"SabriMMO/Variant_Combat/Gameplay",
			"SabriMMO/Variant_Combat/Interfaces",
			"SabriMMO/Variant_Combat/UI",
			"SabriMMO/Variant_SideScrolling",
			"SabriMMO/Variant_SideScrolling/AI",
			"SabriMMO/Variant_SideScrolling/Gameplay",
			"SabriMMO/Variant_SideScrolling/Interfaces",
			"SabriMMO/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
