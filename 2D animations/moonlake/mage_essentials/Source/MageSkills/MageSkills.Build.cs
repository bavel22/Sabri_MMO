// MageSkills.Build.cs
// Drop the Source/MageSkills folder into a UE5 project's Source/<YourGame>/Public+Private,
// or make it its own module by adding this Build.cs and a MageSkills.cpp module file.

using UnrealBuildTool;

public class MageSkills : ModuleRules
{
    public MageSkills(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Paper2D",
            "Niagara",
            "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore"
        });
    }
}
