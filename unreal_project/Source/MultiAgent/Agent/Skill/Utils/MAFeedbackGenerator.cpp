// MAFeedbackGenerator.cpp
// 反馈生成器实现

#include "MAFeedbackGenerator.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Core/Manager/scene_graph_tools/MASceneGraphQuery.h"
#include "MAUESceneQuery.h"
#include "EngineUtils.h"  // For TActorIterator
#include "Serialization/JsonSerializer.h"

//=============================================================================
// 场景图节点 JSON 构建方法实现
//=============================================================================

TSharedPtr<FJsonObject> FMAFeedbackGenerator::BuildNodeJsonFromSceneGraph(const FMASceneGraphNode& Node)
{
    TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject());
    
    // 设置 id
    NodeObject->SetStringField(TEXT("id"), Node.Id);
    
    // 构建 properties 对象
    TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject());
    PropertiesObject->SetStringField(TEXT("category"), Node.Category);
    PropertiesObject->SetStringField(TEXT("type"), Node.Type);
    PropertiesObject->SetStringField(TEXT("label"), Node.Label);
    
    // 添加 location_label
    if (!Node.LocationLabel.IsEmpty())
    {
        PropertiesObject->SetStringField(TEXT("location_label"), Node.LocationLabel);
    }
    
    // 添加可拾取物品特有属性
    if (Node.IsPickupItem())
    {
        PropertiesObject->SetBoolField(TEXT("is_carried"), Node.bIsCarried);
        if (!Node.CarrierId.IsEmpty())
        {
            PropertiesObject->SetStringField(TEXT("carrier_id"), Node.CarrierId);
        }
    }
    
    // 添加动态节点标记
    if (Node.bIsDynamic)
    {
        PropertiesObject->SetBoolField(TEXT("is_dynamic"), true);
    }
    
    // 添加 Features
    for (const auto& Feature : Node.Features)
    {
        PropertiesObject->SetStringField(Feature.Key, Feature.Value);
    }
    
    NodeObject->SetObjectField(TEXT("properties"), PropertiesObject);
    
    // 构建 shape 对象
    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    ShapeObject->SetStringField(TEXT("type"), Node.ShapeType.IsEmpty() ? TEXT("point") : Node.ShapeType);
    
    // 设置 center
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Z)));
    ShapeObject->SetArrayField(TEXT("center"), CenterArray);
    
    NodeObject->SetObjectField(TEXT("shape"), ShapeObject);
    
    return NodeObject;
}

TSharedPtr<FJsonObject> FMAFeedbackGenerator::BuildNodeJsonFromUE5Data(
    const FString& Id,
    const FString& Label,
    const FVector& Location,
    const FString& Type,
    const FString& Category,
    const TMap<FString, FString>& Features)
{
    TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject());
    
    // 设置 id
    NodeObject->SetStringField(TEXT("id"), Id);
    
    // 构建 properties 对象
    TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject());
    if (!Category.IsEmpty())
    {
        PropertiesObject->SetStringField(TEXT("category"), Category);
    }
    if (!Type.IsEmpty())
    {
        PropertiesObject->SetStringField(TEXT("type"), Type);
    }
    PropertiesObject->SetStringField(TEXT("label"), Label);
    
    // 添加 Features
    for (const auto& Feature : Features)
    {
        PropertiesObject->SetStringField(Feature.Key, Feature.Value);
    }
    
    NodeObject->SetObjectField(TEXT("properties"), PropertiesObject);
    
    // 构建 shape 对象
    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
    
    // 设置 center
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
    ShapeObject->SetArrayField(TEXT("center"), CenterArray);
    
    NodeObject->SetObjectField(TEXT("shape"), ShapeObject);
    
    return NodeObject;
}

//=============================================================================
// 场景图查询辅助方法实现
//=============================================================================

UMASceneGraphManager* FMAFeedbackGenerator::GetSceneGraphManager(AMACharacter* Agent)
{
    if (!Agent)
    {
        return nullptr;
    }
    
    UWorld* World = Agent->GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }
    
    return GameInstance->GetSubsystem<UMASceneGraphManager>();
}

bool FMAFeedbackGenerator::GetEntityInfoWithFallback(
    AMACharacter* Agent,
    const FString& EntityId,
    FString& OutLabel,
    FVector& OutLocation,
    FString& OutType)
{
    if (!Agent || EntityId.IsEmpty())
    {
        return false;
    }
    
    // 1. 首先尝试从场景图获取
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, EntityId);
        
        if (Node.IsValid())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            OutType = Node.Type;
            UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Found entity '%s' in scene graph: Label=%s, Location=(%f, %f, %f)"),
                *EntityId, *OutLabel, OutLocation.X, OutLocation.Y, OutLocation.Z);
            return true;
        }
    }
    
    // 2. 场景图未找到，回退到 UE5 场景查询
    UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Entity '%s' not found in scene graph, falling back to UE5 scene query"), *EntityId);
    
    UWorld* World = Agent->GetWorld();
    if (World)
    {
        // 尝试通过名称查找 Actor
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Actor = *It;
            if (Actor && Actor->GetName().Contains(EntityId))
            {
                OutLabel = Actor->GetName();
                OutLocation = Actor->GetActorLocation();
                OutType = Actor->GetClass()->GetName();
                UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Found entity '%s' via UE5 scene query: Label=%s"),
                    *EntityId, *OutLabel);
                return true;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[FMAFeedbackGenerator] Entity '%s' not found in scene graph or UE5 scene"), *EntityId);
    return false;
}

bool FMAFeedbackGenerator::GetRobotInfoFromSceneGraph(
    AMACharacter* Agent,
    const FString& RobotId,
    FString& OutLabel,
    FVector& OutLocation,
    FString& OutAgentType)
{
    if (!Agent || RobotId.IsEmpty())
    {
        return false;
    }
    
    // 1. 首先尝试从场景图获取
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, RobotId);
        
        if (Node.IsValid() && Node.IsRobot())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            OutAgentType = Node.Type;  // 机器人类型存储在 Type 字段
            UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Found robot '%s' in scene graph: Label=%s, Type=%s"),
                *RobotId, *OutLabel, *OutAgentType);
            return true;
        }
    }
    
    // 2. 回退到 UE5 场景查询
    UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Robot '%s' not found in scene graph, falling back to UE5 scene query"), *RobotId);
    
    UWorld* World = Agent->GetWorld();
    if (World)
    {
        AMACharacter* FoundRobot = FMAUESceneQuery::FindRobotByName(World, RobotId);
        if (FoundRobot)
        {
            OutLabel = FoundRobot->AgentID;
            OutLocation = FoundRobot->GetActorLocation();
            // 将 EMAAgentType 枚举转换为字符串
            OutAgentType = UEnum::GetValueAsString(FoundRobot->AgentType);
            // 移除枚举前缀 "EMAAgentType::"
            OutAgentType.RemoveFromStart(TEXT("EMAAgentType::"));
            return true;
        }
    }
    
    return false;
}

bool FMAFeedbackGenerator::GetPickupItemInfoFromSceneGraph(
    AMACharacter* Agent,
    const FString& ItemId,
    FString& OutLabel,
    FVector& OutLocation,
    TMap<FString, FString>& OutFeatures)
{
    if (!Agent || ItemId.IsEmpty())
    {
        return false;
    }
    
    // 1. 首先尝试从场景图获取
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, ItemId);
        
        if (Node.IsValid() && Node.IsPickupItem())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            OutFeatures = Node.Features;
            UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Found pickup item '%s' in scene graph: Label=%s"),
                *ItemId, *OutLabel);
            return true;
        }
    }
    
    // 2. 回退到 UE5 场景查询
    UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Pickup item '%s' not found in scene graph, falling back to UE5 scene query"), *ItemId);
    
    UWorld* World = Agent->GetWorld();
    if (World)
    {
        FMASemanticLabel Label;
        Label.Class = TEXT("object");
        Label.Features.Add(TEXT("label"), ItemId);
        
        FMAUESceneQueryResult Result = FMAUESceneQuery::FindObjectByLabel(World, Label);
        if (Result.bFound)
        {
            OutLabel = Result.Name;
            OutLocation = Result.Location;
            // 从 UE5 查询结果提取特征
            OutFeatures = Result.Label.Features;
            return true;
        }
    }
    
    return false;
}

bool FMAFeedbackGenerator::GetChargingStationInfoFromSceneGraph(
    AMACharacter* Agent,
    const FString& StationId,
    FString& OutLabel,
    FVector& OutLocation)
{
    if (!Agent || StationId.IsEmpty())
    {
        return false;
    }
    
    // 1. 首先尝试从场景图获取
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, StationId);
        
        if (Node.IsValid() && Node.IsChargingStation())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Found charging station '%s' in scene graph: Label=%s"),
                *StationId, *OutLabel);
            return true;
        }
    }
    
    // 2. 回退到 UE5 场景查询
    UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Charging station '%s' not found in scene graph, falling back to UE5 scene query"), *StationId);
    
    // 充电站通常在场景图中，如果没找到则返回失败
    return false;
}

void FMAFeedbackGenerator::AddEntityInfoToFeedback(
    FMASkillExecutionFeedback& Feedback,
    const FString& Prefix,
    const FString& Label,
    const FVector& Location,
    const FString& Type)
{
    // 添加实体标签
    if (!Label.IsEmpty())
    {
        Feedback.Data.Add(FString::Printf(TEXT("%s_label"), *Prefix), Label);
    }
    
    // 添加实体位置
    if (!Location.IsZero())
    {
        Feedback.Data.Add(FString::Printf(TEXT("%s_x"), *Prefix), FString::Printf(TEXT("%.1f"), Location.X));
        Feedback.Data.Add(FString::Printf(TEXT("%s_y"), *Prefix), FString::Printf(TEXT("%.1f"), Location.Y));
        Feedback.Data.Add(FString::Printf(TEXT("%s_z"), *Prefix), FString::Printf(TEXT("%.1f"), Location.Z));
        Feedback.Data.Add(FString::Printf(TEXT("%s_location"), *Prefix), 
            FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), Location.X, Location.Y, Location.Z));
    }
    
    // 添加实体类型
    if (!Type.IsEmpty())
    {
        Feedback.Data.Add(FString::Printf(TEXT("%s_type"), *Prefix), Type);
    }
}

void FMAFeedbackGenerator::AddCommonFieldsToFeedback(
    FMASkillExecutionFeedback& Feedback,
    UMASkillComponent* SkillComp)
{
    if (!SkillComp)
    {
        return;
    }
    
    const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
    
    // 添加 task_id (如果存在)
    if (!Context.TaskId.IsEmpty())
    {
        Feedback.Data.Add(TEXT("task_id"), Context.TaskId);
    }
    
    // 添加 robot_id 和 robot_label (Planner's feedback_processor 需要)
    if (!Feedback.AgentId.IsEmpty())
    {
        Feedback.Data.Add(TEXT("robot_id"), Feedback.AgentId);
        
        // 尝试从场景图获取机器人的 Label
        AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
        if (Agent)
        {
            FString RobotLabel;
            FVector RobotLocation;
            FString RobotType;
            
            if (GetRobotInfoFromSceneGraph(Agent, Feedback.AgentId, RobotLabel, RobotLocation, RobotType))
            {
                // 使用场景图中的 Label
                Feedback.Data.Add(TEXT("robot_label"), RobotLabel);
            }
            else
            {
                // 回退：使用 AgentName，如果为空则使用 AgentId
                FString FallbackLabel = Agent->AgentName.IsEmpty() ? Feedback.AgentId : Agent->AgentName;
                Feedback.Data.Add(TEXT("robot_label"), FallbackLabel);
            }
        }
        else
        {
            // 无法获取 Agent，使用 AgentId 作为回退
            Feedback.Data.Add(TEXT("robot_label"), Feedback.AgentId);
        }
    }
}

//=============================================================================
// 主入口
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::Generate(AMACharacter* Agent, EMACommand Command, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    if (!Agent)
    {
        Feedback.bSuccess = false;
        Feedback.Message = TEXT("Invalid agent");
        return Feedback;
    }
    
    // 统一初始化公共字段
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = UMACommandManager::CommandToString(Command);
    Feedback.bSuccess = bSuccess;
    
    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    
    // 调用具体的反馈生成函数填充 Data 和 Message
    switch (Command)
    {
        case EMACommand::Navigate:
            GenerateNavigateFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::Search:
            GenerateSearchFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::Follow:
            GenerateFollowFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::Charge:
            GenerateChargeFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::Place:
            GeneratePlaceFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::TakeOff:
            GenerateTakeOffFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::Land:
            GenerateLandFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::ReturnHome:
            GenerateReturnHomeFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::TakePhoto:
            GenerateTakePhotoFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::Broadcast:
            GenerateBroadcastFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::HandleHazard:
            GenerateHandleHazardFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::Guide:
            GenerateGuideFeedback(Feedback, Agent, SkillComp, bSuccess, Message);
            break;
        case EMACommand::Idle:
            GenerateIdleFeedback(Feedback, Agent, bSuccess, Message);
            break;
        default:
            Feedback.Message = Message;
            break;
    }
    
    return Feedback;
}

//=============================================================================
// Navigate 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateNavigateFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        FVector Loc = Params.TargetLocation;
        Feedback.Data.Add(TEXT("target_x"), FString::Printf(TEXT("%.1f"), Loc.X));
        Feedback.Data.Add(TEXT("target_y"), FString::Printf(TEXT("%.1f"), Loc.Y));
        
        // 添加附近地标信息
        if (!Context.NearbyLandmarkLabel.IsEmpty())
        {
            // 使用统一格式添加地标信息
            AddEntityInfoToFeedback(Feedback, TEXT("nearby_landmark"), 
                Context.NearbyLandmarkLabel, FVector::ZeroVector, Context.NearbyLandmarkType);
            Feedback.Data.Add(TEXT("nearby_landmark_distance"), FString::Printf(TEXT("%.1f"), Context.NearbyLandmarkDistance));
        }
        
        //=========================================================================
        // 从场景图获取当前位置附近的地标信息
        //=========================================================================
        if (bSuccess && Context.NearbyLandmarkLabel.IsEmpty())
        {
            // 如果 Context 中没有地标信息，尝试从场景图查询
            UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
            if (SceneGraphManager)
            {
                FMASceneGraphNode NearbyLandmark = SceneGraphManager->FindNearestLandmark(Loc, 500.f);
                if (NearbyLandmark.IsValid())
                {
                    FString LandmarkLabel = NearbyLandmark.Label.IsEmpty() ? NearbyLandmark.Id : NearbyLandmark.Label;
                    float Distance = FVector::Dist2D(Loc, NearbyLandmark.Center);
                    
                    AddEntityInfoToFeedback(Feedback, TEXT("nearby_landmark"), 
                        LandmarkLabel, NearbyLandmark.Center, NearbyLandmark.Type);
                    Feedback.Data.Add(TEXT("nearby_landmark_distance"), FString::Printf(TEXT("%.1f"), Distance));
                }
            }
        }
    }
    
    // 生成消息
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        
        if (bSuccess)
        {
            // 成功消息：包含目标位置和附近地标
            FString LandmarkLabel = Context.NearbyLandmarkLabel;
            if (LandmarkLabel.IsEmpty())
            {
                // 尝试从反馈数据中获取
                LandmarkLabel = Feedback.Data.FindRef(TEXT("nearby_landmark_label"));
            }
            
            if (!LandmarkLabel.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Navigation completed to (%.0f, %.0f), near %s"),
                    Params.TargetLocation.X, Params.TargetLocation.Y, *LandmarkLabel);
            }
            else
            {
                Feedback.Message = FString::Printf(TEXT("Navigation completed to (%.0f, %.0f)"),
                    Params.TargetLocation.X, Params.TargetLocation.Y);
            }
        }
        else
        {
            Feedback.Message = TEXT("Navigation failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Navigation completed") : TEXT("Navigation failed");
    }
}

//=============================================================================
// Search 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateSearchFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        int32 FoundCount = Context.FoundObjects.Num();
        Feedback.Data.Add(TEXT("found_count"), FString::FromInt(FoundCount));
        Feedback.Data.Add(TEXT("found"), FoundCount > 0 ? TEXT("true") : TEXT("false"));
        
        // 添加 area_token (搜索区域标识)
        if (!Context.SearchAreaToken.IsEmpty())
        {
            Feedback.Data.Add(TEXT("area_token"), Context.SearchAreaToken);
        }
        
        // 添加 duration_s (搜索持续时间)
        if (Context.SearchDurationSeconds > 0.f)
        {
            Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), Context.SearchDurationSeconds));
        }
        
        // 添加 target_spec (搜索目标规格)
        if (!Context.SearchTargetSpec.IsEmpty())
        {
            Feedback.Data.Add(TEXT("target_spec"), Context.SearchTargetSpec);
        }
        
        //=========================================================================
        // 构建 discovered_nodes JSON 数组
        //=========================================================================
        if (FoundCount > 0)
        {
            UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
            TArray<FMASceneGraphNode> AllNodes;
            if (SceneGraphManager)
            {
                AllNodes = SceneGraphManager->GetAllNodes();
            }
            
            // 构建 discovered_nodes JSON 数组
            TArray<TSharedPtr<FJsonValue>> DiscoveredNodesArray;
            FString FirstFoundObjectId;
            
            for (int32 i = 0; i < FoundCount; ++i)
            {
                FString ObjectName = Context.FoundObjects[i];
                FVector ObjectLocation = Context.FoundLocations.IsValidIndex(i) ? Context.FoundLocations[i] : FVector::ZeroVector;
                
                TSharedPtr<FJsonObject> NodeJson;
                if (SceneGraphManager && AllNodes.Num() > 0)
                {
                    // 通过名称或ID查找节点
                    FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, ObjectName);
                    
                    if (Node.IsValid())
                    {
                        NodeJson = BuildNodeJsonFromSceneGraph(Node);
                        
                        // 记录第一个找到的对象 ID
                        if (i == 0)
                        {
                            FirstFoundObjectId = Node.Id;
                        }
                        
                        UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Search found object '%s' in scene graph: Id=%s, Label=%s"),
                            *ObjectName, *Node.Id, *Node.Label);
                    }
                }
                if (!NodeJson.IsValid())
                {
                    // 从 ObjectAttributes 中提取特征
                    TMap<FString, FString> Features;
                    for (const auto& AttrPair : Context.ObjectAttributes)
                    {
                        // 提取与当前对象相关的属性
                        if (AttrPair.Key.Contains(ObjectName) || AttrPair.Key.StartsWith(FString::Printf(TEXT("object_%d_"), i)))
                        {
                            // 移除前缀，只保留属性名
                            FString Key = AttrPair.Key;
                            Key.RemoveFromStart(FString::Printf(TEXT("object_%d_"), i));
                            Features.Add(Key, AttrPair.Value);
                        }
                    }
                    
                    // 生成一个临时 ID
                    FString TempId = FString::Printf(TEXT("temp_%s_%d"), *ObjectName, i);
                    if (i == 0)
                    {
                        FirstFoundObjectId = TempId;
                    }
                    
                    NodeJson = BuildNodeJsonFromUE5Data(
                        TempId,
                        ObjectName,
                        ObjectLocation,
                        TEXT(""),  // Type 未知
                        TEXT(""),  // Category 未知
                        Features
                    );
                    
                    UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Search object '%s' not in scene graph, using fallback"),
                        *ObjectName);
                }
                
                if (NodeJson.IsValid())
                {
                    DiscoveredNodesArray.Add(MakeShareable(new FJsonValueObject(NodeJson)));
                }
            }
            
            // 将 discovered_nodes 数组序列化为 JSON 字符串并存入 Data
            if (DiscoveredNodesArray.Num() > 0)
            {
                FString DiscoveredNodesJsonString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&DiscoveredNodesJsonString);
                FJsonSerializer::Serialize(DiscoveredNodesArray, Writer);
                Feedback.Data.Add(TEXT("discovered_nodes"), DiscoveredNodesJsonString);
                
                // 构建 found_ids 数组 (Planner's feedback_processor 需要)
                TArray<TSharedPtr<FJsonValue>> FoundIdsArray;
                for (const TSharedPtr<FJsonValue>& NodeValue : DiscoveredNodesArray)
                {
                    if (NodeValue.IsValid() && NodeValue->Type == EJson::Object)
                    {
                        TSharedPtr<FJsonObject> NodeObj = NodeValue->AsObject();
                        if (NodeObj.IsValid())
                        {
                            FString NodeId = NodeObj->GetStringField(TEXT("id"));
                            if (!NodeId.IsEmpty())
                            {
                                FoundIdsArray.Add(MakeShareable(new FJsonValueString(NodeId)));
                            }
                        }
                    }
                }
                
                if (FoundIdsArray.Num() > 0)
                {
                    FString FoundIdsJsonString;
                    TSharedRef<TJsonWriter<>> IdsWriter = TJsonWriterFactory<>::Create(&FoundIdsJsonString);
                    FJsonSerializer::Serialize(FoundIdsArray, IdsWriter);
                    Feedback.Data.Add(TEXT("found_ids"), FoundIdsJsonString);
                }
            }
            if (!FirstFoundObjectId.IsEmpty())
            {
                Feedback.Data.Add(TEXT("object_id"), FirstFoundObjectId);
            }
            
            // 构建找到的对象列表字符串 (用于消息)
            TArray<FString> ObjectNames;
            for (const FString& ObjName : Context.FoundObjects)
            {
                ObjectNames.Add(ObjName);
            }
            Feedback.Data.Add(TEXT("found_objects"), FString::Join(ObjectNames, TEXT(", ")));
        }
    }
    
    // 生成消息
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        int32 FoundCount = Context.FoundObjects.Num();
        
        if (FoundCount > 0)
        {
            // 成功找到对象：列出找到的对象名称
            if (FoundCount == 1)
            {
                Feedback.Message = FString::Printf(TEXT("Search completed, found %s"), *Context.FoundObjects[0]);
            }
            else if (FoundCount <= 3)
            {
                // 列出所有对象名称
                TArray<FString> Names;
                for (const FString& Name : Context.FoundObjects)
                {
                    Names.Add(Name);
                }
                Feedback.Message = FString::Printf(TEXT("Search completed, found %d objects: %s"), 
                    FoundCount, *FString::Join(Names, TEXT(", ")));
            }
            else
            {
                // 对象太多，只显示数量
                Feedback.Message = FString::Printf(TEXT("Search completed, found %d objects"), FoundCount);
            }
        }
        else
        {
            // 未找到对象
            Feedback.Message = TEXT("Search completed, no target found");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Search completed") : TEXT("Search failed");
    }
}

//=============================================================================
// Follow 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateFollowFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        //=========================================================================
        
        // 添加 robot_id (执行 Follow 的机器人 ID)
        if (!Context.FollowRobotId.IsEmpty())
        {
            Feedback.Data.Add(TEXT("robot_id"), Context.FollowRobotId);
        }
        else
        {
            // 回退：使用 Agent 的 ID
            Feedback.Data.Add(TEXT("robot_id"), Agent->AgentID);
        }
        
        // 添加 target_id (被跟随目标的 ID)
        if (!Context.FollowTargetId.IsEmpty())
        {
            Feedback.Data.Add(TEXT("target_id"), Context.FollowTargetId);
        }
        
        // 添加 duration_s (跟随持续时间)
        if (Context.FollowDurationSeconds > 0.f)
        {
            Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), Context.FollowDurationSeconds));
        }
        
        // 添加 target_spec (跟随目标规格)
        if (!Context.FollowTargetSpec.IsEmpty())
        {
            Feedback.Data.Add(TEXT("target_spec"), Context.FollowTargetSpec);
        }
        
        //=========================================================================
        // 添加目标信息（可以是机器人或其他对象）
        //=========================================================================
        FString TargetName = Context.FollowTargetRobotName;
        if (TargetName.IsEmpty())
        {
            TargetName = Context.TargetName;
        }
        
        if (!TargetName.IsEmpty())
        {
            // 尝试从场景图获取目标的详细信息
            UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
            TArray<FMASceneGraphNode> AllNodes;
            if (SceneGraphManager)
            {
                AllNodes = SceneGraphManager->GetAllNodes();
            }
            
            FMASceneGraphNode TargetNode;
            if (SceneGraphManager && AllNodes.Num() > 0)
            {
                // 通过名称或ID查找
                TargetNode = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, TargetName);
            }
            
            if (TargetNode.IsValid())
            {
                TSharedPtr<FJsonObject> TargetNodeJson = BuildNodeJsonFromSceneGraph(TargetNode);
                if (TargetNodeJson.IsValid())
                {
                    FString TargetNodeJsonString;
                    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&TargetNodeJsonString);
                    FJsonSerializer::Serialize(TargetNodeJson.ToSharedRef(), Writer);
                    Feedback.Data.Add(TEXT("target_node"), TargetNodeJsonString);
                }
                
                // 添加目标信息（使用通用的 target 前缀，而不是 target_robot）
                FString TargetLabel = TargetNode.Label.IsEmpty() ? TargetNode.Id : TargetNode.Label;
                AddEntityInfoToFeedback(Feedback, TEXT("target"), TargetLabel, TargetNode.Center, TargetNode.Type);
                
                // 如果没有设置 target_id，使用场景图节点的 ID
                if (Context.FollowTargetId.IsEmpty())
                {
                    Feedback.Data.Add(TEXT("target_id"), TargetNode.Id);
                }
            }
            else
            {
                // 回退：使用 Context 中的信息
                Feedback.Data.Add(TEXT("target_name"), TargetName);
            }
        }
        
        // 添加距离信息 (同时提供 distance 和 robot_target_distance 以保持兼容)
        if (Context.FollowTargetDistance > 0.f)
        {
            FString DistanceStr = FString::Printf(TEXT("%.1f"), Context.FollowTargetDistance);
            Feedback.Data.Add(TEXT("distance"), DistanceStr);
            Feedback.Data.Add(TEXT("robot_target_distance"), DistanceStr);  // Python goal_progress_monitor 需要
        }
        
        // 添加目标是否找到的标志
        Feedback.Data.Add(TEXT("target_found"), Context.bFollowTargetFound ? TEXT("true") : TEXT("false"));
        
        //=========================================================================
        // 处理目标未找到的错误情况
        //=========================================================================
        if (!Context.bFollowTargetFound && !Context.FollowErrorReason.IsEmpty())
        {
            Feedback.Data.Add(TEXT("error_reason"), Context.FollowErrorReason);
        }
    }
    
    //=========================================================================
    // 生成消息
    //=========================================================================
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (!Context.bFollowTargetFound)
        {
            // 目标未找到的错误消息
            if (!Context.FollowErrorReason.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Follow failed: %s"), *Context.FollowErrorReason);
            }
            else
            {
                Feedback.Message = TEXT("Follow failed: target robot not found");
            }
            Feedback.bSuccess = false;
        }
        else if (bSuccess)
        {
            // 成功消息：包含目标机器人名称和距离
            FString TargetName = Context.FollowTargetRobotName;
            if (TargetName.IsEmpty())
            {
                TargetName = Feedback.Data.FindRef(TEXT("target_robot_label"));
            }
            
            if (!TargetName.IsEmpty())
            {
                if (Context.FollowTargetDistance > 0.f)
                {
                    Feedback.Message = FString::Printf(TEXT("Follow completed: followed %s, final distance %.0f"),
                        *TargetName, Context.FollowTargetDistance);
                }
                else
                {
                    Feedback.Message = FString::Printf(TEXT("Follow completed: followed %s"),
                        *TargetName);
                }
            }
            else
            {
                Feedback.Message = TEXT("Follow completed");
            }
        }
        else
        {
            // 其他失败情况
            FString TargetName = Context.FollowTargetRobotName;
            if (!TargetName.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Follow failed: could not follow %s"),
                    *TargetName);
            }
            else
            {
                Feedback.Message = TEXT("Follow failed");
            }
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Follow completed") : TEXT("Follow failed");
    }
}

//=============================================================================
// Charge 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateChargeFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加能量信息
        //=========================================================================
        Feedback.Data.Add(TEXT("energy_before"), FString::Printf(TEXT("%.1f%%"), Context.EnergyBefore));
        Feedback.Data.Add(TEXT("energy_after"), FString::Printf(TEXT("%.1f%%"), Context.EnergyAfter));
        
        //=========================================================================
        // 添加充电站信息
        //=========================================================================
        if (!Context.ChargingStationId.IsEmpty())
        {
            // 尝试从场景图获取充电站详细信息
            UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
            TArray<FMASceneGraphNode> AllNodes;
            if (SceneGraphManager)
            {
                AllNodes = SceneGraphManager->GetAllNodes();
            }
            
            FMASceneGraphNode StationNode;
            if (SceneGraphManager && AllNodes.Num() > 0)
            {
                // 通过名称或ID查找
                StationNode = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, Context.ChargingStationId);
            }
            
            if (StationNode.IsValid())
            {
                TSharedPtr<FJsonObject> StationNodeJson = BuildNodeJsonFromSceneGraph(StationNode);
                if (StationNodeJson.IsValid())
                {
                    FString StationNodeJsonString;
                    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&StationNodeJsonString);
                    FJsonSerializer::Serialize(StationNodeJson.ToSharedRef(), Writer);
                    Feedback.Data.Add(TEXT("charging_station_node"), StationNodeJsonString);
                }
                
                // 同时保留旧格式以保持兼容性
                FString StationLabel = StationNode.Label.IsEmpty() ? StationNode.Id : StationNode.Label;
                AddEntityInfoToFeedback(Feedback, TEXT("charging_station"), StationLabel, StationNode.Center, TEXT("charging_station"));
            }
            else
            {
                // 回退：使用 Context 中的信息
                Feedback.Data.Add(TEXT("charging_station_label"), Context.ChargingStationId);
                if (!Context.ChargingStationLocation.IsZero())
                {
                    AddEntityInfoToFeedback(Feedback, TEXT("charging_station"), Context.ChargingStationId, 
                        Context.ChargingStationLocation, TEXT("charging_station"));
                }
            }
        }
        
        // 添加充电站是否找到的标志
        Feedback.Data.Add(TEXT("charging_station_found"), Context.bChargingStationFound ? TEXT("true") : TEXT("false"));
        
        //=========================================================================
        // 处理充电站未找到的错误情况
        //=========================================================================
        if (!Context.bChargingStationFound && !Context.ChargeErrorReason.IsEmpty())
        {
            Feedback.Data.Add(TEXT("error_reason"), Context.ChargeErrorReason);
        }
    }
    
    //=========================================================================
    // 生成消息
    //=========================================================================
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (!Context.bChargingStationFound)
        {
            // 充电站未找到的错误消息
            if (!Context.ChargeErrorReason.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Charge failed: %s"), *Context.ChargeErrorReason);
            }
            else
            {
                Feedback.Message = TEXT("Charge failed: no charging station available");
            }
            Feedback.bSuccess = false;
        }
        else if (bSuccess)
        {
            // 成功消息：包含充电站ID和能量等级
            FString StationLabel = Feedback.Data.FindRef(TEXT("charging_station_label"));
            if (StationLabel.IsEmpty())
            {
                StationLabel = Context.ChargingStationId;
            }
            
            if (!StationLabel.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Charge completed at %s, energy: %.1f%% -> %.1f%%"),
                    *StationLabel, Context.EnergyBefore, Context.EnergyAfter);
            }
            else
            {
                Feedback.Message = FString::Printf(TEXT("Charge completed, energy: %.1f%% -> %.1f%%"),
                    Context.EnergyBefore, Context.EnergyAfter);
            }
        }
        else
        {
            // 其他失败情况（如充电被中断）
            FString StationLabel = Feedback.Data.FindRef(TEXT("charging_station_label"));
            if (StationLabel.IsEmpty())
            {
                StationLabel = Context.ChargingStationId;
            }
            
            if (!StationLabel.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Charge interrupted at %s, energy: %.1f%%"),
                    *StationLabel, Context.EnergyAfter);
            }
            else
            {
                Feedback.Message = FString::Printf(TEXT("Charge interrupted, energy: %.1f%%"), Context.EnergyAfter);
            }
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Charge completed") : TEXT("Charge interrupted");
    }
}

//=============================================================================
// Place 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GeneratePlaceFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 从场景图获取对象信息
        //=========================================================================
        UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
        TArray<FMASceneGraphNode> AllNodes;
        if (SceneGraphManager)
        {
            AllNodes = SceneGraphManager->GetAllNodes();
        }
        
        // 获取 target (放置的对象) 信息
        FString Object1NodeId = Context.ObjectAttributes.FindRef(TEXT("object1_node_id"));
        FString Object1Name = Context.PlacedObjectName;
        if (!Object1NodeId.IsEmpty() && SceneGraphManager)
        {
            FMASceneGraphNode Object1Node = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, Object1NodeId);
            if (Object1Node.IsValid())
            {
                TSharedPtr<FJsonObject> Object1NodeJson = BuildNodeJsonFromSceneGraph(Object1Node);
                if (Object1NodeJson.IsValid())
                {
                    FString Object1NodeJsonString;
                    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Object1NodeJsonString);
                    FJsonSerializer::Serialize(Object1NodeJson.ToSharedRef(), Writer);
                    Feedback.Data.Add(TEXT("object_node"), Object1NodeJsonString);
                }
                
                // 同时保留旧格式以保持兼容性
                FString Label = Object1Node.Label.IsEmpty() ? Object1Node.Id : Object1Node.Label;
                AddEntityInfoToFeedback(Feedback, TEXT("object"), Label, Object1Node.Center, Object1Node.Type);
                
                // 添加对象特征信息
                for (const auto& Feature : Object1Node.Features)
                {
                    Feedback.Data.Add(FString::Printf(TEXT("object_%s"), *Feature.Key), Feature.Value);
                }
                
                Object1Name = Label;
            }
        }
        else if (!Object1Name.IsEmpty())
        {
            // 回退：使用 Context 中的信息
            Feedback.Data.Add(TEXT("object_label"), Object1Name);
        }
        
        // 获取 surface_target (目标) 信息
        FString Object2NodeId = Context.ObjectAttributes.FindRef(TEXT("object2_node_id"));
        FString Object2Name = Context.PlaceTargetName;
        if (!Object2NodeId.IsEmpty() && SceneGraphManager)
        {
            FMASceneGraphNode Object2Node = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, Object2NodeId);
            if (Object2Node.IsValid())
            {
                TSharedPtr<FJsonObject> Object2NodeJson = BuildNodeJsonFromSceneGraph(Object2Node);
                if (Object2NodeJson.IsValid())
                {
                    FString Object2NodeJsonString;
                    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Object2NodeJsonString);
                    FJsonSerializer::Serialize(Object2NodeJson.ToSharedRef(), Writer);
                    Feedback.Data.Add(TEXT("target_node"), Object2NodeJsonString);
                }
                
                // 同时保留旧格式以保持兼容性
                FString Label = Object2Node.Label.IsEmpty() ? Object2Node.Id : Object2Node.Label;
                AddEntityInfoToFeedback(Feedback, TEXT("target"), Label, Object2Node.Center, Object2Node.Type);
                
                // 如果是机器人，添加 robot_type (机器人类型存储在 Type 字段)
                if (Object2Node.IsRobot())
                {
                    Feedback.Data.Add(TEXT("target_robot_type"), Object2Node.Type);
                }
                
                Object2Name = Label;
            }
        }
        else if (!Object2Name.IsEmpty())
        {
            // 回退：使用 Context 中的信息
            Feedback.Data.Add(TEXT("target_label"), Object2Name);
        }
        
        // 添加最终位置
        if (!Context.PlaceFinalLocation.IsZero())
        {
            Feedback.Data.Add(TEXT("final_x"), FString::Printf(TEXT("%.1f"), Context.PlaceFinalLocation.X));
            Feedback.Data.Add(TEXT("final_y"), FString::Printf(TEXT("%.1f"), Context.PlaceFinalLocation.Y));
            Feedback.Data.Add(TEXT("final_z"), FString::Printf(TEXT("%.1f"), Context.PlaceFinalLocation.Z));
            Feedback.Data.Add(TEXT("final_location"), FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), 
                Context.PlaceFinalLocation.X, Context.PlaceFinalLocation.Y, Context.PlaceFinalLocation.Z));
        }
        
        // 失败时添加错误原因
        if (!bSuccess && !Context.PlaceErrorReason.IsEmpty())
        {
            Feedback.Data.Add(TEXT("error_reason"), Context.PlaceErrorReason);
        }
        
        // 取消时添加取消阶段
        if (!bSuccess && !Context.PlaceCancelledPhase.IsEmpty())
        {
            Feedback.Data.Add(TEXT("cancelled_phase"), Context.PlaceCancelledPhase);
        }
    }
    
    // 生成消息
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        // 获取对象名称（优先使用反馈数据中的标签）
        FString ObjectName = Feedback.Data.FindRef(TEXT("object_label"));
        if (ObjectName.IsEmpty())
        {
            ObjectName = Context.PlacedObjectName;
        }
        
        FString TargetName = Feedback.Data.FindRef(TEXT("target_label"));
        if (TargetName.IsEmpty())
        {
            TargetName = Context.PlaceTargetName;
        }
        
        if (bSuccess)
        {
            // 成功消息：包含对象名、目标名、最终位置
            if (!ObjectName.IsEmpty() && !TargetName.IsEmpty())
            {
                if (!Context.PlaceFinalLocation.IsZero())
                {
                    Feedback.Message = FString::Printf(TEXT("Place succeeded: Moved %s to %s at (%.0f, %.0f, %.0f)"),
                        *ObjectName, *TargetName,
                        Context.PlaceFinalLocation.X, Context.PlaceFinalLocation.Y, Context.PlaceFinalLocation.Z);
                }
                else
                {
                    Feedback.Message = FString::Printf(TEXT("Place succeeded: Moved %s to %s"),
                        *ObjectName, *TargetName);
                }
            }
            else
            {
                Feedback.Message = TEXT("Place completed");
            }
        }
        else
        {
            // 失败消息：包含具体错误原因
            if (!Context.PlaceErrorReason.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Place failed: %s"), *Context.PlaceErrorReason);
            }
            else if (!Context.PlaceCancelledPhase.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("Place cancelled: Stopped while %s"), *Context.PlaceCancelledPhase);
            }
            else
            {
                Feedback.Message = TEXT("Place failed");
            }
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Place completed") : TEXT("Place failed");
    }
}

//=============================================================================
// TakeOff 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateTakeOffFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加高度信息
        //=========================================================================
        Feedback.Data.Add(TEXT("target_height"), FString::Printf(TEXT("%.1f"), Params.TakeOffHeight));
        Feedback.Data.Add(TEXT("final_height"), FString::Printf(TEXT("%.1f"), Context.TakeOffTargetHeight));
        
        // 添加安全高度信息
        if (Context.TakeOffMinSafeHeight > 0.f)
        {
            Feedback.Data.Add(TEXT("min_safe_height"), FString::Printf(TEXT("%.1f"), Context.TakeOffMinSafeHeight));
        }
        
        // 添加高度是否被调整的标志
        Feedback.Data.Add(TEXT("height_adjusted"), Context.bTakeOffHeightAdjusted ? TEXT("true") : TEXT("false"));
        
        //=========================================================================
        // 添加空域信息 (附近建筑物)
        //=========================================================================
        if (!Context.TakeOffNearbyBuildingLabel.IsEmpty())
        {
            // 使用统一格式添加建筑物信息
            AddEntityInfoToFeedback(Feedback, TEXT("nearby_building"), 
                Context.TakeOffNearbyBuildingLabel, FVector::ZeroVector, TEXT("building"));
            Feedback.Data.Add(TEXT("nearby_building_height"), FString::Printf(TEXT("%.1f"), Context.TakeOffNearbyBuildingHeight));
        }
        else
        {
            // 尝试从场景图获取附近建筑物信息
            UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
            if (SceneGraphManager)
            {
                FVector CurrentLoc = Agent->GetActorLocation();
                FMASceneGraphNode NearbyBuilding = SceneGraphManager->FindNearestLandmark(CurrentLoc, 500.f);
                if (NearbyBuilding.IsValid() && NearbyBuilding.IsBuilding())
                {
                    FString BuildingLabel = NearbyBuilding.Label.IsEmpty() ? NearbyBuilding.Id : NearbyBuilding.Label;
                    AddEntityInfoToFeedback(Feedback, TEXT("nearby_building"), 
                        BuildingLabel, NearbyBuilding.Center, TEXT("building"));
                }
            }
        }
        
        // 添加当前位置
        FVector CurrentLoc = Agent->GetActorLocation();
        Feedback.Data.Add(TEXT("current_x"), FString::Printf(TEXT("%.1f"), CurrentLoc.X));
        Feedback.Data.Add(TEXT("current_y"), FString::Printf(TEXT("%.1f"), CurrentLoc.Y));
        Feedback.Data.Add(TEXT("current_z"), FString::Printf(TEXT("%.1f"), CurrentLoc.Z));
    }
    
    //=========================================================================
    // 生成消息
    //=========================================================================
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            // 成功消息：包含高度和空域信息
            FString BuildingLabel = Context.TakeOffNearbyBuildingLabel;
            if (BuildingLabel.IsEmpty())
            {
                BuildingLabel = Feedback.Data.FindRef(TEXT("nearby_building_label"));
            }
            
            if (Context.bTakeOffHeightAdjusted && !BuildingLabel.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("TakeOff completed at height %.0f (adjusted above %s)"),
                    Params.TakeOffHeight, *BuildingLabel);
            }
            else if (!BuildingLabel.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("TakeOff completed at height %.0f, clear of %s"),
                    Params.TakeOffHeight, *BuildingLabel);
            }
            else
            {
                Feedback.Message = FString::Printf(TEXT("TakeOff completed at height %.0f"), Params.TakeOffHeight);
            }
        }
        else
        {
            Feedback.Message = TEXT("TakeOff failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("TakeOff completed") : TEXT("TakeOff failed");
    }
}

//=============================================================================
// Land 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateLandFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加着陆位置信息
        //=========================================================================
        Feedback.Data.Add(TEXT("target_height"), FString::Printf(TEXT("%.1f"), Params.LandHeight));
        
        // 添加着陆位置坐标
        if (!Context.LandTargetLocation.IsZero())
        {
            AddEntityInfoToFeedback(Feedback, TEXT("land"), TEXT(""), Context.LandTargetLocation, TEXT(""));
        }
        
        //=========================================================================
        // 添加地面类型信息
        //=========================================================================
        if (!Context.LandGroundType.IsEmpty())
        {
            Feedback.Data.Add(TEXT("ground_type"), Context.LandGroundType);
        }
        else
        {
            // 尝试从场景图获取地面类型
            UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
            if (SceneGraphManager)
            {
                FVector LandLoc = Context.LandTargetLocation.IsZero() ? Agent->GetActorLocation() : Context.LandTargetLocation;
                FMASceneGraphNode NearbyLandmark = SceneGraphManager->FindNearestLandmark(LandLoc, 200.f);
                if (NearbyLandmark.IsValid())
                {
                    // 根据地标类型推断地面类型
                    if (NearbyLandmark.IsRoad())
                    {
                        Feedback.Data.Add(TEXT("ground_type"), TEXT("road"));
                    }
                    else if (NearbyLandmark.IsIntersection())
                    {
                        Feedback.Data.Add(TEXT("ground_type"), TEXT("intersection"));
                    }
                    else if (NearbyLandmark.IsBuilding())
                    {
                        Feedback.Data.Add(TEXT("ground_type"), TEXT("building_roof"));
                    }
                    else
                    {
                        Feedback.Data.Add(TEXT("ground_type"), TEXT("open_area"));
                    }
                }
            }
        }
        
        // 添加附近地标信息
        if (!Context.LandNearbyLandmarkLabel.IsEmpty())
        {
            AddEntityInfoToFeedback(Feedback, TEXT("nearby_landmark"), 
                Context.LandNearbyLandmarkLabel, FVector::ZeroVector, TEXT(""));
        }
        else
        {
            // 尝试从场景图获取附近地标
            UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
            if (SceneGraphManager)
            {
                FVector LandLoc = Context.LandTargetLocation.IsZero() ? Agent->GetActorLocation() : Context.LandTargetLocation;
                FMASceneGraphNode NearbyLandmark = SceneGraphManager->FindNearestLandmark(LandLoc, 500.f);
                if (NearbyLandmark.IsValid())
                {
                    FString LandmarkLabel = NearbyLandmark.Label.IsEmpty() ? NearbyLandmark.Id : NearbyLandmark.Label;
                    AddEntityInfoToFeedback(Feedback, TEXT("nearby_landmark"), 
                        LandmarkLabel, NearbyLandmark.Center, NearbyLandmark.Type);
                }
            }
        }
        
        // 添加着陆位置是否安全的标志
        Feedback.Data.Add(TEXT("location_safe"), Context.bLandLocationSafe ? TEXT("true") : TEXT("false"));
    }
    
    //=========================================================================
    // 生成消息
    //=========================================================================
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            // 成功消息：包含着陆位置和地面类型
            FString LandmarkLabel = Context.LandNearbyLandmarkLabel;
            if (LandmarkLabel.IsEmpty())
            {
                LandmarkLabel = Feedback.Data.FindRef(TEXT("nearby_landmark_label"));
            }
            
            FString GroundType = Context.LandGroundType;
            if (GroundType.IsEmpty())
            {
                GroundType = Feedback.Data.FindRef(TEXT("ground_type"));
            }
            
            FString LocationInfo;
            if (!LandmarkLabel.IsEmpty())
            {
                LocationInfo = FString::Printf(TEXT(" near %s"), *LandmarkLabel);
            }
            
            FString GroundInfo;
            if (!GroundType.IsEmpty() && GroundType != TEXT("unknown"))
            {
                GroundInfo = FString::Printf(TEXT(" on %s"), *GroundType);
            }
            
            Feedback.Message = FString::Printf(TEXT("Land completed at height %.0f%s%s"),
                Params.LandHeight, *GroundInfo, *LocationInfo);
        }
        else
        {
            if (!Context.bLandLocationSafe)
            {
                Feedback.Message = TEXT("Land failed: unsafe landing location");
            }
            else
            {
                Feedback.Message = TEXT("Land failed");
            }
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Land completed") : TEXT("Land failed");
    }
}

//=============================================================================
// ReturnHome 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateReturnHomeFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加家位置信息
        //=========================================================================
        AddEntityInfoToFeedback(Feedback, TEXT("home"), TEXT(""), Params.HomeLocation, TEXT(""));
        Feedback.Data.Add(TEXT("land_height"), FString::Printf(TEXT("%.1f"), Params.LandHeight));
        
        //=========================================================================
        // 添加家位置标签信息
        //=========================================================================
        if (!Context.HomeLandmarkLabel.IsEmpty())
        {
            Feedback.Data.Add(TEXT("home_landmark_label"), Context.HomeLandmarkLabel);
        }
        else
        {
            // 尝试从场景图获取家位置附近的地标
            UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
            if (SceneGraphManager)
            {
                FMASceneGraphNode NearbyLandmark = SceneGraphManager->FindNearestLandmark(Params.HomeLocation, 500.f);
                if (NearbyLandmark.IsValid())
                {
                    FString LandmarkLabel = NearbyLandmark.Label.IsEmpty() ? NearbyLandmark.Id : NearbyLandmark.Label;
                    Feedback.Data.Add(TEXT("home_landmark_label"), LandmarkLabel);
                    AddEntityInfoToFeedback(Feedback, TEXT("home_landmark"), 
                        LandmarkLabel, NearbyLandmark.Center, NearbyLandmark.Type);
                }
            }
        }
        
        // 添加是否从场景图获取的标志
        Feedback.Data.Add(TEXT("from_scene_graph"), Context.bHomeLocationFromSceneGraph ? TEXT("true") : TEXT("false"));
    }
    
    //=========================================================================
    // 生成消息
    //=========================================================================
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            // 成功消息：包含家位置标签
            FString LandmarkLabel = Context.HomeLandmarkLabel;
            if (LandmarkLabel.IsEmpty())
            {
                LandmarkLabel = Feedback.Data.FindRef(TEXT("home_landmark_label"));
            }
            
            if (!LandmarkLabel.IsEmpty())
            {
                Feedback.Message = FString::Printf(TEXT("ReturnHome completed at (%.0f, %.0f), near %s"),
                    Params.HomeLocation.X, Params.HomeLocation.Y, *LandmarkLabel);
            }
            else
            {
                Feedback.Message = FString::Printf(TEXT("ReturnHome completed at (%.0f, %.0f)"),
                    Params.HomeLocation.X, Params.HomeLocation.Y);
            }
        }
        else
        {
            Feedback.Message = TEXT("ReturnHome failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("ReturnHome completed") : TEXT("ReturnHome failed");
    }
}

//=============================================================================
// TakePhoto 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateTakePhotoFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加目标信息
        //=========================================================================
        if (Context.bPhotoTargetFound)
        {
            // 目标找到，添加目标节点信息
            if (!Context.PhotoTargetId.IsEmpty())
            {
                UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
                if (SceneGraphManager)
                {
                    FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeByIdOrLabel(SceneGraphManager->GetAllNodes(), Context.PhotoTargetId);
                    if (!Node.Id.IsEmpty())
                    {
                        // 使用 BuildNodeJsonFromSceneGraph 添加目标节点信息到反馈数据
                        TSharedPtr<FJsonObject> NodeJson = BuildNodeJsonFromSceneGraph(Node);
                        if (NodeJson.IsValid())
                        {
                            FString NodeJsonStr;
                            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeJsonStr);
                            FJsonSerializer::Serialize(NodeJson.ToSharedRef(), Writer);
                            Feedback.Data.Add(TEXT("target_node"), NodeJsonStr);
                        }
                        
                        // 同时保留旧格式以保持兼容性
                        FString TargetLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
                        AddEntityInfoToFeedback(Feedback, TEXT("target"), TargetLabel, Node.Center, Node.Type);
                    }
                }
            }
            
            // 添加目标名称和 ID
            if (!Context.PhotoTargetName.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_name"), Context.PhotoTargetName);
            }
            if (!Context.PhotoTargetId.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_id"), Context.PhotoTargetId);
            }
        }
        
        // 添加目标是否找到的标志
        Feedback.Data.Add(TEXT("target_found"), Context.bPhotoTargetFound ? TEXT("true") : TEXT("false"));
    }
    
    //=========================================================================
    // 生成消息
    //=========================================================================
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            if (Context.bPhotoTargetFound)
            {
                // 拍照成功，且发现了目标
                FString TargetName = Context.PhotoTargetName;
                if (TargetName.IsEmpty())
                {
                    TargetName = Feedback.Data.FindRef(TEXT("target_label"));
                }
                
                if (!TargetName.IsEmpty())
                {
                    Feedback.Message = FString::Printf(TEXT("Photo taken successfully, found %s"), *TargetName);
                }
                else
                {
                    Feedback.Message = TEXT("Photo taken successfully, found target");
                }
            }
            else
            {
                // 拍照成功，但是没发现目标
                const FMASkillParams& Params = SkillComp->GetSkillParams();
                FString TargetDesc = Params.CommonTarget.Type.IsEmpty() ? TEXT("target") : Params.CommonTarget.Type;
                Feedback.Message = FString::Printf(TEXT("Photo taken successfully, but %s not found"), *TargetDesc);
            }
        }
        else
        {
            // 拍照失败
            Feedback.Message = TEXT("Photo failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Photo taken successfully") : TEXT("Photo failed");
    }
}

//=============================================================================
// Broadcast 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateBroadcastFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加喊话消息
        //=========================================================================
        if (!Context.BroadcastMessage.IsEmpty())
        {
            Feedback.Data.Add(TEXT("message"), Context.BroadcastMessage);
        }
        
        //=========================================================================
        // 添加目标信息
        //=========================================================================
        if (Context.bBroadcastTargetFound && !Context.BroadcastTargetName.IsEmpty())
        {
            Feedback.Data.Add(TEXT("target_name"), Context.BroadcastTargetName);
        }
        
        // 添加目标是否找到的标志
        Feedback.Data.Add(TEXT("target_found"), Context.bBroadcastTargetFound ? TEXT("true") : TEXT("false"));
    }
    
    //=========================================================================
    // 生成消息
    //=========================================================================
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            Feedback.Message = TEXT("Broadcast succeeded");
        }
        else
        {
            Feedback.Message = TEXT("Broadcast failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Broadcast succeeded") : TEXT("Broadcast failed");
    }
}

//=============================================================================
// HandleHazard 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateHandleHazardFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加目标信息
        //=========================================================================
        if (Context.bHazardTargetFound)
        {
            // 目标找到，添加目标节点信息
            if (!Context.HazardTargetId.IsEmpty())
            {
                UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
                if (SceneGraphManager)
                {
                    FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeByIdOrLabel(SceneGraphManager->GetAllNodes(), Context.HazardTargetId);
                    if (!Node.Id.IsEmpty())
                    {
                        // 使用 BuildNodeJsonFromSceneGraph 添加目标节点信息到反馈数据
                        TSharedPtr<FJsonObject> NodeJson = BuildNodeJsonFromSceneGraph(Node);
                        if (NodeJson.IsValid())
                        {
                            FString NodeJsonStr;
                            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeJsonStr);
                            FJsonSerializer::Serialize(NodeJson.ToSharedRef(), Writer);
                            Feedback.Data.Add(TEXT("target_node"), NodeJsonStr);
                        }
                        
                        // 同时保留旧格式以保持兼容性
                        FString TargetLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
                        AddEntityInfoToFeedback(Feedback, TEXT("target"), TargetLabel, Node.Center, Node.Type);
                    }
                }
            }
            
            // 添加目标名称和 ID
            if (!Context.HazardTargetName.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_name"), Context.HazardTargetName);
            }
            if (!Context.HazardTargetId.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_id"), Context.HazardTargetId);
            }
        }
        
        // 添加处理持续时间
        if (Context.HazardHandleDurationSeconds > 0.f)
        {
            Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), Context.HazardHandleDurationSeconds));
        }
        
        // 添加目标是否找到的标志
        Feedback.Data.Add(TEXT("target_found"), Context.bHazardTargetFound ? TEXT("true") : TEXT("false"));
    }
    
    //=========================================================================
    // 生成消息
    //=========================================================================
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            Feedback.Message = TEXT("Hazard handled successfully");
        }
        else
        {
            Feedback.Message = TEXT("Hazard handling failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Hazard handled successfully") : TEXT("Hazard handling failed");
    }
}

//=============================================================================
// Guide 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateGuideFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    AddCommonFieldsToFeedback(Feedback, SkillComp);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加目标信息
        //=========================================================================
        if (Context.bGuideTargetFound)
        {
            // 目标找到，添加目标节点信息
            if (!Context.GuideTargetId.IsEmpty())
            {
                UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
                if (SceneGraphManager)
                {
                    FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeByIdOrLabel(SceneGraphManager->GetAllNodes(), Context.GuideTargetId);
                    if (!Node.Id.IsEmpty())
                    {
                        // 使用 BuildNodeJsonFromSceneGraph 添加目标节点信息到反馈数据
                        TSharedPtr<FJsonObject> NodeJson = BuildNodeJsonFromSceneGraph(Node);
                        if (NodeJson.IsValid())
                        {
                            FString NodeJsonStr;
                            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeJsonStr);
                            FJsonSerializer::Serialize(NodeJson.ToSharedRef(), Writer);
                            Feedback.Data.Add(TEXT("target_node"), NodeJsonStr);
                        }
                        
                        // 同时保留旧格式以保持兼容性
                        FString TargetLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
                        AddEntityInfoToFeedback(Feedback, TEXT("target"), TargetLabel, Node.Center, Node.Type);
                    }
                }
            }
            
            // 添加目标名称和 ID
            if (!Context.GuideTargetName.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_name"), Context.GuideTargetName);
            }
            if (!Context.GuideTargetId.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_id"), Context.GuideTargetId);
            }
        }
        
        // 添加目的地坐标
        if (!Context.GuideDestination.IsZero())
        {
            Feedback.Data.Add(TEXT("destination_x"), FString::Printf(TEXT("%.1f"), Context.GuideDestination.X));
            Feedback.Data.Add(TEXT("destination_y"), FString::Printf(TEXT("%.1f"), Context.GuideDestination.Y));
            Feedback.Data.Add(TEXT("destination_z"), FString::Printf(TEXT("%.1f"), Context.GuideDestination.Z));
            Feedback.Data.Add(TEXT("destination"), FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), 
                Context.GuideDestination.X, Context.GuideDestination.Y, Context.GuideDestination.Z));
        }
        
        // 添加引导持续时间
        if (Context.GuideDurationSeconds > 0.f)
        {
            Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), Context.GuideDurationSeconds));
        }
        
        // 添加目标是否找到的标志
        Feedback.Data.Add(TEXT("target_found"), Context.bGuideTargetFound ? TEXT("true") : TEXT("false"));
    }
    
    //=========================================================================
    // 生成消息
    //=========================================================================
    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            // 成功消息：包含目的地坐标
            if (!Context.GuideDestination.IsZero())
            {
                Feedback.Message = FString::Printf(TEXT("Guide succeeded, target guided to (%.0f, %.0f, %.0f)"),
                    Context.GuideDestination.X, Context.GuideDestination.Y, Context.GuideDestination.Z);
            }
            else
            {
                Feedback.Message = TEXT("Guide succeeded");
            }
        }
        else
        {
            Feedback.Message = TEXT("Guide failed");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Guide succeeded") : TEXT("Guide failed");
    }
}

//=============================================================================
// Idle 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateIdleFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, bool bSuccess, const FString& Message)
{
    // 添加通用字段 (task_id 等)
    if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
    {
        AddCommonFieldsToFeedback(Feedback, SkillComp);
    }
    
    Feedback.Message = Message.IsEmpty() ? TEXT("Idle") : Message;
}


