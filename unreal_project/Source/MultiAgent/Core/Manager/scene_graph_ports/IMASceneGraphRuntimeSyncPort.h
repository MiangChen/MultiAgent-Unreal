// IMASceneGraphRuntimeSyncPort.h
// SceneGraph 运行时同步端口定义 (Hexagonal Port)

#pragma once

#include "CoreMinimal.h"
#include "MASceneGraphNodeTypes.h"

class IMASceneGraphRuntimeSyncPort
{
public:
    virtual ~IMASceneGraphRuntimeSyncPort() = default;

    virtual void UpdateDynamicNodePosition(const FString& NodeId, const FVector& NewPosition) = 0;
    virtual void UpdateDynamicNodeFeature(const FString& NodeId, const FString& Key, const FString& Value) = 0;
    virtual TArray<FMASceneGraphNode> FindNodesByGuid(const FString& ActorGuid) const = 0;
    virtual FMASceneGraphNode GetNodeById(const FString& NodeId) const = 0;
    virtual TArray<FMASceneGraphNode> GetAllNodes() const = 0;
};
