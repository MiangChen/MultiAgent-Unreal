// MACommEnvelopeCodec.cpp
// L3 Infrastructure: Envelope JSON codec

#include "MACommJsonCodec.h"
#include "MACommTypeHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/DateTime.h"

namespace
{
    int64 CurrentTimestampMs()
    {
        return FDateTime::UtcNow().ToUnixTimestamp() * 1000 + FDateTime::UtcNow().GetMillisecond();
    }

    FString TimestampToISO8601(int64 UnixMillis)
    {
        const int64 UnixSeconds = UnixMillis / 1000;
        const int32 Millis = static_cast<int32>(UnixMillis % 1000);

        const FDateTime DateTime = FDateTime::FromUnixTimestamp(UnixSeconds);
        return FString::Printf(TEXT("%04d-%02d-%02dT%02d:%02d:%02d.%03dZ"),
            DateTime.GetYear(),
            DateTime.GetMonth(),
            DateTime.GetDay(),
            DateTime.GetHour(),
            DateTime.GetMinute(),
            DateTime.GetSecond(),
            Millis);
    }

    int64 ISO8601ToTimestamp(const FString& ISOString)
    {
        FDateTime DateTime;
        if (FDateTime::ParseIso8601(*ISOString, DateTime))
        {
            const int64 UnixSeconds = DateTime.ToUnixTimestamp();
            int32 Millis = 0;
            int32 DotIndex = INDEX_NONE;
            if (ISOString.FindChar(TEXT('.'), DotIndex))
            {
                int32 ZIndex = ISOString.Find(TEXT("Z"), ESearchCase::IgnoreCase, ESearchDir::FromStart, DotIndex);
                if (ZIndex == INDEX_NONE)
                {
                    ZIndex = ISOString.Len();
                }

                FString MillisStr = ISOString.Mid(DotIndex + 1, ZIndex - DotIndex - 1);
                while (MillisStr.Len() < 3)
                {
                    MillisStr += TEXT("0");
                }
                if (MillisStr.Len() > 3)
                {
                    MillisStr = MillisStr.Left(3);
                }
                Millis = FCString::Atoi(*MillisStr);
            }

            return UnixSeconds * 1000 + Millis;
        }

        if (ISOString.IsNumeric())
        {
            return FCString::Atoi64(*ISOString);
        }

        UE_LOG(LogMACommTypes, Warning, TEXT("ISO8601ToTimestamp - Failed to parse: %s, using current time"), *ISOString);
        return CurrentTimestampMs();
    }
}

FString MACommJsonCodec::SerializeEnvelope(const FMAMessageEnvelope& Envelope)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

    JsonObject->SetStringField(TEXT("message_id"), Envelope.MessageId);
    JsonObject->SetStringField(TEXT("message_category"), MACommTypeHelpers::MessageCategoryToString(Envelope.MessageCategory));
    JsonObject->SetStringField(TEXT("message_type"), MACommTypeHelpers::MessageTypeToString(Envelope.MessageType));
    JsonObject->SetStringField(TEXT("direction"), MACommTypeHelpers::MessageDirectionToString(Envelope.Direction));

    FString ISOTimestamp = Envelope.TimestampISO;
    if (ISOTimestamp.IsEmpty() && Envelope.Timestamp > 0)
    {
        ISOTimestamp = TimestampToISO8601(Envelope.Timestamp);
    }
    JsonObject->SetStringField(TEXT("timestamp"), ISOTimestamp);

    TSharedPtr<FJsonObject> PayloadObject;
    TSharedRef<TJsonReader<>> PayloadReader = TJsonReaderFactory<>::Create(Envelope.PayloadJson);
    if (FJsonSerializer::Deserialize(PayloadReader, PayloadObject) && PayloadObject.IsValid())
    {
        JsonObject->SetObjectField(TEXT("payload"), PayloadObject);
    }
    else
    {
        JsonObject->SetStringField(TEXT("payload"), Envelope.PayloadJson);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return OutputString;
}

bool MACommJsonCodec::DeserializeEnvelope(const FString& Json, FMAMessageEnvelope& OutEnvelope)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommTypes, Warning, TEXT("DeserializeEnvelope - Failed to parse JSON"));
        return false;
    }

    FString TypeStr;
    if (JsonObject->TryGetStringField(TEXT("message_type"), TypeStr))
    {
        OutEnvelope.MessageType = MACommTypeHelpers::StringToMessageType(TypeStr);
    }

    FString CategoryStr;
    if (JsonObject->TryGetStringField(TEXT("message_category"), CategoryStr))
    {
        OutEnvelope.MessageCategory = MACommTypeHelpers::StringToMessageCategory(CategoryStr);
    }
    else
    {
        OutEnvelope.MessageCategory = MACommTypeHelpers::GetCategoryForMessageType(OutEnvelope.MessageType);
    }

    FString DirectionStr;
    if (JsonObject->TryGetStringField(TEXT("direction"), DirectionStr))
    {
        OutEnvelope.Direction = MACommTypeHelpers::StringToMessageDirection(DirectionStr);
    }
    else
    {
        OutEnvelope.Direction = EMAMessageDirection::PythonToUE5;
    }

    FString TimestampStr;
    double TimestampDouble = 0;
    if (JsonObject->TryGetStringField(TEXT("timestamp"), TimestampStr))
    {
        OutEnvelope.TimestampISO = TimestampStr;
        OutEnvelope.Timestamp = ISO8601ToTimestamp(TimestampStr);
    }
    else if (JsonObject->TryGetNumberField(TEXT("timestamp"), TimestampDouble))
    {
        OutEnvelope.Timestamp = static_cast<int64>(TimestampDouble);
        OutEnvelope.TimestampISO = TimestampToISO8601(OutEnvelope.Timestamp);
    }
    else
    {
        OutEnvelope.Timestamp = CurrentTimestampMs();
        OutEnvelope.TimestampISO = TimestampToISO8601(OutEnvelope.Timestamp);
    }

    JsonObject->TryGetStringField(TEXT("message_id"), OutEnvelope.MessageId);

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
        JsonObject->TryGetStringField(TEXT("payload"), OutEnvelope.PayloadJson);
    }

    return true;
}
