// MAAgentManager.h
// Agent 管理器 - 负责所有 Agent 的生命周期管理
// 支持 JSON 配置驱动的 Agent 创建
// 
// 注意: 编队功能已移至 UMASquad (Squad 的技能)
// 注意: Squad 管理已移至 MASquadManager
// 注意: 命令分发已移至 MACommandManager

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

    /**
     * 根据类型名称生成 Agent
     * @param TypeName 类型名称: DronePhantom4, DroneInspire2, RobotDog, Human
     * @param Location 生成位置 (如果 bAutoPosition 为 true 则忽略)
     * @param Rotation 生成旋转
     * @param bAutoPosition 是否自动计算位置
     * @return 生成的 Agent，失败返回 nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "AgentManager")
    AMACharacter* SpawnAgentByType(const FString& TypeName, FVector Location, FRotator Rotation, bool bAutoPosition = true);

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
    
    // ========== 辅助函数 ==========
    EMAAgentType StringToAgentType(const FString& TypeString) const;
    FString AgentTypeToString(EMAAgentType Type) const;
};
