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
        case EMACommMessageType::WorldState:      return TEXT("world_state");
        case EMACommMessageType::TaskPlanDAG:     return TEXT("task_plan_dag");
        case EMACommMessageType::WorldModelGraph: return TEXT("world_model_graph");
        case EMACommMessageType::SkillList:       return TEXT("skill_list");
        case EMACommMessageType::QueryRequest:    return TEXT("query_request");
        case EMACommMessageType::Custom:
        default:                                  return TEXT("custom");
        }
    }

    EMACommMessageType StringToMessageType(const FString& TypeStr)
    {
        if (TypeStr == TEXT("ui_input"))           return EMACommMessageType::UIInput;
        if (TypeStr == TEXT("button_event"))       return EMACommMessageType::ButtonEvent;
        if (TypeStr == TEXT("task_feedback"))      return EMACommMessageType::TaskFeedback;
        if (TypeStr == TEXT("world_state"))        return EMACommMessageType::WorldState;
        if (TypeStr == TEXT("task_plan_dag"))      return EMACommMessageType::TaskPlanDAG;
        if (TypeStr == TEXT("world_model_graph"))  return EMACommMessageType::WorldModelGraph;
        if (TypeStr == TEXT("skill_list"))         return EMACommMessageType::SkillList;
        if (TypeStr == TEXT("query_request"))      return EMACommMessageType::QueryRequest;
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


//=============================================================================
// FMASkillListMessage 实现 (新格式: 按时间步组织)
// 格式: { "0": { "AgentID": { "skill": "xxx", "params": {...} } }, "1": {...} }
//=============================================================================

bool FMASkillListMessage::FromJson(const FString& Json, FMASkillListMessage& Out)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("FMASkillListMessage::FromJson - Failed to parse JSON"));
        return false;
    }
    
    Out.TimeSteps.Empty();
    Out.TotalTimeSteps = 0;
    
    // 遍历所有字段，按时间步索引解析
    TArray<int32> TimeStepIndices;
    for (const auto& Pair : JsonObject->Values)
    {
        // 尝试将 key 解析为数字（时间步索引）
        if (Pair.Key.IsNumeric())
        {
            int32 StepIndex = FCString::Atoi(*Pair.Key);
            TimeStepIndices.Add(StepIndex);
        }
    }
    
    // 排序时间步
    TimeStepIndices.Sort();
    
    // 解析每个时间步
    for (int32 StepIndex : TimeStepIndices)
    {
        FString StepKey = FString::FromInt(StepIndex);
        const TSharedPtr<FJsonObject>* StepObject;
        
        if (!JsonObject->TryGetObjectField(StepKey, StepObject))
        {
            continue;
        }
        
        FMATimeStepCommands TimeStepCmd;
        TimeStepCmd.TimeStep = StepIndex;
        
        // 遍历该时间步内的所有 Agent
        for (const auto& AgentPair : (*StepObject)->Values)
        {
            FString AgentId = AgentPair.Key;
            const TSharedPtr<FJsonObject>* AgentCmdObject;
            
            if (!AgentPair.Value->TryGetObject(AgentCmdObject))
            {
                continue;
            }
            
            FMAAgentSkillCommand Cmd;
            Cmd.AgentId = AgentId;
            
            // 解析 skill
            (*AgentCmdObject)->TryGetStringField(TEXT("skill"), Cmd.SkillName);
            
            // 解析 params
            const TSharedPtr<FJsonObject>* ParamsObject;
            if ((*AgentCmdObject)->TryGetObjectField(TEXT("params"), ParamsObject))
            {
                // 保存原始 JSON
                FString ParamsJson;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsJson);
                FJsonSerializer::Serialize(ParamsObject->ToSharedRef(), Writer);
                Cmd.Params.RawParamsJson = ParamsJson;
                
                // 解析常用参数
                (*ParamsObject)->TryGetStringField(TEXT("goal_type"), Cmd.Params.GoalType);
                (*ParamsObject)->TryGetStringField(TEXT("task_id"), Cmd.Params.TaskId);
                (*ParamsObject)->TryGetStringField(TEXT("area_token"), Cmd.Params.AreaToken);
                (*ParamsObject)->TryGetStringField(TEXT("target_token"), Cmd.Params.TargetToken);
                
                // 解析 dest (目标位置)
                const TSharedPtr<FJsonObject>* DestObject;
                if ((*ParamsObject)->TryGetObjectField(TEXT("dest"), DestObject))
                {
                    double X = 0, Y = 0, Z = 0;
                    (*DestObject)->TryGetNumberField(TEXT("x"), X);
                    (*DestObject)->TryGetNumberField(TEXT("y"), Y);
                    (*DestObject)->TryGetNumberField(TEXT("z"), Z);
                    Cmd.Params.DestPosition = FVector(X, Y, Z);
                    Cmd.Params.bHasDestPosition = true;
                }
                
                // 解析 target_entity (目标实体名称)
                (*ParamsObject)->TryGetStringField(TEXT("target_entity"), Cmd.Params.TargetEntity);
                
                // 解析 area (搜索区域多边形) - 格式: { "area": { "coords": [[x,y], ...] } }
                const TSharedPtr<FJsonObject>* AreaObject;
                if ((*ParamsObject)->TryGetObjectField(TEXT("area"), AreaObject))
                {
                    const TArray<TSharedPtr<FJsonValue>>* CoordsArray;
                    if ((*AreaObject)->TryGetArrayField(TEXT("coords"), CoordsArray))
                    {
                        for (const auto& CoordValue : *CoordsArray)
                        {
                            const TArray<TSharedPtr<FJsonValue>>* PointArray;
                            if (CoordValue->TryGetArray(PointArray) && PointArray->Num() >= 2)
                            {
                                double PX = (*PointArray)[0]->AsNumber();
                                double PY = (*PointArray)[1]->AsNumber();
                                Cmd.Params.SearchArea.Add(FVector2D(PX, PY));
                            }
                        }
                    }
                }
                
                // 解析 search_area (搜索区域多边形) - 格式: { "search_area": [[x,y], ...] }
                // 这是简化格式，直接是坐标数组
                if (Cmd.Params.SearchArea.Num() == 0)
                {
                    const TArray<TSharedPtr<FJsonValue>>* SearchAreaArray;
                    if ((*ParamsObject)->TryGetArrayField(TEXT("search_area"), SearchAreaArray))
                    {
                        for (const auto& CoordValue : *SearchAreaArray)
                        {
                            const TArray<TSharedPtr<FJsonValue>>* PointArray;
                            if (CoordValue->TryGetArray(PointArray) && PointArray->Num() >= 2)
                            {
                                double PX = (*PointArray)[0]->AsNumber();
                                double PY = (*PointArray)[1]->AsNumber();
                                Cmd.Params.SearchArea.Add(FVector2D(PX, PY));
                            }
                        }
                        UE_LOG(LogMACommTypes, Log, TEXT("  Parsed search_area with %d vertices"), Cmd.Params.SearchArea.Num());
                    }
                }
                
                // 解析 target.features
                const TSharedPtr<FJsonObject>* TargetObject;
                if ((*ParamsObject)->TryGetObjectField(TEXT("target"), TargetObject))
                {
                    // 保存完整的 target JSON 字符串 (用于 Search 技能)
                    // Requirements: 11.5
                    FString TargetJsonStr;
                    TSharedRef<TJsonWriter<>> TargetWriter = TJsonWriterFactory<>::Create(&TargetJsonStr);
                    FJsonSerializer::Serialize(TargetObject->ToSharedRef(), TargetWriter);
                    Cmd.Params.TargetJson = TargetJsonStr;
                    
                    UE_LOG(LogMACommTypes, Verbose, TEXT("  Parsed target: %s"), *TargetJsonStr);
                    
                    const TSharedPtr<FJsonObject>* FeaturesObject;
                    if ((*TargetObject)->TryGetObjectField(TEXT("features"), FeaturesObject))
                    {
                        for (const auto& FeaturePair : (*FeaturesObject)->Values)
                        {
                            FString FeatureValue;
                            if (FeaturePair.Value->TryGetString(FeatureValue))
                            {
                                Cmd.Params.TargetFeatures.Add(FeaturePair.Key, FeatureValue);
                            }
                        }
                    }
                }
                
                //=============================================================
                // 解析 Place 技能参数: object1 和 object2 (Requirements: 10.1, 10.2)
                // 格式: { "object1": { "class": "...", "type": "...", "features": {...} },
                //         "object2": { "class": "...", "type": "...", "features": {...} } }
                //=============================================================
                const TSharedPtr<FJsonObject>* Object1JsonObj;
                if ((*ParamsObject)->TryGetObjectField(TEXT("object1"), Object1JsonObj))
                {
                    FString Object1JsonStr;
                    TSharedRef<TJsonWriter<>> Object1Writer = TJsonWriterFactory<>::Create(&Object1JsonStr);
                    FJsonSerializer::Serialize(Object1JsonObj->ToSharedRef(), Object1Writer);
                    Cmd.Params.Object1Json = Object1JsonStr;
                    
                    UE_LOG(LogMACommTypes, Verbose, TEXT("  Parsed object1: %s"), *Object1JsonStr);
                }
                
                const TSharedPtr<FJsonObject>* Object2JsonObj;
                if ((*ParamsObject)->TryGetObjectField(TEXT("object2"), Object2JsonObj))
                {
                    FString Object2JsonStr;
                    TSharedRef<TJsonWriter<>> Object2Writer = TJsonWriterFactory<>::Create(&Object2JsonStr);
                    FJsonSerializer::Serialize(Object2JsonObj->ToSharedRef(), Object2Writer);
                    Cmd.Params.Object2Json = Object2JsonStr;
                    
                    UE_LOG(LogMACommTypes, Verbose, TEXT("  Parsed object2: %s"), *Object2JsonStr);
                }
            }
            
            TimeStepCmd.Commands.Add(Cmd);
        }
        
        Out.TimeSteps.Add(TimeStepCmd);
    }
    
    Out.TotalTimeSteps = Out.TimeSteps.Num();
    
    UE_LOG(LogMACommTypes, Log, TEXT("FMASkillListMessage::FromJson - Parsed %d time steps"), Out.TotalTimeSteps);
    
    return Out.TotalTimeSteps > 0;
}

FString FMASkillListMessage::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    for (const FMATimeStepCommands& TimeStep : TimeSteps)
    {
        TSharedPtr<FJsonObject> StepObject = MakeShareable(new FJsonObject());
        
        for (const FMAAgentSkillCommand& Cmd : TimeStep.Commands)
        {
            TSharedPtr<FJsonObject> CmdObject = MakeShareable(new FJsonObject());
            CmdObject->SetStringField(TEXT("skill"), Cmd.SkillName);
            
            TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject());
            if (!Cmd.Params.GoalType.IsEmpty())
            {
                ParamsObject->SetStringField(TEXT("goal_type"), Cmd.Params.GoalType);
            }
            if (!Cmd.Params.TaskId.IsEmpty())
            {
                ParamsObject->SetStringField(TEXT("task_id"), Cmd.Params.TaskId);
            }
            if (Cmd.Params.bHasDestPosition)
            {
                TSharedPtr<FJsonObject> DestObject = MakeShareable(new FJsonObject());
                DestObject->SetNumberField(TEXT("x"), Cmd.Params.DestPosition.X);
                DestObject->SetNumberField(TEXT("y"), Cmd.Params.DestPosition.Y);
                DestObject->SetNumberField(TEXT("z"), Cmd.Params.DestPosition.Z);
                ParamsObject->SetObjectField(TEXT("dest"), DestObject);
            }
            
            CmdObject->SetObjectField(TEXT("params"), ParamsObject);
            StepObject->SetObjectField(Cmd.AgentId, CmdObject);
        }
        
        JsonObject->SetObjectField(FString::FromInt(TimeStep.TimeStep), StepObject);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

const FMATimeStepCommands* FMASkillListMessage::GetTimeStep(int32 Step) const
{
    for (const FMATimeStepCommands& TimeStep : TimeSteps)
    {
        if (TimeStep.TimeStep == Step)
        {
            return &TimeStep;
        }
    }
    return nullptr;
}


//=============================================================================
// FMASkillFeedback_Comm 实现
//=============================================================================

TSharedPtr<FJsonObject> FMASkillFeedback_Comm::ToJsonObject() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("agent_id"), AgentId);
    JsonObject->SetStringField(TEXT("skill"), SkillName);
    JsonObject->SetBoolField(TEXT("success"), bSuccess);
    JsonObject->SetStringField(TEXT("message"), Message);
    
    if (Data.Num() > 0)
    {
        TSharedPtr<FJsonObject> DataObject = MakeShareable(new FJsonObject());
        for (const auto& Pair : Data)
        {
            DataObject->SetStringField(Pair.Key, Pair.Value);
        }
        JsonObject->SetObjectField(TEXT("data"), DataObject);
    }
    
    return JsonObject;
}

//=============================================================================
// FMATimeStepFeedbackMessage 实现
//=============================================================================

FString FMATimeStepFeedbackMessage::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetNumberField(TEXT("time_step"), TimeStep);
    
    TArray<TSharedPtr<FJsonValue>> FeedbacksArray;
    for (const FMASkillFeedback_Comm& Feedback : Feedbacks)
    {
        FeedbacksArray.Add(MakeShareable(new FJsonValueObject(Feedback.ToJsonObject())));
    }
    JsonObject->SetArrayField(TEXT("feedbacks"), FeedbacksArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

//=============================================================================
// FMASkillListCompletedMessage 实现
//=============================================================================

FString FMASkillListCompletedMessage::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetBoolField(TEXT("completed"), bCompleted);
    JsonObject->SetBoolField(TEXT("interrupted"), bInterrupted);
    JsonObject->SetNumberField(TEXT("completed_time_steps"), CompletedTimeSteps);
    JsonObject->SetNumberField(TEXT("total_time_steps"), TotalTimeSteps);
    JsonObject->SetStringField(TEXT("message"), Message);
    
    // 添加所有时间步的反馈汇总
    TArray<TSharedPtr<FJsonValue>> TimeStepFeedbacksArray;
    for (const FMATimeStepFeedbackMessage& TSFeedback : AllTimeStepFeedbacks)
    {
        TSharedPtr<FJsonObject> TSObject = MakeShareable(new FJsonObject());
        TSObject->SetNumberField(TEXT("time_step"), TSFeedback.TimeStep);
        
        TArray<TSharedPtr<FJsonValue>> FeedbacksArray;
        for (const FMASkillFeedback_Comm& Feedback : TSFeedback.Feedbacks)
        {
            FeedbacksArray.Add(MakeShareable(new FJsonValueObject(Feedback.ToJsonObject())));
        }
        TSObject->SetArrayField(TEXT("feedbacks"), FeedbacksArray);
        
        TimeStepFeedbacksArray.Add(MakeShareable(new FJsonValueObject(TSObject)));
    }
    JsonObject->SetArrayField(TEXT("all_feedbacks"), TimeStepFeedbacksArray);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}


//=============================================================================
// FMASceneChangeMessage 实现
// Requirements: 11.1, 11.2, 11.3, 11.4, 11.5, 11.6, 11.7, 11.8
//=============================================================================

FString FMASceneChangeMessage::ChangeTypeToString(EMASceneChangeType Type)
{
    switch (Type)
    {
    case EMASceneChangeType::AddNode:       return TEXT("add_node");
    case EMASceneChangeType::DeleteNode:    return TEXT("delete_node");
    case EMASceneChangeType::EditNode:      return TEXT("edit_node");
    case EMASceneChangeType::AddGoal:       return TEXT("add_goal");
    case EMASceneChangeType::DeleteGoal:    return TEXT("delete_goal");
    case EMASceneChangeType::AddZone:       return TEXT("add_zone");
    case EMASceneChangeType::DeleteZone:    return TEXT("delete_zone");
    case EMASceneChangeType::AddEdge:       return TEXT("add_edge");
    case EMASceneChangeType::EditEdge:      return TEXT("edit_edge");
    default:                                return TEXT("unknown");
    }
}

EMASceneChangeType FMASceneChangeMessage::StringToChangeType(const FString& TypeStr)
{
    if (TypeStr == TEXT("add_node"))        return EMASceneChangeType::AddNode;
    if (TypeStr == TEXT("delete_node"))     return EMASceneChangeType::DeleteNode;
    if (TypeStr == TEXT("edit_node"))       return EMASceneChangeType::EditNode;
    if (TypeStr == TEXT("add_goal"))        return EMASceneChangeType::AddGoal;
    if (TypeStr == TEXT("delete_goal"))     return EMASceneChangeType::DeleteGoal;
    if (TypeStr == TEXT("add_zone"))        return EMASceneChangeType::AddZone;
    if (TypeStr == TEXT("delete_zone"))     return EMASceneChangeType::DeleteZone;
    if (TypeStr == TEXT("add_edge"))        return EMASceneChangeType::AddEdge;
    if (TypeStr == TEXT("edit_edge"))       return EMASceneChangeType::EditEdge;
    return EMASceneChangeType::AddNode; // 默认值
}

FString FMASceneChangeMessage::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    
    JsonObject->SetStringField(TEXT("change_type"), ChangeTypeToString(ChangeType));
    JsonObject->SetNumberField(TEXT("timestamp"), static_cast<double>(Timestamp));
    JsonObject->SetStringField(TEXT("message_id"), MessageId);
    
    // 尝试将 Payload 解析为 JSON 对象并嵌入
    TSharedPtr<FJsonObject> PayloadObject;
    TSharedRef<TJsonReader<>> PayloadReader = TJsonReaderFactory<>::Create(Payload);
    if (FJsonSerializer::Deserialize(PayloadReader, PayloadObject) && PayloadObject.IsValid())
    {
        JsonObject->SetObjectField(TEXT("payload"), PayloadObject);
    }
    else
    {
        // 如果 Payload 不是有效 JSON，作为字符串存储
        JsonObject->SetStringField(TEXT("payload"), Payload);
    }
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return OutputString;
}

bool FMASceneChangeMessage::FromJson(const FString& Json, FMASceneChangeMessage& Out)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("FMASceneChangeMessage::FromJson - Failed to parse JSON"));
        return false;
    }
    
    // 解析 change_type
    FString TypeStr;
    if (JsonObject->TryGetStringField(TEXT("change_type"), TypeStr))
    {
        Out.ChangeType = StringToChangeType(TypeStr);
    }
    
    // 解析 timestamp
    double TimestampDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("timestamp"), TimestampDouble))
    {
        Out.Timestamp = static_cast<int64>(TimestampDouble);
    }
    
    // 解析 message_id
    JsonObject->TryGetStringField(TEXT("message_id"), Out.MessageId);
    
    // 解析 payload - 将其序列化回 JSON 字符串
    const TSharedPtr<FJsonObject>* PayloadObject;
    if (JsonObject->TryGetObjectField(TEXT("payload"), PayloadObject))
    {
        FString PayloadString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
        FJsonSerializer::Serialize(PayloadObject->ToSharedRef(), Writer);
        Out.Payload = PayloadString;
    }
    else
    {
        // 尝试作为字符串读取
        JsonObject->TryGetStringField(TEXT("payload"), Out.Payload);
    }
    
    return true;
}
