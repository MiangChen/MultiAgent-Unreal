#include "Agent/Skill/Infrastructure/MASkillSearchPathBuilder.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Infrastructure/MALidarProbe.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Agent/Skill/Infrastructure/MASkillGeometryUtils.h"
#include "Agent/Skill/Infrastructure/MASkillSceneGraphBridge.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"

namespace
{
constexpr float AerialSearchSurfaceClearance = 500.f;
constexpr float AerialSearchProbeStartZ = 15000.f;

FMALidarProbeConfig BuildSearchLidarConfig()
{
    FMALidarProbeConfig Config;
    Config.MaxDistance = 35000.f;
    Config.HalfAngleDegrees = 7.5f;
    Config.RingCount = 2;
    Config.SamplesPerRing = 6;
    Config.MinSurfaceNormalAlignment = 0.7f;
    Config.bTraceComplex = true;
    Config.bIncludeWorldDynamic = false;
    return Config;
}

void ResolveAerialSearchAltitudes(
    AMACharacter& Character,
    TArray<FVector>& SearchPath,
    const float DefaultAltitude)
{
    UWorld* World = Character.GetWorld();
    if (!World)
    {
        for (FVector& Waypoint : SearchPath)
        {
            Waypoint.Z = DefaultAltitude;
        }
        return;
    }

    const FMALidarProbeConfig ProbeConfig = BuildSearchLidarConfig();
    for (FVector& Waypoint : SearchPath)
    {
        const double ProbeOriginZ = FMath::Max3(
            static_cast<double>(DefaultAltitude) + 8000.0,
            Character.GetActorLocation().Z + 8000.0,
            static_cast<double>(AerialSearchProbeStartZ));
        const FVector ProbeOrigin(
            Waypoint.X,
            Waypoint.Y,
            ProbeOriginZ);

        const FMALidarProbeResult ProbeResult =
            FMALidarProbe::SampleCone(*World, ProbeOrigin, FVector::DownVector, ProbeConfig, &Character);

        Waypoint.Z = ProbeResult.bHasHit
            ? FMath::Max(ProbeResult.MeanImpactPoint.Z + AerialSearchSurfaceClearance, DefaultAltitude)
            : DefaultAltitude;
    }
}
}

TArray<FVector> FMASkillSearchPathBuilder::BuildPath(
    AMACharacter& Character,
    const UMASkillComponent& SkillComponent,
    const ESearchMode SearchMode,
    const float ScanWidth)
{
    TArray<FVector> SearchPath;

    const FMASkillParams& Params = SkillComponent.GetSkillParams();
    const TArray<FVector>& Boundary = Params.SearchBoundary;
    if (Boundary.Num() < 3)
    {
        return SearchPath;
    }

    if (SearchMode == ESearchMode::Coverage)
    {
        SearchPath = FMASkillGeometryUtils::GenerateLawnmowerPath(Boundary, ScanWidth);
    }
    else
    {
        SearchPath = FMASkillGeometryUtils::GeneratePatrolWaypoints(Boundary);
        SearchPath = FMASkillGeometryUtils::SortWaypointsByNearestNeighbor(SearchPath, Character.GetActorLocation());
    }

    UMANavigationService* NavService = Character.GetNavigationService();
    if (NavService && NavService->bIsFlying)
    {
        float FlightAltitude = FMath::Max(Character.GetActorLocation().Z, NavService->MinFlightAltitude);
        ResolveAerialSearchAltitudes(Character, SearchPath, FlightAltitude);

        const float PreviewAltitude = SearchPath.Num() > 0 ? SearchPath[0].Z : FlightAltitude;
        UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Flying search sampled downward lidar altitude, first waypoint Z=%.0f, waypoints=%d"),
            *Character.AgentLabel, PreviewAltitude, SearchPath.Num());
    }
    else if (NavService && !NavService->bIsFlying)
    {
        TArray<TArray<FVector>> ObstaclePolygons;

        const TArray<FMASceneGraphNode> Buildings = FMASkillSceneGraphBridge::LoadAllBuildings(&Character);
        for (const FMASceneGraphNode& Building : Buildings)
        {
            if (Building.GuidArray.Num() >= 3)
            {
                // TODO: 从 WorkingCopy 中恢复真实 vertices 后，再把建筑 polygon 送入过滤器。
            }
        }

        if (ObstaclePolygons.Num() > 0)
        {
            SearchPath = FMASkillGeometryUtils::FilterGroundSafeWaypoints(SearchPath, ObstaclePolygons);
        }

        UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Ground robot, filtered path has %d waypoints"),
            *Character.AgentLabel, SearchPath.Num());
    }

    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Generated %s path with %d waypoints"),
        *Character.AgentLabel,
        SearchMode == ESearchMode::Coverage ? TEXT("Coverage") : TEXT("Patrol"),
        SearchPath.Num());

    return SearchPath;
}
