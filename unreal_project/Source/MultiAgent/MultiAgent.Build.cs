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
            // Agent contexts
            Path.Combine(ModuleDirectory, "Agent"),
            Path.Combine(ModuleDirectory, "Agent/CharacterRuntime"),
            Path.Combine(ModuleDirectory, "Agent/CharacterRuntime/Application"),
            Path.Combine(ModuleDirectory, "Agent/CharacterRuntime/Bootstrap"),
            Path.Combine(ModuleDirectory, "Agent/CharacterRuntime/Domain"),
            Path.Combine(ModuleDirectory, "Agent/CharacterRuntime/Feedback"),
            Path.Combine(ModuleDirectory, "Agent/CharacterRuntime/Infrastructure"),
            Path.Combine(ModuleDirectory, "Agent/CharacterRuntime/Runtime"),
            Path.Combine(ModuleDirectory, "Agent/Navigation"),
            Path.Combine(ModuleDirectory, "Agent/Navigation/Application"),
            Path.Combine(ModuleDirectory, "Agent/Navigation/Bootstrap"),
            Path.Combine(ModuleDirectory, "Agent/Navigation/Domain"),
            Path.Combine(ModuleDirectory, "Agent/Navigation/Feedback"),
            Path.Combine(ModuleDirectory, "Agent/Navigation/Infrastructure"),
            Path.Combine(ModuleDirectory, "Agent/Navigation/Runtime"),
            Path.Combine(ModuleDirectory, "Agent/Sensing"),
            Path.Combine(ModuleDirectory, "Agent/Sensing/Application"),
            Path.Combine(ModuleDirectory, "Agent/Sensing/Bootstrap"),
            Path.Combine(ModuleDirectory, "Agent/Sensing/Domain"),
            Path.Combine(ModuleDirectory, "Agent/Sensing/Feedback"),
            Path.Combine(ModuleDirectory, "Agent/Sensing/Infrastructure"),
            Path.Combine(ModuleDirectory, "Agent/Sensing/Runtime"),
            Path.Combine(ModuleDirectory, "Agent/Skill"),
            Path.Combine(ModuleDirectory, "Agent/Skill/Application"),
            Path.Combine(ModuleDirectory, "Agent/Skill/Bootstrap"),
            Path.Combine(ModuleDirectory, "Agent/Skill/Domain"),
            Path.Combine(ModuleDirectory, "Agent/Skill/Feedback"),
            Path.Combine(ModuleDirectory, "Agent/Skill/Infrastructure"),
            Path.Combine(ModuleDirectory, "Agent/Skill/Runtime"),
            Path.Combine(ModuleDirectory, "Agent/Skill/Runtime/Impl"),
            Path.Combine(ModuleDirectory, "Agent/StateTree"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Application"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Bootstrap"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Domain"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Feedback"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Infrastructure"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Runtime"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Runtime/Task"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Runtime/Condition")
        });
    }
}
