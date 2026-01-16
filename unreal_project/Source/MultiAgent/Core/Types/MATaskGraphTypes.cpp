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

//=============================================================================
// Skill Allocation Types Implementation
//=============================================================================

//-----------------------------------------------------------------------------
// FMASkillAssignment
//-----------------------------------------------------------------------------

bool FMASkillAssignment::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMASkillAssignment& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    // Parse skill name
    if (!JsonObject->TryGetStringField(TEXT("skill"), Out.SkillName))
    {
        return false;
    }

    // Parse params (can be any JSON object)
    const TSharedPtr<FJsonObject>* ParamsObject;
    if (JsonObject->TryGetObjectField(TEXT("params"), ParamsObject))
    {
        // Serialize params object to JSON string
        FString ParamsString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsString);
        FJsonSerializer::Serialize(ParamsObject->ToSharedRef(), Writer);
        Out.ParamsJson = ParamsString;
    }
    else
    {
        Out.ParamsJson = TEXT("{}");
    }

    // Status is runtime state, not parsed from JSON
    Out.Status = ESkillExecutionStatus::Pending;

    return true;
}

bool FMASkillAssignment::FromJsonWithError(const TSharedPtr<FJsonObject>& JsonObject, FMASkillAssignment& Out, FString& OutError, const FString& Context)
{
    if (!JsonObject.IsValid())
    {
        OutError = FString::Printf(TEXT("%s: Invalid JSON object"), *Context);
        return false;
    }

    // Parse skill name (required field)
    if (!JsonObject->TryGetStringField(TEXT("skill"), Out.SkillName))
    {
        OutError = FString::Printf(TEXT("%s: Missing required field 'skill'"), *Context);
        return false;
    }

    if (Out.SkillName.IsEmpty())
    {
        OutError = FString::Printf(TEXT("%s: Field 'skill' cannot be empty"), *Context);
        return false;
    }

    // Parse params (can be any JSON object)
    const TSharedPtr<FJsonObject>* ParamsObject;
    if (JsonObject->TryGetObjectField(TEXT("params"), ParamsObject))
    {
        // Serialize params object to JSON string
        FString ParamsString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsString);
        FJsonSerializer::Serialize(ParamsObject->ToSharedRef(), Writer);
        Out.ParamsJson = ParamsString;
    }
    else
    {
        Out.ParamsJson = TEXT("{}");
    }

    // Status is runtime state, not parsed from JSON
    Out.Status = ESkillExecutionStatus::Pending;

    return true;
}

TSharedPtr<FJsonObject> FMASkillAssignment::ToJsonObject() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("skill"), SkillName);

    // Parse params JSON string back to object
    TSharedPtr<FJsonObject> ParamsObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ParamsJson);
    if (FJsonSerializer::Deserialize(Reader, ParamsObject) && ParamsObject.IsValid())
    {
        JsonObject->SetObjectField(TEXT("params"), ParamsObject);
    }
    else
    {
        // If parsing fails, use empty object
        JsonObject->SetObjectField(TEXT("params"), MakeShareable(new FJsonObject()));
    }

    return JsonObject;
}

TSharedPtr<FJsonObject> FMASkillAssignment::GetParamsAsJson() const
{
    TSharedPtr<FJsonObject> ParamsObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ParamsJson);
    FJsonSerializer::Deserialize(Reader, ParamsObject);
    return ParamsObject;
}

bool FMASkillAssignment::operator==(const FMASkillAssignment& Other) const
{
    return SkillName == Other.SkillName &&
           ParamsJson == Other.ParamsJson &&
           Status == Other.Status;
}

//-----------------------------------------------------------------------------
// FMATimeStepData
//-----------------------------------------------------------------------------

bool FMATimeStepData::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATimeStepData& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    Out.RobotSkills.Empty();

    // Iterate through all fields in the JSON object
    // Each field is a robot ID with a skill assignment
    for (const auto& Pair : JsonObject->Values)
    {
        const FString& RobotId = Pair.Key;
        const TSharedPtr<FJsonValue>& Value = Pair.Value;

        if (Value->Type == EJson::Object)
        {
            FMASkillAssignment Skill;
            if (FMASkillAssignment::FromJson(Value->AsObject(), Skill))
            {
                Out.RobotSkills.Add(RobotId, Skill);
            }
        }
    }

    return true;
}

bool FMATimeStepData::FromJsonWithError(const TSharedPtr<FJsonObject>& JsonObject, FMATimeStepData& Out, FString& OutError, int32 TimeStep)
{
    if (!JsonObject.IsValid())
    {
        OutError = FString::Printf(TEXT("TimeStep %d: Invalid JSON object"), TimeStep);
        return false;
    }

    Out.RobotSkills.Empty();
    TArray<FString> SkillErrors;

    // Iterate through all fields in the JSON object
    // Each field is a robot ID with a skill assignment
    for (const auto& Pair : JsonObject->Values)
    {
        const FString& RobotId = Pair.Key;
        const TSharedPtr<FJsonValue>& Value = Pair.Value;

        if (RobotId.IsEmpty())
        {
            SkillErrors.Add(FString::Printf(TEXT("TimeStep %d: Empty robot ID found"), TimeStep));
            continue;
        }

        if (Value->Type != EJson::Object)
        {
            SkillErrors.Add(FString::Printf(TEXT("TimeStep %d, Robot '%s': Expected object, got %s"), 
                TimeStep, *RobotId, *GetJsonTypeName(Value->Type)));
            continue;
        }

        FMASkillAssignment Skill;
        FString SkillError;
        FString Context = FString::Printf(TEXT("TimeStep %d, Robot '%s'"), TimeStep, *RobotId);
        
        if (FMASkillAssignment::FromJsonWithError(Value->AsObject(), Skill, SkillError, Context))
        {
            Out.RobotSkills.Add(RobotId, Skill);
        }
        else
        {
            SkillErrors.Add(SkillError);
        }
    }

    // If we have errors but also some valid skills, report as warning
    if (SkillErrors.Num() > 0)
    {
        OutError = FString::Join(SkillErrors, TEXT("; "));
        // Return true if we have at least some valid skills (partial success)
        return Out.RobotSkills.Num() > 0;
    }

    return true;
}

FString FMATimeStepData::GetJsonTypeName(EJson Type)
{
    switch (Type)
    {
        case EJson::None: return TEXT("None");
        case EJson::Null: return TEXT("Null");
        case EJson::String: return TEXT("String");
        case EJson::Number: return TEXT("Number");
        case EJson::Boolean: return TEXT("Boolean");
        case EJson::Array: return TEXT("Array");
        case EJson::Object: return TEXT("Object");
        default: return TEXT("Unknown");
    }
}

TSharedPtr<FJsonObject> FMATimeStepData::ToJsonObject() const
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

    for (const auto& Pair : RobotSkills)
    {
        const FString& RobotId = Pair.Key;
        const FMASkillAssignment& Skill = Pair.Value;
        JsonObject->SetObjectField(RobotId, Skill.ToJsonObject());
    }

    return JsonObject;
}

bool FMATimeStepData::operator==(const FMATimeStepData& Other) const
{
    if (RobotSkills.Num() != Other.RobotSkills.Num())
    {
        return false;
    }

    for (const auto& Pair : RobotSkills)
    {
        const FString& RobotId = Pair.Key;
        const FMASkillAssignment* OtherSkill = Other.RobotSkills.Find(RobotId);
        
        if (!OtherSkill || !(Pair.Value == *OtherSkill))
        {
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
// FMASkillAllocationData
//-----------------------------------------------------------------------------

bool FMASkillAllocationData::FromJson(const FString& JsonString, FMASkillAllocationData& OutData, FString& OutError)
{
    // Check for empty input
    if (JsonString.IsEmpty())
    {
        OutError = TEXT("JSON input is empty");
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, RootObject))
    {
        // Try to get more specific error information
        const FString ReaderError = Reader->GetErrorMessage();
        if (!ReaderError.IsEmpty())
        {
            OutError = FString::Printf(TEXT("JSON syntax error: %s"), *ReaderError);
        }
        else
        {
            OutError = TEXT("JSON syntax error: Failed to parse JSON");
        }
        return false;
    }
    
    if (!RootObject.IsValid())
    {
        OutError = TEXT("JSON parse error: Root object is null");
        return false;
    }

    // Collect all missing required fields
    TArray<FString> MissingFields;

    // Parse name
    if (!RootObject->TryGetStringField(TEXT("name"), OutData.Name))
    {
        MissingFields.Add(TEXT("name"));
    }

    // Parse description
    if (!RootObject->TryGetStringField(TEXT("description"), OutData.Description))
    {
        MissingFields.Add(TEXT("description"));
    }

    // Check for data field
    const TSharedPtr<FJsonObject>* DataObject;
    if (!RootObject->TryGetObjectField(TEXT("data"), DataObject))
    {
        MissingFields.Add(TEXT("data"));
    }

    // Report all missing fields at once
    if (MissingFields.Num() > 0)
    {
        OutError = FString::Printf(TEXT("Missing required field(s): %s"), *FString::Join(MissingFields, TEXT(", ")));
        return false;
    }

    OutData.Data.Empty();
    TArray<FString> Warnings;
    TArray<FString> Errors;
    TArray<int32> ParsedTimeSteps;

    // Iterate through time steps (keys are string representations of integers)
    for (const auto& Pair : (*DataObject)->Values)
    {
        const FString& TimeStepStr = Pair.Key;
        const TSharedPtr<FJsonValue>& Value = Pair.Value;

        // Validate time step key is a valid integer
        if (!TimeStepStr.IsNumeric())
        {
            Errors.Add(FString::Printf(TEXT("Invalid time step key '%s': must be a numeric string"), *TimeStepStr));
            continue;
        }

        // Convert time step string to integer
        int32 TimeStep = FCString::Atoi(*TimeStepStr);

        // Check for negative time steps
        if (TimeStep < 0)
        {
            Errors.Add(FString::Printf(TEXT("Invalid time step %d: time steps must be non-negative"), TimeStep));
            continue;
        }

        // Check for duplicate time steps (shouldn't happen with map, but just in case)
        if (ParsedTimeSteps.Contains(TimeStep))
        {
            Warnings.Add(FString::Printf(TEXT("Duplicate time step %d found"), TimeStep));
            continue;
        }

        if (Value->Type != EJson::Object)
        {
            Errors.Add(FString::Printf(TEXT("TimeStep %d: Expected object, got %s"), 
                TimeStep, *FMATimeStepData::GetJsonTypeName(Value->Type)));
            continue;
        }

        FMATimeStepData TimeStepData;
        FString TimeStepError;
        
        if (FMATimeStepData::FromJsonWithError(Value->AsObject(), TimeStepData, TimeStepError, TimeStep))
        {
            OutData.Data.Add(TimeStep, TimeStepData);
            ParsedTimeSteps.Add(TimeStep);
            
            // Check if there was a partial error (warning)
            if (!TimeStepError.IsEmpty())
            {
                Warnings.Add(TimeStepError);
            }
        }
        else
        {
            Errors.Add(TimeStepError);
        }
    }

    // Check for non-sequential time steps (warning only)
    if (ParsedTimeSteps.Num() > 0)
    {
        ParsedTimeSteps.Sort();
        for (int32 i = 0; i < ParsedTimeSteps.Num(); ++i)
        {
            if (ParsedTimeSteps[i] != i)
            {
                Warnings.Add(FString::Printf(TEXT("Non-sequential time steps detected: expected %d, found %d"), i, ParsedTimeSteps[i]));
                break;
            }
        }
    }

    // If we have critical errors, fail
    if (Errors.Num() > 0)
    {
        OutError = FString::Join(Errors, TEXT("; "));
        return false;
    }

    // If we have warnings, include them in the error message but still succeed
    if (Warnings.Num() > 0)
    {
        OutError = FString::Printf(TEXT("[Warning] %s"), *FString::Join(Warnings, TEXT("; ")));
        // Don't return false - warnings are non-fatal
    }

    // Validate that we have at least some data
    if (OutData.Data.Num() == 0)
    {
        OutError = TEXT("No valid time step data found in 'data' field");
        return false;
    }

    UE_LOG(LogMATaskGraph, Log, TEXT("Parsed skill allocation: %s (%d time steps)"), 
           *OutData.Name, OutData.Data.Num());

    return true;
}

bool FMASkillAllocationData::ValidateRobotIds(TArray<FString>& OutUndefinedRobots) const
{
    // This method checks if all robot IDs are consistent across time steps
    // Returns true if all robots are defined, false if there are undefined references
    
    TSet<FString> AllRobotIds;
    
    // Collect all robot IDs from all time steps
    for (const auto& TimeStepPair : Data)
    {
        for (const auto& RobotPair : TimeStepPair.Value.RobotSkills)
        {
            AllRobotIds.Add(RobotPair.Key);
        }
    }
    
    // For now, we consider all robot IDs as valid since they are self-defining
    // This method can be extended to validate against a known robot registry
    OutUndefinedRobots.Empty();
    return true;
}

FString FMASkillAllocationData::ToJson() const
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    
    RootObject->SetStringField(TEXT("name"), Name);
    RootObject->SetStringField(TEXT("description"), Description);

    // Build data section
    TSharedPtr<FJsonObject> DataObject = MakeShareable(new FJsonObject());
    
    for (const auto& Pair : Data)
    {
        int32 TimeStep = Pair.Key;
        const FMATimeStepData& TimeStepData = Pair.Value;
        
        // Convert time step to string key
        FString TimeStepStr = FString::FromInt(TimeStep);
        DataObject->SetObjectField(TimeStepStr, TimeStepData.ToJsonObject());
    }

    RootObject->SetObjectField(TEXT("data"), DataObject);

    // Serialize with pretty print
    FString OutputString;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = 
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    return OutputString;
}

TArray<FString> FMASkillAllocationData::GetAllRobotIds() const
{
    TSet<FString> RobotIdSet;

    for (const auto& TimeStepPair : Data)
    {
        const FMATimeStepData& TimeStepData = TimeStepPair.Value;
        for (const auto& RobotPair : TimeStepData.RobotSkills)
        {
            RobotIdSet.Add(RobotPair.Key);
        }
    }

    return RobotIdSet.Array();
}

TArray<int32> FMASkillAllocationData::GetAllTimeSteps() const
{
    TArray<int32> TimeSteps;
    Data.GetKeys(TimeSteps);
    TimeSteps.Sort();
    return TimeSteps;
}

FMASkillAssignment* FMASkillAllocationData::FindSkill(int32 TimeStep, const FString& RobotId)
{
    FMATimeStepData* TimeStepData = Data.Find(TimeStep);
    if (TimeStepData)
    {
        return TimeStepData->RobotSkills.Find(RobotId);
    }
    return nullptr;
}

const FMASkillAssignment* FMASkillAllocationData::FindSkill(int32 TimeStep, const FString& RobotId) const
{
    const FMATimeStepData* TimeStepData = Data.Find(TimeStep);
    if (TimeStepData)
    {
        return TimeStepData->RobotSkills.Find(RobotId);
    }
    return nullptr;
}

bool FMASkillAllocationData::operator==(const FMASkillAllocationData& Other) const
{
    if (Name != Other.Name || Description != Other.Description)
    {
        return false;
    }

    if (Data.Num() != Other.Data.Num())
    {
        return false;
    }

    for (const auto& Pair : Data)
    {
        int32 TimeStep = Pair.Key;
        const FMATimeStepData* OtherTimeStepData = Other.Data.Find(TimeStep);
        
        if (!OtherTimeStepData || !(Pair.Value == *OtherTimeStepData))
        {
            return false;
        }
    }

    return true;
}
