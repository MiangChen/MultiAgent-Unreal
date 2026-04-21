#pragma once

#include "CoreMinimal.h"
#include "Agent/Navigation/Infrastructure/MAPathPlanner.h"
#include "MAConfigNavigationTypes.h"
#include "MAConfigEntityTypes.h"
#include "MAConfigRuntimeTypes.generated.h"

UENUM(BlueprintType)
enum class EMARunMode : uint8
{
    Edit UMETA(DisplayName = "Edit"),
    Modify UMETA(DisplayName = "Modify")
};

struct FMAConfigSnapshot
{
    FString SimulationName = TEXT("MultiAgent Simulation");
    bool bUseSetupUI = false;
    bool bUseStateTree = true;
    bool bEnableEnergyDrain = false;
    bool bEnableInfoChecks = true;
    bool bShowNotification = false;
    FString MapType;
    FString DefaultMapPath;
    FString SceneGraphPath;
    EMARunMode RunMode = EMARunMode::Edit;

    FString PlannerServerURL = TEXT("http://localhost:8080");
    bool bUseMockData = true;
    bool bDebugMode = true;

    bool bEnablePolling = true;
    bool bEnableHITLPolling = false;
    float PollIntervalSeconds = 1.0f;
    float HITLPollIntervalSeconds = 1.0f;

    int32 LocalServerPort = 8080;
    bool bEnableLocalServer = true;

    FVector SpectatorStartPosition = FVector(0, 0, 500);
    FRotator SpectatorStartRotation = FRotator(-45, 0, 0);

    bool bUsePlayerStart = true;
    FVector FallbackOrigin = FVector(0, 0, 100);
    float SpawnRadius = 400.f;

    FString PathPlannerType = TEXT("MultiLayerRaycast");
    int32 RaycastLayerCount = 3;
    float RaycastLayerDistance = 300.f;
    float ScanAngleRange = 120.f;
    float ScanAngleStep = 15.f;

    float ElevationCellSize = 100.f;
    float ElevationSearchRadius = 3000.f;
    float ElevationMaxSlopeAngle = 30.f;
    float ElevationMaxStepHeight = 50.f;
    float PathSmoothingFactor = 0.15f;

    FMAFlightConfig FlightConfig;
    FMAGroundNavigationConfig GroundNavigationConfig;
    FMAFollowConfig FollowConfig;
    FMAGuideConfig GuideConfig;
    FMAHandleHazardConfig HandleHazardConfig;
    FMATakePhotoConfig TakePhotoConfig;
    FMABroadcastConfig BroadcastConfig;

    TArray<FMAAgentConfigData> AgentConfigs;
    TArray<FMAEnvironmentObjectConfig> EnvironmentObjects;

    bool bConfigLoaded = false;

    EMAPathPlannerType GetPathPlannerTypeEnum() const
    {
        if (PathPlannerType.Equals(TEXT("ElevationMap"), ESearchCase::IgnoreCase) ||
            PathPlannerType.Equals(TEXT("AStar"), ESearchCase::IgnoreCase) ||
            PathPlannerType.Equals(TEXT("A*"), ESearchCase::IgnoreCase))
        {
            return EMAPathPlannerType::ElevationMap;
        }
        return EMAPathPlannerType::MultiLayerRaycast;
    }

    FMAPathPlannerConfig GetPathPlannerConfig() const
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
};
