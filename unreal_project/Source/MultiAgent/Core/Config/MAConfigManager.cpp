// MAConfigManager.cpp
// 统一配置管理器实现

#include "MAConfigManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAConfig, Log, All);

//=============================================================================
// 生命周期
//=============================================================================

void UMAConfigManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // 自动加载所有配置
    LoadAllConfigs();
    
    UE_LOG(LogMAConfig, Log, TEXT("MAConfigManager initialized"));
}

void UMAConfigManager::Deinitialize()
{
    Super::Deinitialize();
}

//=============================================================================
// 路径工具
//=============================================================================

FString UMAConfigManager::GetConfigRootPath()
{
    // unreal_project 的上一级目录
    return FPaths::ProjectDir() / TEXT("..");
}

FString UMAConfigManager::GetConfigFilePath(const FString& RelativePath)
{
    return GetConfigRootPath() / TEXT("config") / RelativePath;
}

FString UMAConfigManager::GetDatasetFilePath(const FString& RelativePath)
{
    return GetConfigRootPath() / TEXT("datasets") / RelativePath;
}

FString UMAConfigManager::GetSceneGraphFilePath() const
{
    if (SceneGraphPath.IsEmpty())
    {
        UE_LOG(LogMAConfig, Warning, TEXT("GetSceneGraphFilePath: SceneGraphPath not configured, using default"));
        return FPaths::ProjectDir() / TEXT("datasets/scene_graph_cyberworld.json");
    }
    
    FString FullPath = FPaths::ProjectDir() / SceneGraphPath;
    
    // 检查路径是否以 .json 结尾
    if (!SceneGraphPath.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase))
    {
        // 路径是文件夹，追加 scene_graph.json
        FullPath = FullPath / TEXT("scene_graph.json");
        UE_LOG(LogMAConfig, Log, TEXT("GetSceneGraphFilePath: Path is a folder, appending scene_graph.json: %s"), *FullPath);
    }
    
    return FullPath;
}

//=============================================================================
// 配置加载
//=============================================================================

bool UMAConfigManager::LoadAllConfigs()
{
    bool bSuccess = true;
    
    // 1. 加载 simulation.json (获取 map_type)
    bSuccess &= LoadSimulationConfig();
    
    // 2. 根据 map_type 加载对应的地图配置
    if (bSuccess && !MapType.IsEmpty())
    {
        bSuccess &= LoadMapConfig(MapType);
    }
    
    bConfigLoaded = bSuccess;
    
    if (bSuccess)
    {
        UE_LOG(LogMAConfig, Log, TEXT("All configs loaded successfully"));
        UE_LOG(LogMAConfig, Log, TEXT("  MapType: %s"), *MapType);
        UE_LOG(LogMAConfig, Log, TEXT("  bUseSetupUI: %s"), bUseSetupUI ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  bUseStateTree: %s"), bUseStateTree ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  bEnableEnergyDrain: %s"), bEnableEnergyDrain ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  DefaultMapPath: %s"), *DefaultMapPath);
        UE_LOG(LogMAConfig, Log, TEXT("  SceneGraphPath: %s"), *SceneGraphPath);
        UE_LOG(LogMAConfig, Log, TEXT("  RunMode: %s"), RunMode == EMARunMode::Modify ? TEXT("Modify") : TEXT("Edit"));
        UE_LOG(LogMAConfig, Log, TEXT("  PlannerServerURL: %s"), *PlannerServerURL);
        UE_LOG(LogMAConfig, Log, TEXT("  bUseMockData: %s"), bUseMockData ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  bEnablePolling: %s"), bEnablePolling ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  bEnableHITLPolling: %s"), bEnableHITLPolling ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  PollIntervalSeconds: %.2f"), PollIntervalSeconds);
        UE_LOG(LogMAConfig, Log, TEXT("  HITLPollIntervalSeconds: %.2f"), HITLPollIntervalSeconds);
        UE_LOG(LogMAConfig, Log, TEXT("  LocalServerPort: %d"), LocalServerPort);
        UE_LOG(LogMAConfig, Log, TEXT("  bEnableLocalServer: %s"), bEnableLocalServer ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  PathPlannerType: %s"), *PathPlannerType);
        UE_LOG(LogMAConfig, Log, TEXT("  RaycastLayerCount: %d"), RaycastLayerCount);
        UE_LOG(LogMAConfig, Log, TEXT("  ElevationCellSize: %.1f"), ElevationCellSize);
        UE_LOG(LogMAConfig, Log, TEXT("  ElevationSearchRadius: %.1f"), ElevationSearchRadius);
        UE_LOG(LogMAConfig, Log, TEXT("  ElevationMaxSlopeAngle: %.1f"), ElevationMaxSlopeAngle);
        UE_LOG(LogMAConfig, Log, TEXT("  ElevationMaxStepHeight: %.1f"), ElevationMaxStepHeight);
        UE_LOG(LogMAConfig, Log, TEXT("  PathSmoothingFactor: %.2f"), PathSmoothingFactor);
        UE_LOG(LogMAConfig, Log, TEXT("  AgentConfigs: %d"), AgentConfigs.Num());
        UE_LOG(LogMAConfig, Log, TEXT("  EnvironmentObjects: %d"), EnvironmentObjects.Num());
    }
    
    return bSuccess;
}

bool UMAConfigManager::ReloadConfigs()
{
    AgentConfigs.Empty();
    EnvironmentObjects.Empty();
    bConfigLoaded = false;
    
    return LoadAllConfigs();
}

bool UMAConfigManager::LoadSimulationConfig()
{
    FString ConfigPath = GetConfigFilePath(TEXT("simulation.json"));
    
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
    {
        UE_LOG(LogMAConfig, Warning, TEXT("Failed to load simulation config: %s"), *ConfigPath);
        return false;
    }
    
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogMAConfig, Error, TEXT("Failed to parse simulation config JSON"));
        return false;
    }
    
    // 解析 simulation 部分
    if (const TSharedPtr<FJsonObject>* SimObj; RootObject->TryGetObjectField(TEXT("simulation"), SimObj))
    {
        (*SimObj)->TryGetStringField(TEXT("name"), SimulationName);
        (*SimObj)->TryGetBoolField(TEXT("use_setup_ui"), bUseSetupUI);
        (*SimObj)->TryGetBoolField(TEXT("use_state_tree"), bUseStateTree);
        (*SimObj)->TryGetBoolField(TEXT("enable_energy_drain"), bEnableEnergyDrain);
        (*SimObj)->TryGetStringField(TEXT("map_type"), MapType);
        
        // 解析 run_mode 字段
        FString RunModeStr;
        if ((*SimObj)->TryGetStringField(TEXT("run_mode"), RunModeStr))
        {
            if (RunModeStr.Equals(TEXT("modify"), ESearchCase::IgnoreCase))
            {
                RunMode = EMARunMode::Modify;
            }
            else if (RunModeStr.Equals(TEXT("edit"), ESearchCase::IgnoreCase))
            {
                RunMode = EMARunMode::Edit;
            }
            else
            {
                UE_LOG(LogMAConfig, Warning, TEXT("Invalid run_mode value '%s', defaulting to 'edit'"), *RunModeStr);
                RunMode = EMARunMode::Edit;
            }
        }
        else
        {
            RunMode = EMARunMode::Edit;
        }
    }
    
    // 解析 server 部分
    if (const TSharedPtr<FJsonObject>* ServerObj; RootObject->TryGetObjectField(TEXT("server"), ServerObj))
    {
        (*ServerObj)->TryGetStringField(TEXT("planner_url"), PlannerServerURL);
        (*ServerObj)->TryGetBoolField(TEXT("use_mock_data"), bUseMockData);
        (*ServerObj)->TryGetBoolField(TEXT("debug_mode"), bDebugMode);
        (*ServerObj)->TryGetBoolField(TEXT("enable_polling"), bEnablePolling);
        (*ServerObj)->TryGetBoolField(TEXT("enable_hitl_polling"), bEnableHITLPolling);
        
        double PollInterval = 1.0;
        if ((*ServerObj)->TryGetNumberField(TEXT("poll_interval_seconds"), PollInterval))
        {
            PollIntervalSeconds = static_cast<float>(PollInterval);
        }
        
        double HITLPollInterval = 1.0;
        if ((*ServerObj)->TryGetNumberField(TEXT("hitl_poll_interval_seconds"), HITLPollInterval))
        {
            HITLPollIntervalSeconds = static_cast<float>(HITLPollInterval);
        }
        
        int32 ServerPort = 8080;
        if ((*ServerObj)->TryGetNumberField(TEXT("local_server_port"), ServerPort))
        {
            LocalServerPort = ServerPort;
        }
        (*ServerObj)->TryGetBoolField(TEXT("enable_local_server"), bEnableLocalServer);
    }
    
    // 解析 spawn_settings 部分
    if (const TSharedPtr<FJsonObject>* SpawnObj; RootObject->TryGetObjectField(TEXT("spawn_settings"), SpawnObj))
    {
        (*SpawnObj)->TryGetBoolField(TEXT("use_player_start"), bUsePlayerStart);
        (*SpawnObj)->TryGetNumberField(TEXT("spawn_radius"), SpawnRadius);
        
        if (const TArray<TSharedPtr<FJsonValue>>* OriginArray; (*SpawnObj)->TryGetArrayField(TEXT("fallback_origin"), OriginArray))
        {
            if (OriginArray->Num() >= 3)
            {
                FallbackOrigin = FVector(
                    (*OriginArray)[0]->AsNumber(),
                    (*OriginArray)[1]->AsNumber(),
                    (*OriginArray)[2]->AsNumber()
                );
            }
        }
    }
    
    // 解析 navigation 部分
    if (const TSharedPtr<FJsonObject>* NavObj; RootObject->TryGetObjectField(TEXT("navigation"), NavObj))
    {
        (*NavObj)->TryGetStringField(TEXT("path_planner_type"), PathPlannerType);
        
        // 多层射线扫描配置
        if (const TSharedPtr<FJsonObject>* RaycastObj; (*NavObj)->TryGetObjectField(TEXT("multi_layer_raycast"), RaycastObj))
        {
            int32 LayerCount = 3;
            if ((*RaycastObj)->TryGetNumberField(TEXT("layer_count"), LayerCount))
            {
                RaycastLayerCount = FMath::Clamp(LayerCount, 2, 5);
            }
            
            double LayerDist = 300.0;
            if ((*RaycastObj)->TryGetNumberField(TEXT("layer_distance"), LayerDist))
            {
                RaycastLayerDistance = static_cast<float>(LayerDist);
            }
            
            double AngleRange = 120.0;
            if ((*RaycastObj)->TryGetNumberField(TEXT("scan_angle_range"), AngleRange))
            {
                ScanAngleRange = static_cast<float>(AngleRange);
            }
            
            double AngleStep = 15.0;
            if ((*RaycastObj)->TryGetNumberField(TEXT("scan_angle_step"), AngleStep))
            {
                ScanAngleStep = static_cast<float>(AngleStep);
            }
        }
        
        // 高程图配置
        if (const TSharedPtr<FJsonObject>* ElevationObj; (*NavObj)->TryGetObjectField(TEXT("elevation_map"), ElevationObj))
        {
            double ElevCellSize = 100.0;
            if ((*ElevationObj)->TryGetNumberField(TEXT("cell_size"), ElevCellSize))
            {
                ElevationCellSize = FMath::Clamp(static_cast<float>(ElevCellSize), 50.f, 200.f);
            }
            
            double ElevSearchRadius = 3000.0;
            if ((*ElevationObj)->TryGetNumberField(TEXT("search_radius"), ElevSearchRadius))
            {
                ElevationSearchRadius = FMath::Clamp(static_cast<float>(ElevSearchRadius), 1000.f, 10000.f);
            }
            
            double MaxSlopeAngle = 30.0;
            if ((*ElevationObj)->TryGetNumberField(TEXT("max_slope_angle"), MaxSlopeAngle))
            {
                ElevationMaxSlopeAngle = FMath::Clamp(static_cast<float>(MaxSlopeAngle), 10.f, 45.f);
            }
            
            double MaxStepHeight = 50.0;
            if ((*ElevationObj)->TryGetNumberField(TEXT("max_step_height"), MaxStepHeight))
            {
                ElevationMaxStepHeight = FMath::Clamp(static_cast<float>(MaxStepHeight), 20.f, 100.f);
            }
            
            double SmoothingFactor = 0.15;
            if ((*ElevationObj)->TryGetNumberField(TEXT("path_smoothing_factor"), SmoothingFactor))
            {
                PathSmoothingFactor = FMath::Clamp(static_cast<float>(SmoothingFactor), 0.1f, 0.5f);
            }
        }
    }
    
    // 解析 flight 部分 (统一飞行参数)
    if (const TSharedPtr<FJsonObject>* FlightObj; RootObject->TryGetObjectField(TEXT("flight"), FlightObj))
    {
        double Value;
        if ((*FlightObj)->TryGetNumberField(TEXT("min_altitude"), Value))
        {
            FlightConfig.MinAltitude = static_cast<float>(Value);
        }
        if ((*FlightObj)->TryGetNumberField(TEXT("default_altitude"), Value))
        {
            FlightConfig.DefaultAltitude = static_cast<float>(Value);
        }
        if ((*FlightObj)->TryGetNumberField(TEXT("max_speed"), Value))
        {
            FlightConfig.MaxSpeed = static_cast<float>(Value);
        }
        if ((*FlightObj)->TryGetNumberField(TEXT("obstacle_detection_range"), Value))
        {
            FlightConfig.ObstacleDetectionRange = static_cast<float>(Value);
        }
        if ((*FlightObj)->TryGetNumberField(TEXT("obstacle_avoidance_radius"), Value))
        {
            FlightConfig.ObstacleAvoidanceRadius = static_cast<float>(Value);
        }
        if ((*FlightObj)->TryGetNumberField(TEXT("acceptance_radius"), Value))
        {
            FlightConfig.AcceptanceRadius = static_cast<float>(Value);
        }
        
        UE_LOG(LogMAConfig, Log, TEXT("Loaded flight config: MinAlt=%.0f, DefaultAlt=%.0f, MaxSpeed=%.0f"),
            FlightConfig.MinAltitude, FlightConfig.DefaultAltitude, FlightConfig.MaxSpeed);
    }
    
    // 解析 ground_navigation 部分 (统一地面导航参数)
    if (const TSharedPtr<FJsonObject>* GroundNavObj; RootObject->TryGetObjectField(TEXT("ground_navigation"), GroundNavObj))
    {
        double Value;
        if ((*GroundNavObj)->TryGetNumberField(TEXT("acceptance_radius"), Value))
        {
            GroundNavigationConfig.AcceptanceRadius = static_cast<float>(Value);
        }
        if ((*GroundNavObj)->TryGetNumberField(TEXT("stuck_timeout"), Value))
        {
            GroundNavigationConfig.StuckTimeout = static_cast<float>(Value);
        }
        
        UE_LOG(LogMAConfig, Log, TEXT("Loaded ground navigation config: AcceptRadius=%.0f, StuckTimeout=%.0f"),
            GroundNavigationConfig.AcceptanceRadius, GroundNavigationConfig.StuckTimeout);
    }
    
    // 解析 follow 部分 (统一跟随参数，适用于所有机器人)
    if (const TSharedPtr<FJsonObject>* FollowObj; RootObject->TryGetObjectField(TEXT("follow"), FollowObj))
    {
        double Value;
        if ((*FollowObj)->TryGetNumberField(TEXT("distance"), Value))
        {
            FollowConfig.Distance = static_cast<float>(Value);
        }
        if ((*FollowObj)->TryGetNumberField(TEXT("position_tolerance"), Value))
        {
            FollowConfig.PositionTolerance = static_cast<float>(Value);
        }
        if ((*FollowObj)->TryGetNumberField(TEXT("continuous_time_threshold"), Value))
        {
            FollowConfig.ContinuousTimeThreshold = static_cast<float>(Value);
        }
        
        UE_LOG(LogMAConfig, Log, TEXT("Loaded follow config: Distance=%.0f, PositionTolerance=%.0f, ContinuousTimeThreshold=%.0f"),
            FollowConfig.Distance, FollowConfig.PositionTolerance, FollowConfig.ContinuousTimeThreshold);
    }

    // 解析 guide 部分 (引导技能参数)
    if (const TSharedPtr<FJsonObject>* GuideObj; RootObject->TryGetObjectField(TEXT("guide"), GuideObj))
    {
        FString StrValue;
        double Value;
        if ((*GuideObj)->TryGetStringField(TEXT("target_move_mode"), StrValue))
        {
            GuideConfig.TargetMoveMode = StrValue;
        }
        if ((*GuideObj)->TryGetNumberField(TEXT("wait_distance_threshold"), Value))
        {
            GuideConfig.WaitDistanceThreshold = static_cast<float>(Value);
        }
        
        UE_LOG(LogMAConfig, Log, TEXT("Loaded guide config: TargetMoveMode=%s, WaitDistanceThreshold=%.0f"),
            *GuideConfig.TargetMoveMode, GuideConfig.WaitDistanceThreshold);
    }

    // 解析 handle_hazard 部分 (处理危险技能参数)
    if (const TSharedPtr<FJsonObject>* HandleHazardObj; RootObject->TryGetObjectField(TEXT("handle_hazard"), HandleHazardObj))
    {
        double Value;
        if ((*HandleHazardObj)->TryGetNumberField(TEXT("safe_distance"), Value))
        {
            HandleHazardConfig.SafeDistance = static_cast<float>(Value);
        }
        if ((*HandleHazardObj)->TryGetNumberField(TEXT("duration"), Value))
        {
            HandleHazardConfig.Duration = static_cast<float>(Value);
        }
        if ((*HandleHazardObj)->TryGetNumberField(TEXT("spray_speed"), Value))
        {
            HandleHazardConfig.SpraySpeed = static_cast<float>(Value);
        }
        if ((*HandleHazardObj)->TryGetNumberField(TEXT("spray_width"), Value))
        {
            HandleHazardConfig.SprayWidth = static_cast<float>(Value);
        }
        
        UE_LOG(LogMAConfig, Log, TEXT("Loaded handle_hazard config: SafeDistance=%.0f, Duration=%.1f, SpraySpeed=%.0f, SprayWidth=%.0f"),
            HandleHazardConfig.SafeDistance, HandleHazardConfig.Duration, HandleHazardConfig.SpraySpeed, HandleHazardConfig.SprayWidth);
    }

    // 解析 take_photo 部分 (拍照技能参数)
    if (const TSharedPtr<FJsonObject>* TakePhotoObj; RootObject->TryGetObjectField(TEXT("take_photo"), TakePhotoObj))
    {
        double Value;
        if ((*TakePhotoObj)->TryGetNumberField(TEXT("photo_distance"), Value))
        {
            TakePhotoConfig.PhotoDistance = static_cast<float>(Value);
        }
        if ((*TakePhotoObj)->TryGetNumberField(TEXT("photo_duration"), Value))
        {
            TakePhotoConfig.PhotoDuration = static_cast<float>(Value);
        }
        if ((*TakePhotoObj)->TryGetNumberField(TEXT("camera_fov"), Value))
        {
            TakePhotoConfig.CameraFOV = static_cast<float>(Value);
        }
        if ((*TakePhotoObj)->TryGetNumberField(TEXT("camera_forward_offset"), Value))
        {
            TakePhotoConfig.CameraForwardOffset = static_cast<float>(Value);
        }
        
        UE_LOG(LogMAConfig, Log, TEXT("Loaded take_photo config: PhotoDistance=%.0f, Duration=%.1f, FOV=%.0f, ForwardOffset=%.0f"),
            TakePhotoConfig.PhotoDistance, TakePhotoConfig.PhotoDuration, TakePhotoConfig.CameraFOV, TakePhotoConfig.CameraForwardOffset);
    }

    // 解析 broadcast 部分 (喊话技能参数)
    if (const TSharedPtr<FJsonObject>* BroadcastObj; RootObject->TryGetObjectField(TEXT("broadcast"), BroadcastObj))
    {
        double Value;
        if ((*BroadcastObj)->TryGetNumberField(TEXT("broadcast_distance"), Value))
        {
            BroadcastConfig.BroadcastDistance = static_cast<float>(Value);
        }
        if ((*BroadcastObj)->TryGetNumberField(TEXT("broadcast_duration"), Value))
        {
            BroadcastConfig.BroadcastDuration = static_cast<float>(Value);
        }
        if ((*BroadcastObj)->TryGetNumberField(TEXT("effect_speed"), Value))
        {
            BroadcastConfig.EffectSpeed = static_cast<float>(Value);
        }
        if ((*BroadcastObj)->TryGetNumberField(TEXT("effect_width"), Value))
        {
            BroadcastConfig.EffectWidth = static_cast<float>(Value);
        }
        if ((*BroadcastObj)->TryGetNumberField(TEXT("shock_rate"), Value))
        {
            BroadcastConfig.ShockRate = static_cast<float>(Value);
        }
        
        UE_LOG(LogMAConfig, Log, TEXT("Loaded broadcast config: BroadcastDistance=%.0f, Duration=%.1f, EffectSpeed=%.0f, EffectWidth=%.0f, ShockRate=%.1f"),
            BroadcastConfig.BroadcastDistance, BroadcastConfig.BroadcastDuration, BroadcastConfig.EffectSpeed, BroadcastConfig.EffectWidth, BroadcastConfig.ShockRate);
    }
    
    UE_LOG(LogMAConfig, Log, TEXT("Loaded simulation config from: %s"), *ConfigPath);
    return true;
}


bool UMAConfigManager::LoadMapConfig(const FString& InMapType)
{
    FString ConfigPath = GetConfigFilePath(FString::Printf(TEXT("maps/%s.json"), *InMapType));
    
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
    {
        UE_LOG(LogMAConfig, Warning, TEXT("Failed to load map config: %s"), *ConfigPath);
        return false;
    }
    
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogMAConfig, Error, TEXT("Failed to parse map config JSON: %s"), *ConfigPath);
        return false;
    }
    
    // 解析 map_info 部分
    if (const TSharedPtr<FJsonObject>* MapInfoObj; RootObject->TryGetObjectField(TEXT("map_info"), MapInfoObj))
    {
        (*MapInfoObj)->TryGetStringField(TEXT("map_path"), DefaultMapPath);
        (*MapInfoObj)->TryGetStringField(TEXT("scene_graph_folder"), SceneGraphPath);
    }
    
    // 解析 spectator 部分
    if (const TSharedPtr<FJsonObject>* SpectatorObj; RootObject->TryGetObjectField(TEXT("spectator"), SpectatorObj))
    {
        if (const TArray<TSharedPtr<FJsonValue>>* PosArray; (*SpectatorObj)->TryGetArrayField(TEXT("position"), PosArray))
        {
            if (PosArray->Num() >= 3)
            {
                SpectatorStartPosition = FVector(
                    (*PosArray)[0]->AsNumber(),
                    (*PosArray)[1]->AsNumber(),
                    (*PosArray)[2]->AsNumber()
                );
            }
        }
        
        if (const TArray<TSharedPtr<FJsonValue>>* RotArray; (*SpectatorObj)->TryGetArrayField(TEXT("rotation"), RotArray))
        {
            if (RotArray->Num() >= 3)
            {
                SpectatorStartRotation = FRotator(
                    (*RotArray)[0]->AsNumber(),
                    (*RotArray)[1]->AsNumber(),
                    (*RotArray)[2]->AsNumber()
                );
            }
        }
    }
    
    // 解析 agents 和 environment
    ParseAgentsFromJson(RootObject);
    ParseEnvironmentFromJson(RootObject);
    
    UE_LOG(LogMAConfig, Log, TEXT("Loaded map config [%s] from: %s"), *InMapType, *ConfigPath);
    return true;
}

void UMAConfigManager::ParseAgentsFromJson(const TSharedPtr<FJsonObject>& RootObject)
{
    AgentConfigs.Empty();
    
    // Agent ID 起始值: 5001
    static int32 AgentIdCounter = 5001;
    
    if (const TArray<TSharedPtr<FJsonValue>>* AgentsArray; RootObject->TryGetArrayField(TEXT("agents"), AgentsArray))
    {
        for (const TSharedPtr<FJsonValue>& AgentValue : *AgentsArray)
        {
            const TSharedPtr<FJsonObject>& AgentObj = AgentValue->AsObject();
            if (!AgentObj.IsValid()) continue;
            
            FString TypeName;
            AgentObj->TryGetStringField(TEXT("type"), TypeName);
            
            if (const TArray<TSharedPtr<FJsonValue>>* InstancesArray; AgentObj->TryGetArrayField(TEXT("instances"), InstancesArray))
            {
                for (const TSharedPtr<FJsonValue>& InstanceValue : *InstancesArray)
                {
                    const TSharedPtr<FJsonObject>& InstanceObj = InstanceValue->AsObject();
                    if (!InstanceObj.IsValid()) continue;
                    
                    FMAAgentConfigData Config;
                    Config.Type = TypeName;
                    Config.ID = FString::Printf(TEXT("%d"), AgentIdCounter++);
                    InstanceObj->TryGetStringField(TEXT("label"), Config.Label);
                    
                    if (const TArray<TSharedPtr<FJsonValue>>* PosArray; InstanceObj->TryGetArrayField(TEXT("position"), PosArray))
                    {
                        if (PosArray->Num() >= 3)
                        {
                            Config.Position = FVector(
                                (*PosArray)[0]->AsNumber(),
                                (*PosArray)[1]->AsNumber(),
                                (*PosArray)[2]->AsNumber()
                            );
                            Config.bAutoPosition = false;
                        }
                    }
                    
                    if (const TArray<TSharedPtr<FJsonValue>>* RotArray; InstanceObj->TryGetArrayField(TEXT("rotation"), RotArray))
                    {
                        if (RotArray->Num() >= 3)
                        {
                            Config.Rotation = FRotator(
                                (*RotArray)[0]->AsNumber(),
                                (*RotArray)[1]->AsNumber(),
                                (*RotArray)[2]->AsNumber()
                            );
                        }
                    }
                    
                    AgentConfigs.Add(Config);
                }
            }
        }
    }
}

void UMAConfigManager::ParseEnvironmentFromJson(const TSharedPtr<FJsonObject>& RootObject)
{
    EnvironmentObjects.Empty();

    const TArray<TSharedPtr<FJsonValue>>* EnvArray = nullptr;
    if (RootObject->TryGetArrayField(TEXT("environment"), EnvArray))
    {
        for (const TSharedPtr<FJsonValue>& ObjectValue : *EnvArray)
        {
            const TSharedPtr<FJsonObject>& ObjectObj = ObjectValue->AsObject();
            if (!ObjectObj.IsValid()) continue;
            
            ParseEnvironmentObject(ObjectObj);
        }
    }
    
    UE_LOG(LogMAConfig, Log, TEXT("Parsed environment: %d environment objects"), EnvironmentObjects.Num());
}

void UMAConfigManager::ParseEnvironmentObject(const TSharedPtr<FJsonObject>& ObjectObj)
{
    // 环境对象 ID 起始值 (按类型分配不同范围)
    // 1000-1999: cargo
    // 2000-2999: charging_station
    // 3000-3999: person
    // 4000-4999: vehicle/boat
    // 6000-6999: fire/smoke/wind (特效)
    static int32 CargoIdCounter = 1000;
    static int32 ChargingStationIdCounter = 2000;
    static int32 HumanIdCounter = 3000;
    static int32 VehicleIdCounter = 4000;
    static int32 EffectIdCounter = 6000;

    // 解析通用环境对象配置
    FMAEnvironmentObjectConfig EnvConfig;
    ObjectObj->TryGetStringField(TEXT("label"), EnvConfig.Label);
    ObjectObj->TryGetStringField(TEXT("type"), EnvConfig.Type);
    
    // 跳过缺少必要字段的对象
    if (EnvConfig.Type.IsEmpty())
    {
        UE_LOG(LogMAConfig, Warning, TEXT("Skipping environment object with missing type"));
        return;
    }
    
    // 自动生成 ID (按类型分配)
    if (EnvConfig.Type.Equals(TEXT("cargo"), ESearchCase::IgnoreCase))
    {
        EnvConfig.ID = FString::Printf(TEXT("%d"), CargoIdCounter++);
    }
    else if (EnvConfig.Type.Equals(TEXT("charging_station"), ESearchCase::IgnoreCase))
    {
        EnvConfig.ID = FString::Printf(TEXT("%d"), ChargingStationIdCounter++);
    }
    else if (EnvConfig.Type.Equals(TEXT("person"), ESearchCase::IgnoreCase))
    {
        EnvConfig.ID = FString::Printf(TEXT("%d"), HumanIdCounter++);
    }
    else if (EnvConfig.Type.Equals(TEXT("vehicle"), ESearchCase::IgnoreCase) ||
             EnvConfig.Type.Equals(TEXT("boat"), ESearchCase::IgnoreCase))
    {
        EnvConfig.ID = FString::Printf(TEXT("%d"), VehicleIdCounter++);
    }
    else if (EnvConfig.Type.Equals(TEXT("fire"), ESearchCase::IgnoreCase) ||
             EnvConfig.Type.Equals(TEXT("smoke"), ESearchCase::IgnoreCase) ||
             EnvConfig.Type.Equals(TEXT("wind"), ESearchCase::IgnoreCase))
    {
        EnvConfig.ID = FString::Printf(TEXT("%d"), EffectIdCounter++);
    }
    else
    {
        // 未知类型，使用通用计数器
        static int32 GenericIdCounter = 9000;
        EnvConfig.ID = FString::Printf(TEXT("%d"), GenericIdCounter++);
    }
    
    // 解析 position 数组
    if (const TArray<TSharedPtr<FJsonValue>>* PosArray; ObjectObj->TryGetArrayField(TEXT("position"), PosArray))
    {
        if (PosArray->Num() >= 3)
        {
            EnvConfig.Position = FVector(
                (*PosArray)[0]->AsNumber(),
                (*PosArray)[1]->AsNumber(),
                (*PosArray)[2]->AsNumber()
            );
        }
    }
    
    // 解析 rotation 数组 (可选)
    if (const TArray<TSharedPtr<FJsonValue>>* RotArray; ObjectObj->TryGetArrayField(TEXT("rotation"), RotArray))
    {
        if (RotArray->Num() >= 3)
        {
            EnvConfig.Rotation = FRotator(
                (*RotArray)[0]->AsNumber(),  // Pitch
                (*RotArray)[1]->AsNumber(),  // Yaw
                (*RotArray)[2]->AsNumber()   // Roll
            );
        }
    }
    
    // 解析 features 对象 (可选，默认为空 map)
    if (const TSharedPtr<FJsonObject>* FeaturesObj; ObjectObj->TryGetObjectField(TEXT("features"), FeaturesObj))
    {
        for (const auto& Pair : (*FeaturesObj)->Values)
        {
            FString Value;
            if (Pair.Value->TryGetString(Value))
            {
                EnvConfig.Features.Add(Pair.Key, Value);
            }
        }
    }
    
    // 解析 patrol 配置 (可选)
    if (const TSharedPtr<FJsonObject>* PatrolObj; ObjectObj->TryGetObjectField(TEXT("patrol"), PatrolObj))
    {
        (*PatrolObj)->TryGetBoolField(TEXT("enabled"), EnvConfig.Patrol.bEnabled);
        (*PatrolObj)->TryGetBoolField(TEXT("loop"), EnvConfig.Patrol.bLoop);
        (*PatrolObj)->TryGetNumberField(TEXT("wait_time"), EnvConfig.Patrol.WaitTime);
        
        // 解析 waypoints 数组
        if (const TArray<TSharedPtr<FJsonValue>>* WaypointsArray; (*PatrolObj)->TryGetArrayField(TEXT("waypoints"), WaypointsArray))
        {
            for (const TSharedPtr<FJsonValue>& WaypointValue : *WaypointsArray)
            {
                const TArray<TSharedPtr<FJsonValue>>* PointArray;
                if (WaypointValue->TryGetArray(PointArray) && PointArray->Num() >= 3)
                {
                    FVector Waypoint(
                        (*PointArray)[0]->AsNumber(),
                        (*PointArray)[1]->AsNumber(),
                        (*PointArray)[2]->AsNumber()
                    );
                    EnvConfig.Patrol.Waypoints.Add(Waypoint);
                }
            }
        }
    }
    
    // 添加到 EnvironmentObjects 数组
    EnvironmentObjects.Add(EnvConfig);
}

EMAPathPlannerType UMAConfigManager::GetPathPlannerTypeEnum() const
{
    if (PathPlannerType.Equals(TEXT("ElevationMap"), ESearchCase::IgnoreCase) ||
        PathPlannerType.Equals(TEXT("AStar"), ESearchCase::IgnoreCase) ||
        PathPlannerType.Equals(TEXT("A*"), ESearchCase::IgnoreCase))
    {
        return EMAPathPlannerType::ElevationMap;
    }
    return EMAPathPlannerType::MultiLayerRaycast;
}

FMAPathPlannerConfig UMAConfigManager::GetPathPlannerConfig() const
{
    FMAPathPlannerConfig Config;
    
    Config.RaycastLayerCount = RaycastLayerCount;
    Config.RaycastLayerDistance = RaycastLayerDistance;
    Config.ScanAngleRange = ScanAngleRange;
    Config.ScanAngleStep = ScanAngleStep;
    
    Config.ElevationCellSize = ElevationCellSize;
    Config.ElevationSearchRadius = ElevationSearchRadius;
    Config.ElevationMaxSlopeAngle = ElevationMaxSlopeAngle;
    Config.ElevationMaxStepHeight = ElevationMaxStepHeight;
    Config.PathSmoothingFactor = PathSmoothingFactor;
    
    return Config;
}