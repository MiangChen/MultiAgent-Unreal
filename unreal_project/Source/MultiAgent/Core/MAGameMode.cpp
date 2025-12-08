// MAGameMode.cpp

#include "MAGameMode.h"
#include "../Input/MAPlayerController.h"
#include "../UI/MASelectionHUD.h"
#include "MAAgentManager.h"
#include "GameFramework/SpectatorPawn.h"
#include "Misc/Paths.h"

AMAGameMode::AMAGameMode()
{
    // 玩家使用 SpectatorPawn 作为上帝视角（原生飞行控制）
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    PlayerControllerClass = AMAPlayerController::StaticClass();
    
    // 设置 HUD 类用于绘制选择框
    HUDClass = AMASelectionHUD::StaticClass();
}

void AMAGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // 延迟一帧生成 Agent，确保玩家和子系统已经初始化
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        LoadAndSpawnAgents();
    });
}

UMAAgentManager* AMAGameMode::GetAgentManager() const
{
    return GetWorld()->GetSubsystem<UMAAgentManager>();
}

void AMAGameMode::LoadAndSpawnAgents()
{
    UMAAgentManager* AgentManager = GetAgentManager();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] AgentManager not found!"));
        return;
    }

    // 构建配置文件的绝对路径
    // AgentConfigPath 是相对于 unreal_project 目录的路径
    FString ProjectDir = FPaths::ProjectDir();
    FString ConfigFullPath = FPaths::Combine(ProjectDir, AgentConfigPath);
    ConfigFullPath = FPaths::ConvertRelativePathToFull(ConfigFullPath);
    
    UE_LOG(LogTemp, Log, TEXT("[GameMode] Loading agent config from: %s"), *ConfigFullPath);
    
    if (!AgentManager->LoadAndSpawnFromConfig(ConfigFullPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] Failed to load agent config from: %s"), *ConfigFullPath);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[GameMode] Successfully spawned %d agents from config"), AgentManager->GetAgentCount());
}