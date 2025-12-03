#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "../AgentManager/MAAgent.h"
#include "MAGameMode.generated.h"

class UMAAgentSubsystem;

/**
 * AMAGameMode - 游戏模式
 * 负责初始化各个子系统，协调游戏流程
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

    // 生成人类 Agent（使用蓝图，不通过 Subsystem）
    UFUNCTION(BlueprintCallable, Category = "Agent")
    APawn* SpawnHumanAgent(FVector Location, FRotator Rotation, int32 AgentID);

    // 获取所有 Pawn（包括蓝图人类和 C++ Agent）
    UFUNCTION(BlueprintCallable, Category = "Agent")
    TArray<APawn*> GetAllPawns() const;

    // 人类 Agent 蓝图类
    UPROPERTY(EditDefaultsOnly, Category = "Agent")
    TSubclassOf<APawn> HumanAgentBPClass;

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

private:
    UPROPERTY()
    TArray<APawn*> SpawnedHumanPawns;
};
