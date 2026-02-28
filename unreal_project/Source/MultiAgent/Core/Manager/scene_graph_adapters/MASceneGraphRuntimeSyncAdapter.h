// MASceneGraphRuntimeSyncAdapter.h
// SceneGraph 运行时同步适配器

#pragma once

#include "CoreMinimal.h"
#include "scene_graph_ports/IMASceneGraphRuntimeSyncPort.h"

class UMASceneGraphManager;

class FMASceneGraphRuntimeSyncAdapter final : public IMASceneGraphRuntimeSyncPort
{
public:
    explicit FMASceneGraphRuntimeSyncAdapter(UMASceneGraphManager* InSceneGraphManager);

    virtual void UpdateDynamicNodePosition(const FString& NodeId, const FVector& NewPosition) override;
    virtual void UpdateDynamicNodeFeature(const FString& NodeId, const FString& Key, const FString& Value) override;
    virtual TArray<FMASceneGraphNode> FindNodesByGuid(const FString& ActorGuid) const override;
    virtual FMASceneGraphNode GetNodeById(const FString& NodeId) const override;
    virtual TArray<FMASceneGraphNode> GetAllNodes() const override;

private:
    TWeakObjectPtr<UMASceneGraphManager> SceneGraphManager;
};
