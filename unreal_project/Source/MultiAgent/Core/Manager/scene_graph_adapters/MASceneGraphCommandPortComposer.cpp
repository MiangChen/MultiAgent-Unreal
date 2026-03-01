// MASceneGraphCommandPortComposer.cpp

#include "scene_graph_adapters/MASceneGraphCommandPortComposer.h"
#include "../MASceneGraphManager.h"
#include "scene_graph_services/MASceneGraphCommandService.h"

TSharedPtr<IMASceneGraphCommandPort> FMASceneGraphCommandPortComposer::Create(UMASceneGraphManager& Manager)
{
    FMASceneGraphCommandService::FHandlers Handlers;
    Handlers.AddNode = [&Manager](const FString& NodeJson, FString& OutError)
    {
        return Manager.AddNodeInternal(NodeJson, OutError);
    };
    Handlers.DeleteNode = [&Manager](const FString& NodeId, FString& OutError)
    {
        return Manager.DeleteNodeInternal(NodeId, OutError);
    };
    Handlers.EditNode = [&Manager](const FString& NodeId, const FString& NewNodeJson, FString& OutError)
    {
        return Manager.EditNodeInternal(NodeId, NewNodeJson, OutError);
    };
    Handlers.SetNodeAsGoal = [&Manager](const FString& NodeId, FString& OutError)
    {
        return Manager.SetNodeAsGoalInternal(NodeId, OutError);
    };
    Handlers.UnsetNodeAsGoal = [&Manager](const FString& NodeId, FString& OutError)
    {
        return Manager.UnsetNodeAsGoalInternal(NodeId, OutError);
    };
    Handlers.UpdateDynamicNodePosition = [&Manager](const FString& NodeId, const FVector& NewPosition)
    {
        Manager.UpdateDynamicNodePositionInternal(NodeId, NewPosition);
    };
    Handlers.UpdateDynamicNodeFeature = [&Manager](const FString& NodeId, const FString& Key, const FString& Value)
    {
        Manager.UpdateDynamicNodeFeatureInternal(NodeId, Key, Value);
    };
    Handlers.SaveToSource = [&Manager]()
    {
        return Manager.SaveToSourceInternal();
    };

    return MakeShared<FMASceneGraphCommandService>(MoveTemp(Handlers));
}
