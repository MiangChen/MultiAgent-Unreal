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
 * - 轮询外部服务获取消息
 * - 解析入站消息
 * - 分发消息到对应处理函数
 */
class MULTIAGENT_API FMACommInbound
{
public:
    FMACommInbound(UMACommSubsystem* InOwner);

    //=========================================================================
    // 轮询控制
    //=========================================================================

    /** 启动轮询 */
    void StartPolling();

    /** 停止轮询 */
    void StopPolling();

    /** 是否正在轮询 */
    bool IsPolling() const { return bIsPolling; }

    /** 设置轮询间隔 */
    void SetPollInterval(float IntervalSeconds) { PollIntervalSeconds = IntervalSeconds; }

    //=========================================================================
    // 消息处理
    //=========================================================================

    /** 处理轮询响应 */
    void HandlePollResponse(const FString& ResponseJson);

private:
    //=========================================================================
    // 轮询实现
    //=========================================================================

    /** 执行轮询请求 */
    void PollForMessages();

    /** 轮询 HTTP 请求完成回调 */
    void OnPollRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

    //=========================================================================
    // 消息分发
    //=========================================================================

    /** 处理任务规划 DAG 消息 */
    void HandleTaskPlanDAG(const TSharedPtr<FJsonObject>& PayloadObject);

    /** 处理任务图消息 (新格式，来自 mock_backend) */
    void HandleTaskGraph(const TSharedPtr<FJsonObject>& PayloadObject);

    /** 处理世界模型图消息 (已废弃，保留兼容) */
    void HandleWorldModelGraph(const TSharedPtr<FJsonObject>& PayloadObject);

    /** 处理技能列表消息
     * @param PayloadObject 消息 payload
     * @param bExecutable 是否为可执行的最终技能列表，如果为 true 则直接触发执行
     */
    void HandleSkillList(const TSharedPtr<FJsonObject>& PayloadObject, bool bExecutable = false);

    /** 处理技能状态更新消息 */
    void HandleSkillStatusUpdate(const TSharedPtr<FJsonObject>& PayloadObject);

    /** 处理查询请求消息 */
    void HandleQueryRequest(const TSharedPtr<FJsonObject>& PayloadObject);

    /** 处理索要用户指令消息 */
    void HandleRequestUserCommand(const TSharedPtr<FJsonObject>& PayloadObject);

private:
    /** 所属的通信子系统 */
    UMACommSubsystem* Owner;

    /** 轮询定时器句柄 */
    FTimerHandle PollTimerHandle;

    /** 是否正在轮询 */
    bool bIsPolling = false;

    /** 轮询间隔 (秒) */
    float PollIntervalSeconds = 1.0f;

    /** 轮询端点路径 */
    static constexpr const TCHAR* PollEndpoint = TEXT("/api/sim/poll");
};
