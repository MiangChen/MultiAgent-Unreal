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
        UE_LOG(LogMAConfig, Log, TEXT("  ChargingStations: %d"), ChargingStations.Num());
        UE_LOG(LogMAConfig, Log, TEXT("  PickupItems: %d"), PickupItems.Num());
    }
    
    return bSuccess;
}

bool UMAConfigManager::ReloadConfigs()
{
    AgentConfigs.Empty();
    ChargingStations.Empty();
    PickupItems.Empty();
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
                    Config.TypeName = TypeName;
                    InstanceObj->TryGetStringField(TEXT("label"), Config.ID);
                    
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
    ChargingStations.Empty();
    PickupItems.Empty();
    
    // 解析 environment 对象
    const TSharedPtr<FJsonObject>* EnvObj = nullptr;
    if (!RootObject->TryGetObjectField(TEXT("environment"), EnvObj))
    {
        // 兼容旧格式：直接从根对象解析
        EnvObj = &RootObject;
    }
    
    // 解析 charging_stations 数组
    if (const TArray<TSharedPtr<FJsonValue>>* StationsArray; (*EnvObj)->TryGetArrayField(TEXT("charging_stations"), StationsArray))
    {
        for (const TSharedPtr<FJsonValue>& StationValue : *StationsArray)
        {
            const TSharedPtr<FJsonObject>& StationObj = StationValue->AsObject();
            if (!StationObj.IsValid()) continue;
            
            FMAChargingStationConfig Config;
            StationObj->TryGetStringField(TEXT("id"), Config.ID);
            
            if (const TArray<TSharedPtr<FJsonValue>>* PosArray; StationObj->TryGetArrayField(TEXT("position"), PosArray))
            {
                if (PosArray->Num() >= 3)
                {
                    Config.Position = FVector(
                        (*PosArray)[0]->AsNumber(),
                        (*PosArray)[1]->AsNumber(),
                        (*PosArray)[2]->AsNumber()
                    );
                }
            }
            
            ChargingStations.Add(Config);
        }
    }
    
    // 解析 objects 数组 (PickupItems)
    if (const TArray<TSharedPtr<FJsonValue>>* ObjectsArray; (*EnvObj)->TryGetArrayField(TEXT("objects"), ObjectsArray))
    {
        for (const TSharedPtr<FJsonValue>& ObjectValue : *ObjectsArray)
        {
            const TSharedPtr<FJsonObject>& ObjectObj = ObjectValue->AsObject();
            if (!ObjectObj.IsValid()) continue;
            
            FMAPickupItemConfig Config;
            ObjectObj->TryGetStringField(TEXT("id"), Config.ID);
            ObjectObj->TryGetStringField(TEXT("label"), Config.Name);
            ObjectObj->TryGetStringField(TEXT("type"), Config.Type);
            
            if (const TArray<TSharedPtr<FJsonValue>>* PosArray; ObjectObj->TryGetArrayField(TEXT("position"), PosArray))
            {
                if (PosArray->Num() >= 3)
                {
                    Config.Position = FVector(
                        (*PosArray)[0]->AsNumber(),
                        (*PosArray)[1]->AsNumber(),
                        (*PosArray)[2]->AsNumber()
                    );
                }
            }
            
            if (const TSharedPtr<FJsonObject>* FeaturesObj; ObjectObj->TryGetObjectField(TEXT("features"), FeaturesObj))
            {
                for (const auto& Pair : (*FeaturesObj)->Values)
                {
                    FString Value;
                    if (Pair.Value->TryGetString(Value))
                    {
                        Config.Features.Add(Pair.Key, Value);
                    }
                }
            }
            
            PickupItems.Add(Config);
        }
    }
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
