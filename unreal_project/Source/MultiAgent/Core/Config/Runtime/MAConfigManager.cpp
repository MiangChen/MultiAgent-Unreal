#include "Core/Config/MAConfigManager.h"

#include "Core/Config/Application/MAConfigLoadUseCases.h"
#include "Core/Config/Infrastructure/MAConfigPathResolver.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAConfig, Log, All);

void UMAConfigManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadAllConfigs();
    UE_LOG(LogMAConfig, Log, TEXT("MAConfigManager initialized"));
}

void UMAConfigManager::Deinitialize()
{
    Super::Deinitialize();
}

bool UMAConfigManager::LoadAllConfigs()
{
    FMAConfigSnapshot Snapshot;
    const bool bSuccess = MAConfigLoadUseCases::LoadAllConfigs(Snapshot);
    ApplySnapshot(Snapshot);

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
    FMAConfigSnapshot Snapshot;
    const bool bSuccess = MAConfigLoadUseCases::ReloadConfigs(Snapshot);
    ApplySnapshot(Snapshot);
    return bSuccess;
}

FString UMAConfigManager::GetConfigRootPath()
{
    return FMAConfigPathResolver::GetConfigRootPath();
}

FString UMAConfigManager::GetConfigFilePath(const FString& RelativePath)
{
    return FMAConfigPathResolver::GetConfigFilePath(RelativePath);
}

FString UMAConfigManager::GetDatasetFilePath(const FString& RelativePath)
{
    return FMAConfigPathResolver::GetDatasetFilePath(RelativePath);
}

FString UMAConfigManager::GetSceneGraphFilePath() const
{
    if (SceneGraphPath.IsEmpty())
    {
        UE_LOG(LogMAConfig, Warning, TEXT("GetSceneGraphFilePath: SceneGraphPath not configured, using default"));
    }

    const FString FullPath = FMAConfigPathResolver::ResolveSceneGraphFilePath(SceneGraphPath);
    if (!SceneGraphPath.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase) && !SceneGraphPath.IsEmpty())
    {
        UE_LOG(LogMAConfig, Log, TEXT("GetSceneGraphFilePath: Path is a folder, appending scene_graph.json: %s"), *FullPath);
    }
    return FullPath;
}

EMAPathPlannerType UMAConfigManager::GetPathPlannerTypeEnum() const
{
    FMAConfigSnapshot Snapshot;
    Snapshot.PathPlannerType = PathPlannerType;
    return Snapshot.GetPathPlannerTypeEnum();
}

FMAPathPlannerConfig UMAConfigManager::GetPathPlannerConfig() const
{
    FMAConfigSnapshot Snapshot;
    Snapshot.RaycastLayerCount = RaycastLayerCount;
    Snapshot.RaycastLayerDistance = RaycastLayerDistance;
    Snapshot.ScanAngleRange = ScanAngleRange;
    Snapshot.ScanAngleStep = ScanAngleStep;
    Snapshot.ElevationCellSize = ElevationCellSize;
    Snapshot.ElevationSearchRadius = ElevationSearchRadius;
    Snapshot.ElevationMaxSlopeAngle = ElevationMaxSlopeAngle;
    Snapshot.ElevationMaxStepHeight = ElevationMaxStepHeight;
    Snapshot.PathSmoothingFactor = PathSmoothingFactor;
    return Snapshot.GetPathPlannerConfig();
}

void UMAConfigManager::ApplySnapshot(const FMAConfigSnapshot& Snapshot)
{
    SimulationName = Snapshot.SimulationName;
    bUseSetupUI = Snapshot.bUseSetupUI;
    bUseStateTree = Snapshot.bUseStateTree;
    bEnableEnergyDrain = Snapshot.bEnableEnergyDrain;
    MapType = Snapshot.MapType;
    DefaultMapPath = Snapshot.DefaultMapPath;
    SceneGraphPath = Snapshot.SceneGraphPath;
    RunMode = Snapshot.RunMode;

    PlannerServerURL = Snapshot.PlannerServerURL;
    bUseMockData = Snapshot.bUseMockData;
    bDebugMode = Snapshot.bDebugMode;
    bEnablePolling = Snapshot.bEnablePolling;
    bEnableHITLPolling = Snapshot.bEnableHITLPolling;
    PollIntervalSeconds = Snapshot.PollIntervalSeconds;
    HITLPollIntervalSeconds = Snapshot.HITLPollIntervalSeconds;
    LocalServerPort = Snapshot.LocalServerPort;
    bEnableLocalServer = Snapshot.bEnableLocalServer;

    SpectatorStartPosition = Snapshot.SpectatorStartPosition;
    SpectatorStartRotation = Snapshot.SpectatorStartRotation;
    bUsePlayerStart = Snapshot.bUsePlayerStart;
    FallbackOrigin = Snapshot.FallbackOrigin;
    SpawnRadius = Snapshot.SpawnRadius;

    PathPlannerType = Snapshot.PathPlannerType;
    RaycastLayerCount = Snapshot.RaycastLayerCount;
    RaycastLayerDistance = Snapshot.RaycastLayerDistance;
    ScanAngleRange = Snapshot.ScanAngleRange;
    ScanAngleStep = Snapshot.ScanAngleStep;
    ElevationCellSize = Snapshot.ElevationCellSize;
    ElevationSearchRadius = Snapshot.ElevationSearchRadius;
    ElevationMaxSlopeAngle = Snapshot.ElevationMaxSlopeAngle;
    ElevationMaxStepHeight = Snapshot.ElevationMaxStepHeight;
    PathSmoothingFactor = Snapshot.PathSmoothingFactor;

    FlightConfig = Snapshot.FlightConfig;
    GroundNavigationConfig = Snapshot.GroundNavigationConfig;
    FollowConfig = Snapshot.FollowConfig;
    GuideConfig = Snapshot.GuideConfig;
    HandleHazardConfig = Snapshot.HandleHazardConfig;
    TakePhotoConfig = Snapshot.TakePhotoConfig;
    BroadcastConfig = Snapshot.BroadcastConfig;

    AgentConfigs = Snapshot.AgentConfigs;
    EnvironmentObjects = Snapshot.EnvironmentObjects;
    bConfigLoaded = Snapshot.bConfigLoaded;
}
