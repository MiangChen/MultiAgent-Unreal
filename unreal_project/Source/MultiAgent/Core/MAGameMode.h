#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "../AgentManager/MAAgent.h"
#include "MAGameMode.generated.h"

class UMAAgentSubsystem;
class AMACameraAgent;

/**
 * AMAGameMode - 游戏模式
 * 负责初始化各个子系统，协调游戏流程
 * 所有 Agent 统一通过 UMAAgentSubsystem 管理
 */
UCLASS()
class MULTIAGENT_API AMAGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMAGameMode();

    // 获取 AgentSubsystem
    UFUNCTION(BlueprintCallable, Category = "Subsystem")
    UMAAgentSubsystem* GetAgentSubsystem() const;

    // 人类 Agent C++ 类
    UPROPERTY(EditDefaultsOnly, Category = "Agent")
    TSubclassOf<AMAAgent> HumanAgentClass;

    // 机器狗 Agent C++ 类
    UPROPERTY(EditDefaultsOnly, Category = "Agent")
    TSubclassOf<AMAAgent> RobotDogAgentClass;

    // 生成的人类数量
    UPROPERTY(EditDefaultsOnly, Category = "Agent")
    int32 NumHumans = 2;

    // 生成的机器狗数量
    UPROPERTY(EditDefaultsOnly, Category = "Agent")
    int32 NumRobotDogs = 3;

protected:
    virtual void BeginPlay() override;
    void SpawnInitialAgents();
    
    // 生成并附着摄像头到 Agent
    void SpawnAndAttachCamera(UMAAgentSubsystem* AgentSubsystem, AMAAgent* ParentAgent, FVector RelativeLocation, FRotator RelativeRotation);
    
    // 生成追踪者 Agent
    void SpawnTrackerAgent(UMAAgentSubsystem* AgentSubsystem);
    
    // 追踪者 Agent 引用
    UPROPERTY()
    AMAAgent* TrackerAgent = nullptr;
};
