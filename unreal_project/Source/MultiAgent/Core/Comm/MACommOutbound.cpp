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
    Envelope.MessageCategory = EMAMessageCategory::Instruction;  // UIInput 属于 Instruction 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
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
    Envelope.MessageCategory = EMAMessageCategory::Instruction;  // ButtonEvent 属于 Instruction 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
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
    Envelope.MessageCategory = EMAMessageCategory::Platform;     // TaskFeedback 属于 Platform 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
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
    Envelope.MessageCategory = EMAMessageCategory::Platform;     // TimeStepFeedback 属于 Platform 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
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
    Envelope.MessageCategory = EMAMessageCategory::Platform;     // SkillListCompleted 属于 Platform 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
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
    Envelope.MessageType = EMACommMessageType::TaskGraph;
    Envelope.MessageCategory = EMAMessageCategory::Review;       // TaskGraph 属于 Review 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
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

    // 创建消息信封用于 HITL 格式发送
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::SceneChange;
    Envelope.MessageCategory = EMAMessageCategory::Platform;     // SceneChange 属于 Platform 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
    Envelope.Timestamp = Message.Timestamp;
    Envelope.MessageId = Message.MessageId;
    Envelope.PayloadJson = Message.Payload;

    // 委托给 Owner 发送场景变化消息 (使用专用端点)
    Owner->SendSceneChangeHttpRequest(Envelope.ToJson());
}

void FMACommOutbound::SendSceneChangeMessageByType(EMASceneChangeType ChangeType, const FString& Payload)
{
    FMASceneChangeMessage Message(ChangeType, Payload);
    SendSceneChangeMessage(Message);
}


//=============================================================================
// 技能分配消息发送
//=============================================================================

void FMACommOutbound::SendSkillAllocationMessage(const FMASkillAllocationMessage& Message)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendSkillAllocationMessage: Name=%s, Description=%s"), 
        *Message.Name, *Message.Description);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::SkillAllocation;
    Envelope.MessageCategory = EMAMessageCategory::Review;       // SkillAllocation 属于 Review 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
    Envelope.Timestamp = Message.Timestamp;
    Envelope.MessageId = Message.MessageId;
    Envelope.PayloadJson = Message.ToJson();

    // 委托给 Owner 发送
    Owner->SendMessageEnvelopeInternal(Envelope);
}


//=============================================================================
// HITL 响应消息发送
//=============================================================================

void FMACommOutbound::SendReviewResponse(const FMAReviewResponseMessage& Response)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendReviewResponse: Approved=%s"),
        Response.bApproved ? TEXT("true") : TEXT("false"));

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::SkillAllocation;  // 使用 SkillAllocation 类型表示审阅响应
    Envelope.MessageCategory = EMAMessageCategory::Review;       // Review 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
    Envelope.Timestamp = Response.Timestamp;
    Envelope.MessageId = Response.MessageId;
    Envelope.PayloadJson = Response.ToJson();

    // 委托给 Owner 发送 (使用 HITL 端点)
    Owner->SendMessageEnvelopeInternal(Envelope);
}

void FMACommOutbound::SendReviewResponseSimple(bool bApproved,
    const FString& ModifiedDataJson, const FString& RejectionReason)
{
    FMAReviewResponseMessage Response(bApproved, ModifiedDataJson, RejectionReason);
    SendReviewResponse(Response);
}

void FMACommOutbound::SendDecisionResponse(const FMADecisionResponseMessage& Response)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommOutbound, Log, TEXT("SendDecisionResponse: Decision=%s"),
        *Response.Decision);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::Custom;           // 使用 Custom 类型表示决策响应
    Envelope.MessageCategory = EMAMessageCategory::Decision;     // Decision 类别
    Envelope.Direction = EMAMessageDirection::UE5ToPython;       // 出站消息方向
    Envelope.Timestamp = Response.Timestamp;
    Envelope.MessageId = Response.MessageId;
    Envelope.PayloadJson = Response.ToJson();

    // 委托给 Owner 发送 (使用 HITL 端点)
    Owner->SendMessageEnvelopeInternal(Envelope);
}

void FMACommOutbound::SendDecisionResponseSimple(const FString& Decision,
    const FString& DecisionDataJson, const FString& Comments)
{
    FMADecisionResponseMessage Response(Decision, DecisionDataJson, Comments);
    SendDecisionResponse(Response);
}
