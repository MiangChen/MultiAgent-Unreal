// MASceneGraphRuntimeSyncAdapter.cpp

#include "scene_graph_adapters/MASceneGraphRuntimeSyncAdapter.h"
#include "../MASceneGraphManager.h"

FMASceneGraphRuntimeSyncAdapter::FMASceneGraphRuntimeSyncAdapter(UMASceneGraphManager* InSceneGraphManager)
    : SceneGraphManager(InSceneGraphManager)
{
}

void FMASceneGraphRuntimeSyncAdapter::UpdateDynamicNodePosition(const FString& NodeId, const FVector& NewPosition)
{
    if (SceneGraphManager.IsValid())
    {
        SceneGraphManager->UpdateDynamicNodePosition(NodeId, NewPosition);
    }
}

void FMASceneGraphRuntimeSyncAdapter::UpdateDynamicNodeFeature(const FString& NodeId, const FString& Key, const FString& Value)
{
    if (SceneGraphManager.IsValid())
    {
        SceneGraphManager->UpdateDynamicNodeFeature(NodeId, Key, Value);
    }
}

TArray<FMASceneGraphNode> FMASceneGraphRuntimeSyncAdapter::FindNodesByGuid(const FString& ActorGuid) const
{
    if (!SceneGraphManager.IsValid())
    {
        return {};
    }
    return SceneGraphManager->FindNodesByGuid(ActorGuid);
}

FMASceneGraphNode FMASceneGraphRuntimeSyncAdapter::GetNodeById(const FString& NodeId) const
{
    if (!SceneGraphManager.IsValid())
    {
        return FMASceneGraphNode();
    }
    return SceneGraphManager->GetNodeById(NodeId);
}

TArray<FMASceneGraphNode> FMASceneGraphRuntimeSyncAdapter::GetAllNodes() const
{
    if (!SceneGraphManager.IsValid())
    {
        return {};
    }
    return SceneGraphManager->GetAllNodes();
}
