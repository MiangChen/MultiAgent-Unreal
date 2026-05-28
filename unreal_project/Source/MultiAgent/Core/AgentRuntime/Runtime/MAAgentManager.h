// MAAgentManager.h
// Agent 管理器 - 负责所有 Agent 的生命周期管理

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/Shared/Types/MATypes.h"
#include "MAAgentManager.generated.h"

class AMACharacter;
class UMAConfigManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentSpawned, AMACharacter*, Agent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentDestroyed, AMACharacter*, Agent);

/**
 * Agent 管理器
 * 
 * 职责:
 * - 从 ConfigManager 获取配置并生成 Agent
 * - 管理 Agent 生命周期
 * - 按类型/ID 查询 Agent
 * 
 * 注意: 环境对象 (person, vehicle, boat, cargo 等) 由 MAEnvironmentManager 管理
 */
UCLASS()
class MULTIAGENT_API UMAAgentManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ========== Agent 生成 ==========
    
    /** 从 ConfigManager 加载配置并生成所有 Agent */
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    void SpawnAgentsFromConfig();

    /** 按类型生成单个 Agent */
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    AMACharacter* SpawnAgentByType(const FString& TypeName, FVector Location, FRotator Rotation, bool bAutoPosition = true);

    // ========== Agent 管理 ==========
    
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    bool DestroyAgent(AMACharacter* Agent);

    UFUNCTION(BlueprintPure, Category = "AgentManager")
    TArray<AMACharacter*> GetAllAgents() const { return SpawnedAgents; }

    UFUNCTION(BlueprintPure, Category = "AgentManager")
    TArray<AMACharacter*> GetAgentsByType(EMAAgentType Type) const;

    UFUNCTION(BlueprintPure, Category = "AgentManager")
    AMACharacter* GetAgentByIDOrLabel(const FString& AgentID) const;

    UFUNCTION(BlueprintPure, Category = "AgentManager")
    int32 GetAgentCount() const { return SpawnedAgents.Num(); }

    // ========== 批量操作 ==========
    
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    void StopAllAgents();

    // ========== 委托 ==========
    
    UPROPERTY(BlueprintAssignable, Category = "AgentManager")
    FOnAgentSpawned OnAgentSpawned;

    UPROPERTY(BlueprintAssignable, Category = "AgentManager")
    FOnAgentDestroyed OnAgentDestroyed;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    UPROPERTY()
    TArray<AMACharacter*> SpawnedAgents;

    int32 NextAgentIndex = 0;
    
    UMAConfigManager* GetConfigManager() const;
    AMACharacter* SpawnAgentInternal(const FString& TypeName, const FString& ID, const FString& Label, FVector Location, FRotator Rotation, bool bAutoPosition, int32 Index, int32 TotalCount, float BatteryLevel = 100.f);
    
    FVector CalculateAutoPosition(int32 Index, int32 TotalCount) const;
    FVector AdjustSpawnHeight(FVector Location, bool bIsFlying) const;
    
    EMAAgentType StringToAgentType(const FString& TypeString) const;
    FString GetClassPathForType(const FString& TypeName) const;
};
