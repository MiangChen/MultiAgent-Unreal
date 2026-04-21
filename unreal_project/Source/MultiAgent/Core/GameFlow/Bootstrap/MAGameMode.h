// MAGameMode.h
// 游戏模式 - 负责初始化各个子系统，协调游戏流程
// Agent 通过 Setup UI 界面配置

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MAGameMode.generated.h"

class UMAAgentManager;
class UMAEnvironmentManager;
class UMAEditModeManager;

UCLASS()
class MULTIAGENT_API AMAGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMAGameMode();

    // 获取 AgentManager
    UFUNCTION(BlueprintCallable, Category = "Subsystem")
    UMAAgentManager* GetAgentManager() const;

    // 获取 EnvironmentManager
    UFUNCTION(BlueprintCallable, Category = "Subsystem")
    UMAEnvironmentManager* GetEnvironmentManager() const;

protected:
    virtual void BeginPlay() override;
    
    // 设置 Spectator 初始位置（从 SimConfig.json 读取）
    void SetupSpectatorStart();
    
    // 从配置文件加载并生成所有 Agent
    void LoadAndSpawnAgents();
    
    // 从 Setup 配置生成 Agent
    void SpawnAgentsFromSetupConfig(class UMAGameInstance* GameInstance);
    
    // 查找安全的 Spectator 位置（避免卡在建筑内）
    FVector FindSafeSpectatorLocation(FVector DesiredLocation);

    // 初始化 Edit Mode 管理器
    void InitializeEditModeManager();

    // 初始化外部摄像头
    void InitializeExternalCameras();

    // 设置描边后处理效果
    void SetupOutlinePostProcess();
};
