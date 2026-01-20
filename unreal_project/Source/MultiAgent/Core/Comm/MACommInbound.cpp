// MACommInbound.cpp
// 入站消息处理模块实现

#include "MACommInbound.h"
#include "MACommSubsystem.h"
#include "../Manager/MACommandManager.h"
#include "../Manager/MATempDataManager.h"
#include "../Types/MATaskGraphTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommInbound, Log, All);

// 辅助命名空间 - 消息类别字符串转换
namespace MACommInboundHelpers
{
    EMAMessageCategory StringToMessageCategory(const FString& CategoryStr)
    {
        if (CategoryStr == TEXT("instruction")) return EMAMessageCategory::Instruction;
        if (CategoryStr == TEXT("review"))      return EMAMessageCategory::Review;
        if (CategoryStr == TEXT("decision"))    return EMAMessageCategory::Decision;
        if (CategoryStr == TEXT("platform"))    return EMAMessageCategory::Platform;
        return EMAMessageCategory::Platform; // 默认值
    }

    EMACommMessageType StringToMessageType(const FString& TypeStr)
    {
        if (TypeStr == TEXT("ui_input"))           return EMACommMessageType::UIInput;
        if (TypeStr == TEXT("user_instruction"))   return EMACommMessageType::UIInput;
        if (TypeStr == TEXT("button_event"))       return EMACommMessageType::ButtonEvent;
        if (TypeStr == TEXT("task_feedback"))      return EMACommMessageType::TaskFeedback;
        if (TypeStr == TEXT("world_state"))        return EMACommMessageType::WorldState;
        if (TypeStr == TEXT("scene_change"))       return EMACommMessageType::SceneChange;
        if (TypeStr == TEXT("task_graph"))         return EMACommMessageType::TaskGraph;
        if (TypeStr == TEXT("world_model_graph"))  return EMACommMessageType::WorldModelGraph;
        if (TypeStr == TEXT("skill_list"))         return EMACommMessageType::SkillList;
        if (TypeStr == TEXT("query_request"))      return EMACommMessageType::QueryRequest;
        if (TypeStr == TEXT("skill_allocation"))   return EMACommMessageType::SkillAllocation;
        if (TypeStr == TEXT("skill_status_update")) return EMACommMessageType::SkillStatusUpdate;
        return EMACommMessageType::Custom;
    }

    // 从 message_type 推断 message_category (向后兼容)
    EMAMessageCategory GetCategoryForMessageType(EMACommMessageType Type)
    {
        switch (Type)
        {
        case EMACommMessageType::UIInput:
        case EMACommMessageType::ButtonEvent:
            return EMAMessageCategory::Instruction;
        case EMACommMessageType::TaskGraph:
        case EMACommMessageType::SkillAllocation:
            return EMAMessageCategory::Review;
        case EMACommMessageType::TaskFeedback:
        case EMACommMessageType::WorldState:
        case EMACommMessageType::SceneChange:
        case EMACommMessageType::WorldModelGraph:
        case EMACommMessageType::SkillList:
        case EMACommMessageType::QueryRequest:
        case EMACommMessageType::SkillStatusUpdate:
        case EMACommMessageType::Custom:
        default:
            return EMAMessageCategory::Platform;
        }
    }
}

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

    if (HITLPollIntervalSeconds <= 0.0f)
    {
        HITLPollIntervalSeconds = 1.0f;
        UE_LOG(LogMACommInbound, Warning, TEXT("StartPolling: Invalid HITLPollIntervalSeconds, using default 1.0"));
    }

    bIsPolling = true;

    // 设置 Platform 端点轮询定时器
    FTimerDelegate PlatformTimerDelegate;
    PlatformTimerDelegate.BindRaw(this, &FMACommInbound::PollForMessages);
    
    World->GetTimerManager().SetTimer(
        PollTimerHandle,
        PlatformTimerDelegate,
        PollIntervalSeconds,
        true,  // bLoop = true
        0.5f   // 首次延迟 0.5 秒后开始
    );

    UE_LOG(LogMACommInbound, Log, TEXT("StartPolling: Platform polling started with interval %.2f seconds"), PollIntervalSeconds);

    // 设置 HITL 端点轮询定时器 (如果启用)
    if (bEnableHITLPolling)
    {
        FTimerDelegate HITLTimerDelegate;
        HITLTimerDelegate.BindRaw(this, &FMACommInbound::PollForHITLMessages);
        
        World->GetTimerManager().SetTimer(
            HITLPollTimerHandle,
            HITLTimerDelegate,
            HITLPollIntervalSeconds,
            true,  // bLoop = true
            0.7f   // 首次延迟 0.7 秒后开始 (与 Platform 轮询错开)
        );

        UE_LOG(LogMACommInbound, Log, TEXT("StartPolling: HITL polling started with interval %.2f seconds"), HITLPollIntervalSeconds);
    }
    else
    {
        UE_LOG(LogMACommInbound, Log, TEXT("StartPolling: HITL polling disabled"));
    }
}

void FMACommInbound::StopPolling()
{
    if (!bIsPolling)
    {
        return;
    }

    bIsPolling = false;

    if (Owner)
    {
        if (UWorld* World = Owner->GetWorld())
        {
            // 清除 Platform 轮询定时器
            if (PollTimerHandle.IsValid())
            {
                World->GetTimerManager().ClearTimer(PollTimerHandle);
            }

            // 清除 HITL 轮询定时器
            if (HITLPollTimerHandle.IsValid())
            {
                World->GetTimerManager().ClearTimer(HITLPollTimerHandle);
            }
        }
    }

    UE_LOG(LogMACommInbound, Log, TEXT("StopPolling: Stopped all polling"));
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
    FString FullUrl = Owner->ServerURL + PlatformPollEndpoint;
    
    UE_LOG(LogMACommInbound, Verbose, TEXT("PollForMessages: Polling Platform endpoint %s"), *FullUrl);

    // 创建 HTTP GET 请求
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    HttpRequest->SetURL(FullUrl);
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));
    
    // 设置超时（使用配置的超时时间，给 Python 端足够时间处理 LLM 任务图生成）
    HttpRequest->SetTimeout(HttpRequestTimeoutSeconds);
    
    // 绑定完成回调
    HttpRequest->OnProcessRequestComplete().BindRaw(this, &FMACommInbound::OnPollRequestComplete);
    
    // 发送请求
    if (!HttpRequest->ProcessRequest())
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("PollForMessages: Failed to initiate HTTP request"));
    }
}

void FMACommInbound::PollForHITLMessages()
{
    if (!bIsPolling || !Owner || !bEnableHITLPolling)
    {
        return;
    }

    // Mock 模式下不发送实际请求
    if (Owner->bUseMockData)
    {
        UE_LOG(LogMACommInbound, Verbose, TEXT("PollForHITLMessages: Mock mode, skipping"));
        return;
    }

    // 构建完整 URL
    FString FullUrl = Owner->ServerURL + HITLPollEndpoint;
    
    UE_LOG(LogMACommInbound, Verbose, TEXT("PollForHITLMessages: Polling HITL endpoint %s"), *FullUrl);

    // 创建 HTTP GET 请求
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    
    HttpRequest->SetURL(FullUrl);
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));
    
    // 设置超时（使用配置的超时时间，给 Python 端足够时间处理 LLM 任务图生成）
    HttpRequest->SetTimeout(HttpRequestTimeoutSeconds);
    
    // 绑定完成回调
    HttpRequest->OnProcessRequestComplete().BindRaw(this, &FMACommInbound::OnHITLPollRequestComplete);
    
    // 发送请求
    if (!HttpRequest->ProcessRequest())
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("PollForHITLMessages: Failed to initiate HTTP request"));
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
            UE_LOG(LogMACommInbound, Log, TEXT("Python -> UE5 (Platform): Received data"));
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

void FMACommInbound::OnHITLPollRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
    if (!bConnectedSuccessfully || !Response.IsValid())
    {
        // 连接失败，静默处理（轮询会继续）
        UE_LOG(LogMACommInbound, Verbose, TEXT("OnHITLPollRequestComplete: Connection failed"));
        return;
    }

    int32 ResponseCode = Response->GetResponseCode();
    
    if (ResponseCode == 200)
    {
        FString ResponseContent = Response->GetContentAsString();
        
        if (!ResponseContent.IsEmpty() && ResponseContent != TEXT("[]") && ResponseContent != TEXT("{}"))
        {
            UE_LOG(LogMACommInbound, Log, TEXT("Python -> UE5 (HITL): Received data"));
            UE_LOG(LogMACommInbound, Verbose, TEXT("Response: %s"), *ResponseContent);
            
            // 处理响应 (使用相同的处理函数，会根据 message_category 分发)
            HandlePollResponse(ResponseContent);
        }
    }
    else if (ResponseCode == 204)
    {
        // 204 No Content - 没有新消息，正常情况
        UE_LOG(LogMACommInbound, Verbose, TEXT("OnHITLPollRequestComplete: No new messages"));
    }
    else
    {
        // 其他状态码
        UE_LOG(LogMACommInbound, Warning, TEXT("OnHITLPollRequestComplete: Unexpected response code %d"), ResponseCode);
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
        
        // 获取消息类别 (HITLMessage 格式)
        // 如果不存在，从 message_type 推断 (向后兼容)
        FString MessageCategoryStr;
        EMAMessageCategory MessageCategory;
        
        if (MsgObject->TryGetStringField(TEXT("message_category"), MessageCategoryStr))
        {
            MessageCategory = MACommInboundHelpers::StringToMessageCategory(MessageCategoryStr);
            UE_LOG(LogMACommInbound, Log, TEXT("Python -> UE5: Message type: %s, category: %s"), 
                *MessageTypeStr, *MessageCategoryStr);
        }
        else
        {
            // 从 message_type 推断 category
            EMACommMessageType MessageType = MACommInboundHelpers::StringToMessageType(MessageTypeStr);
            MessageCategory = MACommInboundHelpers::GetCategoryForMessageType(MessageType);
            UE_LOG(LogMACommInbound, Log, TEXT("Python -> UE5: Message type: %s, category inferred: %d"), 
                *MessageTypeStr, static_cast<int32>(MessageCategory));
        }
        
        // 根据消息类别分发到不同处理函数
        switch (MessageCategory)
        {
        case EMAMessageCategory::Instruction:
        case EMAMessageCategory::Review:
        case EMAMessageCategory::Decision:
            // HITL 类别消息
            HandleHITLMessage(MsgObject, MessageCategory);
            break;
            
        case EMAMessageCategory::Platform:
        default:
            // Platform 类别消息
            HandlePlatformMessage(MsgObject);
            break;
        }
    }
}

//=============================================================================
// 消息分发 - 根据消息类别
//=============================================================================

void FMACommInbound::HandleHITLMessage(const TSharedPtr<FJsonObject>& MsgObject, EMAMessageCategory Category)
{
    if (!Owner || !MsgObject.IsValid())
    {
        return;
    }

    FString MessageTypeStr;
    MsgObject->TryGetStringField(TEXT("message_type"), MessageTypeStr);

    // 获取消息 ID (用于 HITL 响应)
    FString MessageId;
    MsgObject->TryGetStringField(TEXT("message_id"), MessageId);

    UE_LOG(LogMACommInbound, Log, TEXT("HandleHITLMessage: Processing HITL message type: %s, category: %d, message_id: %s"), 
        *MessageTypeStr, static_cast<int32>(Category), *MessageId);

    // 获取 payload
    const TSharedPtr<FJsonObject>* PayloadObject;
    if (!MsgObject->TryGetObjectField(TEXT("payload"), PayloadObject))
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleHITLMessage: Message missing payload field"));
        return;
    }

    // 根据消息类别和类型处理
    switch (Category)
    {
    case EMAMessageCategory::Instruction:
        // Instruction 类别: Python -> UE5 的指令请求
        // user_instruction: 请求 UE5 端用户输入指令
        if (MessageTypeStr == TEXT("user_instruction"))
        {
            HandleRequestUserCommand(*PayloadObject);
        }
        else
        {
            UE_LOG(LogMACommInbound, Log, TEXT("HandleHITLMessage: Unknown Instruction message type: %s"), *MessageTypeStr);
        }
        break;

    case EMAMessageCategory::Review:
        // Review 类别: 计划审阅请求
        if (MessageTypeStr == TEXT("task_graph"))
        {
            HandleTaskGraph(*PayloadObject);
        }
        else if (MessageTypeStr == TEXT("skill_allocation"))
        {
            // skill_allocation (REVIEW 类别) - 触发 UI 显示，用户审阅
            // 传递消息 ID 用于后续响应
            HandleSkillAllocationWithMessageId(*PayloadObject, MessageId);
        }
        else
        {
            UE_LOG(LogMACommInbound, Log, TEXT("HandleHITLMessage: Unknown Review message type: %s"), *MessageTypeStr);
        }
        break;

    case EMAMessageCategory::Decision:
        // Decision 类别: 决策请求/响应
        UE_LOG(LogMACommInbound, Log, TEXT("HandleHITLMessage: Received Decision message: %s"), *MessageTypeStr);
        // TODO: 实现决策消息处理
        break;

    default:
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleHITLMessage: Unexpected category for HITL message"));
        break;
    }
}

void FMACommInbound::HandlePlatformMessage(const TSharedPtr<FJsonObject>& MsgObject)
{
    if (!Owner || !MsgObject.IsValid())
    {
        return;
    }

    FString MessageTypeStr;
    MsgObject->TryGetStringField(TEXT("message_type"), MessageTypeStr);

    UE_LOG(LogMACommInbound, Log, TEXT("HandlePlatformMessage: Processing Platform message type: %s"), *MessageTypeStr);

    // 获取 payload
    const TSharedPtr<FJsonObject>* PayloadObject;
    if (!MsgObject->TryGetObjectField(TEXT("payload"), PayloadObject))
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandlePlatformMessage: Message missing payload field"));
        return;
    }

    // 根据消息类型分发处理
    if (MessageTypeStr == TEXT("task_graph"))
    {
        // 任务图消息 (向后兼容 - 也可能从 Platform 端点收到)
        HandleTaskGraph(*PayloadObject);
    }
    else if (MessageTypeStr == TEXT("skill_list"))
    {
        // skill_list (PLATFORM 类别) - 直接执行，无需 UI 交互
        HandleSkillList(*PayloadObject, true);
    }
    else if (MessageTypeStr == TEXT("skill_allocation"))
    {
        // 向后兼容: 如果从 Platform 端点收到 skill_allocation，也触发 UI
        // 但根据设计，skill_allocation 应该是 REVIEW 类别
        UE_LOG(LogMACommInbound, Warning, TEXT("HandlePlatformMessage: skill_allocation received from Platform endpoint, should be REVIEW category"));
        HandleSkillAllocation(*PayloadObject);
    }
    else
    {
        UE_LOG(LogMACommInbound, Log, TEXT("HandlePlatformMessage: Unknown message type: %s"), *MessageTypeStr);
    }
}

//=============================================================================
// 具体消息类型处理
//=============================================================================

void FMACommInbound::HandleTaskGraph(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    // [DLOG] 打印收到的 payload 内容
    {
        FString PayloadDebug;
        TSharedRef<TJsonWriter<>> DebugWriter = TJsonWriterFactory<>::Create(&PayloadDebug);
        FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), DebugWriter);
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleTaskGraph: Raw payload: %s"), *PayloadDebug.Left(2000));
    }

    // 检查是否是 HITLMessage 格式 (payload 包含 "review_type" 和 "data" 字段)
    // GSI 格式: payload = { "review_type": "task_graph", "data": { "meta": {...}, "task_graph": {...} } }
    // 如果是，提取 "data" 字段作为实际的任务图数据
    TSharedPtr<FJsonObject> ActualTaskGraphObject = PayloadObject;
    
    FString ReviewType;
    const TSharedPtr<FJsonObject>* DataObject;
    if (PayloadObject->TryGetStringField(TEXT("review_type"), ReviewType) &&
        PayloadObject->TryGetObjectField(TEXT("data"), DataObject))
    {
        // HITLMessage 格式: { "review_type": "task_graph", "data": { "meta": {...}, "task_graph": {...} } }
        ActualTaskGraphObject = *DataObject;
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleTaskGraph: Detected HITLMessage format, review_type=%s"), *ReviewType);
        
        // [DLOG] 打印提取的 data 内容
        FString DataDebug;
        TSharedRef<TJsonWriter<>> DataWriter = TJsonWriterFactory<>::Create(&DataDebug);
        FJsonSerializer::Serialize(ActualTaskGraphObject.ToSharedRef(), DataWriter);
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleTaskGraph: Extracted data (first 2000 chars): %s"), *DataDebug.Left(2000));
        
        // [DLOG] 检查 data 对象中是否有 task_graph 字段
        bool bHasTaskGraph = ActualTaskGraphObject->HasField(TEXT("task_graph"));
        bool bHasMeta = ActualTaskGraphObject->HasField(TEXT("meta"));
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleTaskGraph: data has task_graph=%s, has meta=%s"), 
            bHasTaskGraph ? TEXT("true") : TEXT("false"),
            bHasMeta ? TEXT("true") : TEXT("false"));
        
        // 如果 data 中没有 task_graph 字段，但有 nodes 字段，说明 data 本身就是 task_graph 内容
        // 这种情况下需要包装成标准格式
        if (!bHasTaskGraph && ActualTaskGraphObject->HasField(TEXT("nodes")))
        {
            UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleTaskGraph: data contains nodes directly, wrapping as task_graph"));
            TSharedPtr<FJsonObject> WrappedObject = MakeShareable(new FJsonObject());
            WrappedObject->SetObjectField(TEXT("task_graph"), ActualTaskGraphObject);
            ActualTaskGraphObject = WrappedObject;
        }
    }
    else
    {
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleTaskGraph: Direct format (no HITLMessage wrapper)"));
        
        // [DLOG] 检查直接格式中是否有 task_graph 字段
        bool bHasTaskGraph = PayloadObject->HasField(TEXT("task_graph"));
        bool bHasMeta = PayloadObject->HasField(TEXT("meta"));
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleTaskGraph: payload has task_graph=%s, has meta=%s"), 
            bHasTaskGraph ? TEXT("true") : TEXT("false"),
            bHasMeta ? TEXT("true") : TEXT("false"));
        
        // 如果 payload 中没有 task_graph 字段，但有 nodes 字段，说明 payload 本身就是 task_graph 内容
        if (!bHasTaskGraph && PayloadObject->HasField(TEXT("nodes")))
        {
            UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleTaskGraph: payload contains nodes directly, wrapping as task_graph"));
            TSharedPtr<FJsonObject> WrappedObject = MakeShareable(new FJsonObject());
            WrappedObject->SetObjectField(TEXT("task_graph"), PayloadObject);
            ActualTaskGraphObject = WrappedObject;
        }
    }

    // 将实际任务图对象序列化回 JSON 字符串
    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(ActualTaskGraphObject.ToSharedRef(), Writer);

    UE_LOG(LogMACommInbound, Log, TEXT("HandleTaskGraph: Received task graph payload"));
    UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleTaskGraph: PayloadJson (first 2000 chars): %s"), *PayloadJson.Left(2000));

    // 使用 FromResponseJson 解析 (支持 meta + task_graph 格式)
    FMATaskGraphData TaskGraphData;
    FString ErrorMessage;
    
    if (FMATaskGraphData::FromResponseJson(PayloadJson, TaskGraphData, ErrorMessage))
    {
        UE_LOG(LogMACommInbound, Log, TEXT("HandleTaskGraph: Parsed task graph with %d nodes, %d edges"),
            TaskGraphData.Nodes.Num(), TaskGraphData.Edges.Num());

        // 验证 DAG 有效性
        if (TaskGraphData.IsValidDAG())
        {
            // 保存到 TempDataManager
            if (UGameInstance* GameInstance = Owner->GetGameInstance())
            {
                if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
                {
                    if (TempDataMgr->SaveTaskGraph(TaskGraphData))
                    {
                        UE_LOG(LogMACommInbound, Log, TEXT("HandleTaskGraph: Saved task graph to temp file"));
                    }
                    else
                    {
                        UE_LOG(LogMACommInbound, Warning, TEXT("HandleTaskGraph: Failed to save task graph to temp file"));
                    }
                }
            }

            // 转换为 FMATaskPlan 格式并广播 (兼容现有委托)
            FMATaskPlan TaskPlan;
            for (const FMATaskNodeData& NodeData : TaskGraphData.Nodes)
            {
                FMATaskPlanNode PlanNode;
                PlanNode.NodeId = NodeData.TaskId;
                PlanNode.TaskType = NodeData.Description;
                if (!NodeData.Location.IsEmpty())
                {
                    PlanNode.Parameters.Add(TEXT("location"), NodeData.Location);
                }
                TaskPlan.Nodes.Add(PlanNode);
            }
            for (const FMATaskEdgeData& EdgeData : TaskGraphData.Edges)
            {
                FMATaskPlanEdge PlanEdge;
                PlanEdge.FromNodeId = EdgeData.FromNodeId;
                PlanEdge.ToNodeId = EdgeData.ToNodeId;
                TaskPlan.Edges.Add(PlanEdge);
            }

            // 广播委托
            Owner->OnTaskPlanReceived.Broadcast(TaskPlan);
        }
        else
        {
            UE_LOG(LogMACommInbound, Warning, TEXT("HandleTaskGraph: Received invalid DAG (contains cycle)"));
        }
    }
    else
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("[DLOG] HandleTaskGraph: Failed to parse task graph: %s"), *ErrorMessage);
        UE_LOG(LogMACommInbound, Warning, TEXT("[DLOG] HandleTaskGraph: PayloadJson was: %s"), *PayloadJson);
    }
}

void FMACommInbound::HandleSkillList(const TSharedPtr<FJsonObject>& PayloadObject, bool bExecutable)
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
        UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillList: Received SkillList with %d time steps, executable=%s"),
            SkillList.TotalTimeSteps, bExecutable ? TEXT("true") : TEXT("false"));

        // 直接获取 MACommandManager 并执行技能列表
        UWorld* World = Owner->GetWorld();
        if (World)
        {
            if (UMACommandManager* CommandMgr = World->GetSubsystem<UMACommandManager>())
            {
                UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillList: Executing skill list with %d time steps"), 
                    SkillList.TotalTimeSteps);
                CommandMgr->ExecuteSkillList(SkillList);
            }
            else
            {
                UE_LOG(LogMACommInbound, Error, TEXT("HandleSkillList: MACommandManager not available"));
            }
        }
        else
        {
            UE_LOG(LogMACommInbound, Error, TEXT("HandleSkillList: World is null"));
        }
    }
    else
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillList: Failed to parse SkillList payload"));
    }
}

void FMACommInbound::HandleSkillAllocation(const TSharedPtr<FJsonObject>& PayloadObject)
{
    // 向后兼容: 没有消息 ID 的情况
    HandleSkillAllocationWithMessageId(PayloadObject, TEXT(""));
}

void FMACommInbound::HandleSkillAllocationWithMessageId(const TSharedPtr<FJsonObject>& PayloadObject, const FString& OriginalMessageId)
{
    if (!Owner)
    {
        return;
    }

    // [DLOG] 打印收到的 payload 内容
    {
        FString PayloadDebug;
        TSharedRef<TJsonWriter<>> DebugWriter = TJsonWriterFactory<>::Create(&PayloadDebug);
        FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), DebugWriter);
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleSkillAllocation: Raw payload: %s"), *PayloadDebug.Left(2000));
    }

    // 检查是否是 HITLMessage 格式 (payload 包含 "review_type" 和 "data" 字段)
    // GSI 格式: payload = { "review_type": "skill_allocation", "data": { "robot_view": {...}, "timestep_skills": {...} } }
    // 如果是，提取 "data" 字段作为实际的技能分配数据
    TSharedPtr<FJsonObject> ActualAllocationObject = PayloadObject;
    
    FString ReviewType;
    const TSharedPtr<FJsonObject>* DataObject;
    if (PayloadObject->TryGetStringField(TEXT("review_type"), ReviewType) &&
        PayloadObject->TryGetObjectField(TEXT("data"), DataObject))
    {
        // HITLMessage 格式: { "review_type": "skill_allocation", "data": { ... } }
        ActualAllocationObject = *DataObject;
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleSkillAllocation: Detected HITLMessage format, review_type=%s"), *ReviewType);
        
        // [DLOG] 打印提取的 data 内容
        FString DataDebug;
        TSharedRef<TJsonWriter<>> DataWriter = TJsonWriterFactory<>::Create(&DataDebug);
        FJsonSerializer::Serialize(ActualAllocationObject.ToSharedRef(), DataWriter);
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleSkillAllocation: Extracted data (first 2000 chars): %s"), *DataDebug.Left(2000));
        
        // [DLOG] 检查 data 对象中的字段
        bool bHasTimestepSkills = ActualAllocationObject->HasField(TEXT("timestep_skills"));
        bool bHasRobotView = ActualAllocationObject->HasField(TEXT("robot_view"));
        bool bHasName = ActualAllocationObject->HasField(TEXT("name"));
        bool bHasData = ActualAllocationObject->HasField(TEXT("data"));
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleSkillAllocation: data has timestep_skills=%s, robot_view=%s, name=%s, data=%s"), 
            bHasTimestepSkills ? TEXT("true") : TEXT("false"),
            bHasRobotView ? TEXT("true") : TEXT("false"),
            bHasName ? TEXT("true") : TEXT("false"),
            bHasData ? TEXT("true") : TEXT("false"));
    }
    else
    {
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleSkillAllocation: Direct format (no HITLMessage wrapper)"));
        
        // [DLOG] 检查直接格式中的字段
        bool bHasTimestepSkills = PayloadObject->HasField(TEXT("timestep_skills"));
        bool bHasRobotView = PayloadObject->HasField(TEXT("robot_view"));
        bool bHasName = PayloadObject->HasField(TEXT("name"));
        bool bHasData = PayloadObject->HasField(TEXT("data"));
        UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleSkillAllocation: payload has timestep_skills=%s, robot_view=%s, name=%s, data=%s"), 
            bHasTimestepSkills ? TEXT("true") : TEXT("false"),
            bHasRobotView ? TEXT("true") : TEXT("false"),
            bHasName ? TEXT("true") : TEXT("false"),
            bHasData ? TEXT("true") : TEXT("false"));
    }

    // 将实际技能分配对象序列化回 JSON 字符串
    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(ActualAllocationObject.ToSharedRef(), Writer);

    UE_LOG(LogMACommInbound, Log, TEXT("[DLOG] HandleSkillAllocation: PayloadJson (first 2000 chars): %s"), *PayloadJson.Left(2000));

    // 解析 payload 为 FMASkillAllocationData
    FMASkillAllocationData AllocationData;
    FString ErrorMessage;
    if (!FMASkillAllocationData::FromJson(PayloadJson, AllocationData, ErrorMessage))
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillAllocation: Failed to parse: %s"), *ErrorMessage);
        UE_LOG(LogMACommInbound, Warning, TEXT("[DLOG] HandleSkillAllocation: PayloadJson was: %s"), *PayloadJson);
        return;
    }

    // 设置原始消息 ID (用于 HITL 响应)
    AllocationData.OriginalMessageId = OriginalMessageId;

    UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillAllocation: Received skill allocation '%s' with %d time steps, OriginalMessageId=%s"),
        *AllocationData.Name, AllocationData.Data.Num(), *OriginalMessageId);

    // 保存到 TempDataManager
    if (UGameInstance* GameInstance = Owner->GetGameInstance())
    {
        if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
        {
            if (TempDataMgr->SaveSkillAllocation(AllocationData))
            {
                UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillAllocation: Saved skill allocation to temp file"));
            }
            else
            {
                UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillAllocation: Failed to save skill allocation to temp file"));
            }
        }
        else
        {
            UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillAllocation: TempDataManager not available"));
        }
    }

    // 广播 OnSkillAllocationReceived 委托
    Owner->OnSkillAllocationReceived.Broadcast(AllocationData);
}

void FMACommInbound::HandleRequestUserCommand(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommInbound, Log, TEXT("HandleRequestUserCommand: Received request"));

    // 广播委托
    Owner->OnRequestUserCommandReceived.Broadcast();
}
