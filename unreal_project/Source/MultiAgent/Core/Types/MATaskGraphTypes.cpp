// MATaskGraphTypes.cpp
// 任务图数据类型实现

#include "MATaskGraphTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY(LogMATaskGraph);

//=============================================================================
// FMARequiredSkill 实现
//=============================================================================

FString FMARequiredSkill::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("skill_name"), SkillName);
    
    TArray<TSharedPtr<FJsonValue>> RobotTypeArray;
    for (const FString& Type : AssignedRobotType)
    {
        RobotTypeArray.Add(MakeShareable(new FJsonValueString(Type)));
    }
    JsonObject->SetArrayField(TEXT("assigned_robot_type"), RobotTypeArray);
    JsonObject->SetNumberField(TEXT("assigned_robot_count"), AssignedRobotCount);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool FMARequiredSkill::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMARequiredSkill& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    Out.SkillName = JsonObject->GetStringField(TEXT("skill_name"));
    Out.AssignedRobotCount = JsonObject->GetIntegerField(TEXT("assigned_robot_count"));

    const TArray<TSharedPtr<FJsonValue>>* RobotTypeArray;
    if (JsonObject->TryGetArrayField(TEXT("assigned_robot_type"), RobotTypeArray))
    {
        Out.AssignedRobotType.Empty();
        for (const TSharedPtr<FJsonValue>& Value : *RobotTypeArray)
        {
            Out.AssignedRobotType.Add(Value->AsString());
        }
    }

    return true;
}

bool FMARequiredSkill::operator==(const FMARequiredSkill& Other) const
{
    return SkillName == Other.SkillName &&
           AssignedRobotType == Other.AssignedRobotType &&
           AssignedRobotCount == Other.AssignedRobotCount;
}

//=============================================================================
// FMATaskNodeData 实现
//=============================================================================

FString FMATaskNodeData::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("task_id"), TaskId);
    JsonObject->SetStringField(TEXT("description"), Description);
    JsonObject->SetStringField(TEXT("location"), Location);

    // Required Skills
    TArray<TSharedPtr<FJsonValue>> SkillsArray;
    for (const FMARequiredSkill& Skill : RequiredSkills)
    {
        TSharedPtr<FJsonObject> SkillObj = MakeShareable(new FJsonObject());
        SkillObj->SetStringField(TEXT("skill_name"), Skill.SkillName);
        
        TArray<TSharedPtr<FJsonValue>> RobotTypeArray;
        for (const FString& Type : Skill.AssignedRobotType)
        {
            RobotTypeArray.Add(MakeShareable(new FJsonValueString(Type)));
        }
        SkillObj->SetArrayField(TEXT("assigned_robot_type"), RobotTypeArray);
        SkillObj->SetNumberField(TEXT("assigned_robot_count"), Skill.AssignedRobotCount);
        
        SkillsArray.Add(MakeShareable(new FJsonValueObject(SkillObj)));
    }
    JsonObject->SetArrayField(TEXT("required_skills"), SkillsArray);

    // Produces
    TArray<TSharedPtr<FJsonValue>> ProducesArray;
    for (const FString& Produce : Produces)
    {
        ProducesArray.Add(MakeShareable(new FJsonValueString(Produce)));
    }
    JsonObject->SetArrayField(TEXT("produces"), ProducesArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool FMATaskNodeData::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATaskNodeData& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    Out.TaskId = JsonObject->GetStringField(TEXT("task_id"));
    Out.Description = JsonObject->GetStringField(TEXT("description"));
    Out.Location = JsonObject->GetStringField(TEXT("location"));

    // Required Skills
    const TArray<TSharedPtr<FJsonValue>>* SkillsArray;
    if (JsonObject->TryGetArrayField(TEXT("required_skills"), SkillsArray))
    {
        Out.RequiredSkills.Empty();
        for (const TSharedPtr<FJsonValue>& Value : *SkillsArray)
        {
            FMARequiredSkill Skill;
            if (FMARequiredSkill::FromJson(Value->AsObject(), Skill))
            {
                Out.RequiredSkills.Add(Skill);
            }
        }
    }

    // Produces
    const TArray<TSharedPtr<FJsonValue>>* ProducesArray;
    if (JsonObject->TryGetArrayField(TEXT("produces"), ProducesArray))
    {
        Out.Produces.Empty();
        for (const TSharedPtr<FJsonValue>& Value : *ProducesArray)
        {
            Out.Produces.Add(Value->AsString());
        }
    }

    return true;
}

bool FMATaskNodeData::operator==(const FMATaskNodeData& Other) const
{
    return TaskId == Other.TaskId &&
           Description == Other.Description &&
           Location == Other.Location &&
           RequiredSkills == Other.RequiredSkills &&
           Produces == Other.Produces;
    // Note: CanvasPosition is intentionally excluded from comparison
}

//=============================================================================
// FMATaskEdgeData 实现
//=============================================================================

FString FMATaskEdgeData::ToJson() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("from"), FromNodeId);
    JsonObject->SetStringField(TEXT("to"), ToNodeId);
    JsonObject->SetStringField(TEXT("type"), EdgeType);
    
    if (!Condition.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("condition"), Condition);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool FMATaskEdgeData::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATaskEdgeData& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    Out.FromNodeId = JsonObject->GetStringField(TEXT("from"));
    Out.ToNodeId = JsonObject->GetStringField(TEXT("to"));
    Out.EdgeType = JsonObject->GetStringField(TEXT("type"));
    
    if (JsonObject->HasField(TEXT("condition")))
    {
        Out.Condition = JsonObject->GetStringField(TEXT("condition"));
    }

    return true;
}

bool FMATaskEdgeData::operator==(const FMATaskEdgeData& Other) const
{
    return FromNodeId == Other.FromNodeId &&
           ToNodeId == Other.ToNodeId &&
           EdgeType == Other.EdgeType &&
           Condition == Other.Condition;
}


//=============================================================================
// FMATaskGraphData 实现
//=============================================================================

FString FMATaskGraphData::ToJson() const
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    
    // Meta section
    TSharedPtr<FJsonObject> MetaObject = MakeShareable(new FJsonObject());
    MetaObject->SetStringField(TEXT("description"), Description);
    RootObject->SetObjectField(TEXT("meta"), MetaObject);

    // Task graph section
    TSharedPtr<FJsonObject> TaskGraphObject = MakeShareable(new FJsonObject());

    // Nodes
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FMATaskNodeData& Node : Nodes)
    {
        TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject());
        NodeObj->SetStringField(TEXT("task_id"), Node.TaskId);
        NodeObj->SetStringField(TEXT("description"), Node.Description);
        NodeObj->SetStringField(TEXT("location"), Node.Location);

        // Required Skills
        TArray<TSharedPtr<FJsonValue>> SkillsArray;
        for (const FMARequiredSkill& Skill : Node.RequiredSkills)
        {
            TSharedPtr<FJsonObject> SkillObj = MakeShareable(new FJsonObject());
            SkillObj->SetStringField(TEXT("skill_name"), Skill.SkillName);
            
            TArray<TSharedPtr<FJsonValue>> RobotTypeArray;
            for (const FString& Type : Skill.AssignedRobotType)
            {
                RobotTypeArray.Add(MakeShareable(new FJsonValueString(Type)));
            }
            SkillObj->SetArrayField(TEXT("assigned_robot_type"), RobotTypeArray);
            SkillObj->SetNumberField(TEXT("assigned_robot_count"), Skill.AssignedRobotCount);
            
            SkillsArray.Add(MakeShareable(new FJsonValueObject(SkillObj)));
        }
        NodeObj->SetArrayField(TEXT("required_skills"), SkillsArray);

        // Produces
        TArray<TSharedPtr<FJsonValue>> ProducesArray;
        for (const FString& Produce : Node.Produces)
        {
            ProducesArray.Add(MakeShareable(new FJsonValueString(Produce)));
        }
        NodeObj->SetArrayField(TEXT("produces"), ProducesArray);

        NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
    }
    TaskGraphObject->SetArrayField(TEXT("nodes"), NodesArray);

    // Edges
    TArray<TSharedPtr<FJsonValue>> EdgesArray;
    for (const FMATaskEdgeData& Edge : Edges)
    {
        TSharedPtr<FJsonObject> EdgeObj = MakeShareable(new FJsonObject());
        EdgeObj->SetStringField(TEXT("from"), Edge.FromNodeId);
        EdgeObj->SetStringField(TEXT("to"), Edge.ToNodeId);
        EdgeObj->SetStringField(TEXT("type"), Edge.EdgeType);
        
        if (!Edge.Condition.IsEmpty())
        {
            EdgeObj->SetStringField(TEXT("condition"), Edge.Condition);
        }

        EdgesArray.Add(MakeShareable(new FJsonValueObject(EdgeObj)));
    }
    TaskGraphObject->SetArrayField(TEXT("edges"), EdgesArray);

    RootObject->SetObjectField(TEXT("task_graph"), TaskGraphObject);

    // Serialize with pretty print
    FString OutputString;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = 
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    return OutputString;
}

bool FMATaskGraphData::FromJson(const FString& Json, FMATaskGraphData& OutData)
{
    FString ErrorMessage;
    return FromJsonWithError(Json, OutData, ErrorMessage);
}

bool FMATaskGraphData::FromJsonWithError(const FString& Json, FMATaskGraphData& OutData, FString& OutError)
{
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON");
        return false;
    }

    // Try response format first (with meta and task_graph)
    if (RootObject->HasField(TEXT("task_graph")))
    {
        return FromResponseJson(Json, OutData, OutError);
    }

    // Try direct task_graph format (nodes and edges at root level)
    if (RootObject->HasField(TEXT("nodes")))
    {
        OutData.Nodes.Empty();
        OutData.Edges.Empty();

        // Parse nodes
        const TArray<TSharedPtr<FJsonValue>>* NodesArray;
        if (RootObject->TryGetArrayField(TEXT("nodes"), NodesArray))
        {
            for (const TSharedPtr<FJsonValue>& Value : *NodesArray)
            {
                FMATaskNodeData Node;
                if (FMATaskNodeData::FromJson(Value->AsObject(), Node))
                {
                    OutData.Nodes.Add(Node);
                }
            }
        }

        // Parse edges
        const TArray<TSharedPtr<FJsonValue>>* EdgesArray;
        if (RootObject->TryGetArrayField(TEXT("edges"), EdgesArray))
        {
            for (const TSharedPtr<FJsonValue>& Value : *EdgesArray)
            {
                FMATaskEdgeData Edge;
                if (FMATaskEdgeData::FromJson(Value->AsObject(), Edge))
                {
                    OutData.Edges.Add(Edge);
                }
            }
        }

        return true;
    }

    OutError = TEXT("Unknown JSON format: missing 'task_graph' or 'nodes' field");
    return false;
}

bool FMATaskGraphData::FromResponseJson(const FString& Json, FMATaskGraphData& OutData, FString& OutError)
{
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON");
        return false;
    }

    OutData.Nodes.Empty();
    OutData.Edges.Empty();

    // Parse meta section
    const TSharedPtr<FJsonObject>* MetaObject;
    if (RootObject->TryGetObjectField(TEXT("meta"), MetaObject))
    {
        OutData.Description = (*MetaObject)->GetStringField(TEXT("description"));
    }

    // Parse task_graph section
    const TSharedPtr<FJsonObject>* TaskGraphObject;
    if (!RootObject->TryGetObjectField(TEXT("task_graph"), TaskGraphObject))
    {
        OutError = TEXT("Missing 'task_graph' field");
        return false;
    }

    // Parse nodes
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if ((*TaskGraphObject)->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *NodesArray)
        {
            FMATaskNodeData Node;
            if (FMATaskNodeData::FromJson(Value->AsObject(), Node))
            {
                OutData.Nodes.Add(Node);
            }
        }
    }

    // Parse edges
    const TArray<TSharedPtr<FJsonValue>>* EdgesArray;
    if ((*TaskGraphObject)->TryGetArrayField(TEXT("edges"), EdgesArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *EdgesArray)
        {
            FMATaskEdgeData Edge;
            if (FMATaskEdgeData::FromJson(Value->AsObject(), Edge))
            {
                OutData.Edges.Add(Edge);
            }
        }
    }

    UE_LOG(LogMATaskGraph, Log, TEXT("Parsed task graph: %d nodes, %d edges"), 
           OutData.Nodes.Num(), OutData.Edges.Num());

    return true;
}

FMATaskNodeData* FMATaskGraphData::FindNode(const FString& TaskId)
{
    for (FMATaskNodeData& Node : Nodes)
    {
        if (Node.TaskId == TaskId)
        {
            return &Node;
        }
    }
    return nullptr;
}

const FMATaskNodeData* FMATaskGraphData::FindNode(const FString& TaskId) const
{
    for (const FMATaskNodeData& Node : Nodes)
    {
        if (Node.TaskId == TaskId)
        {
            return &Node;
        }
    }
    return nullptr;
}

bool FMATaskGraphData::HasNode(const FString& TaskId) const
{
    return FindNode(TaskId) != nullptr;
}

bool FMATaskGraphData::HasEdge(const FString& FromId, const FString& ToId) const
{
    for (const FMATaskEdgeData& Edge : Edges)
    {
        if (Edge.FromNodeId == FromId && Edge.ToNodeId == ToId)
        {
            return true;
        }
    }
    return false;
}

bool FMATaskGraphData::IsValidDAG() const
{
    // Build adjacency list
    TMap<FString, TArray<FString>> AdjList;
    TMap<FString, int32> InDegree;

    for (const FMATaskNodeData& Node : Nodes)
    {
        AdjList.Add(Node.TaskId, TArray<FString>());
        InDegree.Add(Node.TaskId, 0);
    }

    for (const FMATaskEdgeData& Edge : Edges)
    {
        if (!AdjList.Contains(Edge.FromNodeId) || !AdjList.Contains(Edge.ToNodeId))
        {
            // Edge references non-existent node
            return false;
        }
        AdjList[Edge.FromNodeId].Add(Edge.ToNodeId);
        InDegree[Edge.ToNodeId]++;
    }

    // Kahn's algorithm for topological sort
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

        for (const FString& Neighbor : AdjList[Current])
        {
            InDegree[Neighbor]--;
            if (InDegree[Neighbor] == 0)
            {
                Queue.Add(Neighbor);
            }
        }
    }

    // If we processed all nodes, there's no cycle
    return ProcessedCount == Nodes.Num();
}

bool FMATaskGraphData::operator==(const FMATaskGraphData& Other) const
{
    if (Description != Other.Description)
    {
        return false;
    }
    if (Nodes.Num() != Other.Nodes.Num() || Edges.Num() != Other.Edges.Num())
    {
        return false;
    }
    
    // Compare nodes (order matters for this simple comparison)
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        if (!(Nodes[i] == Other.Nodes[i]))
        {
            return false;
        }
    }
    
    // Compare edges
    for (int32 i = 0; i < Edges.Num(); ++i)
    {
        if (!(Edges[i] == Other.Edges[i]))
        {
            return false;
        }
    }
    
    return true;
}
