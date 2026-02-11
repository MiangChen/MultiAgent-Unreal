// MACommInbound.h
// 入站消息处理模块 - 负责处理从外部接收的消息

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "MACommTypes.h"

class UMACommSubsystem;

/**
 * 入站消息处理器
 * 
 * 职责:
 * - 轮询外部服务获取消息 (Platform 和 HITL 端点)
 * - 解析入站消息
 * - 根据消息类别分发到对应处理函数
 */
class MULTIAGENT_API FMACommInbound
{
public:
    FMACommInbound(UMACommSubsystem* InOwner);

    //=========================================================================
    // 轮询控制
    //=========================================================================

    /** 启动轮询 (同时启动 Platform 和 HITL 端点轮询) */
    void StartPolling();

    /** 停止轮询 */
    void StopPolling();

    /** 是否正在轮询 */
    bool IsPolling() const { return bIsPolling; }

    /** 设置轮询间隔 */
    void SetPollInterval(float IntervalSeconds) { PollIntervalSeconds = IntervalSeconds; }

    /** 设置 HITL 轮询间隔 (可独立配置) */
    void SetHITLPollInterval(float IntervalSeconds) { HITLPollIntervalSeconds = IntervalSeconds; }

    /** 设置是否启用 HITL 轮询 */
    void SetEnableHITLPolling(bool bEnable) { bEnableHITLPolling = bEnable; }

    //=========================================================================
    // 消息处理
    //=========================================================================

    /** 处理轮询响应 (根据 message_category 分发) */
    void HandlePollResponse(const FString& ResponseJson);

private:
    //=========================================================================
    // 轮询实现
    //=========================================================================

    /** 执行 Platform 端点轮询请求 (/api/sim/poll) */
    void PollForMessages();

    /** 执行 HITL 端点轮询请求 (/api/hitl/poll) */
    void PollForHITLMessages();

    /** Platform 轮询 HTTP 请求完成回调 */
    void OnPollRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

    /** HITL 轮询 HTTP 请求完成回调 */
    void OnHITLPollRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

    //=========================================================================
    // 消息分发 - 根据消息类别
    //=========================================================================

    /** 处理 HITL 类别消息 (Instruction, Review, Decision) */
    void HandleHITLMessage(const TSharedPtr<FJsonObject>& MsgObject, EMAMessageCategory Category);

    /** 处理 Platform 类别消息 */
    void HandlePlatformMessage(const TSharedPtr<FJsonObject>& MsgObject);

    //=========================================================================
    // 具体消息类型处理
    //=========================================================================

    /** 处理任务图消息 (来自 mock_backend) */
    void HandleTaskGraph(const TSharedPtr<FJsonObject>& PayloadObject);

    /** 处理技能列表消息 (PLATFORM 类别 - 直接执行) */
    void HandleSkillList(const TSharedPtr<FJsonObject>& PayloadObject, bool bExecutable);

    /** 处理技能分配消息 (REVIEW 类别 - UI 交互流程) */
    void HandleSkillAllocation(const TSharedPtr<FJsonObject>& PayloadObject);

    /** 处理索要用户指令消息 */
    void HandleRequestUserCommand(const TSharedPtr<FJsonObject>& PayloadObject);


    /** 处理决策请求消息
     * @param PayloadObject 消息 payload，包含 description, context, decision_type 字段
     */
    void HandleDecision(const TSharedPtr<FJsonObject>& PayloadObject);

private:
    /** 所属的通信子系统 */
    UMACommSubsystem* Owner;

    /** Platform 轮询定时器句柄 */
    FTimerHandle PollTimerHandle;

    /** HITL 轮询定时器句柄 */
    FTimerHandle HITLPollTimerHandle;

    /** 是否正在轮询 */
    bool bIsPolling = false;

    /** 是否启用 HITL 轮询 */
    bool bEnableHITLPolling = true;

    /** Platform 轮询间隔 (秒) */
    float PollIntervalSeconds = 1.0f;

    /** HITL 轮询间隔 (秒) - 可独立配置 */
    float HITLPollIntervalSeconds = 1.0f;

    /** Platform 轮询端点路径 */
    static constexpr const TCHAR* PlatformPollEndpoint = TEXT("/api/sim/poll");

    /** HITL 轮询端点路径 */
    static constexpr const TCHAR* HITLPollEndpoint = TEXT("/api/hitl/poll");

    /** HTTP 请求超时时间 (秒) - 需要足够长以等待 Python 端 LLM 处理 */
    static constexpr float HttpRequestTimeoutSeconds = 30.0f;
};
