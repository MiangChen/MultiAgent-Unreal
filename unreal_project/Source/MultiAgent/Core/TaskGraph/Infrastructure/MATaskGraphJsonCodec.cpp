#include "Core/TaskGraph/Infrastructure/MATaskGraphJsonCodec.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
FString GetOptionalStringField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
{
    FString Value;
    if (JsonObject.IsValid())
    {
        JsonObject->TryGetStringField(FieldName, Value);
    }
    return Value;
}

FString NormalizeEdgeType(const FString& RawType)
{
    if (RawType.IsEmpty() || RawType.Equals(TEXT("normal"), ESearchCase::IgnoreCase))
    {
        return TEXT("sequential");
    }
    return RawType;
}

TSharedPtr<FJsonObject> SerializeRequiredSkill(const FMARequiredSkill& Skill)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    JsonObject->SetStringField(TEXT("skill_name"), Skill.SkillName);

    TArray<TSharedPtr<FJsonValue>> RobotTypeArray;
    for (const FString& Type : Skill.AssignedRobotType)
    {
        RobotTypeArray.Add(MakeShared<FJsonValueString>(Type));
    }
    JsonObject->SetArrayField(TEXT("assigned_robot_type"), RobotTypeArray);
    JsonObject->SetNumberField(TEXT("assigned_robot_count"), Skill.AssignedRobotCount);
    return JsonObject;
}

bool ParseRequiredSkill(const TSharedPtr<FJsonObject>& JsonObject, FMARequiredSkill& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    Out.SkillName = JsonObject->GetStringField(TEXT("skill_name"));
    Out.AssignedRobotCount = JsonObject->GetIntegerField(TEXT("assigned_robot_count"));

    const TArray<TSharedPtr<FJsonValue>>* RobotTypeArray = nullptr;
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

bool ParseRequiredSkillCompactString(const FString& CompactString, FMARequiredSkill& Out)
{
    const FString Trimmed = CompactString.TrimStartAndEnd();
    if (Trimmed.IsEmpty())
    {
        return false;
    }

    TArray<FString> Parts;
    Trimmed.ParseIntoArray(Parts, TEXT(":"));
    if (Parts.Num() < 3)
    {
        Out.SkillName = Trimmed;
        Out.AssignedRobotType.Empty();
        Out.AssignedRobotCount = 1;
        return true;
    }

    const FString RobotTypesPart = Parts[0].TrimStartAndEnd();
    FString CountPart;
    FString SkillNamePart;

    if (Parts.Num() >= 4 && Parts[Parts.Num() - 2].Equals(TEXT("tbd"), ESearchCase::IgnoreCase))
    {
        CountPart = FString::Printf(TEXT("tbd:%s"), *Parts.Last().TrimStartAndEnd());
        TArray<FString> SkillParts;
        for (int32 Index = 1; Index < Parts.Num() - 2; ++Index)
        {
            SkillParts.Add(Parts[Index]);
        }
        SkillNamePart = FString::Join(SkillParts, TEXT(":"));
    }
    else
    {
        CountPart = Parts.Last().TrimStartAndEnd();
        TArray<FString> SkillParts;
        for (int32 Index = 1; Index < Parts.Num() - 1; ++Index)
        {
            SkillParts.Add(Parts[Index]);
        }
        SkillNamePart = FString::Join(SkillParts, TEXT(":"));
    }

    Out.SkillName = SkillNamePart.TrimStartAndEnd();
    Out.AssignedRobotType.Empty();

    TArray<FString> RobotTypes;
    RobotTypesPart.ParseIntoArray(RobotTypes, TEXT("|"));
    for (const FString& RobotType : RobotTypes)
    {
        const FString Normalized = RobotType.TrimStartAndEnd();
        if (!Normalized.IsEmpty())
        {
            Out.AssignedRobotType.Add(Normalized);
        }
    }

    if (CountPart.StartsWith(TEXT("tbd:"), ESearchCase::IgnoreCase))
    {
        Out.AssignedRobotCount = 1;
    }
    else
    {
        Out.AssignedRobotCount = FCString::Atoi(*CountPart);
        if (Out.AssignedRobotCount <= 0)
        {
            Out.AssignedRobotCount = 1;
        }
    }

    return !Out.SkillName.IsEmpty();
}

bool ParseRequiredSkillValue(const TSharedPtr<FJsonValue>& Value, FMARequiredSkill& Out)
{
    if (!Value.IsValid())
    {
        return false;
    }

    if (Value->Type == EJson::Object)
    {
        return ParseRequiredSkill(Value->AsObject(), Out);
    }
    if (Value->Type == EJson::String)
    {
        return ParseRequiredSkillCompactString(Value->AsString(), Out);
    }
    return false;
}

TSharedPtr<FJsonObject> SerializeTaskNode(const FMATaskNodeData& Node)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    JsonObject->SetStringField(TEXT("task_id"), Node.TaskId);
    JsonObject->SetStringField(TEXT("description"), Node.Description);
    JsonObject->SetStringField(TEXT("location"), Node.Location);

    TArray<TSharedPtr<FJsonValue>> SkillsArray;
    for (const FMARequiredSkill& Skill : Node.RequiredSkills)
    {
        SkillsArray.Add(MakeShared<FJsonValueObject>(SerializeRequiredSkill(Skill)));
    }
    JsonObject->SetArrayField(TEXT("required_skills"), SkillsArray);

    TArray<TSharedPtr<FJsonValue>> ProducesArray;
    for (const FString& Produce : Node.Produces)
    {
        ProducesArray.Add(MakeShared<FJsonValueString>(Produce));
    }
    JsonObject->SetArrayField(TEXT("produces"), ProducesArray);
    return JsonObject;
}

bool ParseTaskNode(const TSharedPtr<FJsonObject>& JsonObject, FMATaskNodeData& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    Out.TaskId = JsonObject->GetStringField(TEXT("task_id"));
    Out.Description = GetOptionalStringField(JsonObject, TEXT("description"));
    Out.Location = GetOptionalStringField(JsonObject, TEXT("location"));

    const TArray<TSharedPtr<FJsonValue>>* SkillsArray = nullptr;
    if (JsonObject->TryGetArrayField(TEXT("required_skills"), SkillsArray))
    {
        Out.RequiredSkills.Empty();
        for (const TSharedPtr<FJsonValue>& Value : *SkillsArray)
        {
            FMARequiredSkill Skill;
            if (ParseRequiredSkillValue(Value, Skill))
            {
                Out.RequiredSkills.Add(Skill);
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* ProducesArray = nullptr;
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

TSharedPtr<FJsonObject> SerializeTaskEdge(const FMATaskEdgeData& Edge)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    JsonObject->SetStringField(TEXT("from"), Edge.FromNodeId);
    JsonObject->SetStringField(TEXT("to"), Edge.ToNodeId);
    JsonObject->SetStringField(TEXT("type"), Edge.EdgeType);
    if (!Edge.Condition.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("condition"), Edge.Condition);
    }
    return JsonObject;
}

bool ParseTaskEdge(const TSharedPtr<FJsonObject>& JsonObject, FMATaskEdgeData& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    Out.FromNodeId = JsonObject->GetStringField(TEXT("from"));
    Out.ToNodeId = JsonObject->GetStringField(TEXT("to"));
    Out.EdgeType = NormalizeEdgeType(GetOptionalStringField(JsonObject, TEXT("type")));
    Out.Condition = GetOptionalStringField(JsonObject, TEXT("condition"));
    return true;
}

bool ParseTaskEdgeCompactString(const FString& CompactString, FMATaskEdgeData& Out)
{
    const FString Trimmed = CompactString.TrimStartAndEnd();
    const int32 ArrowIndex = Trimmed.Find(TEXT("->"));
    if (ArrowIndex == INDEX_NONE)
    {
        return false;
    }

    Out.FromNodeId = Trimmed.Left(ArrowIndex).TrimStartAndEnd();
    FString Remainder = Trimmed.Mid(ArrowIndex + 2).TrimStartAndEnd();

    const int32 ColonIndex = Remainder.Find(TEXT(":"));
    if (ColonIndex != INDEX_NONE)
    {
        Out.ToNodeId = Remainder.Left(ColonIndex).TrimStartAndEnd();
        Out.Condition = Remainder.Mid(ColonIndex + 1).TrimStartAndEnd();
        Out.EdgeType = TEXT("conditional");
    }
    else
    {
        Out.ToNodeId = Remainder;
        Out.Condition.Empty();
        Out.EdgeType = TEXT("sequential");
    }

    return !Out.FromNodeId.IsEmpty() && !Out.ToNodeId.IsEmpty();
}

bool ParseTaskEdgeValue(const TSharedPtr<FJsonValue>& Value, FMATaskEdgeData& Out)
{
    if (!Value.IsValid())
    {
        return false;
    }

    if (Value->Type == EJson::Object)
    {
        return ParseTaskEdge(Value->AsObject(), Out);
    }
    if (Value->Type == EJson::String)
    {
        return ParseTaskEdgeCompactString(Value->AsString(), Out);
    }
    return false;
}
}

FString FTaskGraphJsonCodec::Serialize(const FMATaskGraphData& Data)
{
    TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();

    TSharedPtr<FJsonObject> MetaObject = MakeShared<FJsonObject>();
    MetaObject->SetStringField(TEXT("description"), Data.Description);
    RootObject->SetObjectField(TEXT("meta"), MetaObject);

    TSharedPtr<FJsonObject> TaskGraphObject = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FMATaskNodeData& Node : Data.Nodes)
    {
        NodesArray.Add(MakeShared<FJsonValueObject>(SerializeTaskNode(Node)));
    }
    TaskGraphObject->SetArrayField(TEXT("nodes"), NodesArray);

    TArray<TSharedPtr<FJsonValue>> EdgesArray;
    for (const FMATaskEdgeData& Edge : Data.Edges)
    {
        EdgesArray.Add(MakeShared<FJsonValueObject>(SerializeTaskEdge(Edge)));
    }
    TaskGraphObject->SetArrayField(TEXT("edges"), EdgesArray);

    RootObject->SetObjectField(TEXT("task_graph"), TaskGraphObject);

    FString OutputString;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    return OutputString;
}

bool FTaskGraphJsonCodec::TryParseJson(const FString& Json, FMATaskGraphData& OutData, FString& OutError)
{
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON");
        return false;
    }

    if (RootObject->HasField(TEXT("task_graph")))
    {
        return TryParseResponseJson(Json, OutData, OutError);
    }

    if (!RootObject->HasField(TEXT("nodes")))
    {
        OutError = TEXT("Unknown JSON format: missing 'task_graph' or 'nodes' field");
        return false;
    }

    OutData = FMATaskGraphData();

    const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
    if (RootObject->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *NodesArray)
        {
            FMATaskNodeData Node;
            if (ParseTaskNode(Value->AsObject(), Node))
            {
                OutData.Nodes.Add(Node);
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* EdgesArray = nullptr;
    if (RootObject->TryGetArrayField(TEXT("edges"), EdgesArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *EdgesArray)
        {
            FMATaskEdgeData Edge;
            if (ParseTaskEdgeValue(Value, Edge))
            {
                OutData.Edges.Add(Edge);
            }
        }
    }

    return true;
}

bool FTaskGraphJsonCodec::TryParseResponseJson(const FString& Json, FMATaskGraphData& OutData, FString& OutError)
{
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        OutError = TEXT("Failed to parse JSON");
        return false;
    }

    OutData = FMATaskGraphData();

    const TSharedPtr<FJsonObject>* MetaObject = nullptr;
    if (RootObject->TryGetObjectField(TEXT("meta"), MetaObject))
    {
        OutData.Description = GetOptionalStringField(*MetaObject, TEXT("description"));
        if (OutData.Description.IsEmpty())
        {
            OutData.Description = GetOptionalStringField(*MetaObject, TEXT("reasoning"));
        }
    }

    const TSharedPtr<FJsonObject>* TaskGraphObject = nullptr;
    if (!RootObject->TryGetObjectField(TEXT("task_graph"), TaskGraphObject))
    {
        OutError = TEXT("Missing 'task_graph' field");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
    if ((*TaskGraphObject)->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *NodesArray)
        {
            FMATaskNodeData Node;
            if (ParseTaskNode(Value->AsObject(), Node))
            {
                OutData.Nodes.Add(Node);
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* EdgesArray = nullptr;
    if ((*TaskGraphObject)->TryGetArrayField(TEXT("edges"), EdgesArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *EdgesArray)
        {
            FMATaskEdgeData Edge;
            if (ParseTaskEdgeValue(Value, Edge))
            {
                OutData.Edges.Add(Edge);
            }
        }
    }

    UE_LOG(LogMATaskGraph, Log, TEXT("Parsed task graph: %d nodes, %d edges"),
           OutData.Nodes.Num(), OutData.Edges.Num());
    return true;
}
