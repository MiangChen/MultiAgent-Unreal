// MAConfigManager.h
// 统一配置管理器 - 负责加载和管理所有配置

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MAConfigManager.generated.h"

// 前向声明
struct FMAAgentConfig;
struct FMASensorConfig;
struct FMASpawnSettings;

/**
 * Agent 配置数据
 */
USTRUCT(BlueprintType)
struct FMAAgentConfigData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString ID;

    UPROPERTY(BlueprintReadOnly)
    FString TypeName;

    UPROPERTY(BlueprintReadOnly)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly)
    bool bAutoPosition = true;
};

/**
 * 充电站配置数据
 */
USTRUCT(BlueprintType)
struct FMAChargingStationConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString ID;

    UPROPERTY(BlueprintReadOnly)
    FVector Position = FVector::ZeroVector;
};

/**
 * 可拾取物品配置数据
 */
USTRUCT(BlueprintType)
struct FMAPickupItemConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString ID;

    UPROPERTY(BlueprintReadOnly)
    FString Name;

    UPROPERTY(BlueprintReadOnly)
    FString Type;

    UPROPERTY(BlueprintReadOnly)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> Features;
};

/**
 * 统一配置管理器
 * 
 * 职责:
 * - 统一加载所有 JSON 配置文件
 * - 提供配置数据的访问接口
 * - 处理配置文件路径
 * - 支持 UI 设置覆盖默认配置
 */
UCLASS()
class MULTIAGENT_API UMAConfigManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 生命周期
    //=========================================================================
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    //=========================================================================
    // 配置加载
    //=========================================================================

    /** 加载所有配置文件 */
    UFUNCTION(BlueprintCallable, Category = "Config")
    bool LoadAllConfigs();

    /** 重新加载配置 */
    UFUNCTION(BlueprintCallable, Category = "Config")
    bool ReloadConfigs();

    //=========================================================================
    // Simulation 配置
    //=========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    FString SimulationName = TEXT("MultiAgent Simulation");

    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    bool bUseSetupUI = false;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    bool bUseStateTree = true;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    bool bEnableEnergyDrain = false;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    FString DefaultMapPath;

    //=========================================================================
    // Server 配置
    //=========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    FString PlannerServerURL = TEXT("http://localhost:8080");

    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    bool bUseMockData = true;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    bool bDebugMode = true;

    //=========================================================================
    // Polling 配置
    //=========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Config|Polling")
    bool bEnablePolling = true;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Polling")
    float PollIntervalSeconds = 1.0f;

    //=========================================================================
    // Spectator 配置
    //=========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spectator")
    FVector SpectatorStartPosition = FVector(0, 0, 500);

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spectator")
    FRotator SpectatorStartRotation = FRotator(-45, 0, 0);

    //=========================================================================
    // Spawn 配置
    //=========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spawn")
    bool bUsePlayerStart = true;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spawn")
    FVector FallbackOrigin = FVector(0, 0, 100);

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spawn")
    float SpawnRadius = 400.f;

    //=========================================================================
    // Agent 配置
    //=========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Config|Agents")
    TArray<FMAAgentConfigData> AgentConfigs;

    //=========================================================================
    // Environment 配置
    //=========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Config|Environment")
    TArray<FMAChargingStationConfig> ChargingStations;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Environment")
    TArray<FMAPickupItemConfig> PickupItems;

    //=========================================================================
    // 状态查询
    //=========================================================================

    UFUNCTION(BlueprintPure, Category = "Config")
    bool IsConfigLoaded() const { return bConfigLoaded; }

    //=========================================================================
    // 路径工具
    //=========================================================================

    /** 获取配置文件根目录 (unreal_project 的上一级) */
    UFUNCTION(BlueprintPure, Category = "Config")
    static FString GetConfigRootPath();

    /** 获取配置文件完整路径 */
    UFUNCTION(BlueprintPure, Category = "Config")
    static FString GetConfigFilePath(const FString& RelativePath);

    /** 获取数据集文件完整路径 */
    UFUNCTION(BlueprintPure, Category = "Config")
    static FString GetDatasetFilePath(const FString& RelativePath);

private:
    bool bConfigLoaded = false;

    bool LoadSimulationConfig();
    bool LoadAgentsConfig();
    bool LoadEnvironmentConfig();
};
