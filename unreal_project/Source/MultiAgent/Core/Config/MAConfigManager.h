#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/Config/Domain/MAConfigNavigationTypes.h"
#include "Core/Config/Domain/MAConfigEntityTypes.h"
#include "Core/Config/Domain/MAConfigRuntimeTypes.h"
#include "MAConfigManager.generated.h"

struct FMAConfigSnapshot;

UCLASS()
class MULTIAGENT_API UMAConfigManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Config")
    bool LoadAllConfigs();

    UFUNCTION(BlueprintCallable, Category = "Config")
    bool ReloadConfigs();

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

    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    FString DefaultMapPath;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    FString SceneGraphPath;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Simulation")
    EMARunMode RunMode = EMARunMode::Edit;

    UFUNCTION(BlueprintPure, Category = "Config")
    EMARunMode GetRunMode() const { return RunMode; }

    UFUNCTION(BlueprintPure, Category = "Config")
    bool IsModifyMode() const { return RunMode == EMARunMode::Modify; }

    UFUNCTION(BlueprintPure, Category = "Config")
    bool IsEditMode() const { return RunMode == EMARunMode::Edit; }

    UFUNCTION(BlueprintPure, Category = "Config")
    FString GetMapType() const { return MapType; }

    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    FString PlannerServerURL = TEXT("http://localhost:8080");

    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    bool bUseMockData = true;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    bool bDebugMode = true;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Polling")
    bool bEnablePolling = true;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Polling")
    bool bEnableHITLPolling = false;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Polling")
    float PollIntervalSeconds = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Polling")
    float HITLPollIntervalSeconds = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    int32 LocalServerPort = 8080;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Server")
    bool bEnableLocalServer = true;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spectator")
    FVector SpectatorStartPosition = FVector(0, 0, 500);

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spectator")
    FRotator SpectatorStartRotation = FRotator(-45, 0, 0);

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spawn")
    bool bUsePlayerStart = true;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spawn")
    FVector FallbackOrigin = FVector(0, 0, 100);

    UPROPERTY(BlueprintReadOnly, Category = "Config|Spawn")
    float SpawnRadius = 400.f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    FString PathPlannerType = TEXT("MultiLayerRaycast");

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    int32 RaycastLayerCount = 3;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float RaycastLayerDistance = 300.f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ScanAngleRange = 120.f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ScanAngleStep = 15.f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ElevationCellSize = 100.f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ElevationSearchRadius = 3000.f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ElevationMaxSlopeAngle = 30.f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float ElevationMaxStepHeight = 50.f;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    float PathSmoothingFactor = 0.15f;

    EMAPathPlannerType GetPathPlannerTypeEnum() const;
    FMAPathPlannerConfig GetPathPlannerConfig() const;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Flight")
    FMAFlightConfig FlightConfig;

    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAFlightConfig& GetFlightConfig() const { return FlightConfig; }

    UPROPERTY(BlueprintReadOnly, Category = "Config|Navigation")
    FMAGroundNavigationConfig GroundNavigationConfig;

    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAGroundNavigationConfig& GetGroundNavigationConfig() const { return GroundNavigationConfig; }

    UPROPERTY(BlueprintReadOnly, Category = "Config|Follow")
    FMAFollowConfig FollowConfig;

    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAFollowConfig& GetFollowConfig() const { return FollowConfig; }

    UPROPERTY(BlueprintReadOnly, Category = "Config|Guide")
    FMAGuideConfig GuideConfig;

    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAGuideConfig& GetGuideConfig() const { return GuideConfig; }

    UPROPERTY(BlueprintReadOnly, Category = "Config|HandleHazard")
    FMAHandleHazardConfig HandleHazardConfig;

    UFUNCTION(BlueprintPure, Category = "Config")
    const FMAHandleHazardConfig& GetHandleHazardConfig() const { return HandleHazardConfig; }

    UPROPERTY(BlueprintReadOnly, Category = "Config|TakePhoto")
    FMATakePhotoConfig TakePhotoConfig;

    UFUNCTION(BlueprintPure, Category = "Config")
    const FMATakePhotoConfig& GetTakePhotoConfig() const { return TakePhotoConfig; }

    UPROPERTY(BlueprintReadOnly, Category = "Config|Broadcast")
    FMABroadcastConfig BroadcastConfig;

    UFUNCTION(BlueprintPure, Category = "Config")
    const FMABroadcastConfig& GetBroadcastConfig() const { return BroadcastConfig; }

    UPROPERTY(BlueprintReadOnly, Category = "Config|Agents")
    TArray<FMAAgentConfigData> AgentConfigs;

    UPROPERTY(BlueprintReadOnly, Category = "Config|Environment")
    TArray<FMAEnvironmentObjectConfig> EnvironmentObjects;

    UFUNCTION(BlueprintPure, Category = "Config")
    bool IsConfigLoaded() const { return bConfigLoaded; }

    UFUNCTION(BlueprintPure, Category = "Config")
    static FString GetConfigRootPath();

    UFUNCTION(BlueprintPure, Category = "Config")
    static FString GetConfigFilePath(const FString& RelativePath);

    UFUNCTION(BlueprintPure, Category = "Config")
    static FString GetDatasetFilePath(const FString& RelativePath);

    UFUNCTION(BlueprintPure, Category = "Config")
    FString GetSceneGraphFilePath() const;

private:
    bool bConfigLoaded = false;

    void ApplySnapshot(const FMAConfigSnapshot& Snapshot);
};
