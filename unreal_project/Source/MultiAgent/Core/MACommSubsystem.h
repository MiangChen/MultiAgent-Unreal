// MACommSubsystem.h
// 通信子系统 - 负责与外部规划器通信
// Requirements: 2.1, 3.1, 4.1, 4.2, 4.3, 4.4, 5.1, 5.2, 5.3, 5.4, 5.5, 5.6

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "MASimTypes.h"
#include "MACommTypes.h"
#include "MACommSubsystem.generated.h"

/**
 * 通信子系统
 * 
 * 职责:
 * - 与外部规划器通信 (HTTP)
 * - 发送自然语言指令
 * - 发送 UI 输入消息、按钮事件消息、任务反馈消息
 * - 轮询接收入站消息（任务规划 DAG）
 * - 广播规划器响应给订阅者
 * - 支持模拟数据模式用于开发测试
 * 
 * 注意: 世界模型（场景图）现在存储在仿真端本地，由 MASceneGraphManager 管理，
 *       不再通过轮询从后端获取。仿真端通过 scene_change 消息向后端同步场景变化。
 * 
 * Requirements: 2.1, 3.1, 4.1, 4.2, 4.3, 4.4, 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 6.5, 8.1, 8.2, 8.3, 8.4, 8.5
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

    //=========================================================================
    // 命令发送
    //=========================================================================

    /**
     * 发送自然语言指令
     * 根据 bUseMockData 设置决定是发送真实 HTTP 请求还是生成模拟响应
     * @param Command 用户输入的自然语言指令
     * Requirements: 4.2
     */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendNaturalLanguageCommand(const FString& Command);

    //=========================================================================
    // 新增：统一消息发送接口
    // Requirements: 2.1, 3.1, 4.2
    //=========================================================================

    /**
     * 发送 UI 输入消息
     * @param SourceId 输入源标识 (Widget 名称)
     * @param Content 输入内容
     * Requirements: 2.1, 2.4
     */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendUIInputMessage(const FString& SourceId, const FString& Content);

    /**
     * 发送按钮事件消息
     * @param WidgetName 所属界面名称
     * @param ButtonId 按钮标识
     * @param ButtonText 按钮显示文字
     * Requirements: 3.1, 3.5
     */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendButtonEventMessage(const FString& WidgetName, const FString& ButtonId, const FString& ButtonText);

    /**
     * 发送任务反馈消息
     * @param Feedback 任务反馈数据
     * Requirements: 4.2, 4.4
     */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendTaskFeedbackMessage(const FMATaskFeedbackMessage& Feedback);

    /**
     * 发送任务图提交消息
     * 将编辑后的任务图 JSON 发送到后端规划器
     * @param TaskGraphJson 任务图 JSON 字符串
     */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendTaskGraphSubmitMessage(const FString& TaskGraphJson);

    //=========================================================================
    // 事件委托
    //=========================================================================

    /** 
     * 规划器响应委托 - 当收到规划器响应时广播
     * Requirements: 4.3
     */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMAPlannerResponse OnPlannerResponse;

    //=========================================================================
    // 新增：入站消息委托
    // Requirements: 6.5, 7.4
    //=========================================================================

    /**
     * 收到任务规划 DAG 委托
     * 当从规划器收到任务规划 DAG 时广播
     * Requirements: 6.5
     */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMATaskPlanReceived OnTaskPlanReceived;

    /**
     * 收到世界模型更新委托
     * @deprecated 世界模型（场景图）现在存储在仿真端本地，由 MASceneGraphManager 管理，
     *             不再通过轮询从后端获取。此委托保留用于向后兼容，但不会被触发。
     */
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DeprecatedProperty, DeprecationMessage = "World model is now managed locally by MASceneGraphManager"))
    FOnMAWorldModelReceived OnWorldModelReceived;

    //=========================================================================
    // 新增：轮询控制
    // Requirements: 8.1, 8.3
    //=========================================================================

    /**
     * 启动轮询
     * 开始定期从规划器后端轮询消息
     * Requirements: 8.1
     */
    UFUNCTION(BlueprintCallable, Category = "Polling")
    void StartPolling();

    /**
     * 停止轮询
     * 停止定期轮询
     * Requirements: 8.3
     */
    UFUNCTION(BlueprintCallable, Category = "Polling")
    void StopPolling();

    /**
     * 轮询间隔 (秒)
     * 默认 1.0 秒
     * Requirements: 8.5
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polling")
    float PollIntervalSeconds = 1.0f;

    //=========================================================================
    // 场景变化消息发送接口 (Edit Mode)
    // Requirements: 11.1
    //=========================================================================

    /**
     * 发送场景变化消息
     * 用于 Edit Mode 中通知后端规划器场景发生的变化
     * 
     * @param Message 场景变化消息
     * Requirements: 11.1, 11.2, 11.3, 11.4, 11.5, 11.6, 11.7, 11.8
     */
    UFUNCTION(BlueprintCallable, Category = "Communication|SceneChange")
    void SendSceneChangeMessage(const FMASceneChangeMessage& Message);

    /**
     * 发送场景变化消息 (便捷方法)
     * 
     * @param ChangeType 变化类型
     * @param Payload 负载数据 (JSON 格式)
     * Requirements: 11.1
     */
    UFUNCTION(BlueprintCallable, Category = "Communication|SceneChange")
    void SendSceneChangeMessageByType(EMASceneChangeType ChangeType, const FString& Payload);

    /**
     * 是否启用轮询
     * 如果为 true，Initialize() 时自动启动轮询
     * Requirements: 8.4
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polling")
    bool bEnablePolling = true;

    /**
     * 是否正在轮询
     */
    UFUNCTION(BlueprintPure, Category = "Polling")
    bool IsPolling() const { return bIsPolling; }

    //=========================================================================
    // 状态查询
    //=========================================================================

    /** 是否正在等待响应 */
    UFUNCTION(BlueprintPure, Category = "Status")
    bool IsWaitingForResponse() const { return bWaitingForResponse; }

    /** 获取最后发送的指令 */
    UFUNCTION(BlueprintPure, Category = "Status")
    FString GetLastCommand() const { return LastCommand; }

protected:
    //=========================================================================
    // 生命周期
    //=========================================================================

    /** 初始化子系统 - Requirements: 4.1 */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    
    /** 反初始化子系统 */
    virtual void Deinitialize() override;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 
     * 生成模拟响应 (开发测试用)
     * 根据用户指令生成合理的模拟规划结果
     * @param UserCommand 用户输入的指令
     * Requirements: 4.4
     */
    void GenerateMockPlanResponse(const FString& UserCommand);

    /**
     * 广播规划器响应给所有订阅者
     * @param Response 规划器响应数据
     * Requirements: 4.3
     */
    void BroadcastResponse(const FMAPlannerResponse& Response);

    /**
     * 发送消息信封 (内部方法)
     * 统一的消息发送入口，处理 HTTP POST 或 Mock 模式
     * @param Envelope 消息信封
     * Requirements: 5.1, 5.2, 5.6
     */
    void SendMessageEnvelope(const FMAMessageEnvelope& Envelope);

    /**
     * HTTP 请求完成回调
     * 处理 HTTP 响应，包括错误处理和重试逻辑
     * @param Request HTTP 请求对象
     * @param Response HTTP 响应对象
     * @param bConnectedSuccessfully 连接是否成功
     * Requirements: 5.3, 5.4
     */
    void OnHttpRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

    /**
     * 执行 HTTP POST 请求
     * @param Url 完整的请求 URL
     * @param JsonPayload JSON 格式的请求体
     * @param OriginalEnvelope 原始消息信封（用于重试）
     * Requirements: 5.1, 5.2
     */
    void ExecuteHttpPost(const FString& Url, const FString& JsonPayload, const FMAMessageEnvelope& OriginalEnvelope);

    /**
     * 执行重试逻辑
     * @param OriginalEnvelope 原始消息信封
     * Requirements: 5.4
     */
    void ScheduleRetry(const FMAMessageEnvelope& OriginalEnvelope);

    //=========================================================================
    // 轮询相关内部方法
    // Requirements: 8.2, 6.5
    //=========================================================================

    /**
     * 执行轮询请求
     * 发送 GET 请求到 /api/sim/poll
     * Requirements: 8.2
     */
    void PollForMessages();

    /**
     * 处理轮询响应
     * 解析响应 JSON 并分发到对应处理函数
     * 注意: 仅处理 task_plan_dag 消息，world_model_graph 已移至本地管理
     * @param ResponseJson 响应 JSON 字符串
     * Requirements: 6.5
     */
    void HandlePollResponse(const FString& ResponseJson);

    /**
     * 轮询 HTTP 请求完成回调
     * @param Request HTTP 请求对象
     * @param Response HTTP 响应对象
     * @param bConnectedSuccessfully 连接是否成功
     */
    void OnPollRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

private:
    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 是否正在等待响应 */
    bool bWaitingForResponse = false;

    /** 最后发送的指令 */
    FString LastCommand;

    //=========================================================================
    // HTTP 相关私有成员
    // Requirements: 5.3, 5.4
    //=========================================================================

    /** 当前重试计数 */
    int32 RetryCount = 0;

    /** 最大重试次数 */
    static constexpr int32 MaxRetries = 3;

    /** 消息发送端点路径 */
    static constexpr const TCHAR* MessageEndpoint = TEXT("/api/sim/message");

    /** 当前待重试的消息信封 */
    FMAMessageEnvelope PendingEnvelope;

    /** 重试定时器句柄 */
    FTimerHandle RetryTimerHandle;

    /** 获取重试延迟时间（指数退避：1s, 2s, 4s）*/
    float GetRetryDelaySeconds() const;

    /** 从 JSON 配置文件加载配置 */
    void LoadConfigFromJSON();

    //=========================================================================
    // 轮询相关私有成员
    // Requirements: 8.1, 8.3
    //=========================================================================

    /** 轮询定时器句柄 */
    FTimerHandle PollTimerHandle;

    /** 是否正在轮询 */
    bool bIsPolling = false;

    /** 轮询端点路径 */
    static constexpr const TCHAR* PollEndpoint = TEXT("/api/sim/poll");
};
