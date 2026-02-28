// MASceneGraphCommandUseCases.cpp

#include "scene_graph_services/MASceneGraphCommandUseCases.h"
#include "scene_graph_tools/MASceneGraphIO.h"
#include "../scene_graph_tools/MADynamicNodeManager.h"
#include "../../../Agent/Skill/Utils/MALocationUtils.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

TArray<TSharedPtr<FJsonValue>>* FMASceneGraphCommandUseCases::GetNodesArrayMutable(const TSharedPtr<FJsonObject>& WorkingCopy)
{
    if (!WorkingCopy.IsValid())
    {
        return nullptr;
    }

    const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
    if (!WorkingCopy->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        return nullptr;
    }

    return const_cast<TArray<TSharedPtr<FJsonValue>>*>(NodesArray);
}

bool FMASceneGraphCommandUseCases::AddStaticNode(
    FStaticGraphContext& Context,
    const TSharedPtr<FJsonObject>& NodeObject,
    FString& OutError)
{
    if (!NodeObject.IsValid())
    {
        OutError = TEXT("节点 JSON 对象无效");
        return false;
    }

    if (!Context.WorkingCopy.IsValid())
    {
        Context.WorkingCopy = MakeShareable(new FJsonObject());
        Context.WorkingCopy->SetArrayField(TEXT("nodes"), {});
    }

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArrayMutable(Context.WorkingCopy);
    if (!NodesArray)
    {
        Context.WorkingCopy->SetArrayField(TEXT("nodes"), {});
        NodesArray = GetNodesArrayMutable(Context.WorkingCopy);
    }

    if (!NodesArray)
    {
        OutError = TEXT("无法获取节点数组");
        return false;
    }

    NodesArray->Add(MakeShareable(new FJsonValueObject(NodeObject)));
    Context.StaticNodes = FMASceneGraphIO::ParseNodes(*NodesArray);
    return true;
}

bool FMASceneGraphCommandUseCases::DeleteStaticNodeById(
    FStaticGraphContext& Context,
    const FString& NodeId,
    FString& OutDeletedNodeJson,
    FString& OutError)
{
    if (!Context.WorkingCopy.IsValid())
    {
        OutError = TEXT("Working Copy 无效");
        return false;
    }

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArrayMutable(Context.WorkingCopy);
    if (!NodesArray)
    {
        OutError = TEXT("无法获取节点数组");
        return false;
    }

    int32 NodeIndex = INDEX_NONE;
    OutDeletedNodeJson.Empty();
    for (int32 i = 0; i < NodesArray->Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& NodeValue = (*NodesArray)[i];
        if (!NodeValue.IsValid() || NodeValue->Type != EJson::Object)
        {
            continue;
        }

        TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
        if (!NodeObject.IsValid())
        {
            continue;
        }

        FString CurrentId;
        if (!NodeObject->TryGetStringField(TEXT("id"), CurrentId) || CurrentId != NodeId)
        {
            continue;
        }

        NodeIndex = i;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutDeletedNodeJson);
        FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer);
        break;
    }

    if (NodeIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("节点不存在: %s"), *NodeId);
        return false;
    }

    NodesArray->RemoveAt(NodeIndex);
    Context.StaticNodes = FMASceneGraphIO::ParseNodes(*NodesArray);
    return true;
}

bool FMASceneGraphCommandUseCases::ReplaceStaticNodeById(
    FStaticGraphContext& Context,
    const FString& NodeId,
    const TSharedPtr<FJsonObject>& NewNodeObject,
    FString& OutError)
{
    if (!Context.WorkingCopy.IsValid())
    {
        OutError = TEXT("Working Copy 无效");
        return false;
    }

    if (!NewNodeObject.IsValid())
    {
        OutError = TEXT("新节点 JSON 对象无效");
        return false;
    }

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArrayMutable(Context.WorkingCopy);
    if (!NodesArray)
    {
        OutError = TEXT("无法获取节点数组");
        return false;
    }

    int32 NodeIndex = INDEX_NONE;
    for (int32 i = 0; i < NodesArray->Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& NodeValue = (*NodesArray)[i];
        if (!NodeValue.IsValid() || NodeValue->Type != EJson::Object)
        {
            continue;
        }

        TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
        if (!NodeObject.IsValid())
        {
            continue;
        }

        FString CurrentId;
        if (NodeObject->TryGetStringField(TEXT("id"), CurrentId) && CurrentId == NodeId)
        {
            NodeIndex = i;
            break;
        }
    }

    if (NodeIndex == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("静态节点不存在: %s"), *NodeId);
        return false;
    }

    (*NodesArray)[NodeIndex] = MakeShareable(new FJsonValueObject(NewNodeObject));
    Context.StaticNodes = FMASceneGraphIO::ParseNodes(*NodesArray);
    return true;
}

bool FMASceneGraphCommandUseCases::BuildNodeJsonWithGoalFlag(
    FStaticGraphContext& Context,
    const FString& NodeId,
    bool bIsGoal,
    FString& OutModifiedNodeJson,
    FString& OutError)
{
    if (!Context.WorkingCopy.IsValid())
    {
        OutError = TEXT("Working Copy 无效");
        return false;
    }

    TArray<TSharedPtr<FJsonValue>>* NodesArray = GetNodesArrayMutable(Context.WorkingCopy);
    if (!NodesArray)
    {
        OutError = TEXT("无法获取节点数组");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        if (!NodeValue.IsValid() || NodeValue->Type != EJson::Object)
        {
            continue;
        }

        TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
        if (!NodeObject.IsValid())
        {
            continue;
        }

        FString CurrentId;
        if (!NodeObject->TryGetStringField(TEXT("id"), CurrentId) || CurrentId != NodeId)
        {
            continue;
        }

        TSharedPtr<FJsonObject> Properties;
        const TSharedPtr<FJsonObject>* ExistingProps = nullptr;
        if (NodeObject->TryGetObjectField(TEXT("properties"), ExistingProps) && ExistingProps && (*ExistingProps).IsValid())
        {
            Properties = *ExistingProps;
        }
        else
        {
            Properties = MakeShareable(new FJsonObject());
            NodeObject->SetObjectField(TEXT("properties"), Properties);
        }

        if (bIsGoal)
        {
            Properties->SetBoolField(TEXT("is_goal"), true);
        }
        else
        {
            Properties->RemoveField(TEXT("is_goal"));
        }

        OutModifiedNodeJson.Empty();
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutModifiedNodeJson);
        FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer);
        return true;
    }

    OutError = FString::Printf(TEXT("节点不存在: %s"), *NodeId);
    return false;
}

FMASceneGraphNode* FMASceneGraphCommandUseCases::FindDynamicNodeByIdMutable(
    FDynamicGraphContext& Context,
    const FString& NodeId)
{
    for (FMASceneGraphNode& DynNode : Context.DynamicNodes)
    {
        if (DynNode.Id == NodeId || DynNode.Label == NodeId)
        {
            return &DynNode;
        }
    }
    return nullptr;
}

bool FMASceneGraphCommandUseCases::UpdateDynamicNodePosition(
    FDynamicGraphContext& Context,
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& NodeId,
    const FVector& NewPosition,
    FString& OutLocationLabel,
    FString& OutError)
{
    FMASceneGraphNode* Node = FindDynamicNodeByIdMutable(Context, NodeId);
    if (!Node)
    {
        OutError = FString::Printf(TEXT("Node not found: %s"), *NodeId);
        return false;
    }

    if (!FMADynamicNodeManager::UpdateNodePosition(*Node, NewPosition))
    {
        OutError = FString::Printf(TEXT("Failed to update node position: %s"), *NodeId);
        return false;
    }

    OutLocationLabel = FMALocationUtils::InferNearestLocationLabel(
        AllNodes,
        NewPosition,
        5000.f,
        Node->Id);
    Node->LocationLabel = OutLocationLabel;
    return true;
}

bool FMASceneGraphCommandUseCases::UpdateDynamicNodeFeature(
    FDynamicGraphContext& Context,
    const FString& NodeId,
    const FString& Key,
    const FString& Value,
    FString& OutError)
{
    FMASceneGraphNode* Node = FindDynamicNodeByIdMutable(Context, NodeId);
    if (!Node)
    {
        OutError = FString::Printf(TEXT("Node not found: %s"), *NodeId);
        return false;
    }

    if (!FMADynamicNodeManager::UpdateNodeFeature(*Node, Key, Value))
    {
        OutError = FString::Printf(TEXT("Failed to update node feature: %s"), *NodeId);
        return false;
    }

    return true;
}

FMASceneGraphCommandUseCases::ESavePrecondition FMASceneGraphCommandUseCases::CheckSavePreconditions(
    bool bCanPersistToSource,
    const TSharedPtr<FJsonObject>& WorkingCopy,
    const FString& SourcePath)
{
    if (!bCanPersistToSource)
    {
        return ESavePrecondition::DisabledByRunMode;
    }

    if (!WorkingCopy.IsValid())
    {
        return ESavePrecondition::InvalidWorkingCopy;
    }

    if (SourcePath.IsEmpty())
    {
        return ESavePrecondition::EmptySourcePath;
    }

    return ESavePrecondition::Ok;
}

FMASceneGraphCommandUseCases::ESaveResult FMASceneGraphCommandUseCases::SaveWorkingCopy(
    bool bCanPersistToSource,
    const TSharedPtr<FJsonObject>& WorkingCopy,
    const FString& SourcePath,
    const TFunctionRef<bool(const TSharedPtr<FJsonObject>&)>& PersistFn)
{
    const ESavePrecondition Precondition = CheckSavePreconditions(
        bCanPersistToSource,
        WorkingCopy,
        SourcePath);

    if (Precondition == ESavePrecondition::DisabledByRunMode)
    {
        return ESaveResult::SkippedByRunMode;
    }
    if (Precondition == ESavePrecondition::InvalidWorkingCopy)
    {
        return ESaveResult::InvalidWorkingCopy;
    }
    if (Precondition == ESavePrecondition::EmptySourcePath)
    {
        return ESaveResult::EmptySourcePath;
    }

    return PersistFn(WorkingCopy) ? ESaveResult::Saved : ESaveResult::PersistFailed;
}
