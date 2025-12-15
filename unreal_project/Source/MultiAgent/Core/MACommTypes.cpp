// MACommTypes.cpp
// 通信协议消息类型 JSON 序列化/反序列化实现
// Requirements: 1.5, 6.2

#include "MACommTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Guid.h"
#include "Misc/DateTime.h"

DEFINE_LOG_CATEGORY(LogMACommTypes);

//=============================================================================
// 辅助函数 - 消息类型字符串转换
//=============================================================================

namespace MACommTypeHelpers
{
    FString MessageTypeToString(EMACommMessageType Type)
    {
        switch (Type)
        {
        case EMACommMessageType::UIInput:         return TEXT("ui_input");
        case EMACommMessageType::ButtonEvent:     return TEXT("button_event");
        case EMACommMessageType::TaskFeedback:    return TEXT("task_feedback");
        case EMACommMessageType::TaskPlanDAG:     return TEXT("task_plan_dag");
        case EMACommMessageType::WorldModelGraph: return TEXT("world_model_graph");
        case EMACommMessageType::Custom:
        default:                                  return TEXT("custom");
        }
    }

    EMACommMessageType StringToMessageType(const FString& TypeStr)
    {
        if (TypeStr == TEXT("ui_input"))           return EMACommMessageType::UIInput;
        if (TypeStr == TEXT("button_event"))       return EMACommMessageType::ButtonEvent;
        if (TypeStr == TEXT("task_feedback"))      return EMACommMessageType::TaskFeedback;
        if (TypeStr == TEXT("task_plan_dag"))      return EMACommMessageType::TaskPlanDAG;
        if (TypeStr == TEXT("world_model_graph"))  return EMACommMessageType::WorldModelGraph;
        return EMACommMessageType::Custom;
    }
}

//=============================================================================
// FMAMessageEnvelope 实现
//=============================================================================


FString FMAMessageEnvelope::GenerateMessageId()
{
    return FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
}

int64 FMAMessageEnvelope::GetCurrentTimestamp()
{
    return FDateTime::UtcNow().ToUnixTimestamp() * 1000 + FDateTime::UtcNow().GetMillisecond();
}

FString FMAMessageEnvelope::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("message_type"), MACommTypeHelpers::MessageTypeToString(MessageType));
    JsonObject->SetNumberField(TEXT("timestamp"), static_cast<double>(Timestamp));
    JsonObject->SetStringField(TEXT("message_id"), MessageId);
    
    // 解析 PayloadJson 为 JSON 对象并嵌入
    TSharedPtr<FJsonObject> PayloadObject;
    TSharedRef<TJsonReader<>> PayloadReader = TJsonReaderFactory<>::Create(PayloadJson);
    if (FJsonSerializer::Deserialize(PayloadReader, PayloadObject) && PayloadObject.IsValid())
    {
        JsonObject->SetObjectField(TEXT("payload"), PayloadObject);
    }
    else
    {
        // 如果 PayloadJson 不是有效 JSON，作为字符串存储
        JsonObject->SetStringField(TEXT("payload"), PayloadJson);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMAMessageEnvelope::FromJson(const FString& Json, FMAMessageEnvelope& OutEnvelope)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("FMAMessageEnvelope::FromJson - Failed to parse JSON"));
        return false;
    }
    
    // 解析 message_type
    FString TypeStr;
    if (JsonObject->TryGetStringField(TEXT("message_type"), TypeStr))
    {
        OutEnvelope.MessageType = MACommTypeHelpers::StringToMessageType(TypeStr);
    }
    
    // 解析 timestamp
    double TimestampDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("timestamp"), TimestampDouble))
    {
        OutEnvelope.Timestamp = static_cast<int64>(TimestampDouble);
    }
    
    // 解析 message_id
    JsonObject->TryGetStringField(TEXT("message_id"), OutEnvelope.MessageId);
    
    // 解析 payload - 将其序列化回 JSON 字符串
    const TSharedPtr<FJsonObject>* PayloadObject;
    if (JsonObject->TryGetObjectField(TEXT("payload"), PayloadObject))
    {
        FString PayloadString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
        FJsonSerializer::Serialize(PayloadObject->ToSharedRef(), Writer);
        OutEnvelope.PayloadJson = PayloadString;
    }
    else
    {
        // 尝试作为字符串读取
        JsonObject->TryGetStringField(TEXT("payload"), OutEnvelope.PayloadJson);
    }
    
    return true;
}

//=============================================================================
// FMAUIInputMessage 实现
//=============================================================================

FString FMAUIInputMessage::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("input_source_id"), InputSourceId);
    JsonObject->SetStringField(TEXT("input_content"), InputContent);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMAUIInputMessage::FromJson(const FString& Json, FMAUIInputMessage& Out)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("FMAUIInputMessage::FromJson - Failed to parse JSON"));
        return false;
    }
    
    JsonObject->TryGetStringField(TEXT("input_source_id"), Out.InputSourceId);
    JsonObject->TryGetStringField(TEXT("input_content"), Out.InputContent);
    
    return true;
}

//=============================================================================
// FMAButtonEventMessage 实现
//=============================================================================

FString FMAButtonEventMessage::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("widget_name"), WidgetName);
    JsonObject->SetStringField(TEXT("button_id"), ButtonId);
    JsonObject->SetStringField(TEXT("button_text"), ButtonText);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMAButtonEventMessage::FromJson(const FString& Json, FMAButtonEventMessage& Out)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("FMAButtonEventMessage::FromJson - Failed to parse JSON"));
        return false;
    }
    
    JsonObject->TryGetStringField(TEXT("widget_name"), Out.WidgetName);
    JsonObject->TryGetStringField(TEXT("button_id"), Out.ButtonId);
    JsonObject->TryGetStringField(TEXT("button_text"), Out.ButtonText);
    
    return true;
}

//=============================================================================
// FMATaskFeedbackMessage 实现
//=============================================================================

FString FMATaskFeedbackMessage::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("task_id"), TaskId);
    JsonObject->SetStringField(TEXT("status"), Status);
    JsonObject->SetNumberField(TEXT("duration_seconds"), DurationSeconds);
    JsonObject->SetNumberField(TEXT("energy_consumed"), EnergyConsumed);
    JsonObject->SetStringField(TEXT("error_message"), ErrorMessage);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMATaskFeedbackMessage::FromJson(const FString& Json, FMATaskFeedbackMessage& Out)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("FMATaskFeedbackMessage::FromJson - Failed to parse JSON"));
        return false;
    }
    
    JsonObject->TryGetStringField(TEXT("task_id"), Out.TaskId);
    JsonObject->TryGetStringField(TEXT("status"), Out.Status);
    
    double DurationDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("duration_seconds"), DurationDouble))
    {
        Out.DurationSeconds = static_cast<float>(DurationDouble);
    }
    
    double EnergyDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("energy_consumed"), EnergyDouble))
    {
        Out.EnergyConsumed = static_cast<float>(EnergyDouble);
    }
    
    JsonObject->TryGetStringField(TEXT("error_message"), Out.ErrorMessage);
    
    return true;
}


//=============================================================================
// FMATaskPlanNode 实现
//=============================================================================

FString FMATaskPlanNode::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("node_id"), NodeId);
    JsonObject->SetStringField(TEXT("task_type"), TaskType);
    
    // Parameters 作为嵌套对象
    TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject());
    for (const auto& Pair : Parameters)
    {
        ParamsObject->SetStringField(Pair.Key, Pair.Value);
    }
    JsonObject->SetObjectField(TEXT("parameters"), ParamsObject);
    
    // Dependencies 作为字符串数组
    TArray<TSharedPtr<FJsonValue>> DepsArray;
    for (const FString& Dep : Dependencies)
    {
        DepsArray.Add(MakeShareable(new FJsonValueString(Dep)));
    }
    JsonObject->SetArrayField(TEXT("dependencies"), DepsArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMATaskPlanNode::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATaskPlanNode& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }
    
    JsonObject->TryGetStringField(TEXT("node_id"), Out.NodeId);
    JsonObject->TryGetStringField(TEXT("task_type"), Out.TaskType);
    
    // 解析 Parameters
    const TSharedPtr<FJsonObject>* ParamsObject;
    if (JsonObject->TryGetObjectField(TEXT("parameters"), ParamsObject))
    {
        Out.Parameters.Empty();
        for (const auto& Pair : (*ParamsObject)->Values)
        {
            FString Value;
            if (Pair.Value->TryGetString(Value))
            {
                Out.Parameters.Add(Pair.Key, Value);
            }
        }
    }
    
    // 解析 Dependencies
    const TArray<TSharedPtr<FJsonValue>>* DepsArray;
    if (JsonObject->TryGetArrayField(TEXT("dependencies"), DepsArray))
    {
        Out.Dependencies.Empty();
        for (const auto& DepValue : *DepsArray)
        {
            FString Dep;
            if (DepValue->TryGetString(Dep))
            {
                Out.Dependencies.Add(Dep);
            }
        }
    }
    
    return true;
}

//=============================================================================
// FMATaskPlanEdge 实现
//=============================================================================

FString FMATaskPlanEdge::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("from_node_id"), FromNodeId);
    JsonObject->SetStringField(TEXT("to_node_id"), ToNodeId);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMATaskPlanEdge::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATaskPlanEdge& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }
    
    JsonObject->TryGetStringField(TEXT("from_node_id"), Out.FromNodeId);
    JsonObject->TryGetStringField(TEXT("to_node_id"), Out.ToNodeId);
    
    return true;
}

//=============================================================================
// FMATaskPlanDAG 实现
//=============================================================================

FString FMATaskPlanDAG::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    // Nodes 数组
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FMATaskPlanNode& Node : Nodes)
    {
        TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject());
        NodeObject->SetStringField(TEXT("node_id"), Node.NodeId);
        NodeObject->SetStringField(TEXT("task_type"), Node.TaskType);
        
        // Parameters
        TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject());
        for (const auto& Pair : Node.Parameters)
        {
            ParamsObject->SetStringField(Pair.Key, Pair.Value);
        }
        NodeObject->SetObjectField(TEXT("parameters"), ParamsObject);
        
        // Dependencies
        TArray<TSharedPtr<FJsonValue>> DepsArray;
        for (const FString& Dep : Node.Dependencies)
        {
            DepsArray.Add(MakeShareable(new FJsonValueString(Dep)));
        }
        NodeObject->SetArrayField(TEXT("dependencies"), DepsArray);
        
        NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
    }
    JsonObject->SetArrayField(TEXT("nodes"), NodesArray);
    
    // Edges 数组
    TArray<TSharedPtr<FJsonValue>> EdgesArray;
    for (const FMATaskPlanEdge& Edge : Edges)
    {
        TSharedPtr<FJsonObject> EdgeObject = MakeShareable(new FJsonObject());
        EdgeObject->SetStringField(TEXT("from_node_id"), Edge.FromNodeId);
        EdgeObject->SetStringField(TEXT("to_node_id"), Edge.ToNodeId);
        EdgesArray.Add(MakeShareable(new FJsonValueObject(EdgeObject)));
    }
    JsonObject->SetArrayField(TEXT("edges"), EdgesArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMATaskPlanDAG::FromJson(const FString& Json, FMATaskPlanDAG& Out)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("FMATaskPlanDAG::FromJson - Failed to parse JSON"));
        return false;
    }
    
    Out.Nodes.Empty();
    Out.Edges.Empty();
    
    // 解析 Nodes
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (JsonObject->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        for (const auto& NodeValue : *NodesArray)
        {
            const TSharedPtr<FJsonObject>* NodeObject;
            if (NodeValue->TryGetObject(NodeObject))
            {
                FMATaskPlanNode Node;
                if (FMATaskPlanNode::FromJson(*NodeObject, Node))
                {
                    Out.Nodes.Add(Node);
                }
            }
        }
    }
    
    // 解析 Edges
    const TArray<TSharedPtr<FJsonValue>>* EdgesArray;
    if (JsonObject->TryGetArrayField(TEXT("edges"), EdgesArray))
    {
        for (const auto& EdgeValue : *EdgesArray)
        {
            const TSharedPtr<FJsonObject>* EdgeObject;
            if (EdgeValue->TryGetObject(EdgeObject))
            {
                FMATaskPlanEdge Edge;
                if (FMATaskPlanEdge::FromJson(*EdgeObject, Edge))
                {
                    Out.Edges.Add(Edge);
                }
            }
        }
    }
    
    return true;
}

bool FMATaskPlanDAG::IsValidDAG() const
{
    // 使用 Kahn's 算法检测环
    // 如果能完成拓扑排序，则是有效 DAG
    
    if (Nodes.Num() == 0)
    {
        return true; // 空图是有效 DAG
    }
    
    // 构建邻接表和入度表
    TMap<FString, TArray<FString>> AdjList;
    TMap<FString, int32> InDegree;
    
    // 初始化所有节点
    for (const FMATaskPlanNode& Node : Nodes)
    {
        AdjList.FindOrAdd(Node.NodeId);
        InDegree.FindOrAdd(Node.NodeId) = 0;
    }
    
    // 根据边构建邻接表和入度
    for (const FMATaskPlanEdge& Edge : Edges)
    {
        AdjList.FindOrAdd(Edge.FromNodeId).Add(Edge.ToNodeId);
        InDegree.FindOrAdd(Edge.ToNodeId)++;
    }
    
    // 也根据节点的 Dependencies 构建（如果有的话）
    for (const FMATaskPlanNode& Node : Nodes)
    {
        for (const FString& Dep : Node.Dependencies)
        {
            // Dep -> Node (依赖关系：Dep 必须在 Node 之前)
            if (!AdjList.Contains(Dep))
            {
                // 依赖的节点不存在，跳过
                continue;
            }
            // 检查是否已经通过 Edges 添加过
            if (!AdjList[Dep].Contains(Node.NodeId))
            {
                AdjList[Dep].Add(Node.NodeId);
                InDegree[Node.NodeId]++;
            }
        }
    }
    
    // Kahn's 算法
    TArray<FString> Queue;
    for (const auto& Pair : InDegree)
    {
        if (Pair.Value == 0)
        {
            Queue.Add(Pair.Key);
        }
    }
    
    int32 ProcessedCount = 0;
    while (Queue.Num() > 0)
    {
        FString Current = Queue[0];
        Queue.RemoveAt(0);
        ProcessedCount++;
        
        if (AdjList.Contains(Current))
        {
            for (const FString& Neighbor : AdjList[Current])
            {
                InDegree[Neighbor]--;
                if (InDegree[Neighbor] == 0)
                {
                    Queue.Add(Neighbor);
                }
            }
        }
    }
    
    // 如果处理的节点数等于总节点数，则无环
    return ProcessedCount == Nodes.Num();
}


//=============================================================================
// FMAWorldModelEntity 实现
//=============================================================================

FString FMAWorldModelEntity::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("entity_id"), EntityId);
    JsonObject->SetStringField(TEXT("entity_type"), EntityType);
    
    // Properties 作为嵌套对象
    TSharedPtr<FJsonObject> PropsObject = MakeShareable(new FJsonObject());
    for (const auto& Pair : Properties)
    {
        PropsObject->SetStringField(Pair.Key, Pair.Value);
    }
    JsonObject->SetObjectField(TEXT("properties"), PropsObject);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMAWorldModelEntity::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMAWorldModelEntity& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }
    
    JsonObject->TryGetStringField(TEXT("entity_id"), Out.EntityId);
    JsonObject->TryGetStringField(TEXT("entity_type"), Out.EntityType);
    
    // 解析 Properties
    const TSharedPtr<FJsonObject>* PropsObject;
    if (JsonObject->TryGetObjectField(TEXT("properties"), PropsObject))
    {
        Out.Properties.Empty();
        for (const auto& Pair : (*PropsObject)->Values)
        {
            FString Value;
            if (Pair.Value->TryGetString(Value))
            {
                Out.Properties.Add(Pair.Key, Value);
            }
        }
    }
    
    return true;
}

//=============================================================================
// FMAWorldModelRelationship 实现
//=============================================================================

FString FMAWorldModelRelationship::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("entity_a_id"), EntityAId);
    JsonObject->SetStringField(TEXT("entity_b_id"), EntityBId);
    JsonObject->SetStringField(TEXT("relationship_type"), RelationshipType);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMAWorldModelRelationship::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMAWorldModelRelationship& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }
    
    JsonObject->TryGetStringField(TEXT("entity_a_id"), Out.EntityAId);
    JsonObject->TryGetStringField(TEXT("entity_b_id"), Out.EntityBId);
    JsonObject->TryGetStringField(TEXT("relationship_type"), Out.RelationshipType);
    
    return true;
}

//=============================================================================
// FMAWorldModelGraph 实现
//=============================================================================

FString FMAWorldModelGraph::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    // Entities 数组
    TArray<TSharedPtr<FJsonValue>> EntitiesArray;
    for (const FMAWorldModelEntity& Entity : Entities)
    {
        TSharedPtr<FJsonObject> EntityObject = MakeShareable(new FJsonObject());
        EntityObject->SetStringField(TEXT("entity_id"), Entity.EntityId);
        EntityObject->SetStringField(TEXT("entity_type"), Entity.EntityType);
        
        // Properties
        TSharedPtr<FJsonObject> PropsObject = MakeShareable(new FJsonObject());
        for (const auto& Pair : Entity.Properties)
        {
            PropsObject->SetStringField(Pair.Key, Pair.Value);
        }
        EntityObject->SetObjectField(TEXT("properties"), PropsObject);
        
        EntitiesArray.Add(MakeShareable(new FJsonValueObject(EntityObject)));
    }
    JsonObject->SetArrayField(TEXT("entities"), EntitiesArray);
    
    // Relationships 数组
    TArray<TSharedPtr<FJsonValue>> RelationshipsArray;
    for (const FMAWorldModelRelationship& Rel : Relationships)
    {
        TSharedPtr<FJsonObject> RelObject = MakeShareable(new FJsonObject());
        RelObject->SetStringField(TEXT("entity_a_id"), Rel.EntityAId);
        RelObject->SetStringField(TEXT("entity_b_id"), Rel.EntityBId);
        RelObject->SetStringField(TEXT("relationship_type"), Rel.RelationshipType);
        RelationshipsArray.Add(MakeShareable(new FJsonValueObject(RelObject)));
    }
    JsonObject->SetArrayField(TEXT("relationships"), RelationshipsArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMAWorldModelGraph::FromJson(const FString& Json, FMAWorldModelGraph& Out)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("FMAWorldModelGraph::FromJson - Failed to parse JSON"));
        return false;
    }
    
    Out.Entities.Empty();
    Out.Relationships.Empty();
    
    // 解析 Entities
    const TArray<TSharedPtr<FJsonValue>>* EntitiesArray;
    if (JsonObject->TryGetArrayField(TEXT("entities"), EntitiesArray))
    {
        for (const auto& EntityValue : *EntitiesArray)
        {
            const TSharedPtr<FJsonObject>* EntityObject;
            if (EntityValue->TryGetObject(EntityObject))
            {
                FMAWorldModelEntity Entity;
                if (FMAWorldModelEntity::FromJson(*EntityObject, Entity))
                {
                    Out.Entities.Add(Entity);
                }
            }
        }
    }
    
    // 解析 Relationships
    const TArray<TSharedPtr<FJsonValue>>* RelationshipsArray;
    if (JsonObject->TryGetArrayField(TEXT("relationships"), RelationshipsArray))
    {
        for (const auto& RelValue : *RelationshipsArray)
        {
            const TSharedPtr<FJsonObject>* RelObject;
            if (RelValue->TryGetObject(RelObject))
            {
                FMAWorldModelRelationship Rel;
                if (FMAWorldModelRelationship::FromJson(*RelObject, Rel))
                {
                    Out.Relationships.Add(Rel);
                }
            }
        }
    }
    
    return true;
}
