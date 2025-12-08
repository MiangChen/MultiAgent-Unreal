// MAAgentManager.h
// Agent 管理器 - 负责所有 Agent 的生命周期管理和集群行为
// 支持 JSON 配置驱动的 Agent 创建
// 同时作为集群管理器，处理编队等集群行为（类似 CARLA TrafficManager）

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MATypes.h"
#include "MAAgentManager.generated.h"

class AMACharacter;
class UMASensorComponent;

// Sensor 配置结构
USTRUCT(BlueprintType)
struct FMASensorConfig
{
    GENERATED_BODY()

    UPROPERTY()
    FString Type;

    UPROPERTY()
    FVector Position = FVector::ZeroVector;

    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY()
    FIntPoint Resolution = FIntPoint(640, 480);

    UPROPERTY()
    float FOV = 90.f;
};

// Agent 配置结构
USTRUCT(BlueprintType)
struct FMAAgentConfig
{
    GENERATED_BODY()

    UPROPERTY()
    FString ID;

    UPROPERTY()
    EMAAgentType Type = EMAAgentType::Human;

    UPROPERTY()
    FString ClassName;

    UPROPERTY()
    FVector Position = FVector::ZeroVector;

    UPROPERTY()
    bool bAutoPosition = true;

    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY()
    TArray<FMASensorConfig> Sensors;
};

// 生成设置结构
USTRUCT(BlueprintType)
struct FMASpawnSettings
{
    GENERATED_BODY()

    UPROPERTY()
    bool bUsePlayerStart = true;

    UPROPERTY()
    FVector FallbackOrigin = FVector(0.f, 0.f, 100.f);

    UPROPERTY()
    float SpawnRadius = 400.f;

    UPROPERTY()
    bool bProjectToNavMesh = true;
};

// 充电站配置
USTRUCT(BlueprintType)
struct FMAChargingStationConfig
{
    GENERATED_BODY()

    UPROPERTY()
    FString ID;

    UPROPERTY()
    FVector Position = FVector::ZeroVector;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentSpawned, AMACharacter*, Agent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentDestroyed, AMACharacter*, Agent);

UCLASS()
class MULTIAGENT_API UMAAgentManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ========== 配置加载 ==========
    
    // 从 JSON 文件加载配置并生成所有 Agent
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    bool LoadAndSpawnFromConfig(const FString& ConfigPath);
    
    // 仅加载配置（不生成）
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    bool LoadConfig(const FString& ConfigPath);
    
    // 根据已加载的配置生成所有 Agent
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    void SpawnAllAgentsFromConfig();

    // ========== Agent 管理 ==========
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    AMACharacter* SpawnAgent(TSubclassOf<AMACharacter> AgentClass, FVector Location, FRotator Rotation, const FString& AgentID, EMAAgentType Type);

    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    bool DestroyAgent(AMACharacter* Agent);

    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    TArray<AMACharacter*> GetAllAgents() const { return SpawnedAgents; }

    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    TArray<AMACharacter*> GetAgentsByType(EMAAgentType Type) const;

    // 获取所有无人机 (包括 Phantom4, Inspire2 等所有子类型)
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    TArray<AMACharacter*> GetAllDrones() const;

    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    AMACharacter* GetAgentByID(const FString& AgentID) const;

    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    AMACharacter* GetAgentByIndex(int32 Index) const;

    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    int32 GetAgentCount() const { return SpawnedAgents.Num(); }

    // ========== Action 动态发现 ==========
    
    // 获取指定 Agent 的所有可用 Actions
    UFUNCTION(BlueprintCallable, Category = "AgentManager|Actions")
    TArray<FString> GetAgentAvailableActions(AMACharacter* Agent) const;
    
    // 执行指定 Agent 的 Action
    UFUNCTION(BlueprintCallable, Category = "AgentManager|Actions")
    bool ExecuteAgentAction(AMACharacter* Agent, const FString& ActionName, const TMap<FString, FString>& Params);
    
    // 获取所有 Agent 的 Actions 汇总 (C++ only, 不暴露给蓝图)
    TMap<FString, TArray<FString>> GetAllAgentsActions() const;

    // ========== 批量操作 ==========
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    void StopAllAgents();

    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    void MoveAllAgentsTo(FVector Destination, float Radius = 150.f);

    // ========== 编队管理 (Formation Manager) ==========
    
    UFUNCTION(BlueprintCallable, Category = "Formation")
    void StartFormation(AMACharacter* Leader, EMAFormationType Type);
    
    UFUNCTION(BlueprintCallable, Category = "Formation")
    void StopFormation();
    
    UFUNCTION(BlueprintCallable, Category = "Formation")
    void SetFormationType(EMAFormationType Type);
    
    UFUNCTION(BlueprintCallable, Category = "Formation")
    EMAFormationType GetFormationType() const { return CurrentFormationType; }
    
    UFUNCTION(BlueprintCallable, Category = "Formation")
    bool IsInFormation() const { return CurrentFormationType != EMAFormationType::None; }
    
    // 编队参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    float FormationSpacing = 200.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    float FormationUpdateInterval = 0.3f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    float FormationStartMoveThreshold = 150.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
    float FormationStopMoveThreshold = 80.f;

    // ========== 委托 ==========
    UPROPERTY(BlueprintAssignable, Category = "AgentManager")
    FOnAgentSpawned OnAgentSpawned;

    UPROPERTY(BlueprintAssignable, Category = "AgentManager")
    FOnAgentDestroyed OnAgentDestroyed;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    // ========== Agent 管理 ==========
    UPROPERTY()
    TArray<AMACharacter*> SpawnedAgents;

    int32 NextAgentIndex = 0;
    
    // ========== 配置数据 ==========
    FMASpawnSettings SpawnSettings;
    TArray<FMAAgentConfig> AgentConfigs;
    TArray<FMAChargingStationConfig> ChargingStationConfigs;
    bool bConfigLoaded = false;
    
    // 解析 JSON 配置
    bool ParseConfigJson(const FString& JsonString);
    
    // 根据配置创建单个 Agent
    AMACharacter* SpawnAgentFromConfig(const FMAAgentConfig& Config, int32 Index, int32 TotalCount);
    
    // 为 Agent 添加 Sensor
    void AddSensorsToAgent(AMACharacter* Agent, const TArray<FMASensorConfig>& SensorConfigs);
    
    // 计算自动位置
    FVector CalculateAutoPosition(int32 Index, int32 TotalCount) const;
    
    // 生成充电站
    void SpawnChargingStations();
    
    // ========== 编队状态 ==========
    UPROPERTY()
    TWeakObjectPtr<AMACharacter> FormationLeader;
    
    UPROPERTY()
    TArray<AMACharacter*> FormationMembers;
    
    EMAFormationType CurrentFormationType = EMAFormationType::None;
    
    FTimerHandle FormationTimerHandle;
    
    void UpdateFormation();
    TArray<FVector> CalculateFormationPositions(const FVector& LeaderLocation, const FRotator& LeaderRotation) const;
    FVector CalculateFormationOffset(int32 Position, int32 TotalCount) const;
    
    // ========== 辅助函数 ==========
    EMAAgentType StringToAgentType(const FString& TypeString) const;
    FString AgentTypeToString(EMAAgentType Type) const;
};
