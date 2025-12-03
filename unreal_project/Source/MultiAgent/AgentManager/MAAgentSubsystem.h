#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MAAgent.h"
#include "MAAgentSubsystem.generated.h"

class AMAAgent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentSpawned, AMAAgent*, Agent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentDestroyed, AMAAgent*, Agent);

/**
 * UMAAgentSubsystem - 智能体管理子系统 (司命)
 * 负责所有智能体的生命周期管理：spawn、destroy、查询、按类型分组
 */
UCLASS()
class MULTIAGENT_API UMAAgentSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // 生成 Agent
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    AMAAgent* SpawnAgent(TSubclassOf<AMAAgent> AgentClass, FVector Location, FRotator Rotation, int32 AgentID, EAgentType Type);

    // 销毁 Agent
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    bool DestroyAgent(AMAAgent* Agent);

    // 获取所有 Agent
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    TArray<AMAAgent*> GetAllAgents() const { return SpawnedAgents; }

    // 按类型查询
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    TArray<AMAAgent*> GetAgentsByType(EAgentType Type) const;

    // 按 ID 查询
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    AMAAgent* GetAgentByID(int32 AgentID) const;

    // 按名称查询
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    AMAAgent* GetAgentByName(const FString& Name) const;

    // 获取 Agent 数量
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    int32 GetAgentCount() const { return SpawnedAgents.Num(); }

    // 批量停止所有 Agent
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    void StopAllAgents();

    // 批量移动所有 Agent 到目标位置（围绕目标点散开）
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    void MoveAllAgentsTo(FVector Destination, float Radius = 150.f);

    // Agent 生成时的委托
    UPROPERTY(BlueprintAssignable, Category = "AgentManager")
    FOnAgentSpawned OnAgentSpawned;

    // Agent 销毁时的委托
    UPROPERTY(BlueprintAssignable, Category = "AgentManager")
    FOnAgentDestroyed OnAgentDestroyed;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    UPROPERTY()
    TArray<AMAAgent*> SpawnedAgents;

    int32 NextAgentID = 0;
};
