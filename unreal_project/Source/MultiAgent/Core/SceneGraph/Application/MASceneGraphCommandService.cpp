// MASceneGraphCommandService.cpp

#include "Core/SceneGraph/Application/MASceneGraphCommandService.h"

FMASceneGraphCommandService::FMASceneGraphCommandService(FHandlers InHandlers)
    : Handlers(MoveTemp(InHandlers))
{
}

bool FMASceneGraphCommandService::AddNode(const FString& NodeJson, FString& OutError)
{
    if (Handlers.AddNode)
    {
        return Handlers.AddNode(NodeJson, OutError);
    }

    OutError = TEXT("CommandService handler missing: AddNode");
    return false;
}

bool FMASceneGraphCommandService::DeleteNode(const FString& NodeId, FString& OutError)
{
    if (Handlers.DeleteNode)
    {
        return Handlers.DeleteNode(NodeId, OutError);
    }

    OutError = TEXT("CommandService handler missing: DeleteNode");
    return false;
}

bool FMASceneGraphCommandService::EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
{
    if (Handlers.EditNode)
    {
        return Handlers.EditNode(NodeId, NewNodeJson, OutError);
    }

    OutError = TEXT("CommandService handler missing: EditNode");
    return false;
}

bool FMASceneGraphCommandService::SetNodeAsGoal(const FString& NodeId, FString& OutError)
{
    if (Handlers.SetNodeAsGoal)
    {
        return Handlers.SetNodeAsGoal(NodeId, OutError);
    }

    OutError = TEXT("CommandService handler missing: SetNodeAsGoal");
    return false;
}

bool FMASceneGraphCommandService::UnsetNodeAsGoal(const FString& NodeId, FString& OutError)
{
    if (Handlers.UnsetNodeAsGoal)
    {
        return Handlers.UnsetNodeAsGoal(NodeId, OutError);
    }

    OutError = TEXT("CommandService handler missing: UnsetNodeAsGoal");
    return false;
}

void FMASceneGraphCommandService::UpdateDynamicNodePosition(const FString& NodeId, const FVector& NewPosition)
{
    if (Handlers.UpdateDynamicNodePosition)
    {
        Handlers.UpdateDynamicNodePosition(NodeId, NewPosition);
    }
}

void FMASceneGraphCommandService::UpdateDynamicNodeFeature(const FString& NodeId, const FString& Key, const FString& Value)
{
    if (Handlers.UpdateDynamicNodeFeature)
    {
        Handlers.UpdateDynamicNodeFeature(NodeId, Key, Value);
    }
}

bool FMASceneGraphCommandService::SaveToSource()
{
    if (Handlers.SaveToSource)
    {
        return Handlers.SaveToSource();
    }
    return false;
}
