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
        else if (MessageTypeStr == TEXT("task_graph"))
        {
            // 新格式：来自 mock_backend 的任务图
            HandleTaskGraph(*PayloadObject);
        }
        else if (MessageTypeStr == TEXT("world_model_graph"))
        {
            HandleWorldModelGraph(*PayloadObject);
        }
        else if (MessageTypeStr == TEXT("skill_list"))
        {
            HandleSkillList(*PayloadObject);
        }
        else if (MessageTypeStr == TEXT("skill_status_update"))
        {
            HandleSkillStatusUpdate(*PayloadObject);
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
            
            // 保存到 TempDataManager (需求 3.1)
            if (UGameInstance* GameInstance = Owner->GetGameInstance())
            {
                if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
                {
                    // 将 FMATaskPlanDAG 转换为 FMATaskGraphData 格式
                    FMATaskGraphData TaskGraphData;
                    TaskGraphData.Description = TEXT("Task plan received from backend");
                    
                    // 转换节点
                    for (const FMATaskPlanNode& PlanNode : TaskPlan.Nodes)
                    {
                        FMATaskNodeData NodeData;
                        NodeData.TaskId = PlanNode.NodeId;
                        NodeData.Description = PlanNode.TaskType;
                        
                        // 从参数中提取位置信息
                        if (const FString* LocationParam = PlanNode.Parameters.Find(TEXT("location")))
                        {
                            NodeData.Location = *LocationParam;
                        }
                        
                        TaskGraphData.Nodes.Add(NodeData);
                    }
                    
                    // 转换边
                    for (const FMATaskPlanEdge& PlanEdge : TaskPlan.Edges)
                    {
                        FMATaskEdgeData EdgeData;
                        EdgeData.FromNodeId = PlanEdge.FromNodeId;
                        EdgeData.ToNodeId = PlanEdge.ToNodeId;
                        EdgeData.EdgeType = TEXT("sequential");
                        TaskGraphData.Edges.Add(EdgeData);
                    }
                    
                    // 保存到临时文件
                    if (TempDataMgr->SaveTaskGraph(TaskGraphData))
                    {
                        UE_LOG(LogMACommInbound, Log, TEXT("HandleTaskPlanDAG: Saved task graph to temp file"));
                    }
                    else
                    {
                        UE_LOG(LogMACommInbound, Warning, TEXT("HandleTaskPlanDAG: Failed to save task graph to temp file"));
                    }
                }
            }
            
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

void FMACommInbound::HandleTaskGraph(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    // 将 payload 对象序列化回 JSON 字符串
    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), Writer);

    UE_LOG(LogMACommInbound, Log, TEXT("HandleTaskGraph: Received task graph payload"));
    UE_LOG(LogMACommInbound, Verbose, TEXT("HandleTaskGraph: Payload: %s"), *PayloadJson);

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

            // 转换为 FMATaskPlanDAG 格式并广播 (兼容现有委托)
            FMATaskPlanDAG TaskPlan;
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
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleTaskGraph: Failed to parse task graph: %s"), *ErrorMessage);
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

        // 保存到 TempDataManager (需求 3.2)
        if (UGameInstance* GameInstance = Owner->GetGameInstance())
        {
            if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
            {
                // 将 FMASkillListMessage 转换为 FMASkillAllocationData 格式
                // 构建完整的 JSON 格式: { "name": "...", "description": "...", "data": {...} }
                FString SkillListJson = SkillList.ToJson();
                
                // 创建包含 name, description, data 的完整 JSON
                TSharedPtr<FJsonObject> FullJsonObject = MakeShareable(new FJsonObject());
                FullJsonObject->SetStringField(TEXT("name"), TEXT("Received Skill List"));
                FullJsonObject->SetStringField(TEXT("description"), TEXT("Skill list received from backend"));
                
                // 解析 SkillListJson 为 JSON 对象
                TSharedPtr<FJsonObject> DataObject;
                TSharedRef<TJsonReader<>> DataReader = TJsonReaderFactory<>::Create(SkillListJson);
                if (FJsonSerializer::Deserialize(DataReader, DataObject) && DataObject.IsValid())
                {
                    FullJsonObject->SetObjectField(TEXT("data"), DataObject);
                }
                
                // 序列化完整 JSON
                FString FullJson;
                TSharedRef<TJsonWriter<>> FullWriter = TJsonWriterFactory<>::Create(&FullJson);
                FJsonSerializer::Serialize(FullJsonObject.ToSharedRef(), FullWriter);
                
                // 解析为 FMASkillAllocationData
                FMASkillAllocationData AllocationData;
                FString ErrorMessage;
                if (FMASkillAllocationData::FromJson(FullJson, AllocationData, ErrorMessage))
                {
                    // 保存到临时文件
                    if (TempDataMgr->SaveSkillList(AllocationData))
                    {
                        UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillList: Saved skill list to temp file"));
                    }
                    else
                    {
                        UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillList: Failed to save skill list to temp file"));
                    }
                }
                else
                {
                    UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillList: Failed to convert to FMASkillAllocationData: %s"), *ErrorMessage);
                }
            }
        }

        // 注意：根据需求 6.3，不再直接触发技能执行
        // 用户需要手动点击 "Start Executing" 按钮来执行
        // 原代码: CommandManager->ExecuteSkillList(SkillList);
        UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillList: Skill list saved. User should click 'Start Executing' to run."));
        
        // 广播委托，通知 UI 显示通知
        Owner->OnSkillListReceived.Broadcast(SkillList);
    }
    else
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillList: Failed to parse SkillList payload"));
    }
}

void FMACommInbound::HandleSkillStatusUpdate(const TSharedPtr<FJsonObject>& PayloadObject)
{
    if (!Owner)
    {
        return;
    }

    // 将 payload 对象序列化回 JSON 字符串
    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), Writer);

    FMASkillStatusUpdateMessage StatusUpdate;
    if (FMASkillStatusUpdateMessage::FromJson(PayloadJson, StatusUpdate))
    {
        UE_LOG(LogMACommInbound, Log, TEXT("HandleSkillStatusUpdate: TimeStep=%d, RobotId=%s, Status=%s"),
            StatusUpdate.TimeStep, *StatusUpdate.RobotId, *StatusUpdate.Status);

        // 广播委托
        Owner->OnSkillStatusUpdateReceived.Broadcast(StatusUpdate);
    }
    else
    {
        UE_LOG(LogMACommInbound, Warning, TEXT("HandleSkillStatusUpdate: Failed to parse SkillStatusUpdate payload"));
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
