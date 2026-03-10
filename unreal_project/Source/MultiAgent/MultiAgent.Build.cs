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
            // UMG (UI) - 可能需要
            "UMG",
            "Slate",
            "SlateCore",
            // 网络和图像模块 (Camera TCP Stream)
            "Sockets",
            "Networking",
            "ImageWrapper",
            "RenderCore",
            // JSON 解析 (Agent 配置)
            "Json",
            "JsonUtilities",
            // HTTP 模块 (通信子系统)
            "HTTP",
            "HTTPServer",
            // Niagara (特效系统)
            "Niagara",
            // GeometryCore (几何计算模块 - OBB, Prism 等)
            "GeometryCore",
            // DirectoryWatcher (文件监听模块 - TempDataManager)
            "DirectoryWatcher"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // 添加子文件夹的 include 路径
        PublicIncludePaths.AddRange(new string[] {
            ModuleDirectory,
            Path.Combine(ModuleDirectory, "Core"),
            Path.Combine(ModuleDirectory, "Core/AgentRuntime"),
            Path.Combine(ModuleDirectory, "Core/Command"),
            Path.Combine(ModuleDirectory, "Core/EnvironmentCore"),
            Path.Combine(ModuleDirectory, "Core/Selection"),
            Path.Combine(ModuleDirectory, "Core/Shared"),
            Path.Combine(ModuleDirectory, "Core/SkillAllocation"),
            Path.Combine(ModuleDirectory, "Core/Squad"),
            Path.Combine(ModuleDirectory, "Core/TempData"),
            Path.Combine(ModuleDirectory, "Core/TaskGraph"),
            Path.Combine(ModuleDirectory, "Core/Config"),
            Path.Combine(ModuleDirectory, "Core/GameFlow"),
            Path.Combine(ModuleDirectory, "Core/Comm"),
            Path.Combine(ModuleDirectory, "Input"),
            Path.Combine(ModuleDirectory, "Environment"),
            Path.Combine(ModuleDirectory, "UI"),
            Path.Combine(ModuleDirectory, "UI/Core"),
            Path.Combine(ModuleDirectory, "UI/Components"),
            Path.Combine(ModuleDirectory, "UI/Core/Modal"),
            // Agent 相关
            Path.Combine(ModuleDirectory, "Agent"),
            Path.Combine(ModuleDirectory, "Agent/Character"),
            Path.Combine(ModuleDirectory, "Agent/Component"),
            Path.Combine(ModuleDirectory, "Agent/Component/Sensor"),
            // GAS 目录已移除，如需使用请创建对应目录
            // Path.Combine(ModuleDirectory, "Agent/GAS"),
            // Path.Combine(ModuleDirectory, "Agent/GAS/Ability"),
            Path.Combine(ModuleDirectory, "Agent/StateTree"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Task"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Condition")
        });
    }
}
