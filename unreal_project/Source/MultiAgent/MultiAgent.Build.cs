using UnrealBuildTool;
using System.IO;

public class MultiAgent : ModuleRules
{
    public MultiAgent(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", 
            "CoreUObject", 
            "Engine", 
            "InputCore",
            "EnhancedInput",
            "AIModule",
            "NavigationSystem"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // 添加子文件夹的 include 路径
        PublicIncludePaths.AddRange(new string[] {
            Path.Combine(ModuleDirectory, "AgentManager"),
            Path.Combine(ModuleDirectory, "Core")
        });
    }
}
