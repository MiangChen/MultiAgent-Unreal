// MACommOutbound.cpp
// 出站消息发送模块实现

#include "Core/Comm/Application/MACommOutbound.h"
#include "Core/Comm/Runtime/MACommSubsystem.h"
#include "Core/Comm/Infrastructure/Codec/MACommJsonCodec.h"

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

    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::UIInput,
        EMAMessageCategory::Instruction,
        MACommJsonCodec::SerializeUIInput(UIInputMsg));

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

    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::ButtonEvent,
        EMAMessageCategory::Instruction,
        MACommJsonCodec::SerializeButtonEvent(ButtonEventMsg));

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

    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::TaskFeedback,
        EMAMessageCategory::Platform,
        MACommJsonCodec::SerializeTaskFeedback(Feedback));

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

    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::TaskFeedback,
        EMAMessageCategory::Platform,
        MACommJsonCodec::SerializeTimeStepFeedback(Feedback));

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

    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::TaskFeedback,
        EMAMessageCategory::Platform,
        MACommJsonCodec::SerializeSkillListCompleted(Message));

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

    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::TaskGraph,
        EMAMessageCategory::Review,
        TaskGraphJson);

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

    const FString ChangeTypeStr = MACommJsonCodec::SceneChangeTypeToString(Message.ChangeType);

    UE_LOG(LogMACommOutbound, Log, TEXT("SendSceneChangeMessage: Type=%s, MessageId=%s"),
        *ChangeTypeStr, *Message.MessageId);
    UE_LOG(LogMACommOutbound, Verbose, TEXT("Payload: %s"), *Message.Payload);

    // 序列化场景变化消息为 JSON
    const FString MessageJson = MACommJsonCodec::SerializeSceneChange(Message);

    // Mock 模式下仅记录日志
    if (Owner->bUseMockData)
    {
        UE_LOG(LogMACommOutbound, Log, TEXT("Mock mode: Scene change message logged but not sent"));
        UE_LOG(LogMACommOutbound, Log, TEXT("  ChangeType: %s"), *ChangeTypeStr);
        UE_LOG(LogMACommOutbound, Verbose, TEXT("  Full JSON: %s"), *MessageJson);
        return;
    }

    // 创建消息信封用于 HITL 格式发送
    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::SceneChange,
        EMAMessageCategory::Platform,
        Message.Payload,
        Message.Timestamp,
        Message.MessageId);

    // 委托给 Owner 发送场景变化消息 (使用专用端点)
    Owner->SendSceneChangeHttpRequest(MACommJsonCodec::SerializeEnvelope(Envelope));
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

    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::SkillAllocation,
        EMAMessageCategory::Review,
        MACommJsonCodec::SerializeSkillAllocation(Message),
        Message.Timestamp,
        Message.MessageId);

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

    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::SkillAllocation,
        EMAMessageCategory::Review,
        MACommJsonCodec::SerializeReviewResponse(Response),
        Response.Timestamp,
        Response.MessageId);

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

    const FMAMessageEnvelope Envelope = MACommJsonCodec::MakeOutboundEnvelope(
        EMACommMessageType::Custom,
        EMAMessageCategory::Decision,
        MACommJsonCodec::SerializeDecisionResponse(Response),
        Response.Timestamp,
        Response.MessageId);

    // 委托给 Owner 发送 (使用 HITL 端点)
    Owner->SendMessageEnvelopeInternal(Envelope);
}

void FMACommOutbound::SendDecisionResponseSimple(const FString& Decision,
    const FString& DecisionDataJson, const FString& UserFeedback)
{
    FMADecisionResponseMessage Response(Decision, DecisionDataJson, UserFeedback);
    SendDecisionResponse(Response);
}
