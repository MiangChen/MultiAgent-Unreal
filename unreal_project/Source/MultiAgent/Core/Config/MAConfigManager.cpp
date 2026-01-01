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

//=============================================================================
// 配置加载
//=============================================================================

bool UMAConfigManager::LoadAllConfigs()
{
    bool bSuccess = true;
    
    bSuccess &= LoadSimulationConfig();
    bSuccess &= LoadAgentsConfig();
    bSuccess &= LoadEnvironmentConfig();
    
    bConfigLoaded = bSuccess;
    
    if (bSuccess)
    {
        UE_LOG(LogMAConfig, Log, TEXT("All configs loaded successfully"));
        UE_LOG(LogMAConfig, Log, TEXT("  bUseSetupUI: %s"), bUseSetupUI ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  bUseStateTree: %s"), bUseStateTree ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  DefaultMapPath: %s"), *DefaultMapPath);
        UE_LOG(LogMAConfig, Log, TEXT("  PlannerServerURL: %s"), *PlannerServerURL);
        UE_LOG(LogMAConfig, Log, TEXT("  bUseMockData: %s"), bUseMockData ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  bEnablePolling: %s"), bEnablePolling ? TEXT("true") : TEXT("false"));
        UE_LOG(LogMAConfig, Log, TEXT("  PollIntervalSeconds: %.2f"), PollIntervalSeconds);
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
        (*SimObj)->TryGetStringField(TEXT("default_map"), DefaultMapPath);
    }
    
    // 解析 server 部分
    if (const TSharedPtr<FJsonObject>* ServerObj; RootObject->TryGetObjectField(TEXT("server"), ServerObj))
    {
        (*ServerObj)->TryGetStringField(TEXT("planner_url"), PlannerServerURL);
        (*ServerObj)->TryGetBoolField(TEXT("use_mock_data"), bUseMockData);
        (*ServerObj)->TryGetBoolField(TEXT("debug_mode"), bDebugMode);
        (*ServerObj)->TryGetBoolField(TEXT("enable_polling"), bEnablePolling);
        
        double PollInterval = 1.0;
        if ((*ServerObj)->TryGetNumberField(TEXT("poll_interval_seconds"), PollInterval))
        {
            PollIntervalSeconds = static_cast<float>(PollInterval);
        }
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
    
    UE_LOG(LogMAConfig, Log, TEXT("Loaded simulation config from: %s"), *ConfigPath);
    return true;
}

bool UMAConfigManager::LoadAgentsConfig()
{
    FString ConfigPath = GetConfigFilePath(TEXT("agents.json"));
    
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
    {
        UE_LOG(LogMAConfig, Warning, TEXT("Failed to load agents config: %s"), *ConfigPath);
        return false;
    }
    
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogMAConfig, Error, TEXT("Failed to parse agents config JSON"));
        return false;
    }
    
    AgentConfigs.Empty();
    
    // 解析 agents 数组
    if (const TArray<TSharedPtr<FJsonValue>>* AgentsArray; RootObject->TryGetArrayField(TEXT("agents"), AgentsArray))
    {
        for (const TSharedPtr<FJsonValue>& AgentValue : *AgentsArray)
        {
            const TSharedPtr<FJsonObject>& AgentObj = AgentValue->AsObject();
            if (!AgentObj.IsValid()) continue;
            
            FString TypeName;
            AgentObj->TryGetStringField(TEXT("type"), TypeName);
            
            // 解析 instances 数组
            if (const TArray<TSharedPtr<FJsonValue>>* InstancesArray; AgentObj->TryGetArrayField(TEXT("instances"), InstancesArray))
            {
                for (const TSharedPtr<FJsonValue>& InstanceValue : *InstancesArray)
                {
                    const TSharedPtr<FJsonObject>& InstanceObj = InstanceValue->AsObject();
                    if (!InstanceObj.IsValid()) continue;
                    
                    FMAAgentConfigData Config;
                    Config.TypeName = TypeName;
                    InstanceObj->TryGetStringField(TEXT("label"), Config.ID);
                    
                    // 解析 position
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
                    
                    // 解析 rotation
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
    
    UE_LOG(LogMAConfig, Log, TEXT("Loaded %d agent configs from: %s"), AgentConfigs.Num(), *ConfigPath);
    return AgentConfigs.Num() > 0;
}

bool UMAConfigManager::LoadEnvironmentConfig()
{
    FString ConfigPath = GetConfigFilePath(TEXT("environment.json"));
    
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
    {
        UE_LOG(LogMAConfig, Warning, TEXT("Failed to load environment config: %s"), *ConfigPath);
        return true; // 环境配置是可选的
    }
    
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogMAConfig, Error, TEXT("Failed to parse environment config JSON"));
        return true; // 环境配置是可选的
    }
    
    ChargingStations.Empty();
    PickupItems.Empty();
    
    // 解析 charging_stations 数组
    if (const TArray<TSharedPtr<FJsonValue>>* StationsArray; RootObject->TryGetArrayField(TEXT("charging_stations"), StationsArray))
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
    if (const TArray<TSharedPtr<FJsonValue>>* ObjectsArray; RootObject->TryGetArrayField(TEXT("objects"), ObjectsArray))
    {
        for (const TSharedPtr<FJsonValue>& ObjectValue : *ObjectsArray)
        {
            const TSharedPtr<FJsonObject>& ObjectObj = ObjectValue->AsObject();
            if (!ObjectObj.IsValid()) continue;
            
            FMAPickupItemConfig Config;
            ObjectObj->TryGetStringField(TEXT("id"), Config.ID);
            ObjectObj->TryGetStringField(TEXT("label"), Config.Name);
            ObjectObj->TryGetStringField(TEXT("type"), Config.Type);
            
            // 解析 position
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
            
            // 解析 features
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
    
    UE_LOG(LogMAConfig, Log, TEXT("Loaded %d charging stations and %d pickup items from: %s"), 
        ChargingStations.Num(), PickupItems.Num(), *ConfigPath);
    return true;
}
