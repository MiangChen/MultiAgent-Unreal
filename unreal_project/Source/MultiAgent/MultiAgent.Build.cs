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
            Path.Combine(ModuleDirectory, "Core"),
            Path.Combine(ModuleDirectory, "Core/Manager"),
            Path.Combine(ModuleDirectory, "Core/Types"),
            Path.Combine(ModuleDirectory, "Core/Config"),
            Path.Combine(ModuleDirectory, "Core/GameFlow"),
            Path.Combine(ModuleDirectory, "Core/Comm"),
            Path.Combine(ModuleDirectory, "Input"),
            Path.Combine(ModuleDirectory, "Environment"),
            Path.Combine(ModuleDirectory, "UI"),
            Path.Combine(ModuleDirectory, "UI/Core"),
            Path.Combine(ModuleDirectory, "UI/Components"),
            Path.Combine(ModuleDirectory, "UI/Modal"),
            // Agent 相关
            Path.Combine(ModuleDirectory, "Agent"),
            Path.Combine(ModuleDirectory, "Agent/Character"),
            Path.Combine(ModuleDirectory, "Agent/Component"),
            Path.Combine(ModuleDirectory, "Agent/Component/Sensor"),
            Path.Combine(ModuleDirectory, "Agent/GAS"),
            Path.Combine(ModuleDirectory, "Agent/GAS/Ability"),
            Path.Combine(ModuleDirectory, "Agent/StateTree"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Task"),
            Path.Combine(ModuleDirectory, "Agent/StateTree/Condition")
        });
    }
}