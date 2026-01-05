// MACommOutbound.cpp
// 出站消息发送模块实现

#include "MACommOutbound.h"
#include "MACommSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommOutbound, Log, All);

FMACommOutbound::FMACommOutbound(UMACommSubsystem* InOwner)
    : Owner(InOwner)
{
}

//=============================================================================
// UI 消息发送
//=============================================================================

void FMACommOutbound::SendUIInputMessage(const FString& SourceId, const FString& Content)
{
    if (!Owner)
    {
        return;
    }

    if (Content.IsEmpty())
    {
        UE_LOG(LogMACommOutbound, Warning, TEXT("SendUIInputMessage: Empty content ignored"));
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendUIInputMessage: SourceId=%s, Content=%s"), *SourceId, *Content);

    // 创建 UI 输入消息
    FMAUIInputMessage UIInputMsg(SourceId, Content);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::UIInput;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();
    Envelope.PayloadJson = UIInputMsg.ToJson();

    // 委托给 Owner 发送
    Owner->SendMessageEnvelopeInternal(Envelope);
}

void FMACommOutbound::SendButtonEventMessage(const FString& WidgetName, const FString& ButtonId, const FString& ButtonText)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendButtonEventMessage: Widget=%s, ButtonId=%s, ButtonText=%s"), 
        *WidgetName, *ButtonId, *ButtonText);

    // 创建按钮事件消息
    FMAButtonEventMessage ButtonEventMsg(WidgetName, ButtonId, ButtonText);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::ButtonEvent;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();
    Envelope.PayloadJson = ButtonEventMsg.ToJson();

    // 委托给 Owner 发送
    Owner->SendMessageEnvelopeInternal(Envelope);
}

//=============================================================================
// 任务反馈发送
//=============================================================================

void FMACommOutbound::SendTaskFeedbackMessage(const FMATaskFeedbackMessage& Feedback)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendTaskFeedbackMessage: TaskId=%s, Status=%s"), 
        *Feedback.TaskId, *Feedback.Status);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::TaskFeedback;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();
    Envelope.PayloadJson = Feedback.ToJson();

    // 委托给 Owner 发送
    Owner->SendMessageEnvelopeInternal(Envelope);
}

void FMACommOutbound::SendTimeStepFeedback(const FMATimeStepFeedbackMessage& Feedback)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendTimeStepFeedback: TimeStep=%d, FeedbackCount=%d"), 
        Feedback.TimeStep, Feedback.Feedbacks.Num());

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::TaskFeedback;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();
    Envelope.PayloadJson = Feedback.ToJson();

    // 委托给 Owner 发送
    Owner->SendMessageEnvelopeInternal(Envelope);
}

void FMACommOutbound::SendSkillListCompletedFeedback(const FMASkillListCompletedMessage& Message)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendSkillListCompletedFeedback: bCompleted=%s, bInterrupted=%s, TotalTimeSteps=%d, CompletedTimeSteps=%d"),
        Message.bCompleted ? TEXT("true") : TEXT("false"),
        Message.bInterrupted ? TEXT("true") : TEXT("false"),
        Message.TotalTimeSteps,
        Message.CompletedTimeSteps);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::TaskFeedback;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();

    // 构建 payload
    TSharedPtr<FJsonObject> PayloadObject = MakeShareable(new FJsonObject());
    PayloadObject->SetStringField(TEXT("feedback_type"), TEXT("skill_list_completed"));
    PayloadObject->SetBoolField(TEXT("completed"), Message.bCompleted);
    PayloadObject->SetBoolField(TEXT("interrupted"), Message.bInterrupted);
    PayloadObject->SetNumberField(TEXT("total_time_steps"), Message.TotalTimeSteps);
    PayloadObject->SetNumberField(TEXT("completed_time_steps"), Message.CompletedTimeSteps);

    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), Writer);

    Envelope.PayloadJson = PayloadJson;

    // 委托给 Owner 发送
    Owner->SendMessageEnvelopeInternal(Envelope);
}

//=============================================================================
// 任务图发送
//=============================================================================

void FMACommOutbound::SendTaskGraphSubmitMessage(const FString& TaskGraphJson)
{
    if (!Owner)
    {
        return;
    }

    if (TaskGraphJson.IsEmpty())
    {
        UE_LOG(LogMACommOutbound, Warning, TEXT("SendTaskGraphSubmitMessage: Empty task graph JSON ignored"));
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendTaskGraphSubmitMessage: Sending task graph to backend"));
    UE_LOG(LogMACommOutbound, Verbose, TEXT("TaskGraphJson: %s"), *TaskGraphJson);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::TaskGraphSubmit;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();
    Envelope.PayloadJson = TaskGraphJson;

    // 委托给 Owner 发送
    Owner->SendMessageEnvelopeInternal(Envelope);
}

//=============================================================================
// 场景变化消息发送
//=============================================================================

void FMACommOutbound::SendSceneChangeMessage(const FMASceneChangeMessage& Message)
{
    if (!Owner)
    {
        return;
    }

    FString ChangeTypeStr = FMASceneChangeMessage::ChangeTypeToString(Message.ChangeType);

    UE_LOG(LogMACommOutbound, Log, TEXT("SendSceneChangeMessage: Type=%s, MessageId=%s"),
        *ChangeTypeStr, *Message.MessageId);
    UE_LOG(LogMACommOutbound, Verbose, TEXT("Payload: %s"), *Message.Payload);

    // 序列化场景变化消息为 JSON
    FString MessageJson = Message.ToJson();

    // Mock 模式下仅记录日志
    if (Owner->bUseMockData)
    {
        UE_LOG(LogMACommOutbound, Log, TEXT("Mock mode: Scene change message logged but not sent"));
        UE_LOG(LogMACommOutbound, Log, TEXT("  ChangeType: %s"), *ChangeTypeStr);
        UE_LOG(LogMACommOutbound, Verbose, TEXT("  Full JSON: %s"), *MessageJson);
        return;
    }

    // 委托给 Owner 发送场景变化消息
    Owner->SendSceneChangeHttpRequest(MessageJson);
}

void FMACommOutbound::SendSceneChangeMessageByType(EMASceneChangeType ChangeType, const FString& Payload)
{
    FMASceneChangeMessage Message(ChangeType, Payload);
    SendSceneChangeMessage(Message);
}

//=============================================================================
// 世界状态响应发送
//=============================================================================

void FMACommOutbound::SendWorldStateResponse(const FString& QueryType, const FString& DataJson)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendWorldStateResponse: QueryType=%s"), *QueryType);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::WorldState;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();

    // 构建 payload
    TSharedPtr<FJsonObject> PayloadObject = MakeShareable(new FJsonObject());
    PayloadObject->SetStringField(TEXT("query_type"), QueryType);

    // 解析 DataJson 并嵌入
    TSharedPtr<FJsonValue> DataValue;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DataJson);
    if (FJsonSerializer::Deserialize(Reader, DataValue) && DataValue.IsValid())
    {
        PayloadObject->SetField(TEXT("data"), DataValue);
    }
    else
    {
        PayloadObject->SetStringField(TEXT("data"), DataJson);
    }

    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), Writer);

    Envelope.PayloadJson = PayloadJson;

    // 委托给 Owner 发送
    Owner->SendMessageEnvelopeInternal(Envelope);
}
