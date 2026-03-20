#include "Core/Config/Application/MAConfigLoadUseCases.h"

#include "Core/Config/Domain/MAConfigRuntimeTypes.h"
#include "Core/Config/Infrastructure/MAConfigJsonLoader.h"
#include "Core/Config/Infrastructure/MAConfigPathResolver.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAConfigLoad, Log, All);

namespace
{
int32 GAgentIdCounter = 5001;
int32 GCargoIdCounter = 1000;
int32 GChargingStationIdCounter = 2000;
int32 GHumanIdCounter = 3000;
int32 GVehicleIdCounter = 4000;
int32 GEffectIdCounter = 6000;
int32 GGenericEnvironmentIdCounter = 9000;

bool TryReadVector(const TArray<TSharedPtr<FJsonValue>>* Array, FVector& OutVector)
{
    if (Array == nullptr || Array->Num() < 3)
    {
        return false;
    }

    OutVector = FVector((*Array)[0]->AsNumber(), (*Array)[1]->AsNumber(), (*Array)[2]->AsNumber());
    return true;
}

bool TryReadRotator(const TArray<TSharedPtr<FJsonValue>>* Array, FRotator& OutRotator)
{
    if (Array == nullptr || Array->Num() < 3)
    {
        return false;
    }

    OutRotator = FRotator((*Array)[0]->AsNumber(), (*Array)[1]->AsNumber(), (*Array)[2]->AsNumber());
    return true;
}

void ParseSimulationConfig(const TSharedPtr<FJsonObject>& RootObject, FMAConfigSnapshot& Snapshot)
{
    if (const TSharedPtr<FJsonObject>* SimObj; RootObject->TryGetObjectField(TEXT("simulation"), SimObj))
    {
        (*SimObj)->TryGetStringField(TEXT("name"), Snapshot.SimulationName);
        (*SimObj)->TryGetBoolField(TEXT("use_setup_ui"), Snapshot.bUseSetupUI);
        (*SimObj)->TryGetBoolField(TEXT("use_state_tree"), Snapshot.bUseStateTree);
        (*SimObj)->TryGetBoolField(TEXT("enable_energy_drain"), Snapshot.bEnableEnergyDrain);
        (*SimObj)->TryGetBoolField(TEXT("enable_info_checks"), Snapshot.bEnableInfoChecks);
        (*SimObj)->TryGetStringField(TEXT("map_type"), Snapshot.MapType);

        FString RunModeStr;
        if ((*SimObj)->TryGetStringField(TEXT("run_mode"), RunModeStr))
        {
            if (RunModeStr.Equals(TEXT("modify"), ESearchCase::IgnoreCase))
            {
                Snapshot.RunMode = EMARunMode::Modify;
            }
            else
            {
                Snapshot.RunMode = EMARunMode::Edit;
            }
        }
        else
        {
            Snapshot.RunMode = EMARunMode::Edit;
        }
    }

    if (const TSharedPtr<FJsonObject>* ServerObj; RootObject->TryGetObjectField(TEXT("server"), ServerObj))
    {
        (*ServerObj)->TryGetStringField(TEXT("planner_url"), Snapshot.PlannerServerURL);
        (*ServerObj)->TryGetBoolField(TEXT("use_mock_data"), Snapshot.bUseMockData);
        (*ServerObj)->TryGetBoolField(TEXT("debug_mode"), Snapshot.bDebugMode);
        (*ServerObj)->TryGetBoolField(TEXT("enable_polling"), Snapshot.bEnablePolling);
        (*ServerObj)->TryGetBoolField(TEXT("enable_hitl_polling"), Snapshot.bEnableHITLPolling);

        double Value = 0.0;
        if ((*ServerObj)->TryGetNumberField(TEXT("poll_interval_seconds"), Value))
        {
            Snapshot.PollIntervalSeconds = static_cast<float>(Value);
        }
        if ((*ServerObj)->TryGetNumberField(TEXT("hitl_poll_interval_seconds"), Value))
        {
            Snapshot.HITLPollIntervalSeconds = static_cast<float>(Value);
        }

        int32 Port = Snapshot.LocalServerPort;
        if ((*ServerObj)->TryGetNumberField(TEXT("local_server_port"), Port))
        {
            Snapshot.LocalServerPort = Port;
        }
        (*ServerObj)->TryGetBoolField(TEXT("enable_local_server"), Snapshot.bEnableLocalServer);
    }

    if (const TSharedPtr<FJsonObject>* SpawnObj; RootObject->TryGetObjectField(TEXT("spawn_settings"), SpawnObj))
    {
        (*SpawnObj)->TryGetBoolField(TEXT("use_player_start"), Snapshot.bUsePlayerStart);
        (*SpawnObj)->TryGetNumberField(TEXT("spawn_radius"), Snapshot.SpawnRadius);

        if (const TArray<TSharedPtr<FJsonValue>>* OriginArray; (*SpawnObj)->TryGetArrayField(TEXT("fallback_origin"), OriginArray))
        {
            TryReadVector(OriginArray, Snapshot.FallbackOrigin);
        }
    }

    if (const TSharedPtr<FJsonObject>* NavObj; RootObject->TryGetObjectField(TEXT("navigation"), NavObj))
    {
        (*NavObj)->TryGetStringField(TEXT("path_planner_type"), Snapshot.PathPlannerType);

        if (const TSharedPtr<FJsonObject>* RaycastObj; (*NavObj)->TryGetObjectField(TEXT("multi_layer_raycast"), RaycastObj))
        {
            int32 LayerCount = Snapshot.RaycastLayerCount;
            if ((*RaycastObj)->TryGetNumberField(TEXT("layer_count"), LayerCount))
            {
                Snapshot.RaycastLayerCount = FMath::Clamp(LayerCount, 2, 5);
            }

            double Value = 0.0;
            if ((*RaycastObj)->TryGetNumberField(TEXT("layer_distance"), Value))
            {
                Snapshot.RaycastLayerDistance = static_cast<float>(Value);
            }
            if ((*RaycastObj)->TryGetNumberField(TEXT("scan_angle_range"), Value))
            {
                Snapshot.ScanAngleRange = static_cast<float>(Value);
            }
            if ((*RaycastObj)->TryGetNumberField(TEXT("scan_angle_step"), Value))
            {
                Snapshot.ScanAngleStep = static_cast<float>(Value);
            }
        }

        if (const TSharedPtr<FJsonObject>* ElevationObj; (*NavObj)->TryGetObjectField(TEXT("elevation_map"), ElevationObj))
        {
            double Value = 0.0;
            if ((*ElevationObj)->TryGetNumberField(TEXT("cell_size"), Value))
            {
                Snapshot.ElevationCellSize = FMath::Clamp(static_cast<float>(Value), 50.f, 200.f);
            }
            if ((*ElevationObj)->TryGetNumberField(TEXT("search_radius"), Value))
            {
                Snapshot.ElevationSearchRadius = FMath::Clamp(static_cast<float>(Value), 1000.f, 10000.f);
            }
            if ((*ElevationObj)->TryGetNumberField(TEXT("max_slope_angle"), Value))
            {
                Snapshot.ElevationMaxSlopeAngle = FMath::Clamp(static_cast<float>(Value), 10.f, 45.f);
            }
            if ((*ElevationObj)->TryGetNumberField(TEXT("max_step_height"), Value))
            {
                Snapshot.ElevationMaxStepHeight = FMath::Clamp(static_cast<float>(Value), 20.f, 100.f);
            }
            if ((*ElevationObj)->TryGetNumberField(TEXT("path_smoothing_factor"), Value))
            {
                Snapshot.PathSmoothingFactor = FMath::Clamp(static_cast<float>(Value), 0.1f, 0.5f);
            }
        }
    }

    if (const TSharedPtr<FJsonObject>* FlightObj; RootObject->TryGetObjectField(TEXT("flight"), FlightObj))
    {
        double Value = 0.0;
        if ((*FlightObj)->TryGetNumberField(TEXT("min_altitude"), Value)) Snapshot.FlightConfig.MinAltitude = static_cast<float>(Value);
        if ((*FlightObj)->TryGetNumberField(TEXT("default_altitude"), Value)) Snapshot.FlightConfig.DefaultAltitude = static_cast<float>(Value);
        if ((*FlightObj)->TryGetNumberField(TEXT("max_speed"), Value)) Snapshot.FlightConfig.MaxSpeed = static_cast<float>(Value);
        if ((*FlightObj)->TryGetNumberField(TEXT("obstacle_detection_range"), Value)) Snapshot.FlightConfig.ObstacleDetectionRange = static_cast<float>(Value);
        if ((*FlightObj)->TryGetNumberField(TEXT("obstacle_avoidance_radius"), Value)) Snapshot.FlightConfig.ObstacleAvoidanceRadius = static_cast<float>(Value);
        if ((*FlightObj)->TryGetNumberField(TEXT("acceptance_radius"), Value)) Snapshot.FlightConfig.AcceptanceRadius = static_cast<float>(Value);
    }

    if (const TSharedPtr<FJsonObject>* GroundNavObj; RootObject->TryGetObjectField(TEXT("ground_navigation"), GroundNavObj))
    {
        double Value = 0.0;
        if ((*GroundNavObj)->TryGetNumberField(TEXT("acceptance_radius"), Value)) Snapshot.GroundNavigationConfig.AcceptanceRadius = static_cast<float>(Value);
        if ((*GroundNavObj)->TryGetNumberField(TEXT("stuck_timeout"), Value)) Snapshot.GroundNavigationConfig.StuckTimeout = static_cast<float>(Value);
    }

    if (const TSharedPtr<FJsonObject>* FollowObj; RootObject->TryGetObjectField(TEXT("follow"), FollowObj))
    {
        double Value = 0.0;
        if ((*FollowObj)->TryGetNumberField(TEXT("distance"), Value)) Snapshot.FollowConfig.Distance = static_cast<float>(Value);
        if ((*FollowObj)->TryGetNumberField(TEXT("position_tolerance"), Value)) Snapshot.FollowConfig.PositionTolerance = static_cast<float>(Value);
        if ((*FollowObj)->TryGetNumberField(TEXT("continuous_time_threshold"), Value)) Snapshot.FollowConfig.ContinuousTimeThreshold = static_cast<float>(Value);
    }

    if (const TSharedPtr<FJsonObject>* GuideObj; RootObject->TryGetObjectField(TEXT("guide"), GuideObj))
    {
        (*GuideObj)->TryGetStringField(TEXT("target_move_mode"), Snapshot.GuideConfig.TargetMoveMode);
        double Value = 0.0;
        if ((*GuideObj)->TryGetNumberField(TEXT("wait_distance_threshold"), Value)) Snapshot.GuideConfig.WaitDistanceThreshold = static_cast<float>(Value);
    }

    if (const TSharedPtr<FJsonObject>* HazardObj; RootObject->TryGetObjectField(TEXT("handle_hazard"), HazardObj))
    {
        double Value = 0.0;
        if ((*HazardObj)->TryGetNumberField(TEXT("safe_distance"), Value)) Snapshot.HandleHazardConfig.SafeDistance = static_cast<float>(Value);
        if ((*HazardObj)->TryGetNumberField(TEXT("duration"), Value)) Snapshot.HandleHazardConfig.Duration = static_cast<float>(Value);
        if ((*HazardObj)->TryGetNumberField(TEXT("spray_speed"), Value)) Snapshot.HandleHazardConfig.SpraySpeed = static_cast<float>(Value);
        if ((*HazardObj)->TryGetNumberField(TEXT("spray_width"), Value)) Snapshot.HandleHazardConfig.SprayWidth = static_cast<float>(Value);
    }

    if (const TSharedPtr<FJsonObject>* PhotoObj; RootObject->TryGetObjectField(TEXT("take_photo"), PhotoObj))
    {
        double Value = 0.0;
        if ((*PhotoObj)->TryGetNumberField(TEXT("photo_distance"), Value)) Snapshot.TakePhotoConfig.PhotoDistance = static_cast<float>(Value);
        if ((*PhotoObj)->TryGetNumberField(TEXT("photo_duration"), Value)) Snapshot.TakePhotoConfig.PhotoDuration = static_cast<float>(Value);
        if ((*PhotoObj)->TryGetNumberField(TEXT("camera_fov"), Value)) Snapshot.TakePhotoConfig.CameraFOV = static_cast<float>(Value);
        if ((*PhotoObj)->TryGetNumberField(TEXT("camera_forward_offset"), Value)) Snapshot.TakePhotoConfig.CameraForwardOffset = static_cast<float>(Value);
    }

    if (const TSharedPtr<FJsonObject>* BroadcastObj; RootObject->TryGetObjectField(TEXT("broadcast"), BroadcastObj))
    {
        double Value = 0.0;
        if ((*BroadcastObj)->TryGetNumberField(TEXT("broadcast_distance"), Value)) Snapshot.BroadcastConfig.BroadcastDistance = static_cast<float>(Value);
        if ((*BroadcastObj)->TryGetNumberField(TEXT("broadcast_duration"), Value)) Snapshot.BroadcastConfig.BroadcastDuration = static_cast<float>(Value);
        if ((*BroadcastObj)->TryGetNumberField(TEXT("effect_speed"), Value)) Snapshot.BroadcastConfig.EffectSpeed = static_cast<float>(Value);
        if ((*BroadcastObj)->TryGetNumberField(TEXT("effect_width"), Value)) Snapshot.BroadcastConfig.EffectWidth = static_cast<float>(Value);
        if ((*BroadcastObj)->TryGetNumberField(TEXT("shock_rate"), Value)) Snapshot.BroadcastConfig.ShockRate = static_cast<float>(Value);
    }
}

void ParseAgentsFromJson(const TSharedPtr<FJsonObject>& RootObject, FMAConfigSnapshot& Snapshot)
{
    Snapshot.AgentConfigs.Empty();

    if (const TArray<TSharedPtr<FJsonValue>>* AgentsArray; RootObject->TryGetArrayField(TEXT("agents"), AgentsArray))
    {
        for (const TSharedPtr<FJsonValue>& AgentValue : *AgentsArray)
        {
            const TSharedPtr<FJsonObject>& AgentObj = AgentValue->AsObject();
            if (!AgentObj.IsValid())
            {
                continue;
            }

            FString TypeName;
            AgentObj->TryGetStringField(TEXT("type"), TypeName);
            if (const TArray<TSharedPtr<FJsonValue>>* InstancesArray; AgentObj->TryGetArrayField(TEXT("instances"), InstancesArray))
            {
                for (const TSharedPtr<FJsonValue>& InstanceValue : *InstancesArray)
                {
                    const TSharedPtr<FJsonObject>& InstanceObj = InstanceValue->AsObject();
                    if (!InstanceObj.IsValid())
                    {
                        continue;
                    }

                    FMAAgentConfigData Config;
                    Config.Type = TypeName;
                    Config.ID = FString::Printf(TEXT("%d"), GAgentIdCounter++);
                    InstanceObj->TryGetStringField(TEXT("label"), Config.Label);

                    if (const TArray<TSharedPtr<FJsonValue>>* PosArray; InstanceObj->TryGetArrayField(TEXT("position"), PosArray))
                    {
                        if (TryReadVector(PosArray, Config.Position))
                        {
                            Config.bAutoPosition = false;
                        }
                    }

                    if (const TArray<TSharedPtr<FJsonValue>>* RotArray; InstanceObj->TryGetArrayField(TEXT("rotation"), RotArray))
                    {
                        TryReadRotator(RotArray, Config.Rotation);
                    }

                    double BatteryVal = 100.0;
                    if (InstanceObj->TryGetNumberField(TEXT("battery_level"), BatteryVal))
                    {
                        Config.BatteryLevel = static_cast<float>(FMath::Clamp(BatteryVal, 0.0, 100.0));
                    }

                    Snapshot.AgentConfigs.Add(Config);
                }
            }
        }
    }
}

FString AllocateEnvironmentId(const FString& Type)
{
    if (Type.Equals(TEXT("cargo"), ESearchCase::IgnoreCase)) return FString::Printf(TEXT("%d"), GCargoIdCounter++);
    if (Type.Equals(TEXT("charging_station"), ESearchCase::IgnoreCase)) return FString::Printf(TEXT("%d"), GChargingStationIdCounter++);
    if (Type.Equals(TEXT("person"), ESearchCase::IgnoreCase)) return FString::Printf(TEXT("%d"), GHumanIdCounter++);
    if (Type.Equals(TEXT("vehicle"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("boat"), ESearchCase::IgnoreCase)) return FString::Printf(TEXT("%d"), GVehicleIdCounter++);
    if (Type.Equals(TEXT("fire"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("smoke"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("wind"), ESearchCase::IgnoreCase)) return FString::Printf(TEXT("%d"), GEffectIdCounter++);
    return FString::Printf(TEXT("%d"), GGenericEnvironmentIdCounter++);
}

void ParseEnvironmentObject(const TSharedPtr<FJsonObject>& ObjectObj, FMAConfigSnapshot& Snapshot)
{
    FMAEnvironmentObjectConfig EnvConfig;
    ObjectObj->TryGetStringField(TEXT("label"), EnvConfig.Label);
    ObjectObj->TryGetStringField(TEXT("type"), EnvConfig.Type);
    if (EnvConfig.Type.IsEmpty())
    {
        UE_LOG(LogMAConfigLoad, Warning, TEXT("Skipping environment object with missing type"));
        return;
    }

    bool bEnabled = true;
    ObjectObj->TryGetBoolField(TEXT("enabled"), bEnabled);
    if (!bEnabled)
    {
        return;
    }

    EnvConfig.ID = AllocateEnvironmentId(EnvConfig.Type);

    if (const TArray<TSharedPtr<FJsonValue>>* PosArray; ObjectObj->TryGetArrayField(TEXT("position"), PosArray))
    {
        TryReadVector(PosArray, EnvConfig.Position);
    }
    if (const TArray<TSharedPtr<FJsonValue>>* RotArray; ObjectObj->TryGetArrayField(TEXT("rotation"), RotArray))
    {
        TryReadRotator(RotArray, EnvConfig.Rotation);
    }

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

    if (const TSharedPtr<FJsonObject>* PatrolObj; ObjectObj->TryGetObjectField(TEXT("patrol"), PatrolObj))
    {
        (*PatrolObj)->TryGetBoolField(TEXT("enabled"), EnvConfig.Patrol.bEnabled);
        (*PatrolObj)->TryGetBoolField(TEXT("loop"), EnvConfig.Patrol.bLoop);
        (*PatrolObj)->TryGetNumberField(TEXT("wait_time"), EnvConfig.Patrol.WaitTime);

        if (const TArray<TSharedPtr<FJsonValue>>* WaypointsArray; (*PatrolObj)->TryGetArrayField(TEXT("waypoints"), WaypointsArray))
        {
            for (const TSharedPtr<FJsonValue>& WaypointValue : *WaypointsArray)
            {
                const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
                if (WaypointValue->TryGetArray(PointArray) && PointArray->Num() >= 3)
                {
                    FVector Waypoint;
                    if (TryReadVector(PointArray, Waypoint))
                    {
                        EnvConfig.Patrol.Waypoints.Add(Waypoint);
                    }
                }
            }
        }
    }

    Snapshot.EnvironmentObjects.Add(EnvConfig);
}

void ParseEnvironmentFromJson(const TSharedPtr<FJsonObject>& RootObject, FMAConfigSnapshot& Snapshot)
{
    Snapshot.EnvironmentObjects.Empty();

    const TArray<TSharedPtr<FJsonValue>>* EnvArray = nullptr;
    if (RootObject->TryGetArrayField(TEXT("environment"), EnvArray))
    {
        for (const TSharedPtr<FJsonValue>& ObjectValue : *EnvArray)
        {
            const TSharedPtr<FJsonObject>& ObjectObj = ObjectValue->AsObject();
            if (ObjectObj.IsValid())
            {
                ParseEnvironmentObject(ObjectObj, Snapshot);
            }
        }
    }
}

bool LoadSimulationConfig(FMAConfigSnapshot& Snapshot)
{
    const FString ConfigPath = FMAConfigPathResolver::GetConfigFilePath(TEXT("simulation.json"));
    TSharedPtr<FJsonObject> RootObject;
    if (!FMAConfigJsonLoader::LoadJsonObjectFromFile(ConfigPath, RootObject))
    {
        UE_LOG(LogMAConfigLoad, Warning, TEXT("Failed to load simulation config: %s"), *ConfigPath);
        return false;
    }

    ParseSimulationConfig(RootObject, Snapshot);
    UE_LOG(LogMAConfigLoad, Log, TEXT("Loaded simulation config from: %s"), *ConfigPath);
    return true;
}

bool LoadMapConfig(const FString& MapType, FMAConfigSnapshot& Snapshot)
{
    const FString ConfigPath = FMAConfigPathResolver::GetConfigFilePath(FString::Printf(TEXT("maps/%s.json"), *MapType));
    TSharedPtr<FJsonObject> RootObject;
    if (!FMAConfigJsonLoader::LoadJsonObjectFromFile(ConfigPath, RootObject))
    {
        UE_LOG(LogMAConfigLoad, Warning, TEXT("Failed to load map config: %s"), *ConfigPath);
        return false;
    }

    if (const TSharedPtr<FJsonObject>* MapInfoObj; RootObject->TryGetObjectField(TEXT("map_info"), MapInfoObj))
    {
        (*MapInfoObj)->TryGetStringField(TEXT("map_path"), Snapshot.DefaultMapPath);
        (*MapInfoObj)->TryGetStringField(TEXT("scene_graph_folder"), Snapshot.SceneGraphPath);
    }

    if (const TSharedPtr<FJsonObject>* SpectatorObj; RootObject->TryGetObjectField(TEXT("spectator"), SpectatorObj))
    {
        if (const TArray<TSharedPtr<FJsonValue>>* PosArray; (*SpectatorObj)->TryGetArrayField(TEXT("position"), PosArray))
        {
            TryReadVector(PosArray, Snapshot.SpectatorStartPosition);
        }
        if (const TArray<TSharedPtr<FJsonValue>>* RotArray; (*SpectatorObj)->TryGetArrayField(TEXT("rotation"), RotArray))
        {
            TryReadRotator(RotArray, Snapshot.SpectatorStartRotation);
        }
    }

    ParseAgentsFromJson(RootObject, Snapshot);
    ParseEnvironmentFromJson(RootObject, Snapshot);
    UE_LOG(LogMAConfigLoad, Log, TEXT("Loaded map config [%s] from: %s"), *MapType, *ConfigPath);
    return true;
}
}

bool MAConfigLoadUseCases::LoadAllConfigs(FMAConfigSnapshot& Snapshot)
{
    bool bSuccess = LoadSimulationConfig(Snapshot);
    if (bSuccess && !Snapshot.MapType.IsEmpty())
    {
        bSuccess &= LoadMapConfig(Snapshot.MapType, Snapshot);
    }

    Snapshot.bConfigLoaded = bSuccess;
    return bSuccess;
}

bool MAConfigLoadUseCases::ReloadConfigs(FMAConfigSnapshot& Snapshot)
{
    Snapshot = FMAConfigSnapshot();
    return LoadAllConfigs(Snapshot);
}
