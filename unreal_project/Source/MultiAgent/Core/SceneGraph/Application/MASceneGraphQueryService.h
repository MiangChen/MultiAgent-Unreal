// MASceneGraphQueryService.h
// SceneGraph 查询服务 (P1c: Manager 内部查询转发)

#pragma once

#include "CoreMinimal.h"
#include "Core/SceneGraph/Infrastructure/Ports/IMASceneGraphQueryPort.h"

class FMASceneGraphQueryService final : public IMASceneGraphQueryPort
{
public:
    FMASceneGraphQueryService(
        const TArray<FMASceneGraphNode>* InStaticNodes,
        const TArray<FMASceneGraphNode>* InDynamicNodes);

    virtual TArray<FMASceneGraphNode> GetAllNodes() const override;
    virtual FMASceneGraphNode GetNodeById(const FString& NodeId) const override;
    virtual TArray<FMASceneGraphNode> FindNodesByGuid(const FString& ActorGuid) const override;
    virtual TArray<FMASceneGraphNode> GetAllBuildings() const override;
    virtual TArray<FMASceneGraphNode> GetAllRoads() const override;
    virtual TArray<FMASceneGraphNode> GetAllIntersections() const override;
    virtual TArray<FMASceneGraphNode> GetAllProps() const override;
    virtual TArray<FMASceneGraphNode> GetAllRobots() const override;
    virtual TArray<FMASceneGraphNode> GetAllPickupItems() const override;
    virtual TArray<FMASceneGraphNode> GetAllChargingStations() const override;
    virtual TArray<FMASceneGraphNode> GetAllGoals() const override;
    virtual TArray<FMASceneGraphNode> GetAllZones() const override;

    virtual FMASceneGraphNode FindNodeByLabel(const FMASemanticLabel& Label) const override;
    virtual FMASceneGraphNode FindNearestNode(const FMASemanticLabel& Label, const FVector& FromLocation) const override;
    virtual TArray<FMASceneGraphNode> FindNodesInBoundary(
        const FMASemanticLabel& Label,
        const TArray<FVector>& BoundaryVertices) const override;
    virtual bool IsPointInsideBuilding(const FVector& Point) const override;
    virtual FMASceneGraphNode FindNearestLandmark(const FVector& Location, float MaxDistance) const override;
    virtual TSharedPtr<FJsonObject> NodeToJsonObject(const FMASceneGraphNode& Node) const override;

    virtual FString BuildWorldStateJson(
        const FString& CategoryFilter,
        const FString& TypeFilter,
        const FString& LabelFilter) const override;

private:
    TArray<FMASceneGraphNode> BuildAllNodes() const;

    const TArray<FMASceneGraphNode>* StaticNodes;
    const TArray<FMASceneGraphNode>* DynamicNodes;
};
