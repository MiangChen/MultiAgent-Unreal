#include "Agent/Skill/Infrastructure/MASkillSearchPathBuilder.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Agent/Skill/Infrastructure/MASkillGeometryUtils.h"
#include "Agent/Skill/Infrastructure/MASkillSceneGraphBridge.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"

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
    if (NavService && !NavService->bIsFlying)
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
