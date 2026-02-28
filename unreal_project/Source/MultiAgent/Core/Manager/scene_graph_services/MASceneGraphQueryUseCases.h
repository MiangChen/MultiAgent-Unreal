// MASceneGraphQueryUseCases.h
// SceneGraph 读用例 (P3a 进行中)

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MASceneGraphNodeTypes.h"
#include "MASceneGraphTypes.h"

class FMASceneGraphQueryUseCases
{
public:
    static TArray<FMASceneGraphNode> BuildAllNodes(
        const TArray<FMASceneGraphNode>* StaticNodes,
        const TArray<FMASceneGraphNode>* DynamicNodes);

    static FMASceneGraphNode GetNodeById(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& NodeId);

    static TArray<FMASceneGraphNode> FindNodesByGuid(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& ActorGuid);
    static TArray<FMASceneGraphNode> FindAllNodesByLabel(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FMASemanticLabel& Label);
    static FMASceneGraphNode FindNodeByIdOrLabel(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& NodeIdOrLabel);
    static FString GetIdByLabel(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& Label);
    static FString GetLabelById(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& NodeId);

    static TArray<FMASceneGraphNode> GetAllBuildings(const TArray<FMASceneGraphNode>& AllNodes);
    static TArray<FMASceneGraphNode> GetAllRoads(const TArray<FMASceneGraphNode>& AllNodes);
    static TArray<FMASceneGraphNode> GetAllIntersections(const TArray<FMASceneGraphNode>& AllNodes);
    static TArray<FMASceneGraphNode> GetAllProps(const TArray<FMASceneGraphNode>& AllNodes);
    static TArray<FMASceneGraphNode> GetAllRobots(const TArray<FMASceneGraphNode>& AllNodes);
    static TArray<FMASceneGraphNode> GetAllPickupItems(const TArray<FMASceneGraphNode>& AllNodes);
    static TArray<FMASceneGraphNode> GetAllChargingStations(const TArray<FMASceneGraphNode>& AllNodes);
    static TArray<FMASceneGraphNode> GetAllGoals(const TArray<FMASceneGraphNode>& AllNodes);
    static TArray<FMASceneGraphNode> GetAllZones(const TArray<FMASceneGraphNode>& AllNodes);

    static FMASceneGraphNode FindNodeByLabel(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FMASemanticLabel& Label);

    static FMASceneGraphNode FindNearestNode(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FMASemanticLabel& Label,
        const FVector& FromLocation);

    static TArray<FMASceneGraphNode> FindNodesInBoundary(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FMASemanticLabel& Label,
        const TArray<FVector>& BoundaryVertices);

    static bool IsPointInsideBuilding(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FVector& Point);

    static FMASceneGraphNode FindNearestLandmark(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FVector& Location,
        float MaxDistance);

    static TSharedPtr<FJsonObject> NodeToJsonObject(const FMASceneGraphNode& Node);

    static FString BuildWorldStateJson(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& CategoryFilter,
        const FString& TypeFilter,
        const FString& LabelFilter);

private:
    static bool PassesFilter(
        const FMASceneGraphNode& Node,
        const FString& CategoryFilter,
        const FString& TypeFilter,
        const FString& LabelFilter);
};
