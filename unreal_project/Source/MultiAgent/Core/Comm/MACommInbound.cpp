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

    UE_LOG(LogMACommInbound, Log, TEXT("HandleHITLMessage: Processing HITL message type: %s, category: %d"), 
        *MessageTypeStr, static_cast<int32>(Category));

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
            HandleSkillAllocation(*PayloadObject);
        }
        else
        {
            UE_LOG(LogMACommInbound, Log, TEXT("HandleHITLMessage: Unknown Review message type: %s"), *MessageTypeStr);
        }
        break;

    case EMAMessageCategory::Decision:
        // Decision 类别: 决策请求/响应
        UE_LOG(LogMACommInbound, Log, TEXT("HandleHITLMessage: Received Decision message: %s"), *MessageTypeStr);
        HandleDecision(*PayloadObject);
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
    if (MessageTypeStr == TEXT("skill_list"))
    {
        // skill_list (PLATFORM 类别) - 直接执行，无需 UI 交互
        HandleSkillList(*PayloadObject, true);
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

    // 检查是否是 HITLMessage 格式 (payload 包含 "review_type" 和 "data" 字段)
    TSharedPtr<FJsonObject> ActualTaskGraphObject = PayloadObject;
    
    FString ReviewType;
    const TSharedPtr<FJsonObject>* DataObject;
    if (PayloadObject->TryGetStringField(TEXT("review_type"), ReviewType) &&
        PayloadObject->TryGetObjectField(TEXT("data"), DataObject))
    {
        // HITLMessage 格式
        ActualTaskGraphObject = *DataObject;
        UE_LOG(LogMACommInbound, Log, TEXT("HandleTaskGraph: Detected HITLMessage format, review_type=%s"), *ReviewType);
        
        // 如果 data 中没有 task_graph 字段，但有 nodes 字段，包装成标准格式
        if (!ActualTaskGraphObject->HasField(TEXT("task_graph")) && ActualTaskGraphObject->HasField(TEXT("nodes")))
        {
            TSharedPtr<FJsonObject> WrappedObject = MakeShareable(new FJsonObject());
            WrappedObject->SetObjectField(TEXT("task_graph"), ActualTaskGraphObject);
            ActualTaskGraphObject = WrappedObject;
        }
    }
    else
    {
        // 直接格式: 如果没有 task_graph 字段但有 nodes 字段，包装成标准格式
        if (!PayloadObject->HasField(TEXT("task_graph")) && PayloadObject->HasField(TEXT("nodes")))
        {
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
                }
            }

            // 转换为 FMATaskPlan 格式并广播
            FMATaskPlan TaskPlan;
            for (const FMATaskNodeData& NodeData : TaskGraphData.Nodes)
            {
                FMATaskPlanNode PlanNode;
                PlanNode.NodeId = NodeData.TaskId;
                // 优先使用 Description，若为空则从 required_skills 生成摘要
                if (!NodeData.Description.IsEmpty())
                {
                    PlanNode.TaskType = NodeData.Description;
                }
                else if (NodeData.RequiredSkills.Num() > 0)
                {
                    // 用第一个技能的名称作为 TaskType
                    PlanNode.TaskType = NodeData.RequiredSkills[0].SkillName;
                }
                else
                {
                    PlanNode.TaskType = NodeData.TaskId;
                }
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
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleTaskGraph: Failed to parse task graph: %s"), *ErrorMessage);
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

        // 广播委托 (用于 UI 通知)
        Owner->OnSkillListReceived.Broadcast(SkillList, bExecutable);

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
    }
    else
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillList: Failed to parse SkillList payload"));
    }
}

void FMACommInbound::HandleSkillAllocation(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    // 检查是否是 HITLMessage 格式 (payload 包含 "review_type" 和 "data" 字段)
    TSharedPtr<FJsonObject> ActualAllocationObject = PayloadObject;
    
    FString ReviewType;
    const TSharedPtr<FJsonObject>* DataObject;
    if (PayloadObject->TryGetStringField(TEXT("review_type"), ReviewType) &&
        PayloadObject->TryGetObjectField(TEXT("data"), DataObject))
    {
        // HITLMessage 格式
        ActualAllocationObject = *DataObject;
        UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillAllocation: Detected HITLMessage format, review_type=%s"), *ReviewType);
    }

    // 将实际技能分配对象序列化回 JSON 字符串
    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(ActualAllocationObject.ToSharedRef(), Writer);

    // 解析 payload 为 FMASkillAllocationData
    FMASkillAllocationData AllocationData;
    FString ErrorMessage;
    if (!FMASkillAllocationData::FromJson(PayloadJson, AllocationData, ErrorMessage))
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillAllocation: Failed to parse: %s"), *ErrorMessage);
        return;
    }

    UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillAllocation: Received skill allocation '%s' with %d time steps"),
        *AllocationData.Name, AllocationData.Data.Num());

    // 保存到 TempDataManager
    if (UGameInstance* GameInstance = Owner->GetGameInstance())
    {
        if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
        {
            if (TempDataMgr->SaveSkillAllocation(AllocationData))
            {
                UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillAllocation: Saved skill allocation to temp file"));
            }
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

void FMACommInbound::HandleDecision(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    UE_LOG(LogMACommInbound, Log, TEXT("HandleDecision: Processing decision message"));

    // 提取必需字段: description, decision_type
    FString Description;
    if (!PayloadObject->TryGetStringField(TEXT("description"), Description))
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleDecision: Missing required 'description' field, discarding message"));
        return;
    }

    FString DecisionType;
    if (!PayloadObject->TryGetStringField(TEXT("decision_type"), DecisionType))
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleDecision: Missing required 'decision_type' field, discarding message"));
        return;
    }

    // 提取 context 字段并序列化为 JSON 字符串
    FString ContextJson;
    const TSharedPtr<FJsonObject>* ContextObject;
    if (PayloadObject->TryGetObjectField(TEXT("context"), ContextObject))
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContextJson);
        FJsonSerializer::Serialize((*ContextObject).ToSharedRef(), Writer);
    }

    UE_LOG(LogMACommInbound, Log, TEXT("HandleDecision: DecisionType=%s, Description=%s"), *DecisionType, *Description);

    // 广播 OnDecisionReceived 委托
    Owner->OnDecisionReceived.Broadcast(Description, ContextJson);
}
