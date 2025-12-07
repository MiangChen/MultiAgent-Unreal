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
            "GameplayStateTreeModule",
            // 网络和图像模块 (Camera TCP Stream)
            "Sockets",
            "Networking",
            "ImageWrapper",
            "RenderCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // 添加子文件夹的 include 路径
        PublicIncludePaths.AddRange(new string[] {
            Path.Combine(ModuleDirectory, "Core"),
            Path.Combine(ModuleDirectory, "Input"),
            Path.Combine(ModuleDirectory, "Environment"),
            // Agent 相关
            Path.Combine(ModuleDirectory, "Agent"),
            Path.Combine(ModuleDirectory, "Agent/Character"),
            Path.Combine(ModuleDirectory, "Agent/GAS"),
            Path.Combine(ModuleDirectory, "Agent/GAS/Ability"),
            Path.Combine(ModuleDirectory, "Agent/StateTree"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Task"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Condition"),
            // Module 可组装组件
            Path.Combine(ModuleDirectory, "Module"),
            Path.Combine(ModuleDirectory, "Module/Sensor")
        });
    }
}