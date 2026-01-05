// MACommOutbound.h
// 出站消息发送模块 - 负责从 UE5 向外部发送消息

#pragma once

#include "CoreMinimal.h"
#include "MACommTypes.h"

class UMACommSubsystem;

/**
 * 出站消息发送器
 * 
 * 职责:
 * - 封装所有从 UE5 向外部发送消息的逻辑
 * - 构建消息信封
 * - 处理 Mock 模式下的模拟响应
 */
class MULTIAGENT_API FMACommOutbound
{
public:
    FMACommOutbound(UMACommSubsystem* InOwner);

    //=========================================================================
    // UI 消息发送
    //=========================================================================

    /** 发送 UI 输入消息 */
    void SendUIInputMessage(const FString& SourceId, const FString& Content);

    /** 发送按钮事件消息 */
    void SendButtonEventMessage(const FString& WidgetName, const FString& ButtonId, const FString& ButtonText);

    //=========================================================================
    // 任务反馈发送
    //=========================================================================

    /** 发送任务反馈消息 */
    void SendTaskFeedbackMessage(const FMATaskFeedbackMessage& Feedback);

    /** 发送时间步执行反馈 */
    void SendTimeStepFeedback(const FMATimeStepFeedbackMessage& Feedback);

    /** 发送技能列表执行完成反馈 */
    void SendSkillListCompletedFeedback(const FMASkillListCompletedMessage& Message);

    //=========================================================================
    // 任务图发送
    //=========================================================================

    /** 发送任务图提交消息 */
    void SendTaskGraphSubmitMessage(const FString& TaskGraphJson);

    //=========================================================================
    // 场景变化消息发送
    //=========================================================================

    /** 发送场景变化消息 */
    void SendSceneChangeMessage(const FMASceneChangeMessage& Message);

    /** 发送场景变化消息 (便捷方法) */
    void SendSceneChangeMessageByType(EMASceneChangeType ChangeType, const FString& Payload);

    //=========================================================================
    // 世界状态响应发送
    //=========================================================================

    /** 发送世界状态响应 */
    void SendWorldStateResponse(const FString& QueryType, const FString& DataJson);

private:
    /** 所属的通信子系统 */
    UMACommSubsystem* Owner;
};
