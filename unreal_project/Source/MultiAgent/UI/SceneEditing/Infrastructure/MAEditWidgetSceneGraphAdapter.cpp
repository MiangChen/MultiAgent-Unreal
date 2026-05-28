#include "MAEditWidgetSceneGraphAdapter.h"

#include "../../../Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "../../../Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"
#include "../../../Environment/Utils/MAGoalActor.h"
#include "../../../Environment/Utils/MAZoneActor.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    FString ResolveActorSearchId(const AActor* Actor, bool& bOutIsGoalOrZone)
    {
        bOutIsGoalOrZone = false;

        if (!Actor)
        {
            return FString();
        }

        if (const AMAGoalActor* GoalActor = Cast<AMAGoalActor>(Actor))
        {
            bOutIsGoalOrZone = true;
            return GoalActor->GetNodeId();
        }

        if (const AMAZoneActor* ZoneActor = Cast<AMAZoneActor>(Actor))
        {
            bOutIsGoalOrZone = true;
            return ZoneActor->GetNodeId();
        }

        return Actor->GetActorGuid().ToString();
    }
}

bool FMAEditWidgetSceneGraphAdapter::ResolveActorNodes(
    UWorld* World,
    AActor* Actor,
    TArray<FMASceneGraphNode>& OutNodes,
    FString& OutError) const
{
    OutNodes.Empty();
    OutError.Empty();

    if (!World || !Actor)
    {
        OutError = TEXT("Actor selection is invalid");
        return false;
    }

    UMASceneGraphManager* SceneGraphManager = FMASceneGraphBootstrap::Resolve(World);
    if (!SceneGraphManager)
    {
        OutError = TEXT("SceneGraphManager not found");
        return false;
    }

    bool bIsGoalOrZone = false;
    const FString SearchId = ResolveActorSearchId(Actor, bIsGoalOrZone);
    if (SearchId.IsEmpty())
    {
        OutError = TEXT("Unable to resolve actor node id");
        return false;
    }

    if (bIsGoalOrZone)
    {
        const FMASceneGraphNode Node = SceneGraphManager->GetNodeById(SearchId);
        if (Node.IsValid())
        {
            OutNodes.Add(Node);
            return true;
        }

        OutError = FString::Printf(TEXT("Node not found: %s"), *SearchId);
        return false;
    }

    OutNodes = SceneGraphManager->FindNodesByGuid(SearchId);
    if (OutNodes.Num() == 0)
    {
        OutError = TEXT("No matching scene graph node found");
        return false;
    }

    return true;
}
