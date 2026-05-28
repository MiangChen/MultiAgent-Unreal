// MACommBasicMessagesCodec.cpp
// L3 Infrastructure: 基础消息 JSON codec

#include "MACommJsonCodec.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

FString MACommJsonCodec::SerializeUIInput(const FMAUIInputMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("instruction_id"), Message.InputSourceId);
    JsonObject->SetStringField(TEXT("instruction_text"), Message.InputContent);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeUIInput(const FString& Json, FMAUIInputMessage& OutMessage)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeUIInput - Failed to parse JSON"));
        return false;
    }

    JsonObject->TryGetStringField(TEXT("instruction_id"), OutMessage.InputSourceId);
    JsonObject->TryGetStringField(TEXT("instruction_text"), OutMessage.InputContent);
    return true;
}

FString MACommJsonCodec::SerializeButtonEvent(const FMAButtonEventMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("widget_name"), Message.WidgetName);
    JsonObject->SetStringField(TEXT("button_id"), Message.ButtonId);
    JsonObject->SetStringField(TEXT("button_text"), Message.ButtonText);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeButtonEvent(const FString& Json, FMAButtonEventMessage& OutMessage)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeButtonEvent - Failed to parse JSON"));
        return false;
    }

    JsonObject->TryGetStringField(TEXT("widget_name"), OutMessage.WidgetName);
    JsonObject->TryGetStringField(TEXT("button_id"), OutMessage.ButtonId);
    JsonObject->TryGetStringField(TEXT("button_text"), OutMessage.ButtonText);
    return true;
}

FString MACommJsonCodec::SerializeTaskFeedback(const FMATaskFeedbackMessage& Message)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("task_id"), Message.TaskId);
    JsonObject->SetStringField(TEXT("status"), Message.Status);
    JsonObject->SetNumberField(TEXT("duration_seconds"), Message.DurationSeconds);
    JsonObject->SetNumberField(TEXT("energy_consumed"), Message.EnergyConsumed);
    JsonObject->SetStringField(TEXT("error_message"), Message.ErrorMessage);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeTaskFeedback(const FString& Json, FMATaskFeedbackMessage& OutMessage)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeTaskFeedback - Failed to parse JSON"));
        return false;
    }

    JsonObject->TryGetStringField(TEXT("task_id"), OutMessage.TaskId);
    JsonObject->TryGetStringField(TEXT("status"), OutMessage.Status);

    double DurationDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("duration_seconds"), DurationDouble))
    {
        OutMessage.DurationSeconds = static_cast<float>(DurationDouble);
    }

    double EnergyDouble = 0;
    if (JsonObject->TryGetNumberField(TEXT("energy_consumed"), EnergyDouble))
    {
        OutMessage.EnergyConsumed = static_cast<float>(EnergyDouble);
    }

    JsonObject->TryGetStringField(TEXT("error_message"), OutMessage.ErrorMessage);
    return true;
}
