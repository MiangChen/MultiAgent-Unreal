#include "Agent/Skill/Infrastructure/MASkillSceneGraphBridge.h"

#include "Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"
#include "Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"
#include "Core/SceneGraph/Runtime/MASceneGraphManager.h"

UMASceneGraphManager* FMASkillSceneGraphBridge::ResolveManager(const UObject* WorldContextObject)
{
    return FMASceneGraphBootstrap::Resolve(WorldContextObject);
}

TArray<FMASceneGraphNode> FMASkillSceneGraphBridge::LoadAllNodes(const UObject* WorldContextObject)
{
    FMASceneGraphNodesFeedback Feedback = FMASceneGraphBootstrap::LoadAllNodes(WorldContextObject);
    return Feedback.bSuccess ? MoveTemp(Feedback.Nodes) : TArray<FMASceneGraphNode>();
}

TArray<FMASceneGraphNode> FMASkillSceneGraphBridge::LoadAllBuildings(const UObject* WorldContextObject)
{
    return FMASceneGraphQueryUseCases::GetAllBuildings(LoadAllNodes(WorldContextObject));
}

FMASceneGraphNode FMASkillSceneGraphBridge::LoadNodeById(const UObject* WorldContextObject, const FString& NodeId)
{
    if (NodeId.IsEmpty())
    {
        return FMASceneGraphNode();
    }

    const FMASceneGraphNodeFeedback Feedback = FMASceneGraphBootstrap::LoadNodeById(WorldContextObject, NodeId);
    return Feedback.bSuccess ? Feedback.Node : FMASceneGraphNode();
}

bool FMASkillSceneGraphBridge::ContainsNodeId(const UObject* WorldContextObject, const FString& NodeId)
{
    if (NodeId.IsEmpty())
    {
        return false;
    }

    if (UMASceneGraphManager* SceneGraphManager = ResolveManager(WorldContextObject))
    {
        return SceneGraphManager->IsIdExists(NodeId);
    }

    return false;
}

FMASceneGraphNode FMASkillSceneGraphBridge::FindNearestLandmark(
    const UObject* WorldContextObject,
    const FVector& Location,
    const float MaxDistance)
{
    const TArray<FMASceneGraphNode> AllNodes = LoadAllNodes(WorldContextObject);
    return AllNodes.Num() > 0
        ? FMASceneGraphQueryUseCases::FindNearestLandmark(AllNodes, Location, MaxDistance)
        : FMASceneGraphNode();
}

bool FMASkillSceneGraphBridge::IsPointInsideBuilding(const UObject* WorldContextObject, const FVector& Point)
{
    const TArray<FMASceneGraphNode> AllNodes = LoadAllNodes(WorldContextObject);
    return AllNodes.Num() > 0 && FMASceneGraphQueryUseCases::IsPointInsideBuilding(AllNodes, Point);
}
