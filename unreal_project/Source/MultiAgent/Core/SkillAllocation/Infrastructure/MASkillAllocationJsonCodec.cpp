#include "Core/SkillAllocation/Infrastructure/MASkillAllocationJsonCodec.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
bool DeserializeSkillAssignment(const TSharedPtr<FJsonObject>& JsonObject, FMASkillAssignment& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    if (JsonObject->TryGetStringField(TEXT("skill"), Out.SkillName))
    {
        const TSharedPtr<FJsonObject>* ParamsObject = nullptr;
        if (JsonObject->TryGetObjectField(TEXT("params"), ParamsObject) && ParamsObject)
        {
            FString ParamsString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsString);
            FJsonSerializer::Serialize(ParamsObject->ToSharedRef(), Writer);
            Out.ParamsJson = ParamsString;
        }
        else
        {
            Out.ParamsJson = TEXT("{}");
        }
    }
    else if (JsonObject->TryGetStringField(TEXT("skill_str"), Out.SkillName))
    {
        FString TaskId;
        if (JsonObject->TryGetStringField(TEXT("task_id"), TaskId))
        {
            Out.ParamsJson = FString::Printf(TEXT("{\"task_id\":\"%s\"}"), *TaskId);
        }
        else
        {
            Out.ParamsJson = TEXT("{}");
        }
    }
    else
    {
        return false;
    }

    Out.Status = ESkillExecutionStatus::Pending;
    return true;
}

bool DeserializeSkillAssignmentWithError(
    const TSharedPtr<FJsonObject>& JsonObject,
    FMASkillAssignment& Out,
    FString& OutError,
    const FString& Context)
{
    if (!JsonObject.IsValid())
    {
        OutError = FString::Printf(TEXT("%s: Invalid JSON object"), *Context);
        return false;
    }

    if (!DeserializeSkillAssignment(JsonObject, Out))
    {
        OutError = FString::Printf(TEXT("%s: Missing required field 'skill' or 'skill_str'"), *Context);
        return false;
    }

    if (Out.SkillName.IsEmpty())
    {
        OutError = FString::Printf(TEXT("%s: Field 'skill' or 'skill_str' cannot be empty"), *Context);
        return false;
    }

    return true;
}

TSharedPtr<FJsonObject> SerializeSkillAssignment(const FMASkillAssignment& Skill)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    JsonObject->SetStringField(TEXT("skill"), Skill.SkillName);

    TSharedPtr<FJsonObject> ParamsObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Skill.ParamsJson);
    if (FJsonSerializer::Deserialize(Reader, ParamsObject) && ParamsObject.IsValid())
    {
        JsonObject->SetObjectField(TEXT("params"), ParamsObject);
    }
    else
    {
        JsonObject->SetObjectField(TEXT("params"), MakeShared<FJsonObject>());
    }

    return JsonObject;
}

FString GetJsonTypeName(const EJson Type)
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

bool DeserializeTimeStepData(const TSharedPtr<FJsonObject>& JsonObject, FMATimeStepData& Out)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    Out.RobotSkills.Empty();
    for (const auto& Pair : JsonObject->Values)
    {
        if (Pair.Value->Type == EJson::Object)
        {
            FMASkillAssignment Skill;
            if (DeserializeSkillAssignment(Pair.Value->AsObject(), Skill))
            {
                Out.RobotSkills.Add(Pair.Key, Skill);
            }
        }
    }

    return true;
}

bool DeserializeTimeStepDataWithError(
    const TSharedPtr<FJsonObject>& JsonObject,
    FMATimeStepData& Out,
    FString& OutError,
    const int32 TimeStep)
{
    if (!JsonObject.IsValid())
    {
        OutError = FString::Printf(TEXT("TimeStep %d: Invalid JSON object"), TimeStep);
        return false;
    }

    Out.RobotSkills.Empty();
    TArray<FString> SkillErrors;

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
            SkillErrors.Add(FString::Printf(
                TEXT("TimeStep %d, Robot '%s': Expected object, got %s"),
                TimeStep,
                *RobotId,
                *GetJsonTypeName(Value->Type)));
            continue;
        }

        FMASkillAssignment Skill;
        FString SkillError;
        const FString Context = FString::Printf(TEXT("TimeStep %d, Robot '%s'"), TimeStep, *RobotId);
        if (DeserializeSkillAssignmentWithError(Value->AsObject(), Skill, SkillError, Context))
        {
            Out.RobotSkills.Add(RobotId, Skill);
        }
        else
        {
            SkillErrors.Add(SkillError);
        }
    }

    if (SkillErrors.Num() > 0)
    {
        OutError = FString::Join(SkillErrors, TEXT("; "));
        return Out.RobotSkills.Num() > 0;
    }

    return true;
}

TSharedPtr<FJsonObject> SerializeTimeStepData(const FMATimeStepData& Data)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    for (const auto& Pair : Data.RobotSkills)
    {
        JsonObject->SetObjectField(Pair.Key, SerializeSkillAssignment(Pair.Value));
    }
    return JsonObject;
}

bool DeserializeInternal(
    const TSharedPtr<FJsonObject>& RootObject,
    const TSharedPtr<FJsonObject>* DataObject,
    FMASkillAllocationData& OutData,
    FString& OutError)
{
    OutData.Data.Empty();
    TArray<FString> Warnings;
    TArray<FString> Errors;
    TArray<int32> ParsedTimeSteps;

    for (const auto& Pair : (*DataObject)->Values)
    {
        const FString& TimeStepStr = Pair.Key;
        const TSharedPtr<FJsonValue>& Value = Pair.Value;

        if (!TimeStepStr.IsNumeric())
        {
            Errors.Add(FString::Printf(TEXT("Invalid time step key '%s': must be a numeric string"), *TimeStepStr));
            continue;
        }

        const int32 TimeStep = FCString::Atoi(*TimeStepStr);
        if (TimeStep < 0)
        {
            Errors.Add(FString::Printf(TEXT("Invalid time step %d: time steps must be non-negative"), TimeStep));
            continue;
        }

        if (ParsedTimeSteps.Contains(TimeStep))
        {
            Warnings.Add(FString::Printf(TEXT("Duplicate time step %d found"), TimeStep));
            continue;
        }

        if (Value->Type != EJson::Object)
        {
            Errors.Add(FString::Printf(TEXT("TimeStep %d: Expected object, got %s"), TimeStep, *GetJsonTypeName(Value->Type)));
            continue;
        }

        FMATimeStepData TimeStepData;
        FString TimeStepError;
        if (DeserializeTimeStepDataWithError(Value->AsObject(), TimeStepData, TimeStepError, TimeStep))
        {
            OutData.Data.Add(TimeStep, TimeStepData);
            ParsedTimeSteps.Add(TimeStep);
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

    if (ParsedTimeSteps.Num() > 0)
    {
        ParsedTimeSteps.Sort();
        for (int32 Index = 0; Index < ParsedTimeSteps.Num(); ++Index)
        {
            if (ParsedTimeSteps[Index] != Index)
            {
                Warnings.Add(FString::Printf(TEXT("Non-sequential time steps detected: expected %d, found %d"), Index, ParsedTimeSteps[Index]));
                break;
            }
        }
    }

    if (Errors.Num() > 0)
    {
        OutError = FString::Join(Errors, TEXT("; "));
        return false;
    }

    if (Warnings.Num() > 0)
    {
        OutError = FString::Printf(TEXT("[Warning] %s"), *FString::Join(Warnings, TEXT("; ")));
    }

    if (OutData.Data.Num() == 0)
    {
        OutError = TEXT("No valid time step data found in 'data' field");
        return false;
    }

    UE_LOG(LogMASkillAllocation, Log, TEXT("Parsed skill allocation: %s (%d time steps)"), *OutData.Name, OutData.Data.Num());
    return true;
}
}

bool FMASkillAllocationJsonCodec::Deserialize(const FString& JsonString, FMASkillAllocationData& OutData, FString& OutError)
{
    if (JsonString.IsEmpty())
    {
        OutError = TEXT("JSON input is empty");
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, RootObject))
    {
        const FString ReaderError = Reader->GetErrorMessage();
        OutError = ReaderError.IsEmpty()
            ? TEXT("JSON syntax error: Failed to parse JSON")
            : FString::Printf(TEXT("JSON syntax error: %s"), *ReaderError);
        return false;
    }

    if (!RootObject.IsValid())
    {
        OutError = TEXT("JSON parse error: Root object is null");
        return false;
    }

    const TSharedPtr<FJsonObject>* TimestepSkillsObject = nullptr;
    if (RootObject->TryGetObjectField(TEXT("timestep_skills"), TimestepSkillsObject))
    {
        UE_LOG(LogMASkillAllocation, Log, TEXT("FMASkillAllocationJsonCodec - Detected GSI format (timestep_skills)"));
        OutData.Name = TEXT("GSI Allocation");
        OutData.Description = TEXT("Skill allocation from GSI planner");
        return DeserializeInternal(RootObject, TimestepSkillsObject, OutData, OutError);
    }

    UE_LOG(LogMASkillAllocation, Log, TEXT("FMASkillAllocationJsonCodec - Detected Mock Backend format (name/description/data)"));

    TArray<FString> MissingFields;
    if (!RootObject->TryGetStringField(TEXT("name"), OutData.Name))
    {
        MissingFields.Add(TEXT("name"));
    }
    if (!RootObject->TryGetStringField(TEXT("description"), OutData.Description))
    {
        MissingFields.Add(TEXT("description"));
    }

    const TSharedPtr<FJsonObject>* DataObject = nullptr;
    if (!RootObject->TryGetObjectField(TEXT("data"), DataObject))
    {
        MissingFields.Add(TEXT("data"));
    }

    if (MissingFields.Num() > 0)
    {
        OutError = FString::Printf(TEXT("Missing required field(s): %s"), *FString::Join(MissingFields, TEXT(", ")));
        return false;
    }

    return DeserializeInternal(RootObject, DataObject, OutData, OutError);
}

FString FMASkillAllocationJsonCodec::Serialize(const FMASkillAllocationData& Data)
{
    TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
    RootObject->SetStringField(TEXT("name"), Data.Name);
    RootObject->SetStringField(TEXT("description"), Data.Description);

    TSharedPtr<FJsonObject> DataObject = MakeShared<FJsonObject>();
    for (const auto& Pair : Data.Data)
    {
        DataObject->SetObjectField(FString::FromInt(Pair.Key), SerializeTimeStepData(Pair.Value));
    }
    RootObject->SetObjectField(TEXT("data"), DataObject);

    FString OutputString;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    return OutputString;
}
