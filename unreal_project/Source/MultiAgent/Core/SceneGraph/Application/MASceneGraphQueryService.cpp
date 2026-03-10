// MASceneGraphQueryService.cpp

#include "Core/SceneGraph/Application/MASceneGraphQueryService.h"
#include "Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"

FMASceneGraphQueryService::FMASceneGraphQueryService(
    const TArray<FMASceneGraphNode>* InStaticNodes,
    const TArray<FMASceneGraphNode>* InDynamicNodes)
    : StaticNodes(InStaticNodes)
    , DynamicNodes(InDynamicNodes)
{
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::BuildAllNodes() const
{
    return FMASceneGraphQueryUseCases::BuildAllNodes(StaticNodes, DynamicNodes);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllNodes() const
{
    return BuildAllNodes();
}

FMASceneGraphNode FMASceneGraphQueryService::GetNodeById(const FString& NodeId) const
{
    return FMASceneGraphQueryUseCases::GetNodeById(BuildAllNodes(), NodeId);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::FindNodesByGuid(const FString& ActorGuid) const
{
    return FMASceneGraphQueryUseCases::FindNodesByGuid(BuildAllNodes(), ActorGuid);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllBuildings() const
{
    return FMASceneGraphQueryUseCases::GetAllBuildings(BuildAllNodes());
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllRoads() const
{
    return FMASceneGraphQueryUseCases::GetAllRoads(BuildAllNodes());
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllIntersections() const
{
    return FMASceneGraphQueryUseCases::GetAllIntersections(BuildAllNodes());
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllProps() const
{
    return FMASceneGraphQueryUseCases::GetAllProps(BuildAllNodes());
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllRobots() const
{
    return FMASceneGraphQueryUseCases::GetAllRobots(BuildAllNodes());
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllPickupItems() const
{
    return FMASceneGraphQueryUseCases::GetAllPickupItems(BuildAllNodes());
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllChargingStations() const
{
    return FMASceneGraphQueryUseCases::GetAllChargingStations(BuildAllNodes());
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllGoals() const
{
    return FMASceneGraphQueryUseCases::GetAllGoals(BuildAllNodes());
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::GetAllZones() const
{
    return FMASceneGraphQueryUseCases::GetAllZones(BuildAllNodes());
}

FMASceneGraphNode FMASceneGraphQueryService::FindNodeByLabel(const FMASemanticLabel& Label) const
{
    return FMASceneGraphQueryUseCases::FindNodeByLabel(BuildAllNodes(), Label);
}

FMASceneGraphNode FMASceneGraphQueryService::FindNearestNode(const FMASemanticLabel& Label, const FVector& FromLocation) const
{
    return FMASceneGraphQueryUseCases::FindNearestNode(BuildAllNodes(), Label, FromLocation);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryService::FindNodesInBoundary(
    const FMASemanticLabel& Label,
    const TArray<FVector>& BoundaryVertices) const
{
    return FMASceneGraphQueryUseCases::FindNodesInBoundary(BuildAllNodes(), Label, BoundaryVertices);
}

bool FMASceneGraphQueryService::IsPointInsideBuilding(const FVector& Point) const
{
    return FMASceneGraphQueryUseCases::IsPointInsideBuilding(BuildAllNodes(), Point);
}

FMASceneGraphNode FMASceneGraphQueryService::FindNearestLandmark(const FVector& Location, float MaxDistance) const
{
    return FMASceneGraphQueryUseCases::FindNearestLandmark(BuildAllNodes(), Location, MaxDistance);
}

TSharedPtr<FJsonObject> FMASceneGraphQueryService::NodeToJsonObject(const FMASceneGraphNode& Node) const
{
    return FMASceneGraphQueryUseCases::NodeToJsonObject(Node);
}

FString FMASceneGraphQueryService::BuildWorldStateJson(
    const FString& CategoryFilter,
    const FString& TypeFilter,
    const FString& LabelFilter) const
{
    return FMASceneGraphQueryUseCases::BuildWorldStateJson(
        BuildAllNodes(),
        CategoryFilter,
        TypeFilter,
        LabelFilter);
}
