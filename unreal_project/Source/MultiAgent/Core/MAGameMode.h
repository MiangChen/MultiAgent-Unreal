// MAGameMode.h
// 游戏模式 - 负责初始化各个子系统，协调游戏流程
// 使用 JSON 配置驱动 Agent 创建

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MAGameMode.generated.h"

class UMAAgentManager;

UCLASS()
class MULTIAGENT_API AMAGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMAGameMode();

    // 获取 AgentManager
    UFUNCTION(BlueprintCallable, Category = "Subsystem")
    UMAAgentManager* GetAgentManager() const;

    // Agent 配置文件路径（相对于项目根目录）
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    FString AgentConfigPath = TEXT("../config/agents.json");

protected:
    virtual void BeginPlay() override;
    
    // 从配置文件加载并生成所有 Agent
    void LoadAndSpawnAgents();
};