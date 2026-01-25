// MAConfigManager.h
// 统一配置管理器 - 负责加载和管理所有配置

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../../Utils/MAPathPlanner.h"
#include "MAConfigManager.generated.h"

// 前向声明
struct FMAAgentConfig;
struct FMASensorConfig;
struct FMASpawnSettings;

/**
 * 运行模式枚举
 * - Edit: 运行模式，修改只存在于内存中，不保存到文件
 * - Modify: 标注模式，修改会保存到源文件
 */
UENUM(BlueprintType)
enum class EMARunMode : uint8
{
    Edit    UMETA(DisplayName = "Edit"),      // 运行模式，不保存
    Modify  UMETA(DisplayName = "Modify")     // 标注模式，保存到文件
};

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
 * - 根据 map_type 自动加载对应地图配置
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

    /** 地图类型 (如 Downtown, SciFiModernCity, Port) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    FString MapType;

    /** 地图路径 (从地图配置自动推导) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    FString DefaultMapPath;

    /** 场景图文件夹路径 (从地图配置自动推导) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    FString SceneGraphPath;

    /** 运行模式: Edit (运行模式) 或 Modify (标注模式) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    EMARunMode RunMode = EMARunMode::Edit;

    //=========================================================================
    // 运行模式查询 API
    //=========================================================================

    /** 获取当前运行模式 */
    UFUNCTION(BlueprintPure, Category = "Config")
    EMARunMode GetRunMode() const { return RunMode; }

    /** 是否为 Modify 模式 (标注模式，修改会保存到文件) */
    UFUNCTION(BlueprintPure, Category = "Config")
    bool IsModifyMode() const { return RunMode == EMARunMode::Modify; }

    /** 是否为 Edit 模式 (运行模式，修改只存在于内存中) */
    UFUNCTION(BlueprintPure, Category = "Config")
    bool IsEditMode() const { return RunMode == EMARunMode::Edit; }

    /** 获取当前地图类型 */
    UFUNCTION(BlueprintPure, Category = "Config")
    FString GetMapType() const { return MapType; }

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

    /** 是否启用 HITL 端点轮询 (默认 false) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Polling")
    bool bEnableHITLPolling = false;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Polling")
    float PollIntervalSeconds = 1.0f;

    /** HITL 轮询间隔 (秒) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Polling")
    float HITLPollIntervalSeconds = 1.0f;

    //=========================================================================
    // Local HTTP Server 配置
    //=========================================================================

    /** 本地HTTP服务器端口 (默认8080) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    int32 LocalServerPort = 8080;

    /** 是否启用本地HTTP服务器 (默认true) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    bool bEnableLocalServer = true;

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
    // Navigation 配置 (手动导航路径规划)
    //=========================================================================

    /** 路径规划算法类型: "MultiLayerRaycast" 或 "AStar" */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    FString PathPlannerType = TEXT("MultiLayerRaycast");

    /** 多层射线扫描 - 射线层数 (2-5) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    int32 RaycastLayerCount = 3;

    /** 多层射线扫描 - 每层距离 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float RaycastLayerDistance = 300.f;

    /** 多层射线扫描 - 扫描角度范围 (±度) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ScanAngleRange = 120.f;

    /** 多层射线扫描 - 扫描角度步长 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ScanAngleStep = 15.f;

    //=========================================================================
    // ElevationMap 配置 (2.5D 高程图路径规划)
    //=========================================================================

    /** 高程图 - 栅格单元大小 (cm, 范围 50-200) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ElevationCellSize = 100.f;

    /** 高程图 - 搜索半径 (cm, 范围 1000-10000) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ElevationSearchRadius = 3000.f;

    /** 高程图 - 最大可通行坡度角 (度, 范围 10-45) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ElevationMaxSlopeAngle = 30.f;

    /** 高程图 - 最大可通行台阶高度 (cm, 范围 20-100) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ElevationMaxStepHeight = 50.f;

    /** 高程图 - 路径平滑因子 (范围 0.1-0.5) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float PathSmoothingFactor = 0.15f;

    /** 获取路径规划器类型枚举 */
    EMAPathPlannerType GetPathPlannerTypeEnum() const;

    /** 获取路径规划器配置结构体 */
    FMAPathPlannerConfig GetPathPlannerConfig() const;

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

    /** 获取场景图文件完整路径 (基于配置的SceneGraphPath) */
    UFUNCTION(BlueprintPure, Category = "Config")
    FString GetSceneGraphFilePath() const;

private:
    bool bConfigLoaded = false;

    bool LoadSimulationConfig();
    bool LoadMapConfig(const FString& InMapType);
    
    // 辅助方法：从 JSON 对象解析 agents 数组
    void ParseAgentsFromJson(const TSharedPtr<FJsonObject>& RootObject);
    // 辅助方法：从 JSON 对象解析 environment
    void ParseEnvironmentFromJson(const TSharedPtr<FJsonObject>& RootObject);
};
