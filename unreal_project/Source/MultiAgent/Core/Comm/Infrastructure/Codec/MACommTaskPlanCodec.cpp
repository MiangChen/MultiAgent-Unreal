// MACommTaskPlanCodec.cpp
// L3 Infrastructure: TaskPlan/WorldModel JSON codec

#include "MACommJsonCodec.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    TSharedPtr<FJsonObject> SerializeTaskPlanNode(const FMATaskPlanNode& Node)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
        JsonObject->SetStringField(TEXT("node_id"), Node.NodeId);
        JsonObject->SetStringField(TEXT("task_type"), Node.TaskType);

        TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject());
        for (const auto& Pair : Node.Parameters)
        {
            ParamsObject->SetStringField(Pair.Key, Pair.Value);
        }
        JsonObject->SetObjectField(TEXT("parameters"), ParamsObject);

        TArray<TSharedPtr<FJsonValue>> DepsArray;
        for (const FString& Dep : Node.Dependencies)
        {
            DepsArray.Add(MakeShareable(new FJsonValueString(Dep)));
        }
        JsonObject->SetArrayField(TEXT("dependencies"), DepsArray);
        return JsonObject;
    }

    bool DeserializeTaskPlanNode(const TSharedPtr<FJsonObject>& JsonObject, FMATaskPlanNode& Out)
    {
        if (!JsonObject.IsValid())
        {
            return false;
        }

        JsonObject->TryGetStringField(TEXT("node_id"), Out.NodeId);
        JsonObject->TryGetStringField(TEXT("task_type"), Out.TaskType);

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

    TSharedPtr<FJsonObject> SerializeTaskPlanEdge(const FMATaskPlanEdge& Edge)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
        JsonObject->SetStringField(TEXT("from_node_id"), Edge.FromNodeId);
        JsonObject->SetStringField(TEXT("to_node_id"), Edge.ToNodeId);
        return JsonObject;
    }

    bool DeserializeTaskPlanEdge(const TSharedPtr<FJsonObject>& JsonObject, FMATaskPlanEdge& Out)
    {
        if (!JsonObject.IsValid())
        {
            return false;
        }

        JsonObject->TryGetStringField(TEXT("from_node_id"), Out.FromNodeId);
        JsonObject->TryGetStringField(TEXT("to_node_id"), Out.ToNodeId);
        return true;
    }

    TSharedPtr<FJsonObject> SerializeWorldModelEntity(const FMAWorldModelEntity& Entity)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
        JsonObject->SetStringField(TEXT("entity_id"), Entity.EntityId);
        JsonObject->SetStringField(TEXT("entity_type"), Entity.EntityType);

        TSharedPtr<FJsonObject> PropsObject = MakeShareable(new FJsonObject());
        for (const auto& Pair : Entity.Properties)
        {
            PropsObject->SetStringField(Pair.Key, Pair.Value);
        }
        JsonObject->SetObjectField(TEXT("properties"), PropsObject);
        return JsonObject;
    }

    bool DeserializeWorldModelEntity(const TSharedPtr<FJsonObject>& JsonObject, FMAWorldModelEntity& Out)
    {
        if (!JsonObject.IsValid())
        {
            return false;
        }

        JsonObject->TryGetStringField(TEXT("entity_id"), Out.EntityId);
        JsonObject->TryGetStringField(TEXT("entity_type"), Out.EntityType);

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

    TSharedPtr<FJsonObject> SerializeWorldModelRelationship(const FMAWorldModelRelationship& Relationship)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
        JsonObject->SetStringField(TEXT("entity_a_id"), Relationship.EntityAId);
        JsonObject->SetStringField(TEXT("entity_b_id"), Relationship.EntityBId);
        JsonObject->SetStringField(TEXT("relationship_type"), Relationship.RelationshipType);
        return JsonObject;
    }

    bool DeserializeWorldModelRelationship(const TSharedPtr<FJsonObject>& JsonObject, FMAWorldModelRelationship& Out)
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
}

FString MACommJsonCodec::SerializeTaskPlan(const FMATaskPlan& TaskPlan)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FMATaskPlanNode& Node : TaskPlan.Nodes)
    {
        NodesArray.Add(MakeShareable(new FJsonValueObject(SerializeTaskPlanNode(Node))));
    }
    JsonObject->SetArrayField(TEXT("nodes"), NodesArray);

    TArray<TSharedPtr<FJsonValue>> EdgesArray;
    for (const FMATaskPlanEdge& Edge : TaskPlan.Edges)
    {
        EdgesArray.Add(MakeShareable(new FJsonValueObject(SerializeTaskPlanEdge(Edge))));
    }
    JsonObject->SetArrayField(TEXT("edges"), EdgesArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeTaskPlan(const FString& Json, FMATaskPlan& OutTaskPlan)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeTaskPlan - Failed to parse JSON"));
        return false;
    }

    OutTaskPlan.Nodes.Empty();
    OutTaskPlan.Edges.Empty();

    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (JsonObject->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        for (const auto& NodeValue : *NodesArray)
        {
            const TSharedPtr<FJsonObject>* NodeObject;
            if (NodeValue->TryGetObject(NodeObject))
            {
                FMATaskPlanNode Node;
                if (DeserializeTaskPlanNode(*NodeObject, Node))
                {
                    OutTaskPlan.Nodes.Add(Node);
                }
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* EdgesArray;
    if (JsonObject->TryGetArrayField(TEXT("edges"), EdgesArray))
    {
        for (const auto& EdgeValue : *EdgesArray)
        {
            const TSharedPtr<FJsonObject>* EdgeObject;
            if (EdgeValue->TryGetObject(EdgeObject))
            {
                FMATaskPlanEdge Edge;
                if (DeserializeTaskPlanEdge(*EdgeObject, Edge))
                {
                    OutTaskPlan.Edges.Add(Edge);
                }
            }
        }
    }

    return true;
}

bool MACommJsonCodec::ValidateTaskPlanDAG(const FMATaskPlan& TaskPlan)
{
    if (TaskPlan.Nodes.Num() == 0)
    {
        return true;
    }

    TMap<FString, TArray<FString>> AdjList;
    TMap<FString, int32> InDegree;

    for (const FMATaskPlanNode& Node : TaskPlan.Nodes)
    {
        AdjList.FindOrAdd(Node.NodeId);
        InDegree.FindOrAdd(Node.NodeId) = 0;
    }

    for (const FMATaskPlanEdge& Edge : TaskPlan.Edges)
    {
        AdjList.FindOrAdd(Edge.FromNodeId).Add(Edge.ToNodeId);
        InDegree.FindOrAdd(Edge.ToNodeId)++;
    }

    for (const FMATaskPlanNode& Node : TaskPlan.Nodes)
    {
        for (const FString& Dep : Node.Dependencies)
        {
            if (!AdjList.Contains(Dep))
            {
                continue;
            }
            if (!AdjList[Dep].Contains(Node.NodeId))
            {
                AdjList[Dep].Add(Node.NodeId);
                InDegree[Node.NodeId]++;
            }
        }
    }

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
        const FString Current = Queue[0];
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

    return ProcessedCount == TaskPlan.Nodes.Num();
}

FString MACommJsonCodec::SerializeWorldModelGraph(const FMAWorldModelGraph& Graph)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

    TArray<TSharedPtr<FJsonValue>> EntitiesArray;
    for (const FMAWorldModelEntity& Entity : Graph.Entities)
    {
        EntitiesArray.Add(MakeShareable(new FJsonValueObject(SerializeWorldModelEntity(Entity))));
    }
    JsonObject->SetArrayField(TEXT("entities"), EntitiesArray);

    TArray<TSharedPtr<FJsonValue>> RelationshipsArray;
    for (const FMAWorldModelRelationship& Rel : Graph.Relationships)
    {
        RelationshipsArray.Add(MakeShareable(new FJsonValueObject(SerializeWorldModelRelationship(Rel))));
    }
    JsonObject->SetArrayField(TEXT("relationships"), RelationshipsArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeWorldModelGraph(const FString& Json, FMAWorldModelGraph& OutGraph)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeWorldModelGraph - Failed to parse JSON"));
        return false;
    }

    OutGraph.Entities.Empty();
    OutGraph.Relationships.Empty();

    const TArray<TSharedPtr<FJsonValue>>* EntitiesArray;
    if (JsonObject->TryGetArrayField(TEXT("entities"), EntitiesArray))
    {
        for (const auto& EntityValue : *EntitiesArray)
        {
            const TSharedPtr<FJsonObject>* EntityObject;
            if (EntityValue->TryGetObject(EntityObject))
            {
                FMAWorldModelEntity Entity;
                if (DeserializeWorldModelEntity(*EntityObject, Entity))
                {
                    OutGraph.Entities.Add(Entity);
                }
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* RelationshipsArray;
    if (JsonObject->TryGetArrayField(TEXT("relationships"), RelationshipsArray))
    {
        for (const auto& RelValue : *RelationshipsArray)
        {
            const TSharedPtr<FJsonObject>* RelObject;
            if (RelValue->TryGetObject(RelObject))
            {
                FMAWorldModelRelationship Rel;
                if (DeserializeWorldModelRelationship(*RelObject, Rel))
                {
                    OutGraph.Relationships.Add(Rel);
                }
            }
        }
    }

    return true;
}
