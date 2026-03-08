// MACommSceneAndResponseCodec.cpp
// L3 Infrastructure: Scene/Review/Decision JSON codec

#include "MACommJsonCodec.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

FString MACommJsonCodec::SceneChangeTypeToString(EMASceneChangeType Type)
{
    switch (Type)
    {
    case EMASceneChangeType::AddNode:           return TEXT("add_node");
    case EMASceneChangeType::DeleteNode:        return TEXT("delete_node");
    case EMASceneChangeType::EditNode:          return TEXT("edit_node");
    case EMASceneChangeType::AddGoal:           return TEXT("add_goal");
    case EMASceneChangeType::DeleteGoal:        return TEXT("delete_goal");
    case EMASceneChangeType::AddZone:           return TEXT("add_zone");
    case EMASceneChangeType::DeleteZone:        return TEXT("delete_zone");
    case EMASceneChangeType::AddEdge:           return TEXT("add_edge");
    case EMASceneChangeType::EditEdge:          return TEXT("edit_edge");
    default:                                    return TEXT("unknown");
    }
}

EMASceneChangeType MACommJsonCodec::SceneChangeTypeFromString(const FString& TypeStr)
{
    if (TypeStr == TEXT("add_node"))            return EMASceneChangeType::AddNode;
    if (TypeStr == TEXT("delete_node"))         return EMASceneChangeType::DeleteNode;
    if (TypeStr == TEXT("edit_node"))           return EMASceneChangeType::EditNode;
    if (TypeStr == TEXT("add_goal"))            return EMASceneChangeType::AddGoal;
    if (TypeStr == TEXT("delete_goal"))         return EMASceneChangeType::DeleteGoal;
    if (TypeStr == TEXT("add_zone"))            return EMASceneChangeType::AddZone;
    if (TypeStr == TEXT("delete_zone"))         return EMASceneChangeType::DeleteZone;
    if (TypeStr == TEXT("add_edge"))            return EMASceneChangeType::AddEdge;
    if (TypeStr == TEXT("edit_edge"))           return EMASceneChangeType::EditEdge;
    return EMASceneChangeType::AddNode;
}

FString MACommJsonCodec::SerializeSceneChange(const FMASceneChangeMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

    JsonObject->SetStringField(TEXT("change_type"), SceneChangeTypeToString(Message.ChangeType));
    JsonObject->SetNumberField(TEXT("timestamp"), static_cast<double>(Message.Timestamp));
    JsonObject->SetStringField(TEXT("message_id"), Message.MessageId);

    TSharedPtr<FJsonObject> PayloadObject;
    TSharedRef<TJsonReader<>> PayloadReader = TJsonReaderFactory<>::Create(Message.Payload);
    if (FJsonSerializer::Deserialize(PayloadReader, PayloadObject) && PayloadObject.IsValid())
    {
        JsonObject->SetObjectField(TEXT("payload"), PayloadObject);
    }
    else
    {
        JsonObject->SetStringField(TEXT("payload"), Message.Payload);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeSceneChange(const FString& Json, FMASceneChangeMessage& OutMessage)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeSceneChange - Failed to parse JSON"));
        return false;
    }

    FString TypeStr;
    if (JsonObject->TryGetStringField(TEXT("change_type"), TypeStr))
    {
        OutMessage.ChangeType = SceneChangeTypeFromString(TypeStr);
    }

    double TimestampDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("timestamp"), TimestampDouble))
    {
        OutMessage.Timestamp = static_cast<int64>(TimestampDouble);
    }

    JsonObject->TryGetStringField(TEXT("message_id"), OutMessage.MessageId);

    const TSharedPtr<FJsonObject>* PayloadObject;
    if (JsonObject->TryGetObjectField(TEXT("payload"), PayloadObject))
    {
        FString PayloadString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
        FJsonSerializer::Serialize(PayloadObject->ToSharedRef(), Writer);
        OutMessage.Payload = PayloadString;
    }
    else
    {
        JsonObject->TryGetStringField(TEXT("payload"), OutMessage.Payload);
    }

    return true;
}

FString MACommJsonCodec::SerializeSkillAllocation(const FMASkillAllocationMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

    JsonObject->SetStringField(TEXT("name"), Message.Name);
    JsonObject->SetStringField(TEXT("description"), Message.Description);
    JsonObject->SetNumberField(TEXT("timestamp"), static_cast<double>(Message.Timestamp));
    JsonObject->SetStringField(TEXT("message_id"), Message.MessageId);

    TSharedPtr<FJsonObject> DataObject;
    TSharedRef<TJsonReader<>> DataReader = TJsonReaderFactory<>::Create(Message.DataJson);
    if (FJsonSerializer::Deserialize(DataReader, DataObject) && DataObject.IsValid())
    {
        JsonObject->SetObjectField(TEXT("data"), DataObject);
    }
    else
    {
        JsonObject->SetStringField(TEXT("data"), Message.DataJson);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeSkillAllocation(const FString& Json, FMASkillAllocationMessage& OutMessage)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeSkillAllocation - Failed to parse JSON"));
        return false;
    }

    JsonObject->TryGetStringField(TEXT("name"), OutMessage.Name);
    JsonObject->TryGetStringField(TEXT("description"), OutMessage.Description);

    double TimestampDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("timestamp"), TimestampDouble))
    {
        OutMessage.Timestamp = static_cast<int64>(TimestampDouble);
    }

    JsonObject->TryGetStringField(TEXT("message_id"), OutMessage.MessageId);

    const TSharedPtr<FJsonObject>* DataObject;
    if (JsonObject->TryGetObjectField(TEXT("data"), DataObject))
    {
        FString DataString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&DataString);
        FJsonSerializer::Serialize(DataObject->ToSharedRef(), Writer);
        OutMessage.DataJson = DataString;
    }
    else
    {
        JsonObject->TryGetStringField(TEXT("data"), OutMessage.DataJson);
    }

    return true;
}

FString MACommJsonCodec::SerializeSkillStatusUpdate(const FMASkillStatusUpdateMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetNumberField(TEXT("time_step"), Message.TimeStep);
    JsonObject->SetStringField(TEXT("robot_id"), Message.RobotId);
    JsonObject->SetStringField(TEXT("status"), Message.Status);
    JsonObject->SetStringField(TEXT("error_message"), Message.ErrorMessage);
    JsonObject->SetNumberField(TEXT("timestamp"), static_cast<double>(Message.Timestamp));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeSkillStatusUpdate(const FString& Json, FMASkillStatusUpdateMessage& OutMessage)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeSkillStatusUpdate - Failed to parse JSON"));
        return false;
    }

    double TimeStepDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("time_step"), TimeStepDouble))
    {
        OutMessage.TimeStep = static_cast<int32>(TimeStepDouble);
    }

    JsonObject->TryGetStringField(TEXT("robot_id"), OutMessage.RobotId);
    JsonObject->TryGetStringField(TEXT("status"), OutMessage.Status);
    JsonObject->TryGetStringField(TEXT("error_message"), OutMessage.ErrorMessage);

    double TimestampDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("timestamp"), TimestampDouble))
    {
        OutMessage.Timestamp = static_cast<int64>(TimestampDouble);
    }

    return true;
}

FString MACommJsonCodec::SerializeReviewResponse(const FMAReviewResponseMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetBoolField(TEXT("approved"), Message.bApproved);

    if (!Message.ModifiedDataJson.IsEmpty())
    {
        TSharedPtr<FJsonObject> ModifiedDataObject;
        TSharedRef<TJsonReader<>> DataReader = TJsonReaderFactory<>::Create(Message.ModifiedDataJson);
        if (FJsonSerializer::Deserialize(DataReader, ModifiedDataObject) && ModifiedDataObject.IsValid())
        {
            JsonObject->SetObjectField(TEXT("modified_data"), ModifiedDataObject);
        }
        else
        {
            JsonObject->SetStringField(TEXT("modified_data"), Message.ModifiedDataJson);
        }
    }

    if (!Message.RejectionReason.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("rejection_reason"), Message.RejectionReason);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeReviewResponse(const FString& Json, FMAReviewResponseMessage& OutMessage)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeReviewResponse - Failed to parse JSON"));
        return false;
    }

    JsonObject->TryGetBoolField(TEXT("approved"), OutMessage.bApproved);

    const TSharedPtr<FJsonObject>* ModifiedDataObject;
    if (JsonObject->TryGetObjectField(TEXT("modified_data"), ModifiedDataObject))
    {
        FString DataString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&DataString);
        FJsonSerializer::Serialize(ModifiedDataObject->ToSharedRef(), Writer);
        OutMessage.ModifiedDataJson = DataString;
    }
    else
    {
        JsonObject->TryGetStringField(TEXT("modified_data"), OutMessage.ModifiedDataJson);
    }

    JsonObject->TryGetStringField(TEXT("rejection_reason"), OutMessage.RejectionReason);
    return true;
}

FString MACommJsonCodec::SerializeDecisionResponse(const FMADecisionResponseMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("decision"), Message.Decision);

    if (!Message.DecisionDataJson.IsEmpty())
    {
        TSharedPtr<FJsonObject> DecisionDataObject;
        TSharedRef<TJsonReader<>> DataReader = TJsonReaderFactory<>::Create(Message.DecisionDataJson);
        if (FJsonSerializer::Deserialize(DataReader, DecisionDataObject) && DecisionDataObject.IsValid())
        {
            JsonObject->SetObjectField(TEXT("decision_data"), DecisionDataObject);
        }
        else
        {
            JsonObject->SetStringField(TEXT("decision_data"), Message.DecisionDataJson);
        }
    }

    if (!Message.UserFeedback.IsEmpty())
    {
        JsonObject->SetStringField(TEXT("user_feedback"), Message.UserFeedback);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeDecisionResponse(const FString& Json, FMADecisionResponseMessage& OutMessage)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeDecisionResponse - Failed to parse JSON"));
        return false;
    }

    JsonObject->TryGetStringField(TEXT("decision"), OutMessage.Decision);

    const TSharedPtr<FJsonObject>* DecisionDataObject;
    if (JsonObject->TryGetObjectField(TEXT("decision_data"), DecisionDataObject))
    {
        FString DataString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&DataString);
        FJsonSerializer::Serialize(DecisionDataObject->ToSharedRef(), Writer);
        OutMessage.DecisionDataJson = DataString;
    }
    else
    {
        JsonObject->TryGetStringField(TEXT("decision_data"), OutMessage.DecisionDataJson);
    }

    JsonObject->TryGetStringField(TEXT("user_feedback"), OutMessage.UserFeedback);
    return true;
}
