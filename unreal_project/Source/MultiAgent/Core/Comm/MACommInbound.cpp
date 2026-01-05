// MACommInbound.cpp
// 入站消息处理模块实现

#include "MACommInbound.h"
#include "MACommSubsystem.h"
#include "../Manager/MACommandManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "TimerManager.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommInbound, Log, All);

FMACommInbound::FMACommInbound(UMACommSubsystem* InOwner)
    : Owner(InOwner)
{
}

//=============================================================================
// 轮询控制
//=============================================================================

void FMACommInbound::StartPolling()
{
    if (bIsPolling)
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("StartPolling: Already polling"));
        return;
    }

    if (!Owner)
    {
        UE_LOG(LogMACommInbound, Error, TEXT("StartPolling: Owner is null"));
        return;
    }

    UWorld* World = Owner->GetWorld();
    if (!World)
    {
        UE_LOG(LogMACommInbound, Error, TEXT("StartPolling: World is null"));
        return;
    }

    // 确保轮询间隔有效
    if (PollIntervalSeconds <= 0.0f)
    {
        PollIntervalSeconds = 1.0f;
        UE_LOG(LogMACommInbound, Warning, TEXT("StartPolling: Invalid PollIntervalSeconds, using default 1.0"));
    }

    bIsPolling = true;

    // 设置定时器，定期调用 PollForMessages
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindRaw(this, &FMACommInbound::PollForMessages);
    
    World->GetTimerManager().SetTimer(
        PollTimerHandle,
        TimerDelegate,
        PollIntervalSeconds,
        true,  // bLoop = true
        0.5f   // 首次延迟 0.5 秒后开始
    );

    UE_LOG(LogMACommInbound, Log, TEXT("StartPolling: Started with interval %.2f seconds"), PollIntervalSeconds);
}

void FMACommInbound::StopPolling()
{
    if (!bIsPolling)
    {
        return;
    }

    bIsPolling = false;

    if (PollTimerHandle.IsValid() && Owner)
    {
        if (UWorld* World = Owner->GetWorld())
        {
            World->GetTimerManager().ClearTimer(PollTimerHandle);
        }
    }

    UE_LOG(LogMACommInbound, Log, TEXT("StopPolling: Stopped"));
}

//=============================================================================
// 轮询实现
//=============================================================================

void FMACommInbound::PollForMessages()
{
    if (!bIsPolling || !Owner)
    {
        return;
    }

    // Mock 模式下不发送实际请求
    if (Owner->bUseMockData)
    {
        UE_LOG(LogMACommInbound, Verbose, TEXT("PollForMessages: Mock mode, skipping"));
        return;
    }

    // 构建完整 URL
    FString FullUrl = Owner->ServerURL + PollEndpoint;
    
    UE_LOG(LogMACommInbound, Verbose, TEXT("PollForMessages: Polling %s"), *FullUrl);

    // 创建 HTTP GET 请求
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    HttpRequest->SetURL(FullUrl);
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));
    
    // 设置超时（5秒，比发送消息短）
    HttpRequest->SetTimeout(5.0f);
    
    // 绑定完成回调
    HttpRequest->OnProcessRequestComplete().BindRaw(this, &FMACommInbound::OnPollRequestComplete);
    
    // 发送请求
    if (!HttpRequest->ProcessRequest())
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("PollForMessages: Failed to initiate HTTP request"));
    }
}

void FMACommInbound::OnPollRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
    if (!bConnectedSuccessfully || !Response.IsValid())
    {
        // 连接失败，静默处理（轮询会继续）
        UE_LOG(LogMACommInbound, Verbose, TEXT("OnPollRequestComplete: Connection failed"));
        return;
    }

    int32 ResponseCode = Response->GetResponseCode();
    
    if (ResponseCode == 200)
    {
        FString ResponseContent = Response->GetContentAsString();
        
        if (!ResponseContent.IsEmpty() && ResponseContent != TEXT("[]") && ResponseContent != TEXT("{}"))
        {
            UE_LOG(LogMACommInbound, Log, TEXT("Python -> UE5: Received data"));
            UE_LOG(LogMACommInbound, Verbose, TEXT("Response: %s"), *ResponseContent);
            
            // 处理响应
            HandlePollResponse(ResponseContent);
        }
    }
    else if (ResponseCode == 204)
    {
        // 204 No Content - 没有新消息，正常情况
        UE_LOG(LogMACommInbound, Verbose, TEXT("OnPollRequestComplete: No new messages"));
    }
    else
    {
        // 其他状态码
        UE_LOG(LogMACommInbound, Warning, TEXT("OnPollRequestComplete: Unexpected response code %d"), ResponseCode);
    }
}

//=============================================================================
// 消息处理
//=============================================================================

void FMACommInbound::HandlePollResponse(const FString& ResponseJson)
{
    UE_LOG(LogMACommInbound, Log, TEXT("HandlePollResponse: Processing response"));
    
    // 响应可能是单个消息信封或消息信封数组
    TSharedPtr<FJsonValue> JsonValue;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseJson);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandlePollResponse: Failed to parse JSON"));
        return;
    }
    
    // 收集所有消息信封
    TArray<TSharedPtr<FJsonValue>> MessageArray;
    
    if (JsonValue->Type == EJson::Array)
    {
        MessageArray = JsonValue->AsArray();
    }
    else if (JsonValue->Type == EJson::Object)
    {
        MessageArray.Add(JsonValue);
    }
    else
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandlePollResponse: Unexpected JSON type"));
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
            UE_LOG(LogMACommInbound, Warning, TEXT("HandlePollResponse: Message missing message_type field"));
            continue;
        }
        
        UE_LOG(LogMACommInbound, Log, TEXT("Python -> UE5: Message type: %s"), *MessageTypeStr);
        
        // 获取 payload
        const TSharedPtr<FJsonObject>* PayloadObject;
        if (!MsgObject->TryGetObjectField(TEXT("payload"), PayloadObject))
        {
            UE_LOG(LogMACommInbound, Warning, TEXT("HandlePollResponse: Message missing payload field"));
            continue;
        }
        
        // 根据消息类型分发处理
        if (MessageTypeStr == TEXT("task_plan_dag"))
        {
            HandleTaskPlanDAG(*PayloadObject);
        }
        else if (MessageTypeStr == TEXT("world_model_graph"))
        {
            HandleWorldModelGraph(*PayloadObject);
        }
        else if (MessageTypeStr == TEXT("skill_list"))
        {
            HandleSkillList(*PayloadObject);
        }
        else if (MessageTypeStr == TEXT("query_request"))
        {
            HandleQueryRequest(*PayloadObject);
        }
        else
        {
            UE_LOG(LogMACommInbound, Log, TEXT("HandlePollResponse: Unknown message type: %s"), *MessageTypeStr);
        }
    }
}

//=============================================================================
// 消息分发
//=============================================================================

void FMACommInbound::HandleTaskPlanDAG(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    // 将 payload 对象序列化回 JSON 字符串
    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), Writer);
    
    FMATaskPlanDAG TaskPlan;
    if (FMATaskPlanDAG::FromJson(PayloadJson, TaskPlan))
    {
        // 验证 DAG 有效性
        if (TaskPlan.IsValidDAG())
        {
            UE_LOG(LogMACommInbound, Log, TEXT("HandleTaskPlanDAG: Broadcasting TaskPlanDAG with %d nodes, %d edges"),
                TaskPlan.Nodes.Num(), TaskPlan.Edges.Num());
            
            // 广播委托
            Owner->OnTaskPlanReceived.Broadcast(TaskPlan);
        }
        else
        {
            UE_LOG(LogMACommInbound, Warning, TEXT("HandleTaskPlanDAG: Received invalid DAG (contains cycle)"));
        }
    }
    else
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleTaskPlanDAG: Failed to parse TaskPlanDAG payload"));
    }
}

void FMACommInbound::HandleWorldModelGraph(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    // 将 payload 对象序列化回 JSON 字符串
    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), Writer);
    
    FMAWorldModelGraph WorldModel;
    if (FMAWorldModelGraph::FromJson(PayloadJson, WorldModel))
    {
        UE_LOG(LogMACommInbound, Log, TEXT("HandleWorldModelGraph: Broadcasting WorldModelGraph with %d entities, %d relationships"),
            WorldModel.Entities.Num(), WorldModel.Relationships.Num());
        
        // 广播委托
        Owner->OnWorldModelReceived.Broadcast(WorldModel);
    }
    else
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleWorldModelGraph: Failed to parse WorldModelGraph payload"));
    }
}

void FMACommInbound::HandleSkillList(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    // 将 payload 对象序列化回 JSON 字符串
    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), Writer);

    FMASkillListMessage SkillList;
    if (FMASkillListMessage::FromJson(PayloadJson, SkillList))
    {
        UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillList: Received SkillList with %d time steps"),
            SkillList.TotalTimeSteps);

        // 直接调用 CommandManager 执行
        if (UWorld* World = Owner->GetWorld())
        {
            if (UMACommandManager* CommandManager = World->GetSubsystem<UMACommandManager>())
            {
                CommandManager->ExecuteSkillList(SkillList);
            }
        }
    }
    else
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillList: Failed to parse SkillList payload"));
    }
}

void FMACommInbound::HandleQueryRequest(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    FString QueryType;
    PayloadObject->TryGetStringField(TEXT("query_type"), QueryType);

    UE_LOG(LogMACommInbound, Log, TEXT("HandleQueryRequest: Received query request: %s"), *QueryType);

    // 世界查询功能已移除，返回错误响应
    FString ResponseJson = TEXT("{\"error\": \"World query not supported\"}");
    Owner->GetOutbound()->SendWorldStateResponse(QueryType, ResponseJson);
}
