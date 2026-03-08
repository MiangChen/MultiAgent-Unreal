#include "MACommJsonCodec.h"
#include "Misc/Guid.h"
#include "Misc/DateTime.h"

namespace
{
    int64 CurrentTimestampMs()
    {
        return FDateTime::UtcNow().ToUnixTimestamp() * 1000 + FDateTime::UtcNow().GetMillisecond();
    }

    FString NewMessageId()
    {
        return FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
    }
}

FMAMessageEnvelope MACommJsonCodec::MakeOutboundEnvelope(
    EMACommMessageType MessageType,
    EMAMessageCategory MessageCategory,
    const FString& PayloadJson,
    int64 Timestamp,
    const FString& MessageId)
{
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = MessageType;
    Envelope.MessageCategory = MessageCategory;
    Envelope.Direction = EMAMessageDirection::UE5ToPython;
    Envelope.Timestamp = Timestamp > 0 ? Timestamp : CurrentTimestampMs();
    Envelope.MessageId = MessageId.IsEmpty() ? NewMessageId() : MessageId;
    Envelope.PayloadJson = PayloadJson;
    return Envelope;
}
