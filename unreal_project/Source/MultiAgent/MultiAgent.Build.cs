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
            "NavigationSystem",
            // GAS 模块
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
            // State Tree 模块
            "StateTreeModule",
            "GameplayStateTreeModule"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // 添加子文件夹的 include 路径
        PublicIncludePaths.AddRange(new string[] {
            Path.Combine(ModuleDirectory, "AgentManager"),
            Path.Combine(ModuleDirectory, "Core"),
            Path.Combine(ModuleDirectory, "GAS"),
            Path.Combine(ModuleDirectory, "GAS/Abilities"),
            Path.Combine(ModuleDirectory, "Interaction"),
            Path.Combine(ModuleDirectory, "StateTree")
        });
    }
}
