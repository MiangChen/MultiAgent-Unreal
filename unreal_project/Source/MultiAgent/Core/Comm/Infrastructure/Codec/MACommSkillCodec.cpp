// MACommSkillCodec.cpp
// L3 Infrastructure: SkillList/Feedback JSON codec

#include "MACommJsonCodec.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    TSharedPtr<FJsonObject> SerializeSkillFeedbackObject(const FMASkillFeedback_Comm& Feedback)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

        JsonObject->SetStringField(TEXT("agent_id"), Feedback.AgentId);
        JsonObject->SetStringField(TEXT("skill"), Feedback.SkillName);
        JsonObject->SetBoolField(TEXT("success"), Feedback.bSuccess);
        JsonObject->SetStringField(TEXT("message"), Feedback.Message);

        if (Feedback.Data.Num() > 0)
        {
            TSharedPtr<FJsonObject> DataObject = MakeShareable(new FJsonObject());
            for (const auto& Pair : Feedback.Data)
            {
                const FString& Key = Pair.Key;
                const FString& Value = Pair.Value;

                if (Value.Equals(TEXT("true"), ESearchCase::IgnoreCase))
                {
                    DataObject->SetBoolField(Key, true);
                }
                else if (Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
                {
                    DataObject->SetBoolField(Key, false);
                }
                else if ((Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]"))) ||
                         (Value.StartsWith(TEXT("{")) && Value.EndsWith(TEXT("}"))))
                {
                    TSharedPtr<FJsonValue> ParsedValue;
                    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Value);
                    if (FJsonSerializer::Deserialize(Reader, ParsedValue) && ParsedValue.IsValid())
                    {
                        DataObject->SetField(Key, ParsedValue);
                    }
                    else
                    {
                        DataObject->SetStringField(Key, Value);
                    }
                }
                else if (Value.IsNumeric())
                {
                    if (Value.Contains(TEXT(".")))
                    {
                        DataObject->SetNumberField(Key, FCString::Atod(*Value));
                    }
                    else
                    {
                        DataObject->SetNumberField(Key, FCString::Atoi(*Value));
                    }
                }
                else
                {
                    DataObject->SetStringField(Key, Value);
                }
            }
            JsonObject->SetObjectField(TEXT("data"), DataObject);
        }

        return JsonObject;
    }
}

bool MACommJsonCodec::DeserializeSkillList(const FString& Json, FMASkillListMessage& OutSkillList)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeSkillList - Failed to parse JSON"));
        return false;
    }

    OutSkillList.TimeSteps.Empty();
    OutSkillList.TotalTimeSteps = 0;

    TArray<int32> TimeStepIndices;
    for (const auto& Pair : JsonObject->Values)
    {
        if (Pair.Key.IsNumeric())
        {
            TimeStepIndices.Add(FCString::Atoi(*Pair.Key));
        }
    }

    TimeStepIndices.Sort();

    for (int32 StepIndex : TimeStepIndices)
    {
        const TSharedPtr<FJsonObject>* StepObject;
        if (!JsonObject->TryGetObjectField(FString::FromInt(StepIndex), StepObject))
        {
            continue;
        }

        FMATimeStepCommands TimeStepCmd;
        TimeStepCmd.TimeStep = StepIndex;

        for (const auto& AgentPair : (*StepObject)->Values)
        {
            const TSharedPtr<FJsonObject>* AgentCmdObject;
            if (!AgentPair.Value->TryGetObject(AgentCmdObject))
            {
                continue;
            }

            FMAAgentSkillCommand Cmd;
            Cmd.AgentId = AgentPair.Key;
            (*AgentCmdObject)->TryGetStringField(TEXT("skill"), Cmd.SkillName);

            const TSharedPtr<FJsonObject>* ParamsObject;
            if ((*AgentCmdObject)->TryGetObjectField(TEXT("params"), ParamsObject))
            {
                FString ParamsJson;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsJson);
                FJsonSerializer::Serialize(ParamsObject->ToSharedRef(), Writer);
                Cmd.Params.RawParamsJson = ParamsJson;
            }

            TimeStepCmd.Commands.Add(Cmd);
        }

        OutSkillList.TimeSteps.Add(TimeStepCmd);
    }

    OutSkillList.TotalTimeSteps = OutSkillList.TimeSteps.Num();
    UE_LOG(LogMACommTypes, Log, TEXT("DeserializeSkillList - Parsed %d time steps"), OutSkillList.TotalTimeSteps);
    return OutSkillList.TotalTimeSteps > 0;
}

FString MACommJsonCodec::SerializeSkillList(const FMASkillListMessage& SkillList)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

    for (const FMATimeStepCommands& TimeStep : SkillList.TimeSteps)
    {
        TSharedPtr<FJsonObject> StepObject = MakeShareable(new FJsonObject());

        for (const FMAAgentSkillCommand& Cmd : TimeStep.Commands)
        {
            TSharedPtr<FJsonObject> CmdObject = MakeShareable(new FJsonObject());
            CmdObject->SetStringField(TEXT("skill"), Cmd.SkillName);

            if (!Cmd.Params.RawParamsJson.IsEmpty())
            {
                TSharedPtr<FJsonObject> ParamsObject;
                TSharedRef<TJsonReader<>> ParamsReader = TJsonReaderFactory<>::Create(Cmd.Params.RawParamsJson);
                if (FJsonSerializer::Deserialize(ParamsReader, ParamsObject) && ParamsObject.IsValid())
                {
                    CmdObject->SetObjectField(TEXT("params"), ParamsObject);
                }
                else
                {
                    CmdObject->SetObjectField(TEXT("params"), MakeShareable(new FJsonObject()));
                }
            }
            else
            {
                CmdObject->SetObjectField(TEXT("params"), MakeShareable(new FJsonObject()));
            }

            StepObject->SetObjectField(Cmd.AgentId, CmdObject);
        }

        JsonObject->SetObjectField(FString::FromInt(TimeStep.TimeStep), StepObject);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

const FMATimeStepCommands* MACommJsonCodec::FindSkillListTimeStep(const FMASkillListMessage& SkillList, int32 Step)
{
    for (const FMATimeStepCommands& TimeStep : SkillList.TimeSteps)
    {
        if (TimeStep.TimeStep == Step)
        {
            return &TimeStep;
        }
    }
    return nullptr;
}

FString MACommJsonCodec::SerializeTimeStepFeedback(const FMATimeStepFeedbackMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

    JsonObject->SetStringField(TEXT("feedback_type"), TEXT("timestep"));
    JsonObject->SetNumberField(TEXT("time_step"), Message.TimeStep);

    TArray<TSharedPtr<FJsonValue>> FeedbacksArray;
    for (const FMASkillFeedback_Comm& Feedback : Message.Feedbacks)
    {
        FeedbacksArray.Add(MakeShareable(new FJsonValueObject(SerializeSkillFeedbackObject(Feedback))));
    }
    JsonObject->SetArrayField(TEXT("feedbacks"), FeedbacksArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

FString MACommJsonCodec::SerializeSkillListCompleted(const FMASkillListCompletedMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

    JsonObject->SetStringField(TEXT("feedback_type"), TEXT("skill_list_completed"));
    JsonObject->SetBoolField(TEXT("completed"), Message.bCompleted);
    JsonObject->SetBoolField(TEXT("interrupted"), Message.bInterrupted);
    JsonObject->SetNumberField(TEXT("completed_time_steps"), Message.CompletedTimeSteps);
    JsonObject->SetNumberField(TEXT("total_time_steps"), Message.TotalTimeSteps);
    JsonObject->SetStringField(TEXT("message"), Message.Message);

    TArray<TSharedPtr<FJsonValue>> TimeStepFeedbacksArray;
    for (const FMATimeStepFeedbackMessage& TSFeedback : Message.AllTimeStepFeedbacks)
    {
        TSharedPtr<FJsonObject> TSObject = MakeShareable(new FJsonObject());
        TSObject->SetNumberField(TEXT("time_step"), TSFeedback.TimeStep);

        TArray<TSharedPtr<FJsonValue>> FeedbacksArray;
        for (const FMASkillFeedback_Comm& Feedback : TSFeedback.Feedbacks)
        {
            FeedbacksArray.Add(MakeShareable(new FJsonValueObject(SerializeSkillFeedbackObject(Feedback))));
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
