// MACommSubsystem.cpp
// 通信子系统实现
// Requirements: 2.1, 2.4, 3.1, 3.5, 4.1, 4.2, 4.3, 4.4, 5.1, 5.2, 5.3, 5.4, 5.6, 6.5, 7.4, 8.1, 8.2, 8.3, 8.4, 8.5

#include "MACommSubsystem.h"
#include "MAGameInstance.h"
#include "MACommTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommSubsystem, Log, All);

//=============================================================================
// 生命周期
//=============================================================================

void UMACommSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 直接从 JSON 配置文件加载，避免初始化顺序问题
    // Requirements: 4.1, 5.5 - 支持可配置的服务器 URL 和 Mock 模式
    LoadConfigFromJSON();

    UE_LOG(LogMACommSubsystem, Log, TEXT("MACommSubsystem initialized"));
    UE_LOG(LogMACommSubsystem, Log, TEXT("  ServerURL: %s"), *ServerURL);
    UE_LOG(LogMACommSubsystem, Log, TEXT("  bUseMockData: %s"), bUseMockData ? TEXT("true") : TEXT("false"));
    UE_LOG(LogMACommSubsystem, Log, TEXT("  bEnablePolling: %s"), bEnablePolling ? TEXT("true") : TEXT("false"));
    UE_LOG(LogMACommSubsystem, Log, TEXT("  PollIntervalSeconds: %.2f"), PollIntervalSeconds);

    // Requirements: 8.4 - 如果配置启用，自动启动轮询
    if (bEnablePolling && !bUseMockData)
    {
        StartPolling();
    }
}

void UMACommSubsystem::LoadConfigFromJSON()
{
    // 配置文件路径: 项目根目录/config/SimConfig.json
    FString ConfigPath = FPaths::ProjectDir() / TEXT("config/SimConfig.json");
    
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("Failed to load config file: %s, using defaults"), *ConfigPath);
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("Failed to parse JSON config: %s"), *ConfigPath);
        return;
    }

    // 读取配置项
    if (JsonObject->HasField(TEXT("PlannerServerURL")))
    {
        ServerURL = JsonObject->GetStringField(TEXT("PlannerServerURL"));
    }

    if (JsonObject->HasField(TEXT("bUseMockData")))
    {
        bUseMockData = JsonObject->GetBoolField(TEXT("bUseMockData"));
    }

    // Requirements: 8.1, 8.5 - 读取轮询配置
    if (JsonObject->HasField(TEXT("bEnablePolling")))
    {
        bEnablePolling = JsonObject->GetBoolField(TEXT("bEnablePolling"));
    }

    if (JsonObject->HasField(TEXT("PollIntervalSeconds")))
    {
        PollIntervalSeconds = static_cast<float>(JsonObject->GetNumberField(TEXT("PollIntervalSeconds")));
    }

    UE_LOG(LogMACommSubsystem, Log, TEXT("Loaded config from: %s"), *ConfigPath);
}

void UMACommSubsystem::Deinitialize()
{
    UE_LOG(LogMACommSubsystem, Log, TEXT("MACommSubsystem deinitializing"));
    
    // 停止轮询
    StopPolling();
    
    // 清理重试定时器
    if (RetryTimerHandle.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(RetryTimerHandle);
        }
    }
    
    // 清理状态
    bWaitingForResponse = false;
    LastCommand.Empty();
    RetryCount = 0;
    
    Super::Deinitialize();
}

//=============================================================================
// 命令发送
//=============================================================================

void UMACommSubsystem::SendNaturalLanguageCommand(const FString& Command)
{
    // Requirements: 4.2, 9.3 - 收到自然语言指令时转换为 UI 输入消息发送
    // 向后兼容：内部调用 SendUIInputMessage，使用 "legacy_command" 作为 input_source_id
    
    if (Command.IsEmpty())
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("SendNaturalLanguageCommand: Empty command ignored"));
        
        // 广播失败响应
        BroadcastResponse(FMAPlannerResponse::Failure(TEXT("指令不能为空")));
        return;
    }

    UE_LOG(LogMACommSubsystem, Log, TEXT("SendNaturalLanguageCommand: %s"), *Command);
    
    // 保存指令并设置等待状态
    LastCommand = Command;
    bWaitingForResponse = true;

    // Requirements: 9.3 - 内部转换为 UI 输入消息
    // 使用 "legacy_command" 作为 input_source_id 以保持向后兼容
    SendUIInputMessage(TEXT("legacy_command"), Command);
}

//=============================================================================
// 内部方法
//=============================================================================

void UMACommSubsystem::GenerateMockPlanResponse(const FString& UserCommand)
{
    // Requirements: 4.4 - 生成模拟数据用于开发测试
    
    FMAPlannerResponse Response;
    Response.bSuccess = true;
    
    // 根据指令内容生成不同的模拟响应
    FString LowerCommand = UserCommand.ToLower();
    
    if (LowerCommand.Contains(TEXT("move")) || LowerCommand.Contains(TEXT("移动")) || LowerCommand.Contains(TEXT("go")))
    {
        Response.Message = TEXT("移动指令已接收");
        Response.PlanText = FString::Printf(
            TEXT("规划结果:\n")
            TEXT("1. 解析目标位置\n")
            TEXT("2. 计算最优路径\n")
            TEXT("3. 执行移动任务\n")
            TEXT("\n原始指令: %s"), *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("patrol")) || LowerCommand.Contains(TEXT("巡逻")))
    {
        Response.Message = TEXT("巡逻指令已接收");
        Response.PlanText = FString::Printf(
            TEXT("规划结果:\n")
            TEXT("1. 设置巡逻路径点\n")
            TEXT("2. 配置巡逻参数\n")
            TEXT("3. 开始循环巡逻\n")
            TEXT("\n原始指令: %s"), *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("stop")) || LowerCommand.Contains(TEXT("停止")))
    {
        Response.Message = TEXT("停止指令已接收");
        Response.PlanText = FString::Printf(
            TEXT("规划结果:\n")
            TEXT("1. 中断当前任务\n")
            TEXT("2. 停止移动\n")
            TEXT("3. 进入待命状态\n")
            TEXT("\n原始指令: %s"), *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("status")) || LowerCommand.Contains(TEXT("状态")))
    {
        Response.Message = TEXT("状态查询已处理");
        Response.PlanText = FString::Printf(
            TEXT("系统状态:\n")
            TEXT("- 通信状态: 正常\n")
            TEXT("- 模拟模式: 已启用\n")
            TEXT("- 服务器: %s\n")
            TEXT("\n原始指令: %s"), *ServerURL, *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("help")) || LowerCommand.Contains(TEXT("帮助")))
    {
        Response.Message = TEXT("帮助信息");
        Response.PlanText = TEXT(
            "可用指令:\n"
            "- move/移动 [目标]: 移动到指定位置\n"
            "- patrol/巡逻: 开始巡逻任务\n"
            "- stop/停止: 停止当前任务\n"
            "- status/状态: 查询系统状态\n"
            "- help/帮助: 显示帮助信息");
    }
    else
    {
        // 默认响应
        Response.Message = FString::Printf(TEXT("收到指令: %s"), *UserCommand);
        Response.PlanText = FString::Printf(
            TEXT("规划结果:\n")
            TEXT("指令已记录，等待进一步处理。\n")
            TEXT("\n原始指令: %s"), *UserCommand);
    }

    // 广播响应
    BroadcastResponse(Response);

    UE_LOG(LogMACommSubsystem, Log, TEXT("Mock response generated - Success: %s, Message: %s"), 
        Response.bSuccess ? TEXT("true") : TEXT("false"), *Response.Message);
}

void UMACommSubsystem::BroadcastResponse(const FMAPlannerResponse& Response)
{
    // Requirements: 4.3 - 通过委托广播响应给订阅者
    
    bWaitingForResponse = false;
    
    // 广播给所有订阅者
    OnPlannerResponse.Broadcast(Response);
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("Response broadcasted to subscribers"));
}

//=============================================================================
// 统一消息发送接口
// Requirements: 2.1, 2.4, 3.1, 3.5, 4.2, 4.4
//=============================================================================

void UMACommSubsystem::SendUIInputMessage(const FString& SourceId, const FString& Content)
{
    // Requirements: 2.1, 2.4 - 创建 UI 输入消息并包装为信封
    
    if (Content.IsEmpty())
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("SendUIInputMessage: Empty content ignored"));
        return;
    }

    UE_LOG(LogMACommSubsystem, Log, TEXT("SendUIInputMessage: SourceId=%s, Content=%s"), *SourceId, *Content);

    // 创建 UI 输入消息
    FMAUIInputMessage UIInputMsg(SourceId, Content);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::UIInput;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();
    Envelope.PayloadJson = UIInputMsg.ToJson();

    // 发送消息信封
    SendMessageEnvelope(Envelope);
}

void UMACommSubsystem::SendButtonEventMessage(const FString& WidgetName, const FString& ButtonId, const FString& ButtonText)
{
    // Requirements: 3.1, 3.5 - 创建按钮事件消息并包装为信封
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("SendButtonEventMessage: Widget=%s, ButtonId=%s, ButtonText=%s"), 
        *WidgetName, *ButtonId, *ButtonText);

    // 创建按钮事件消息
    FMAButtonEventMessage ButtonEventMsg(WidgetName, ButtonId, ButtonText);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::ButtonEvent;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();
    Envelope.PayloadJson = ButtonEventMsg.ToJson();

    // 发送消息信封
    SendMessageEnvelope(Envelope);
}

void UMACommSubsystem::SendTaskFeedbackMessage(const FMATaskFeedbackMessage& Feedback)
{
    // Requirements: 4.2, 4.4 - 创建任务反馈消息并包装为信封
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("SendTaskFeedbackMessage: TaskId=%s, Status=%s"), 
        *Feedback.TaskId, *Feedback.Status);

    // 创建消息信封
    FMAMessageEnvelope Envelope;
    Envelope.MessageType = EMACommMessageType::TaskFeedback;
    Envelope.Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
    Envelope.MessageId = FMAMessageEnvelope::GenerateMessageId();
    Envelope.PayloadJson = Feedback.ToJson();

    // 发送消息信封
    SendMessageEnvelope(Envelope);
}

//=============================================================================
// 场景变化消息发送接口 (Edit Mode)
// Requirements: 11.1, 11.2, 11.3, 11.4, 11.5, 11.6, 11.7, 11.8
//=============================================================================

void UMACommSubsystem::SendSceneChangeMessage(const FMASceneChangeMessage& Message)
{
    // Requirements: 11.1 - 通过 MACommSubsystem 发送场景变化消息
    
    FString ChangeTypeStr = FMASceneChangeMessage::ChangeTypeToString(Message.ChangeType);
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("SendSceneChangeMessage: Type=%s, MessageId=%s"), 
        *ChangeTypeStr, *Message.MessageId);
    UE_LOG(LogMACommSubsystem, Verbose, TEXT("Payload: %s"), *Message.Payload);

    // 序列化场景变化消息为 JSON
    FString MessageJson = Message.ToJson();

    // Mock 模式下仅记录日志
    if (bUseMockData)
    {
        UE_LOG(LogMACommSubsystem, Log, TEXT("Mock mode: Scene change message logged but not sent"));
        UE_LOG(LogMACommSubsystem, Log, TEXT("  ChangeType: %s"), *ChangeTypeStr);
        UE_LOG(LogMACommSubsystem, Verbose, TEXT("  Full JSON: %s"), *MessageJson);
        return;
    }

    // 发送 HTTP POST 请求到场景变化端点
    // 使用专门的场景变化端点 /api/sim/scene_change
    FString FullUrl = ServerURL + TEXT("/api/sim/scene_change");
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("SendSceneChangeMessage: Sending to %s"), *FullUrl);

    // 创建 HTTP 请求
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    HttpRequest->SetURL(FullUrl);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetContentAsString(MessageJson);
    
    // 设置超时（10秒）
    HttpRequest->SetTimeout(10.0f);
    
    // 绑定完成回调 - 使用简单的日志回调，不需要重试逻辑
    HttpRequest->OnProcessRequestComplete().BindLambda(
        [ChangeTypeStr](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            if (!bConnectedSuccessfully || !Response.IsValid())
            {
                UE_LOG(LogMACommSubsystem, Warning, TEXT("SendSceneChangeMessage: Failed to send %s message - connection error"), *ChangeTypeStr);
                return;
            }
            
            int32 ResponseCode = Response->GetResponseCode();
            if (ResponseCode >= 200 && ResponseCode < 300)
            {
                UE_LOG(LogMACommSubsystem, Log, TEXT("SendSceneChangeMessage: %s message sent successfully"), *ChangeTypeStr);
            }
            else
            {
                UE_LOG(LogMACommSubsystem, Warning, TEXT("SendSceneChangeMessage: %s message failed with code %d"), *ChangeTypeStr, ResponseCode);
            }
        }
    );
    
    // 发送请求
    if (!HttpRequest->ProcessRequest())
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("SendSceneChangeMessage: Failed to initiate HTTP request for %s"), *ChangeTypeStr);
    }
}

void UMACommSubsystem::SendSceneChangeMessageByType(EMASceneChangeType ChangeType, const FString& Payload)
{
    // Requirements: 11.1 - 便捷方法，创建消息并发送
    
    FMASceneChangeMessage Message(ChangeType, Payload);
    SendSceneChangeMessage(Message);
}

void UMACommSubsystem::SendMessageEnvelope(const FMAMessageEnvelope& Envelope)
{
    // Requirements: 5.1, 5.2, 5.6 - 统一的消息发送入口
    
    FString EnvelopeJson = Envelope.ToJson();
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("SendMessageEnvelope: Type=%d, MessageId=%s"), 
        static_cast<int32>(Envelope.MessageType), *Envelope.MessageId);
    UE_LOG(LogMACommSubsystem, Verbose, TEXT("Envelope JSON: %s"), *EnvelopeJson);

    // Requirements: 5.6 - Mock 模式下不发送实际 HTTP 请求
    if (bUseMockData)
    {
        UE_LOG(LogMACommSubsystem, Log, TEXT("Mock mode: Message envelope logged but not sent"));
        
        // 在 Mock 模式下，生成模拟响应
        // 对于 UI 输入消息，生成模拟的规划器响应
        if (Envelope.MessageType == EMACommMessageType::UIInput)
        {
            FMAUIInputMessage UIInputMsg;
            if (FMAUIInputMessage::FromJson(Envelope.PayloadJson, UIInputMsg))
            {
                GenerateMockPlanResponse(UIInputMsg.InputContent);
            }
        }
        return;
    }

    // Requirements: 5.1, 5.2 - 发送真实的 HTTP POST 请求
    // 重置重试计数
    RetryCount = 0;
    
    // 构建完整 URL
    FString FullUrl = ServerURL + MessageEndpoint;
    
    // 执行 HTTP POST
    ExecuteHttpPost(FullUrl, EnvelopeJson, Envelope);
}

//=============================================================================
// HTTP 通信实现
// Requirements: 5.1, 5.2, 5.3, 5.4
//=============================================================================

void UMACommSubsystem::ExecuteHttpPost(const FString& Url, const FString& JsonPayload, const FMAMessageEnvelope& OriginalEnvelope)
{
    // Requirements: 5.1, 5.2 - 使用 UE5 的 FHttpModule 和 IHttpRequest
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("ExecuteHttpPost: URL=%s, RetryCount=%d"), *Url, RetryCount);
    
    // 保存原始信封用于可能的重试
    PendingEnvelope = OriginalEnvelope;
    
    // 创建 HTTP 请求
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    // 设置请求参数
    HttpRequest->SetURL(Url);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetContentAsString(JsonPayload);
    
    // 设置超时（10秒）
    HttpRequest->SetTimeout(10.0f);
    
    // 绑定完成回调
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UMACommSubsystem::OnHttpRequestComplete);
    
    // 发送请求
    if (!HttpRequest->ProcessRequest())
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("Failed to initiate HTTP request to %s"), *Url);
        
        // 尝试重试
        ScheduleRetry(OriginalEnvelope);
    }
}

void UMACommSubsystem::OnHttpRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
    // Requirements: 5.3, 5.4 - 处理 HTTP 响应，包括错误处理和重试逻辑
    
    if (!bConnectedSuccessfully || !Response.IsValid())
    {
        // 连接失败
        UE_LOG(LogMACommSubsystem, Error, TEXT("HTTP request failed: Connection error"));
        
        // 尝试重试
        ScheduleRetry(PendingEnvelope);
        return;
    }
    
    int32 ResponseCode = Response->GetResponseCode();
    FString ResponseContent = Response->GetContentAsString();
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("HTTP response: Code=%d"), ResponseCode);
    UE_LOG(LogMACommSubsystem, Verbose, TEXT("Response content: %s"), *ResponseContent);
    
    // 根据状态码处理响应
    if (ResponseCode >= 200 && ResponseCode < 300)
    {
        // 成功响应 (2xx)
        UE_LOG(LogMACommSubsystem, Log, TEXT("HTTP request successful"));
        
        // 重置重试计数
        RetryCount = 0;
        
        // 如果是 UI 输入消息，解析响应并广播
        if (PendingEnvelope.MessageType == EMACommMessageType::UIInput)
        {
            // 尝试解析响应为规划器响应
            FMAPlannerResponse PlannerResponse;
            PlannerResponse.bSuccess = true;
            PlannerResponse.Message = TEXT("请求已处理");
            
            // 如果响应内容不为空，尝试解析
            if (!ResponseContent.IsEmpty())
            {
                PlannerResponse.PlanText = ResponseContent;
            }
            
            BroadcastResponse(PlannerResponse);
        }
    }
    else if (ResponseCode >= 400 && ResponseCode < 500)
    {
        // 客户端错误 (4xx) - 不重试
        UE_LOG(LogMACommSubsystem, Error, TEXT("HTTP client error: %d - %s"), ResponseCode, *ResponseContent);
        
        // 广播失败响应
        FMAPlannerResponse ErrorResponse;
        ErrorResponse.bSuccess = false;
        ErrorResponse.Message = FString::Printf(TEXT("请求错误 (%d)"), ResponseCode);
        ErrorResponse.PlanText = ResponseContent;
        BroadcastResponse(ErrorResponse);
        
        // 重置重试计数
        RetryCount = 0;
    }
    else if (ResponseCode >= 500)
    {
        // 服务器错误 (5xx) - 尝试重试
        UE_LOG(LogMACommSubsystem, Warning, TEXT("HTTP server error: %d - %s"), ResponseCode, *ResponseContent);
        
        // 尝试重试
        ScheduleRetry(PendingEnvelope);
    }
    else
    {
        // 其他状态码
        UE_LOG(LogMACommSubsystem, Warning, TEXT("Unexpected HTTP response code: %d"), ResponseCode);
        
        // 广播响应
        FMAPlannerResponse UnexpectedResponse;
        UnexpectedResponse.bSuccess = false;
        UnexpectedResponse.Message = FString::Printf(TEXT("意外的响应码 (%d)"), ResponseCode);
        BroadcastResponse(UnexpectedResponse);
    }
}

void UMACommSubsystem::ScheduleRetry(const FMAMessageEnvelope& OriginalEnvelope)
{
    // Requirements: 5.4 - 实现指数退避重试逻辑（最多 3 次，1s/2s/4s）
    
    RetryCount++;
    
    if (RetryCount > MaxRetries)
    {
        // 超过最大重试次数
        UE_LOG(LogMACommSubsystem, Error, TEXT("HTTP request failed after %d retries"), MaxRetries);
        
        // 广播失败响应
        FMAPlannerResponse FailureResponse;
        FailureResponse.bSuccess = false;
        FailureResponse.Message = FString::Printf(TEXT("请求失败，已重试 %d 次"), MaxRetries);
        BroadcastResponse(FailureResponse);
        
        // 重置重试计数
        RetryCount = 0;
        return;
    }
    
    float DelaySeconds = GetRetryDelaySeconds();
    UE_LOG(LogMACommSubsystem, Log, TEXT("Scheduling retry %d/%d in %.1f seconds"), RetryCount, MaxRetries, DelaySeconds);
    
    // 获取 World 用于定时器
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("Cannot schedule retry: World is null"));
        RetryCount = 0;
        return;
    }
    
    // 设置定时器进行重试
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindLambda([this, OriginalEnvelope]()
    {
        FString FullUrl = ServerURL + MessageEndpoint;
        FString JsonPayload = OriginalEnvelope.ToJson();
        ExecuteHttpPost(FullUrl, JsonPayload, OriginalEnvelope);
    });
    
    World->GetTimerManager().SetTimer(RetryTimerHandle, TimerDelegate, DelaySeconds, false);
}

float UMACommSubsystem::GetRetryDelaySeconds() const
{
    // 指数退避：1s, 2s, 4s
    // RetryCount: 1 -> 1s, 2 -> 2s, 3 -> 4s
    return FMath::Pow(2.0f, static_cast<float>(RetryCount - 1));
}

//=============================================================================
// 轮询机制实现
// Requirements: 8.1, 8.2, 8.3, 8.4, 8.5
//=============================================================================

void UMACommSubsystem::StartPolling()
{
    // Requirements: 8.1 - 启动定时器
    
    if (bIsPolling)
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("StartPolling: Already polling"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("StartPolling: World is null"));
        return;
    }

    // 确保轮询间隔有效
    if (PollIntervalSeconds <= 0.0f)
    {
        PollIntervalSeconds = 1.0f;
        UE_LOG(LogMACommSubsystem, Warning, TEXT("StartPolling: Invalid PollIntervalSeconds, using default 1.0"));
    }

    bIsPolling = true;

    // 设置定时器，定期调用 PollForMessages
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindUObject(this, &UMACommSubsystem::PollForMessages);
    
    World->GetTimerManager().SetTimer(
        PollTimerHandle,
        TimerDelegate,
        PollIntervalSeconds,
        true,  // bLoop = true
        0.5f   // 首次延迟 0.5 秒后开始
    );

    UE_LOG(LogMACommSubsystem, Log, TEXT("StartPolling: Started with interval %.2f seconds"), PollIntervalSeconds);
}

void UMACommSubsystem::StopPolling()
{
    // Requirements: 8.3 - 停止定时器
    
    if (!bIsPolling)
    {
        return;
    }

    bIsPolling = false;

    if (PollTimerHandle.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(PollTimerHandle);
        }
    }

    UE_LOG(LogMACommSubsystem, Log, TEXT("StopPolling: Stopped"));
}

void UMACommSubsystem::PollForMessages()
{
    // Requirements: 8.2 - 发送 GET 请求到 /api/sim/poll
    
    if (!bIsPolling)
    {
        return;
    }

    // Mock 模式下不发送实际请求
    if (bUseMockData)
    {
        UE_LOG(LogMACommSubsystem, Verbose, TEXT("PollForMessages: Mock mode, skipping"));
        return;
    }

    // 构建完整 URL
    FString FullUrl = ServerURL + PollEndpoint;
    
    UE_LOG(LogMACommSubsystem, Verbose, TEXT("PollForMessages: Polling %s"), *FullUrl);

    // 创建 HTTP GET 请求
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    HttpRequest->SetURL(FullUrl);
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));
    
    // 设置超时（5秒，比发送消息短）
    HttpRequest->SetTimeout(5.0f);
    
    // 绑定完成回调
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UMACommSubsystem::OnPollRequestComplete);
    
    // 发送请求
    if (!HttpRequest->ProcessRequest())
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("PollForMessages: Failed to initiate HTTP request"));
    }
}

void UMACommSubsystem::OnPollRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
    // 处理轮询响应
    
    if (!bConnectedSuccessfully || !Response.IsValid())
    {
        // 连接失败，静默处理（轮询会继续）
        UE_LOG(LogMACommSubsystem, Verbose, TEXT("OnPollRequestComplete: Connection failed"));
        return;
    }

    int32 ResponseCode = Response->GetResponseCode();
    
    if (ResponseCode == 200)
    {
        FString ResponseContent = Response->GetContentAsString();
        
        if (!ResponseContent.IsEmpty() && ResponseContent != TEXT("[]") && ResponseContent != TEXT("{}"))
        {
            UE_LOG(LogMACommSubsystem, Log, TEXT("OnPollRequestComplete: Received data"));
            UE_LOG(LogMACommSubsystem, Verbose, TEXT("Response: %s"), *ResponseContent);
            
            // 处理响应
            HandlePollResponse(ResponseContent);
        }
    }
    else if (ResponseCode == 204)
    {
        // 204 No Content - 没有新消息，正常情况
        UE_LOG(LogMACommSubsystem, Verbose, TEXT("OnPollRequestComplete: No new messages"));
    }
    else
    {
        // 其他状态码
        UE_LOG(LogMACommSubsystem, Warning, TEXT("OnPollRequestComplete: Unexpected response code %d"), ResponseCode);
    }
}

void UMACommSubsystem::HandlePollResponse(const FString& ResponseJson)
{
    // Requirements: 6.5, 7.4 - 解析响应 JSON 并分发到对应处理函数
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("HandlePollResponse: Processing response"));
    
    // 响应可能是单个消息信封或消息信封数组
    // 首先尝试解析为数组
    TSharedPtr<FJsonValue> JsonValue;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseJson);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("HandlePollResponse: Failed to parse JSON"));
        return;
    }
    
    // 收集所有消息信封
    TArray<TSharedPtr<FJsonValue>> MessageArray;
    
    if (JsonValue->Type == EJson::Array)
    {
        // 响应是数组
        MessageArray = JsonValue->AsArray();
    }
    else if (JsonValue->Type == EJson::Object)
    {
        // 响应是单个对象
        MessageArray.Add(JsonValue);
    }
    else
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("HandlePollResponse: Unexpected JSON type"));
        return;
    }
    
    // 处理每个消息
    for (const TSharedPtr<FJsonValue>& MsgValue : MessageArray)
    {
        if (!MsgValue.IsValid() || MsgValue->Type != EJson::Object)
        {
            continue;
        }
        
        TSharedPtr<FJsonObject> MsgObject = MsgValue->AsObject();
        if (!MsgObject.IsValid())
        {
            continue;
        }
        
        // 获取消息类型
        FString MessageTypeStr;
        if (!MsgObject->TryGetStringField(TEXT("message_type"), MessageTypeStr))
        {
            UE_LOG(LogMACommSubsystem, Warning, TEXT("HandlePollResponse: Message missing message_type field"));
            continue;
        }
        
        UE_LOG(LogMACommSubsystem, Log, TEXT("HandlePollResponse: Processing message type: %s"), *MessageTypeStr);
        
        // 根据消息类型分发处理
        if (MessageTypeStr == TEXT("task_plan_dag"))
        {
            // Requirements: 6.5 - 解析 Task_Plan_DAG 并广播
            const TSharedPtr<FJsonObject>* PayloadObject;
            if (MsgObject->TryGetObjectField(TEXT("payload"), PayloadObject))
            {
                // 将 payload 对象序列化回 JSON 字符串
                FString PayloadJson;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
                FJsonSerializer::Serialize(PayloadObject->ToSharedRef(), Writer);
                
                FMATaskPlanDAG TaskPlan;
                if (FMATaskPlanDAG::FromJson(PayloadJson, TaskPlan))
                {
                    // 验证 DAG 有效性
                    if (TaskPlan.IsValidDAG())
                    {
                        UE_LOG(LogMACommSubsystem, Log, TEXT("HandlePollResponse: Broadcasting TaskPlanDAG with %d nodes, %d edges"),
                            TaskPlan.Nodes.Num(), TaskPlan.Edges.Num());
                        
                        // 广播委托
                        OnTaskPlanReceived.Broadcast(TaskPlan);
                    }
                    else
                    {
                        UE_LOG(LogMACommSubsystem, Warning, TEXT("HandlePollResponse: Received invalid DAG (contains cycle)"));
                    }
                }
                else
                {
                    UE_LOG(LogMACommSubsystem, Warning, TEXT("HandlePollResponse: Failed to parse TaskPlanDAG payload"));
                }
            }
        }
        else if (MessageTypeStr == TEXT("world_model_graph"))
        {
            // Requirements: 7.4 - 解析 World_Model_Graph 并广播
            const TSharedPtr<FJsonObject>* PayloadObject;
            if (MsgObject->TryGetObjectField(TEXT("payload"), PayloadObject))
            {
                // 将 payload 对象序列化回 JSON 字符串
                FString PayloadJson;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
                FJsonSerializer::Serialize(PayloadObject->ToSharedRef(), Writer);
                
                FMAWorldModelGraph WorldModel;
                if (FMAWorldModelGraph::FromJson(PayloadJson, WorldModel))
                {
                    UE_LOG(LogMACommSubsystem, Log, TEXT("HandlePollResponse: Broadcasting WorldModelGraph with %d entities, %d relationships"),
                        WorldModel.Entities.Num(), WorldModel.Relationships.Num());
                    
                    // 广播委托
                    OnWorldModelReceived.Broadcast(WorldModel);
                }
                else
                {
                    UE_LOG(LogMACommSubsystem, Warning, TEXT("HandlePollResponse: Failed to parse WorldModelGraph payload"));
                }
            }
        }
        else
        {
            // 未知消息类型
            UE_LOG(LogMACommSubsystem, Log, TEXT("HandlePollResponse: Unknown message type: %s"), *MessageTypeStr);
        }
    }
}
