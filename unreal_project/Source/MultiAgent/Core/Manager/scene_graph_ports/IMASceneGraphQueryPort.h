// IMASceneGraphQueryPort.h
// SceneGraph 查询端口定义 (Hexagonal Port)

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MASceneGraphNodeTypes.h"
#include "MASceneGraphTypes.h"

class IMASceneGraphQueryPort
{
public:
    virtual ~IMASceneGraphQueryPort() = default;

    virtual TArray<FMASceneGraphNode> GetAllNodes() const = 0;
    virtual FMASceneGraphNode GetNodeById(const FString& NodeId) const = 0;
    virtual TArray<FMASceneGraphNode> FindNodesByGuid(const FString& ActorGuid) const = 0;
    virtual TArray<FMASceneGraphNode> GetAllBuildings() const = 0;
    virtual TArray<FMASceneGraphNode> GetAllRoads() const = 0;
    virtual TArray<FMASceneGraphNode> GetAllIntersections() const = 0;
    virtual TArray<FMASceneGraphNode> GetAllProps() const = 0;
    virtual TArray<FMASceneGraphNode> GetAllRobots() const = 0;
    virtual TArray<FMASceneGraphNode> GetAllPickupItems() const = 0;
    virtual TArray<FMASceneGraphNode> GetAllChargingStations() const = 0;
    virtual TArray<FMASceneGraphNode> GetAllGoals() const = 0;
    virtual TArray<FMASceneGraphNode> GetAllZones() const = 0;

    virtual FMASceneGraphNode FindNodeByLabel(const FMASemanticLabel& Label) const = 0;
    virtual FMASceneGraphNode FindNearestNode(const FMASemanticLabel& Label, const FVector& FromLocation) const = 0;
    virtual TArray<FMASceneGraphNode> FindNodesInBoundary(
        const FMASemanticLabel& Label,
        const TArray<FVector>& BoundaryVertices) const = 0;
    virtual bool IsPointInsideBuilding(const FVector& Point) const = 0;
    virtual FMASceneGraphNode FindNearestLandmark(const FVector& Location, float MaxDistance) const = 0;
    virtual TSharedPtr<FJsonObject> NodeToJsonObject(const FMASceneGraphNode& Node) const = 0;

    virtual FString BuildWorldStateJson(
        const FString& CategoryFilter,
        const FString& TypeFilter,
        const FString& LabelFilter) const = 0;
};
