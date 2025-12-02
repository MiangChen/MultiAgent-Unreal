#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MAAgent.h"
#include "MAGameMode.generated.h"

class AMACharacter;

UCLASS()
class MULTIAGENT_API AMAGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMAGameMode();

    // 生成 MAAgent
    UFUNCTION(BlueprintCallable, Category = "Agent")
    AMAAgent* SpawnAgent(TSubclassOf<AMAAgent> AgentClass, FVector Location, FRotator Rotation, int32 AgentID, EAgentType Type);

    // 生成人类 Agent（使用蓝图）
    UFUNCTION(BlueprintCallable, Category = "Agent")
    APawn* SpawnHumanAgent(FVector Location, FRotator Rotation, int32 AgentID);

    // 获取所有 MAAgent
    UFUNCTION(BlueprintCallable, Category = "Agent")
    TArray<AMAAgent*> GetAllAgents() const { return SpawnedAgents; }

    // 获取所有 Pawn（包括人类和机器狗）
    UFUNCTION(BlueprintCallable, Category = "Agent")
    TArray<APawn*> GetAllPawns() const;

    // 获取指定类型的 Agent
    UFUNCTION(BlueprintCallable, Category = "Agent")
    TArray<AMAAgent*> GetAgentsByType(EAgentType Type) const;

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
    TArray<AMAAgent*> SpawnedAgents;

    UPROPERTY()
    TArray<APawn*> SpawnedHumanPawns;
};
