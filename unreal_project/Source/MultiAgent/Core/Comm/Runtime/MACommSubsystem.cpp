// MACommSubsystem.cpp
// 通信子系统实现

#include "Core/Comm/Runtime/MACommSubsystem.h"
#include "Core/Comm/Application/MACommOutbound.h"
#include "Core/Comm/Application/MACommInbound.h"
#include "Core/Comm/Infrastructure/MACommHttpServer.h"
#include "Core/Comm/Infrastructure/Codec/MACommJsonCodec.h"
#include "Core/Config/MAConfigManager.h"
#include "Core/Comm/Domain/MACommTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "TimerManager.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommSubsystem, Log, All);

//=============================================================================
// 生命周期
//=============================================================================

void UMACommSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 确保 ConfigManager 先初始化
    Collection.InitializeDependency<UMAConfigManager>();

    // 从 ConfigManager 获取配置
    if (UMAConfigManager* ConfigMgr = GetGameInstance()->GetSubsystem<UMAConfigManager>())
    {
        ServerURL = ConfigMgr->PlannerServerURL;
        bUseMockData = ConfigMgr->bUseMockData;
        bEnablePolling = ConfigMgr->bEnablePolling;
        bEnableHITLPolling = ConfigMgr->bEnableHITLPolling;
        PollIntervalSeconds = ConfigMgr->PollIntervalSeconds;
        HITLPollIntervalSeconds = ConfigMgr->HITLPollIntervalSeconds;
        LocalServerPort = ConfigMgr->LocalServerPort;
        bEnableLocalServer = ConfigMgr->bEnableLocalServer;
    }

    UE_LOG(LogMACommSubsystem, Log, TEXT("MACommSubsystem initialized"));
    UE_LOG(LogMACommSubsystem, Log, TEXT("  ServerURL: %s"), *ServerURL);
    UE_LOG(LogMACommSubsystem, Log, TEXT("  bUseMockData: %s"), bUseMockData ? TEXT("true") : TEXT("false"));
    UE_LOG(LogMACommSubsystem, Log, TEXT("  bEnablePolling: %s"), bEnablePolling ? TEXT("true") : TEXT("false"));
    UE_LOG(LogMACommSubsystem, Log, TEXT("  bEnableHITLPolling: %s"), bEnableHITLPolling ? TEXT("true") : TEXT("false"));
    UE_LOG(LogMACommSubsystem, Log, TEXT("  PollIntervalSeconds: %.2f"), PollIntervalSeconds);
    UE_LOG(LogMACommSubsystem, Log, TEXT("  HITLPollIntervalSeconds: %.2f"), HITLPollIntervalSeconds);
    UE_LOG(LogMACommSubsystem, Log, TEXT("  LocalServerPort: %d"), LocalServerPort);
    UE_LOG(LogMACommSubsystem, Log, TEXT("  bEnableLocalServer: %s"), bEnableLocalServer ? TEXT("true") : TEXT("false"));

    // 创建子模块
    Outbound = MakeUnique<FMACommOutbound>(this);
    Inbound = MakeUnique<FMACommInbound>(this);
    HttpServer = MakeUnique<FMACommHttpServer>(this);

    // 设置入站模块的轮询配置
    Inbound->SetPollInterval(PollIntervalSeconds);
    Inbound->SetEnableHITLPolling(bEnableHITLPolling);
    Inbound->SetHITLPollInterval(HITLPollIntervalSeconds);

    // 如果配置启用，自动启动轮询
    if (bEnablePolling && !bUseMockData)
    {
        StartPolling();
    }

    // 如果配置启用，启动本地 HTTP 服务器
    if (bEnableLocalServer)
    {
        StartHttpServer();
    }
}

void UMACommSubsystem::Deinitialize()
{
    UE_LOG(LogMACommSubsystem, Log, TEXT("MACommSubsystem deinitializing"));
    
    // 停止轮询
    StopPolling();
    
    // 停止 HTTP 服务器
    StopHttpServer();
    
    // 清理重试定时器
    if (RetryTimerHandle.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(RetryTimerHandle);
        }
    }
    
    // 清理子模块
    HttpServer.Reset();
    Inbound.Reset();
    Outbound.Reset();
    
    // 清理状态
    bWaitingForResponse = false;
    LastCommand.Empty();
    RetryCount = 0;
    
    Super::Deinitialize();
}

//=============================================================================
// 出站消息发送接口 (委托给 MACommOutbound)
//=============================================================================

void UMACommSubsystem::SendNaturalLanguageCommand(const FString& Command)
{
    if (Command.IsEmpty())
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("SendNaturalLanguageCommand: Empty command ignored"));
        BroadcastResponse(FMAPlannerResponse::Failure(TEXT("Command cannot be empty")));
        return;
    }

    UE_LOG(LogMACommSubsystem, Log, TEXT("SendNaturalLanguageCommand: %s"), *Command);
    
    LastCommand = Command;
    bWaitingForResponse = true;
    
    // 向后兼容：使用 "legacy_command" 作为 instruction_id
    if (Outbound)
    {
        Outbound->SendUIInputMessage(TEXT("legacy_command"), Command);
    }
}

void UMACommSubsystem::SendUIInputMessage(const FString& SourceId, const FString& Content)
{
    if (Outbound)
    {
        Outbound->SendUIInputMessage(SourceId, Content);
    }
}

void UMACommSubsystem::SendButtonEventMessage(const FString& WidgetName, const FString& ButtonId, const FString& ButtonText)
{
    if (Outbound)
    {
        Outbound->SendButtonEventMessage(WidgetName, ButtonId, ButtonText);
    }
}

void UMACommSubsystem::SendTaskFeedbackMessage(const FMATaskFeedbackMessage& Feedback)
{
    if (Outbound)
    {
        Outbound->SendTaskFeedbackMessage(Feedback);
    }
}

void UMACommSubsystem::SendTimeStepFeedback(const FMATimeStepFeedbackMessage& Feedback)
{
    if (Outbound)
    {
        Outbound->SendTimeStepFeedback(Feedback);
    }
}

void UMACommSubsystem::SendSkillListCompletedFeedback(const FMASkillListCompletedMessage& Message)
{
    if (Outbound)
    {
        Outbound->SendSkillListCompletedFeedback(Message);
    }
}

void UMACommSubsystem::SendTaskGraphSubmitMessage(const FString& TaskGraphJson)
{
    if (Outbound)
    {
        Outbound->SendTaskGraphSubmitMessage(TaskGraphJson);
    }
}

void UMACommSubsystem::SendSceneChangeMessage(const FMASceneChangeMessage& Message)
{
    if (Outbound)
    {
        Outbound->SendSceneChangeMessage(Message);
    }
}

void UMACommSubsystem::SendSceneChangeMessageByType(EMASceneChangeType ChangeType, const FString& Payload)
{
    if (Outbound)
    {
        Outbound->SendSceneChangeMessageByType(ChangeType, Payload);
    }
}

void UMACommSubsystem::SendSkillAllocationMessage(const FMASkillAllocationMessage& Message)
{
    if (Outbound)
    {
        Outbound->SendSkillAllocationMessage(Message);
    }
}

//=============================================================================
// HITL 响应发送接口 (委托给 MACommOutbound)
//=============================================================================

void UMACommSubsystem::SendReviewResponse(const FMAReviewResponseMessage& Response)
{
    if (Outbound)
    {
        Outbound->SendReviewResponse(Response);
    }
}

void UMACommSubsystem::SendReviewResponseSimple(bool bApproved,
    const FString& ModifiedDataJson,
    const FString& RejectionReason,
    const FString& OriginalMessageId)
{
    if (Outbound)
    {
        Outbound->SendReviewResponseSimple(bApproved, ModifiedDataJson, RejectionReason, OriginalMessageId);
    }
}

void UMACommSubsystem::SendDecisionResponse(const FMADecisionResponseMessage& Response)
{
    if (Outbound)
    {
        Outbound->SendDecisionResponse(Response);
    }
}

void UMACommSubsystem::SendDecisionResponseSimple(const FString& Decision,
    const FString& DecisionDataJson,
    const FString& UserFeedback,
    const FString& OriginalMessageId)
{
    if (Outbound)
    {
        Outbound->SendDecisionResponseSimple(Decision, DecisionDataJson, UserFeedback, OriginalMessageId);
    }
}

//=============================================================================
// 轮询控制接口 (委托给 MACommInbound)
//=============================================================================

void UMACommSubsystem::StartPolling()
{
    if (Inbound)
    {
        Inbound->StartPolling();
    }
}

void UMACommSubsystem::StopPolling()
{
    if (Inbound)
    {
        Inbound->StopPolling();
    }
}

bool UMACommSubsystem::IsPolling() const
{
    return Inbound ? Inbound->IsPolling() : false;
}

//=============================================================================
// HTTP 服务器控制接口 (委托给 MACommHttpServer)
//=============================================================================

void UMACommSubsystem::StartHttpServer()
{
    if (HttpServer)
    {
        HttpServer->Start(LocalServerPort);
    }
}

void UMACommSubsystem::StopHttpServer()
{
    if (HttpServer)
    {
        HttpServer->Stop();
    }
}

bool UMACommSubsystem::IsHttpServerRunning() const
{
    return HttpServer ? HttpServer->IsRunning() : false;
}

//=============================================================================
// 内部方法 - 供子模块调用
//=============================================================================

void UMACommSubsystem::SendMessageEnvelopeInternal(const FMAMessageEnvelope& Envelope)
{
    const FString EnvelopeJson = MACommJsonCodec::SerializeEnvelope(Envelope);
    
    UE_LOG(LogMACommSubsystem, Verbose, TEXT("SendMessageEnvelopeInternal: Type=%d, Category=%d, MessageId=%s"),
        static_cast<int32>(Envelope.MessageType), 
        static_cast<int32>(Envelope.MessageCategory),
        *Envelope.MessageId);
    
    if (bUseMockData)
    {
        UE_LOG(LogMACommSubsystem, Log, TEXT("Mock mode: Message envelope logged but not sent"));
        
        // 在 Mock 模式下，生成模拟响应
        if (Envelope.MessageType == EMACommMessageType::UIInput)
        {
            FMAUIInputMessage UIInputMsg;
            if (MACommJsonCodec::DeserializeUIInput(Envelope.PayloadJson, UIInputMsg))
            {
                GenerateMockPlanResponse(UIInputMsg.InputContent);
            }
        }
        return;
    }
    
    // 重置重试计数
    RetryCount = 0;
    
    // 根据消息类别选择正确的端点
    FString Endpoint = GetEndpointForCategory(Envelope.MessageCategory);
    FString FullUrl = ServerURL + Endpoint;
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("SendMessageEnvelopeInternal: Routing to endpoint %s"), *Endpoint);
    
    // 执行 HTTP POST
    ExecuteHttpPost(FullUrl, EnvelopeJson, Envelope);
}

FString UMACommSubsystem::GetEndpointForCategory(EMAMessageCategory Category) const
{
    switch (Category)
    {
    case EMAMessageCategory::Instruction:
    case EMAMessageCategory::Review:
    case EMAMessageCategory::Decision:
        return HITLMessageEndpoint;
    case EMAMessageCategory::Platform:
    default:
        return PlatformMessageEndpoint;
    }
}

void UMACommSubsystem::SendHITLMessageInternal(const FMAMessageEnvelope& Envelope)
{
    const FString EnvelopeJson = MACommJsonCodec::SerializeEnvelope(Envelope);
    
    UE_LOG(LogMACommSubsystem, Log, TEXT("SendHITLMessageInternal: Type=%d, Category=%d, MessageId=%s"),
        static_cast<int32>(Envelope.MessageType), 
        static_cast<int32>(Envelope.MessageCategory),
        *Envelope.MessageId);
    
    if (bUseMockData)
    {
        UE_LOG(LogMACommSubsystem, Log, TEXT("Mock mode: HITL message logged but not sent"));
        return;
    }
    
    // 重置重试计数
    RetryCount = 0;
    
    // HITL 消息始终发送到 HITL 端点
    FString FullUrl = ServerURL + HITLMessageEndpoint;
    
    // 执行 HTTP POST
    ExecuteHttpPost(FullUrl, EnvelopeJson, Envelope);
}

void UMACommSubsystem::SendSceneChangeHttpRequest(const FString& MessageJson)
{
    FString FullUrl = ServerURL + TEXT("/api/sim/scene_change");

    UE_LOG(LogMACommSubsystem, Log, TEXT("SendSceneChangeHttpRequest: Sending to %s"), *FullUrl);

    // 创建 HTTP 请求
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

    HttpRequest->SetURL(FullUrl);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetContentAsString(MessageJson);
    HttpRequest->SetTimeout(10.0f);

    // 绑定完成回调
    HttpRequest->OnProcessRequestComplete().BindLambda(
        [](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            if (!bConnectedSuccessfully || !Response.IsValid())
            {
                UE_LOG(LogMACommSubsystem, Warning, TEXT("SendSceneChangeHttpRequest: Connection error"));
                return;
            }

            int32 ResponseCode = Response->GetResponseCode();
            if (ResponseCode >= 200 && ResponseCode < 300)
            {
                UE_LOG(LogMACommSubsystem, Log, TEXT("SendSceneChangeHttpRequest: Success"));
            }
            else
            {
                UE_LOG(LogMACommSubsystem, Warning, TEXT("SendSceneChangeHttpRequest: Failed with code %d"), ResponseCode);
            }
        }
    );

    if (!HttpRequest->ProcessRequest())
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("SendSceneChangeHttpRequest: Failed to initiate HTTP request"));
    }
}

void UMACommSubsystem::BroadcastResponse(const FMAPlannerResponse& Response)
{
    bWaitingForResponse = false;
    OnPlannerResponse.Broadcast(Response);
    UE_LOG(LogMACommSubsystem, Log, TEXT("Response broadcasted to subscribers"));
}

void UMACommSubsystem::GenerateMockPlanResponse(const FString& UserCommand)
{
    FMAPlannerResponse Response;
    Response.bSuccess = true;
    
    FString LowerCommand = UserCommand.ToLower();
    
    if (LowerCommand.Contains(TEXT("move")) || LowerCommand.Contains(TEXT("go")))
    {
        Response.Message = TEXT("Move command received");
        Response.PlanText = FString::Printf(
            TEXT("Planning result:\n1. Parse target location\n2. Calculate optimal path\n3. Execute move task\n\nOriginal command: %s"), *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("patrol")))
    {
        Response.Message = TEXT("Patrol command received");
        Response.PlanText = FString::Printf(
            TEXT("Planning result:\n1. Set patrol waypoints\n2. Configure patrol parameters\n3. Start patrol loop\n\nOriginal command: %s"), *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("stop")))
    {
        Response.Message = TEXT("Stop command received");
        Response.PlanText = FString::Printf(
            TEXT("Planning result:\n1. Interrupt current task\n2. Stop movement\n3. Enter standby state\n\nOriginal command: %s"), *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("status")))
    {
        Response.Message = TEXT("Status query processed");
        Response.PlanText = FString::Printf(
            TEXT("System status:\n- Communication: Normal\n- Mock mode: Enabled\n- Server: %s\n\nOriginal command: %s"), *ServerURL, *UserCommand);
    }
    else if (LowerCommand.Contains(TEXT("help")))
    {
        Response.Message = TEXT("Help information");
        Response.PlanText = TEXT("Available commands:\n- move [target]: Move to specified location\n- patrol: Start patrol task\n- stop: Stop current task\n- status: Query system status\n- help: Show help information");
    }
    else
    {
        Response.Message = FString::Printf(TEXT("Command received: %s"), *UserCommand);
        Response.PlanText = FString::Printf(
            TEXT("Planning result:\nCommand recorded, awaiting further processing.\n\nOriginal command: %s"), *UserCommand);
    }

    BroadcastResponse(Response);
    UE_LOG(LogMACommSubsystem, Log, TEXT("Mock response generated - Success: %s, Message: %s"), 
        Response.bSuccess ? TEXT("true") : TEXT("false"), *Response.Message);
}

//=============================================================================
// HTTP 客户端通信
//=============================================================================

void UMACommSubsystem::ExecuteHttpPost(const FString& Url, const FString& JsonPayload, const FMAMessageEnvelope& OriginalEnvelope)
{
    UE_LOG(LogMACommSubsystem, Verbose, TEXT("ExecuteHttpPost: URL=%s, RetryCount=%d"), *Url, RetryCount);
    
    PendingEnvelope = OriginalEnvelope;
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    HttpRequest->SetURL(Url);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetContentAsString(JsonPayload);
    HttpRequest->SetTimeout(10.0f);
    
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UMACommSubsystem::OnHttpRequestComplete);
    
    if (!HttpRequest->ProcessRequest())
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("Failed to initiate HTTP request to %s"), *Url);
        ScheduleRetry(OriginalEnvelope);
    }
}

void UMACommSubsystem::OnHttpRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
    if (!bConnectedSuccessfully || !Response.IsValid())
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("HTTP request failed: Connection error"));
        ScheduleRetry(PendingEnvelope);
        return;
    }
    
    int32 ResponseCode = Response->GetResponseCode();
    FString ResponseContent = Response->GetContentAsString();
    
    UE_LOG(LogMACommSubsystem, Verbose, TEXT("HTTP response: Code=%d"), ResponseCode);
    
    if (ResponseCode >= 200 && ResponseCode < 300)
    {
        UE_LOG(LogMACommSubsystem, Log, TEXT("HTTP request successful"));
        RetryCount = 0;
        
        if (PendingEnvelope.MessageType == EMACommMessageType::UIInput)
        {
            FMAPlannerResponse PlannerResponse;
            PlannerResponse.bSuccess = true;
            PlannerResponse.Message = TEXT("Request processed");
            if (!ResponseContent.IsEmpty())
            {
                PlannerResponse.PlanText = ResponseContent;
            }
            BroadcastResponse(PlannerResponse);
        }
    }
    else if (ResponseCode >= 400 && ResponseCode < 500)
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("HTTP client error: %d - %s"), ResponseCode, *ResponseContent);
        
        FMAPlannerResponse ErrorResponse;
        ErrorResponse.bSuccess = false;
        ErrorResponse.Message = FString::Printf(TEXT("Request error (%d)"), ResponseCode);
        ErrorResponse.PlanText = ResponseContent;
        BroadcastResponse(ErrorResponse);
        RetryCount = 0;
    }
    else if (ResponseCode >= 500)
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("HTTP server error: %d - %s"), ResponseCode, *ResponseContent);
        ScheduleRetry(PendingEnvelope);
    }
    else
    {
        UE_LOG(LogMACommSubsystem, Warning, TEXT("Unexpected HTTP response code: %d"), ResponseCode);
        
        FMAPlannerResponse UnexpectedResponse;
        UnexpectedResponse.bSuccess = false;
        UnexpectedResponse.Message = FString::Printf(TEXT("Unexpected response code (%d)"), ResponseCode);
        BroadcastResponse(UnexpectedResponse);
    }
}

void UMACommSubsystem::ScheduleRetry(const FMAMessageEnvelope& OriginalEnvelope)
{
    RetryCount++;
    
    if (RetryCount > MaxRetries)
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("HTTP request failed after %d retries"), MaxRetries);
        
        FMAPlannerResponse FailureResponse;
        FailureResponse.bSuccess = false;
        FailureResponse.Message = FString::Printf(TEXT("Request failed after %d retries"), MaxRetries);
        BroadcastResponse(FailureResponse);
        RetryCount = 0;
        return;
    }
    
    float DelaySeconds = GetRetryDelaySeconds();
    UE_LOG(LogMACommSubsystem, Log, TEXT("Scheduling retry %d/%d in %.1f seconds"), RetryCount, MaxRetries, DelaySeconds);
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMACommSubsystem, Error, TEXT("Cannot schedule retry: World is null"));
        RetryCount = 0;
        return;
    }
    
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindLambda([this, OriginalEnvelope]()
    {
        // 根据消息类别选择正确的端点
        FString Endpoint = GetEndpointForCategory(OriginalEnvelope.MessageCategory);
        FString FullUrl = ServerURL + Endpoint;
        const FString JsonPayload = MACommJsonCodec::SerializeEnvelope(OriginalEnvelope);
        ExecuteHttpPost(FullUrl, JsonPayload, OriginalEnvelope);
    });
    
    World->GetTimerManager().SetTimer(RetryTimerHandle, TimerDelegate, DelaySeconds, false);
}

float UMACommSubsystem::GetRetryDelaySeconds() const
{
    // 指数退避：1s, 2s, 4s
    return FMath::Pow(2.0f, static_cast<float>(RetryCount - 1));
}
