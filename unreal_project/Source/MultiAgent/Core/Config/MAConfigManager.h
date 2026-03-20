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
 * 飞行参数配置 (统一配置，供所有飞行相关类使用)
 */
USTRUCT(BlueprintType)
struct FMAFlightConfig
{
    GENERATED_BODY()

    /** 最低飞行高度 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float MinAltitude = 800.f;

    /** 默认飞行高度 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float DefaultAltitude = 1000.f;

    /** 最大飞行速度 (cm/s) */
    UPROPERTY(BlueprintReadOnly)
    float MaxSpeed = 600.f;

    /** 障碍物检测距离 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float ObstacleDetectionRange = 800.f;

    /** 障碍物避让半径 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float ObstacleAvoidanceRadius = 150.f;

    /** 到达判定半径 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float AcceptanceRadius = 200.f;
};

/**
 * 地面导航参数配置 (统一配置，供所有地面导航相关类使用)
 */
USTRUCT(BlueprintType)
struct FMAGroundNavigationConfig
{
    GENERATED_BODY()

    /** 到达判定半径 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float AcceptanceRadius = 200.f;

    /** 卡住超时时间 (秒) */
    UPROPERTY(BlueprintReadOnly)
    float StuckTimeout = 10.f;
};

/**
 * 跟随参数配置 (统一配置，供所有机器人的跟随技能使用)
 */
USTRUCT(BlueprintType)
struct FMAFollowConfig
{
    GENERATED_BODY()

    /** 跟随距离 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float Distance = 300.f;

    /** 跟随位置容差 (cm) - 判断是否到达跟随位置 */
    UPROPERTY(BlueprintReadOnly)
    float PositionTolerance = 200.f;

    /** 持续跟踪时间阈值 (秒) - 达到此时间视为跟随成功 */
    UPROPERTY(BlueprintReadOnly)
    float ContinuousTimeThreshold = 30.f;
};

/**
 * 引导参数配置 (Guide 技能)
 */
USTRUCT(BlueprintType)
struct FMAGuideConfig
{
    GENERATED_BODY()

    /** 被引导对象移动模式: "direct" 或 "navmesh" */
    UPROPERTY(BlueprintReadOnly)
    FString TargetMoveMode = TEXT("navmesh");

    /** 等待距离阈值 (cm) - 机器人与被引导对象距离超过此值时停下等待 */
    UPROPERTY(BlueprintReadOnly)
    float WaitDistanceThreshold = 500.f;
};

/**
 * 处理危险参数配置 (HandleHazard 技能)
 */
USTRUCT(BlueprintType)
struct FMAHandleHazardConfig
{
    GENERATED_BODY()

    /** 安全距离 (cm) - 机器人与危险源保持的距离 */
    UPROPERTY(BlueprintReadOnly)
    float SafeDistance = 500.f;

    /** 处理持续时间 (秒) */
    UPROPERTY(BlueprintReadOnly)
    float Duration = 15.0f;

    /** 喷射速度/范围 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float SpraySpeed = 500.f;

    /** 水柱宽度 (cm) */
    UPROPERTY(BlueprintReadOnly)
    float SprayWidth = 30.f;
};

/**
 * 拍照参数配置 (TakePhoto 技能)
 */
USTRUCT(BlueprintType)
struct FMATakePhotoConfig
{
    GENERATED_BODY()

    /** 拍照距离 (cm) - 与目标保持的水平距离 */
    UPROPERTY(BlueprintReadOnly)
    float PhotoDistance = 500.f;

    /** 拍照持续时间 (秒) - 相机画面显示时长 */
    UPROPERTY(BlueprintReadOnly)
    float PhotoDuration = 3.0f;

    /** 相机视场角 */
    UPROPERTY(BlueprintReadOnly)
    float CameraFOV = 60.f;

    /** 相机前置偏移 (cm) - 模拟眼睛在前部 */
    UPROPERTY(BlueprintReadOnly)
    float CameraForwardOffset = 50.f;
};

/**
 * 喊话参数配置 (Broadcast 技能)
 */
USTRUCT(BlueprintType)
struct FMABroadcastConfig
{
    GENERATED_BODY()

    /** 喊话距离 (cm) - 与目标保持的水平距离 */
    UPROPERTY(BlueprintReadOnly)
    float BroadcastDistance = 500.f;

    /** 喊话持续时间 (秒) */
    UPROPERTY(BlueprintReadOnly)
    float BroadcastDuration = 5.0f;

    /** 特效速度/范围 (cm) - 占位参数 */
    UPROPERTY(BlueprintReadOnly)
    float EffectSpeed = 500.f;

    /** 特效宽度 (cm) - 占位参数 */
    UPROPERTY(BlueprintReadOnly)
    float EffectWidth = 30.f;

    /** 声波发射频率 (次/秒) */
    UPROPERTY(BlueprintReadOnly)
    float ShockRate = 5.0f;
};

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

    /** 唯一标识 (如 "5001") */
    UPROPERTY(BlueprintReadOnly)
    FString ID;

    /** 显示标签 (如 "UAV-1") */
    UPROPERTY(BlueprintReadOnly)
    FString Label;

    /** 机器人类型 (UAV, UGV, Quadruped, Humanoid, FixedWingUAV) */
    UPROPERTY(BlueprintReadOnly)
    FString Type;

    UPROPERTY(BlueprintReadOnly)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly)
    bool bAutoPosition = true;

    /** 初始电量百分比 (0-100) */
    UPROPERTY(BlueprintReadOnly)
    float BatteryLevel = 100.f;
};

/**
 * 巡逻配置数据
 */
USTRUCT(BlueprintType)
struct FMAPatrolConfig
{
    GENERATED_BODY()

    /** 是否启用巡逻 */
    UPROPERTY(BlueprintReadOnly)
    bool bEnabled = false;

    /** 巡逻路径点 */
    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> Waypoints;

    /** 是否循环巡逻 */
    UPROPERTY(BlueprintReadOnly)
    bool bLoop = true;

    /** 到达每个点后的等待时间 (秒) */
    UPROPERTY(BlueprintReadOnly)
    float WaitTime = 2.0f;
};

/**
 * 环境对象配置数据 (统一配置结构)
 * 支持类型: person, vehicle, boat, fire, smoke, wind, cargo
 */
USTRUCT(BlueprintType)
struct FMAEnvironmentObjectConfig
{
    GENERATED_BODY()

    /** 唯一标识 */
    UPROPERTY(BlueprintReadOnly)
    FString ID;

    /** 显示标签 */
    UPROPERTY(BlueprintReadOnly)
    FString Label;

    /** 对象类型: person/vehicle/boat/fire/smoke/wind/cargo */
    UPROPERTY(BlueprintReadOnly)
    FString Type;

    /** 位置 */
    UPROPERTY(BlueprintReadOnly)
    FVector Position = FVector::ZeroVector;

    /** 旋转 */
    UPROPERTY(BlueprintReadOnly)
    FRotator Rotation = FRotator::ZeroRotator;

    /** 特征键值对 (如 color, subtype, animation, gender 等) */
    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> Features;

    /** 巡逻配置 (可选) */
    UPROPERTY(BlueprintReadOnly)
    FMAPatrolConfig Patrol;
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

    /** 是否启用 Info 级别的条件检查 (如高优先级目标发现等) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    bool bEnableInfoChecks = true;

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
    // Flight 配置 (统一飞行参数)
    //=========================================================================

    /** 飞行参数配置 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Flight")
    FMAFlightConfig FlightConfig;

    /** 获取飞行配置 */
    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAFlightConfig& GetFlightConfig() const { return FlightConfig; }

    //=========================================================================
    // Ground Navigation 配置 (统一地面导航参数)
    //=========================================================================

    /** 地面导航参数配置 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    FMAGroundNavigationConfig GroundNavigationConfig;

    /** 获取地面导航配置 */
    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAGroundNavigationConfig& GetGroundNavigationConfig() const { return GroundNavigationConfig; }

    //=========================================================================
    // Follow 配置 (统一跟随参数，适用于所有机器人)
    //=========================================================================

    /** 跟随参数配置 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Follow")
    FMAFollowConfig FollowConfig;

    /** 获取跟随配置 */
    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAFollowConfig& GetFollowConfig() const { return FollowConfig; }

    //=========================================================================
    // Guide 配置 (引导技能参数)
    //=========================================================================

    /** 引导参数配置 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Guide")
    FMAGuideConfig GuideConfig;

    /** 获取引导配置 */
    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAGuideConfig& GetGuideConfig() const { return GuideConfig; }

    //=========================================================================
    // HandleHazard 配置 (处理危险技能参数)
    //=========================================================================

    /** 处理危险参数配置 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|HandleHazard")
    FMAHandleHazardConfig HandleHazardConfig;

    /** 获取处理危险配置 */
    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAHandleHazardConfig& GetHandleHazardConfig() const { return HandleHazardConfig; }

    //=========================================================================
    // TakePhoto 配置 (拍照技能参数)
    //=========================================================================

    /** 拍照参数配置 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|TakePhoto")
    FMATakePhotoConfig TakePhotoConfig;

    /** 获取拍照配置 */
    UFUNCTION(BlueprintPure, Category = "Config")
    const FMATakePhotoConfig& GetTakePhotoConfig() const { return TakePhotoConfig; }

    //=========================================================================
    // Broadcast 配置 (喊话技能参数)
    //=========================================================================

    /** 喊话参数配置 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Broadcast")
    FMABroadcastConfig BroadcastConfig;

    /** 获取喊话配置 */
    UFUNCTION(BlueprintPure, Category = "Config")
    const FMABroadcastConfig& GetBroadcastConfig() const { return BroadcastConfig; }

    //=========================================================================
    // Agent 配置
    //=========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Config|Agents")
    TArray<FMAAgentConfigData> AgentConfigs;

    //=========================================================================
    // Environment 配置
    //=========================================================================

    /** 所有环境对象配置 (person, vehicle, boat, cargo, charging_station, fire, smoke, wind) */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Environment")
    TArray<FMAEnvironmentObjectConfig> EnvironmentObjects;

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
    // 辅助方法：解析单个环境对象
    void ParseEnvironmentObject(const TSharedPtr<FJsonObject>& ObjectObj);
};
