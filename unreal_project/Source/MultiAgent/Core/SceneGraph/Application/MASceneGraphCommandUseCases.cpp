// MASceneGraphCommandUseCases.cpp

#include "Core/SceneGraph/Application/MASceneGraphCommandUseCases.h"
#include "Core/SceneGraph/Infrastructure/Tools/MASceneGraphIO.h"
#include "Core/SceneGraph/Infrastructure/Ports/IMASceneGraphEventPublisherPort.h"
#include "Core/SceneGraph/Infrastructure/Tools/MADynamicNodeManager.h"
#include "Core/SceneGraph/Infrastructure/Tools/MASceneGraphLabelInputParser.h"
#include "Core/SceneGraph/Infrastructure/Tools/MASceneGraphNodeJsonBuilder.h"
#include "Core/SceneGraph/Infrastructure/Tools/MASceneGraphNodeJsonValidator.h"
#include "../../../Agent/Skill/Utils/MALocationUtils.h"
#include "Serialization/JsonReader.h"
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

bool FMASceneGraphCommandUseCases::ResolveLabelInput(
    const FString& InputText,
    const TFunctionRef<FString()>& NextIdProvider,
    FResolvedLabelInput& OutResolved,
    FString& OutError)
{
    OutResolved = FResolvedLabelInput{};

    FMASceneGraphLabelInputParseResult ParseResult;
    if (!FMASceneGraphLabelInputParser::Parse(InputText, ParseResult, OutError))
    {
        return false;
    }

    OutResolved.Type = ParseResult.Type;
    OutResolved.bHasCategory = ParseResult.bHasCategory;
    if (ParseResult.bHasId)
    {
        OutResolved.Id = ParseResult.Id;
        return true;
    }

    if (ParseResult.bHasCategory)
    {
        OutResolved.Id = NextIdProvider();
        OutResolved.bUsedAutoId = true;
        return true;
    }

    OutError = TEXT("缺少必填字段: id (或使用 cate:xxx 格式自动分配)");
    return false;
}

bool FMASceneGraphCommandUseCases::ContainsNodeId(
    const TArray<FMASceneGraphNode>& StaticNodes,
    const FString& NodeId)
{
    for (const FMASceneGraphNode& Node : StaticNodes)
    {
        if (Node.Id == NodeId)
        {
            return true;
        }
    }
    return false;
}

int32 FMASceneGraphCommandUseCases::CountNodeType(
    const TArray<FMASceneGraphNode>& StaticNodes,
    const FString& Type)
{
    int32 Count = 0;
    const FString LowerType = Type.ToLower();
    for (const FMASceneGraphNode& Node : StaticNodes)
    {
        if (Node.Type.ToLower() == LowerType)
        {
            ++Count;
        }
    }
    return Count;
}

FString FMASceneGraphCommandUseCases::BuildGeneratedLabel(
    const TArray<FMASceneGraphNode>& StaticNodes,
    const FString& Type)
{
    const FString CapitalizedType = CapitalizeFirstLetter(Type);
    const int32 Count = CountNodeType(StaticNodes, Type);
    return FString::Printf(TEXT("%s-%d"), *CapitalizedType, Count + 1);
}

FString FMASceneGraphCommandUseCases::CalculateNextNumericId(
    const TArray<FMASceneGraphNode>& StaticNodes)
{
    int32 MaxId = 0;
    for (const FMASceneGraphNode& Node : StaticNodes)
    {
        if (!Node.Id.IsNumeric())
        {
            continue;
        }

        const int32 IdNum = FCString::Atoi(*Node.Id);
        if (IdNum > MaxId)
        {
            MaxId = IdNum;
        }
    }

    return FString::FromInt(MaxId + 1);
}

bool FMASceneGraphCommandUseCases::AddPointSceneNode(
    const FAddPointSceneNodeInput& Input,
    const TFunctionRef<bool(const FString&, FString&)>& AddNodeFn,
    const TFunctionRef<bool()>& SaveToSourceFn,
    FString& OutError)
{
    const FString NodeJson = FMASceneGraphNodeJsonBuilder::BuildPointNodeJson(
        Input.Id,
        Input.Type,
        Input.Label,
        Input.WorldLocation,
        Input.Guid);

    if (!AddNodeFn(NodeJson, OutError))
    {
        return false;
    }

    SaveToSourceFn();
    return true;
}

bool FMASceneGraphCommandUseCases::ParseAndValidateNodeJson(
    const FString& NodeJson,
    TSharedPtr<FJsonObject>& OutNodeObject,
    FString& OutError)
{
    OutNodeObject.Reset();

    if (NodeJson.IsEmpty())
    {
        OutError = TEXT("JSON 字符串为空");
        return false;
    }

    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodeJson);
    if (!FJsonSerializer::Deserialize(Reader, OutNodeObject) || !OutNodeObject.IsValid())
    {
        OutError = TEXT("JSON 解析失败");
        return false;
    }

    if (!FMASceneGraphNodeJsonValidator::ValidateNodeJsonStructure(OutNodeObject, OutError))
    {
        return false;
    }

    return true;
}

bool FMASceneGraphCommandUseCases::PrepareStaticNodeForAdd(
    const FString& NodeJson,
    const TFunctionRef<bool(const FString&)>& IsIdExistsFn,
    TSharedPtr<FJsonObject>& OutNodeObject,
    FString& OutNodeId,
    FString& OutError)
{
    OutNodeId.Empty();
    if (!ParseAndValidateNodeJson(NodeJson, OutNodeObject, OutError))
    {
        return false;
    }

    if (!OutNodeObject->TryGetStringField(TEXT("id"), OutNodeId))
    {
        OutError = TEXT("JSON 缺少 id 字段");
        return false;
    }

    if (IsIdExistsFn(OutNodeId))
    {
        OutError = FString::Printf(TEXT("ID 已存在: %s"), *OutNodeId);
        return false;
    }

    return true;
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

bool FMASceneGraphCommandUseCases::AddStaticNodeFromJson(
    FStaticGraphContext& Context,
    const FString& NodeJson,
    const TFunctionRef<bool(const FString&)>& IsIdExistsFn,
    FString& OutNodeId,
    FString& OutError)
{
    TSharedPtr<FJsonObject> NodeObject;
    if (!PrepareStaticNodeForAdd(
        NodeJson,
        IsIdExistsFn,
        NodeObject,
        OutNodeId,
        OutError))
    {
        return false;
    }

    return AddStaticNode(Context, NodeObject, OutError);
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

bool FMASceneGraphCommandUseCases::ReplaceStaticNodeByIdFromJson(
    FStaticGraphContext& Context,
    const FString& NodeId,
    const FString& NewNodeJson,
    FString& OutError)
{
    TSharedPtr<FJsonObject> NewNodeObject;
    if (!ParseAndValidateNodeJson(NewNodeJson, NewNodeObject, OutError))
    {
        return false;
    }

    return ReplaceStaticNodeById(Context, NodeId, NewNodeObject, OutError);
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

FMASceneGraphNode* FMASceneGraphCommandUseCases::FindDynamicNodeByIdOrLabelMutable(
    FDynamicGraphContext& Context,
    const FString& NodeIdOrLabel)
{
    return FindDynamicNodeByIdMutable(Context, NodeIdOrLabel);
}

bool FMASceneGraphCommandUseCases::HasDynamicNode(
    FDynamicGraphContext& Context,
    const FString& NodeIdOrLabel)
{
    return FindDynamicNodeByIdMutable(Context, NodeIdOrLabel) != nullptr;
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

bool FMASceneGraphCommandUseCases::BindDynamicNodeGuid(
    FDynamicGraphContext& Context,
    const FString& NodeIdOrLabel,
    const FString& ActorGuid,
    FString& OutBoundNodeId,
    FString& OutBoundNodeLabel,
    FString& OutError)
{
    OutBoundNodeId.Empty();
    OutBoundNodeLabel.Empty();

    if (NodeIdOrLabel.IsEmpty())
    {
        OutError = TEXT("NodeIdOrLabel 为空");
        return false;
    }

    if (ActorGuid.IsEmpty())
    {
        OutError = TEXT("ActorGuid 为空");
        return false;
    }

    FMASceneGraphNode* Node = FindDynamicNodeByIdMutable(Context, NodeIdOrLabel);
    if (!Node)
    {
        OutError = FString::Printf(TEXT("未找到节点: %s"), *NodeIdOrLabel);
        return false;
    }

    if (!FMADynamicNodeManager::UpdateNodeGuid(*Node, ActorGuid))
    {
        OutError = FString::Printf(TEXT("更新节点 GUID 失败: %s"), *NodeIdOrLabel);
        return false;
    }

    OutBoundNodeId = Node->Id;
    OutBoundNodeLabel = Node->Label;
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

bool FMASceneGraphCommandUseCases::EditDynamicNodeFromJson(
    FDynamicGraphContext& Context,
    const FString& NodeId,
    const TSharedPtr<FJsonObject>& NewNodeObject,
    FString& OutError)
{
    if (!NewNodeObject.IsValid())
    {
        OutError = TEXT("新节点 JSON 对象无效");
        return false;
    }

    FMASceneGraphNode* Node = FindDynamicNodeByIdMutable(Context, NodeId);
    if (!Node)
    {
        OutError = FString::Printf(TEXT("动态节点不存在: %s"), *NodeId);
        return false;
    }

    FString NewId;
    if (NewNodeObject->TryGetStringField(TEXT("id"), NewId) && !NewId.IsEmpty())
    {
        Node->Id = NewId;
    }

    FString NewGuid;
    if (NewNodeObject->TryGetStringField(TEXT("guid"), NewGuid))
    {
        Node->Guid = NewGuid;
    }

    const TSharedPtr<FJsonObject>* PropertiesObject = nullptr;
    if (NewNodeObject->TryGetObjectField(TEXT("properties"), PropertiesObject) && PropertiesObject && (*PropertiesObject).IsValid())
    {
        FString NewType;
        if ((*PropertiesObject)->TryGetStringField(TEXT("type"), NewType) && !NewType.IsEmpty())
        {
            Node->Type = NewType;
        }

        FString NewLabel;
        if ((*PropertiesObject)->TryGetStringField(TEXT("label"), NewLabel) && !NewLabel.IsEmpty())
        {
            Node->Label = NewLabel;
        }

        FString NewCategory;
        if ((*PropertiesObject)->TryGetStringField(TEXT("category"), NewCategory) && !NewCategory.IsEmpty())
        {
            Node->Category = NewCategory;
        }

        bool bNewIsDynamic = false;
        if ((*PropertiesObject)->TryGetBoolField(TEXT("is_dynamic"), bNewIsDynamic))
        {
            Node->bIsDynamic = bNewIsDynamic;
        }

        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*PropertiesObject)->Values)
        {
            const FString& Key = Pair.Key;
            if (Key == TEXT("type") || Key == TEXT("label") || Key == TEXT("category") || Key == TEXT("is_dynamic"))
            {
                continue;
            }

            FString Value;
            if (Pair.Value.IsValid() && Pair.Value->TryGetString(Value))
            {
                Node->Features.Add(Key, Value);
            }
        }
    }

    const TSharedPtr<FJsonObject>* ShapeObject = nullptr;
    if (NewNodeObject->TryGetObjectField(TEXT("shape"), ShapeObject) && ShapeObject && (*ShapeObject).IsValid())
    {
        FString NewShapeType;
        if ((*ShapeObject)->TryGetStringField(TEXT("type"), NewShapeType) && !NewShapeType.IsEmpty())
        {
            Node->ShapeType = NewShapeType;
        }

        const TArray<TSharedPtr<FJsonValue>>* CenterArray = nullptr;
        if ((*ShapeObject)->TryGetArrayField(TEXT("center"), CenterArray) && CenterArray && CenterArray->Num() == 3)
        {
            Node->Center.X = (*CenterArray)[0]->AsNumber();
            Node->Center.Y = (*CenterArray)[1]->AsNumber();
            Node->Center.Z = (*CenterArray)[2]->AsNumber();
        }
    }

    Node->RawJson = FMADynamicNodeManager::GenerateRawJson(*Node);
    return true;
}

bool FMASceneGraphCommandUseCases::ValidateEditDynamicNodeInput(
    const FString& NodeId,
    const FString& NewNodeJson,
    FString& OutError)
{
    if (NodeId.IsEmpty())
    {
        OutError = TEXT("节点 ID 为空");
        return false;
    }

    if (NewNodeJson.IsEmpty())
    {
        OutError = TEXT("新节点 JSON 为空");
        return false;
    }

    return true;
}

bool FMASceneGraphCommandUseCases::EditDynamicNodeFromJsonString(
    FDynamicGraphContext& Context,
    const FString& NodeId,
    const FString& NewNodeJson,
    FString& OutError)
{
    if (!ValidateEditDynamicNodeInput(NodeId, NewNodeJson, OutError))
    {
        return false;
    }

    TSharedPtr<FJsonObject> NewNodeObject;
    if (!ParseAndValidateNodeJson(NewNodeJson, NewNodeObject, OutError))
    {
        return false;
    }

    return EditDynamicNodeFromJson(Context, NodeId, NewNodeObject, OutError);
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

FString FMASceneGraphCommandUseCases::CapitalizeFirstLetter(const FString& Type)
{
    if (Type.IsEmpty())
    {
        return Type;
    }

    FString Result = Type;
    Result[0] = FChar::ToUpper(Result[0]);
    return Result;
}

FMASceneGraphCommandUseCases::FSaveResultReport FMASceneGraphCommandUseCases::BuildSaveResultReport(
    ESaveResult SaveResult,
    const FString& SourcePath)
{
    switch (SaveResult)
    {
        case ESaveResult::Saved:
            return {true, ELogVerbosity::Log, FString::Printf(TEXT("SaveToSource: Successfully saved to %s"), *SourcePath)};
        case ESaveResult::SkippedByRunMode:
            return {false, ELogVerbosity::Warning, TEXT("SaveToSource: Cannot save in Edit mode. Changes are only kept in memory.")};
        case ESaveResult::InvalidWorkingCopy:
            return {false, ELogVerbosity::Error, TEXT("SaveToSource: WorkingCopy is invalid")};
        case ESaveResult::EmptySourcePath:
            return {false, ELogVerbosity::Error, TEXT("SaveToSource: SourceFilePath is empty")};
        case ESaveResult::PersistFailed:
        default:
            return {false, ELogVerbosity::Error, FString::Printf(TEXT("SaveToSource: Failed to save to %s"), *SourcePath)};
    }
}

FMASceneGraphCommandUseCases::FSceneChangePublishReport FMASceneGraphCommandUseCases::PublishSceneChange(
    IMASceneGraphEventPublisherPort* EventPublisherPort,
    EMASceneChangeType ChangeType,
    const FString& Payload)
{
    if (!EventPublisherPort)
    {
        return {false, ELogVerbosity::Warning, TEXT("NotifySceneChange: EventPublisherPort not available")};
    }

    EventPublisherPort->PublishSceneChange(ChangeType, Payload);
    return {true, ELogVerbosity::Log, FString::Printf(TEXT("NotifySceneChange: Delegated type=%d"), static_cast<int32>(ChangeType))};
}
