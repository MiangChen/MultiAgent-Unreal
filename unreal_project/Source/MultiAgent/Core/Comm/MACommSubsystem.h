// MACommSubsystem.h
// 通信子系统 - 负责与外部规划器通信
//
// 模块划分:
// - MACommSubsystem: 主控制器，管理生命周期和配置
// - MACommOutbound: 出站消息发送 (UE5 -> 外部)
// - MACommInbound: 入站消息处理 (外部 -> UE5)
// - MACommHttpServer: HTTP 服务器 (被动接收请求)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "../Types/MASimTypes.h"
#include "MACommTypes.h"
#include "MACommOutbound.h"
#include "MACommInbound.h"
#include "MACommHttpServer.h"
#include "MACommSubsystem.generated.h"

/**
 * 通信子系统
 * 
 * 职责:
 * - 管理通信模块的生命周期
 * - 提供统一的对外接口
 * - 管理 HTTP 客户端通信
 * - 广播规划器响应给订阅者
 */
UCLASS()
class MULTIAGENT_API UMACommSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 配置
    //=========================================================================

    /** 规划器服务器 URL */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
    FString ServerURL = TEXT("http://localhost:8080");

    /** 是否使用模拟数据 (开发测试用) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
    bool bUseMockData = true;

    /** 是否启用轮询 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polling")
    bool bEnablePolling = true;

    /** 是否启用 HITL 端点轮询 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polling")
    bool bEnableHITLPolling = false;

    /** 轮询间隔 (秒) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polling")
    float PollIntervalSeconds = 1.0f;

    /** HITL 轮询间隔 (秒) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polling")
    float HITLPollIntervalSeconds = 1.0f;

    /** 本地 HTTP 服务器端口 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server")
    int32 LocalServerPort = 8080;

    /** 是否启用本地 HTTP 服务器 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server")
    bool bEnableLocalServer = true;

    //=========================================================================
    // 出站消息发送接口 (委托给 MACommOutbound)
    //=========================================================================

    /** 发送自然语言指令 (向后兼容) */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendNaturalLanguageCommand(const FString& Command);

    /** 发送 UI 输入消息 (委托给 Outbound) */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendUIInputMessage(const FString& SourceId, const FString& Content);

    /** 发送按钮事件消息 */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendButtonEventMessage(const FString& WidgetName, const FString& ButtonId, const FString& ButtonText);

    /** 发送任务反馈消息 */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendTaskFeedbackMessage(const FMATaskFeedbackMessage& Feedback);

    /** 发送时间步执行反馈 */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendTimeStepFeedback(const FMATimeStepFeedbackMessage& Feedback);

    /** 发送技能列表执行完成反馈 */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendSkillListCompletedFeedback(const FMASkillListCompletedMessage& Message);

    /** 发送任务图提交消息 */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendTaskGraphSubmitMessage(const FString& TaskGraphJson);

    /** 发送场景变化消息 */
    UFUNCTION(BlueprintCallable, Category = "Communication|SceneChange")
    void SendSceneChangeMessage(const FMASceneChangeMessage& Message);

    /** 发送场景变化消息 (便捷方法) */
    UFUNCTION(BlueprintCallable, Category = "Communication|SceneChange")
    void SendSceneChangeMessageByType(EMASceneChangeType ChangeType, const FString& Payload);

    /** 发送技能分配消息 */
    UFUNCTION(BlueprintCallable, Category = "Communication|SkillAllocation")
    void SendSkillAllocationMessage(const FMASkillAllocationMessage& Message);

    //=========================================================================
    // HITL 响应发送接口 (委托给 MACommOutbound)
    //=========================================================================

    /** 发送审阅响应消息 */
    UFUNCTION(BlueprintCallable, Category = "Communication|HITL")
    void SendReviewResponse(const FMAReviewResponseMessage& Response);

    /** 发送审阅响应消息 (便捷方法) */
    UFUNCTION(BlueprintCallable, Category = "Communication|HITL")
    void SendReviewResponseSimple(bool bApproved,
        const FString& ModifiedDataJson = TEXT(""), const FString& RejectionReason = TEXT(""));

    /** 发送决策响应消息 */
    UFUNCTION(BlueprintCallable, Category = "Communication|HITL")
    void SendDecisionResponse(const FMADecisionResponseMessage& Response);

    /** 发送决策响应消息 (便捷方法) */
    UFUNCTION(BlueprintCallable, Category = "Communication|HITL")
    void SendDecisionResponseSimple(const FString& Decision,
        const FString& DecisionDataJson = TEXT(""), const FString& Comments = TEXT(""));

    //=========================================================================
    // 轮询控制接口 (委托给 MACommInbound)
    //=========================================================================

    /** 启动轮询 */
    UFUNCTION(BlueprintCallable, Category = "Polling")
    void StartPolling();

    /** 停止轮询 */
    UFUNCTION(BlueprintCallable, Category = "Polling")
    void StopPolling();

    /** 是否正在轮询 */
    UFUNCTION(BlueprintPure, Category = "Polling")
    bool IsPolling() const;

    //=========================================================================
    // HTTP 服务器控制接口 (委托给 MACommHttpServer)
    //=========================================================================

    /** 启动 HTTP 服务器 */
    UFUNCTION(BlueprintCallable, Category = "Server")
    void StartHttpServer();

    /** 停止 HTTP 服务器 */
    UFUNCTION(BlueprintCallable, Category = "Server")
    void StopHttpServer();

    /** 服务器是否运行中 */
    UFUNCTION(BlueprintPure, Category = "Server")
    bool IsHttpServerRunning() const;

    //=========================================================================
    // 事件委托
    //=========================================================================

    /** 规划器响应委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMAPlannerResponse OnPlannerResponse;

    /** 收到任务规划 DAG 委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMATaskPlanReceived OnTaskPlanReceived;

    /** 收到技能分配数据委托 (用于 UI 交互流程) */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMASkillAllocationDataReceived OnSkillAllocationReceived;

    /** 收到索要用户指令请求委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMARequestUserCommandReceived OnRequestUserCommandReceived;

    /** 收到技能列表委托 (PLATFORM 类别 - 直接执行) */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMASkillListReceived OnSkillListReceived;

    //=========================================================================
    // 状态查询
    //=========================================================================

    /** 是否正在等待响应 */
    UFUNCTION(BlueprintPure, Category = "Status")
    bool IsWaitingForResponse() const { return bWaitingForResponse; }

    /** 获取最后发送的指令 */
    UFUNCTION(BlueprintPure, Category = "Status")
    FString GetLastCommand() const { return LastCommand; }

    //=========================================================================
    // 模块访问器
    //=========================================================================

    /** 获取出站消息模块 */
    FMACommOutbound* GetOutbound() const { return Outbound.Get(); }

    /** 获取入站消息模块 */
    FMACommInbound* GetInbound() const { return Inbound.Get(); }

protected:
    //=========================================================================
    // 生命周期
    //=========================================================================

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    //=========================================================================
    // 内部方法 - 供子模块调用
    //=========================================================================

    friend class FMACommOutbound;
    friend class FMACommInbound;

    /** 发送消息信封 (内部方法) */
    void SendMessageEnvelopeInternal(const FMAMessageEnvelope& Envelope);

    /** 发送 HITL 消息 (内部方法) - 用于 Instruction, Review, Decision 类别 */
    void SendHITLMessageInternal(const FMAMessageEnvelope& Envelope);

    /** 根据消息类别获取发送端点 */
    FString GetEndpointForCategory(EMAMessageCategory Category) const;

    /** 发送场景变化 HTTP 请求 (内部方法) */
    void SendSceneChangeHttpRequest(const FString& MessageJson);

    /** 广播规划器响应 */
    void BroadcastResponse(const FMAPlannerResponse& Response);

    /** 生成模拟响应 */
    void GenerateMockPlanResponse(const FString& UserCommand);

private:
    //=========================================================================
    // HTTP 客户端通信
    //=========================================================================

    /** 执行 HTTP POST 请求 */
    void ExecuteHttpPost(const FString& Url, const FString& JsonPayload, const FMAMessageEnvelope& OriginalEnvelope);

    /** HTTP 请求完成回调 */
    void OnHttpRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

    /** 执行重试逻辑 */
    void ScheduleRetry(const FMAMessageEnvelope& OriginalEnvelope);

    /** 获取重试延迟时间 */
    float GetRetryDelaySeconds() const;

private:
    //=========================================================================
    // 子模块
    //=========================================================================

    /** 出站消息模块 */
    TUniquePtr<FMACommOutbound> Outbound;

    /** 入站消息模块 */
    TUniquePtr<FMACommInbound> Inbound;

    /** HTTP 服务器模块 */
    TUniquePtr<FMACommHttpServer> HttpServer;

    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 是否正在等待响应 */
    bool bWaitingForResponse = false;

    /** 最后发送的指令 */
    FString LastCommand;

    /** 当前重试计数 */
    int32 RetryCount = 0;

    /** 最大重试次数 */
    static constexpr int32 MaxRetries = 3;

    /** 消息发送端点路径 - Platform 类别 */
    static constexpr const TCHAR* PlatformMessageEndpoint = TEXT("/api/sim/message");

    /** 消息发送端点路径 - HITL 类别 (Instruction, Review, Decision) */
    static constexpr const TCHAR* HITLMessageEndpoint = TEXT("/api/hitl/message");

    /** 当前待重试的消息信封 */
    FMAMessageEnvelope PendingEnvelope;

    /** 重试定时器句柄 */
    FTimerHandle RetryTimerHandle;
};
