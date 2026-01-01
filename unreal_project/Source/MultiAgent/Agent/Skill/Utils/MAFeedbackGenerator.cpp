// MAFeedbackGenerator.cpp
// 反馈生成器实现
// Requirements: 9.1, 9.2, 9.3, 9.4, 9.5

#include "MAFeedbackGenerator.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Core/Manager/MASceneGraphQuery.h"
#include "MASceneQuery.h"
#include "EngineUtils.h"  // For TActorIterator

//=============================================================================
// 场景图查询辅助方法实现 (Requirements: 9.1, 9.2)
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
    
    // 1. 首先尝试从场景图获取 (Requirements: 9.1)
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeById(AllNodes, EntityId);
        
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
    
    // 2. 场景图未找到，回退到 UE5 场景查询 (Requirements: 9.2)
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
    
    // 1. 首先尝试从场景图获取 (Requirements: 9.1)
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeById(AllNodes, RobotId);
        
        if (Node.IsValid() && Node.IsRobot())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            OutAgentType = Node.AgentType;
            UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Found robot '%s' in scene graph: Label=%s, AgentType=%s"),
                *RobotId, *OutLabel, *OutAgentType);
            return true;
        }
        
        // 也尝试通过 Label 查找
        Node = FMASceneGraphQuery::FindNodeByLabelString(AllNodes, RobotId);
        if (Node.IsValid() && Node.IsRobot())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            OutAgentType = Node.AgentType;
            return true;
        }
    }
    
    // 2. 回退到 UE5 场景查询 (Requirements: 9.2)
    UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Robot '%s' not found in scene graph, falling back to UE5 scene query"), *RobotId);
    
    UWorld* World = Agent->GetWorld();
    if (World)
    {
        AMACharacter* FoundRobot = FMASceneQuery::FindRobotByName(World, RobotId);
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
    
    // 1. 首先尝试从场景图获取 (Requirements: 9.1)
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeById(AllNodes, ItemId);
        
        if (Node.IsValid() && Node.IsPickupItem())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            OutFeatures = Node.Features;
            UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Found pickup item '%s' in scene graph: Label=%s"),
                *ItemId, *OutLabel);
            return true;
        }
        
        // 也尝试通过 Label 查找
        Node = FMASceneGraphQuery::FindNodeByLabelString(AllNodes, ItemId);
        if (Node.IsValid() && Node.IsPickupItem())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            OutFeatures = Node.Features;
            return true;
        }
    }
    
    // 2. 回退到 UE5 场景查询 (Requirements: 9.2)
    UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Pickup item '%s' not found in scene graph, falling back to UE5 scene query"), *ItemId);
    
    UWorld* World = Agent->GetWorld();
    if (World)
    {
        FMASemanticLabel Label;
        Label.Class = TEXT("object");
        Label.Features.Add(TEXT("label"), ItemId);
        
        FMASceneQueryResult Result = FMASceneQuery::FindObjectByLabel(World, Label);
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
    
    // 1. 首先尝试从场景图获取 (Requirements: 9.1)
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeById(AllNodes, StationId);
        
        if (Node.IsValid() && Node.IsChargingStation())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Found charging station '%s' in scene graph: Label=%s"),
                *StationId, *OutLabel);
            return true;
        }
        
        // 也尝试通过 Label 查找
        Node = FMASceneGraphQuery::FindNodeByLabelString(AllNodes, StationId);
        if (Node.IsValid() && Node.IsChargingStation())
        {
            OutLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            OutLocation = Node.Center;
            return true;
        }
    }
    
    // 2. 回退到 UE5 场景查询 (Requirements: 9.2)
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
    // 添加实体标签 (Requirements: 9.3)
    if (!Label.IsEmpty())
    {
        Feedback.Data.Add(FString::Printf(TEXT("%s_label"), *Prefix), Label);
    }
    
    // 添加实体位置 (Requirements: 9.4)
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

//=============================================================================
// 场景图状态更新实现 (Requirements: 9.5)
//=============================================================================

void FMAFeedbackGenerator::UpdateSceneGraphEntityPosition(
    AMACharacter* Agent,
    const FString& EntityId,
    const FVector& NewLocation,
    bool bIsRobot)
{
    if (!Agent || EntityId.IsEmpty())
    {
        return;
    }
    
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (!SceneGraphManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FMAFeedbackGenerator] Cannot update scene graph: SceneGraphManager not available"));
        return;
    }
    
    if (bIsRobot)
    {
        SceneGraphManager->UpdateRobotPosition(EntityId, NewLocation);
        UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Updated robot '%s' position in scene graph to (%f, %f, %f)"),
            *EntityId, NewLocation.X, NewLocation.Y, NewLocation.Z);
    }
    else
    {
        SceneGraphManager->UpdatePickupItemPosition(EntityId, NewLocation);
        UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Updated pickup item '%s' position in scene graph to (%f, %f, %f)"),
            *EntityId, NewLocation.X, NewLocation.Y, NewLocation.Z);
    }
}

void FMAFeedbackGenerator::UpdateSceneGraphItemCarrierStatus(
    AMACharacter* Agent,
    const FString& ItemId,
    bool bIsCarried,
    const FString& CarrierId)
{
    if (!Agent || ItemId.IsEmpty())
    {
        return;
    }
    
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (!SceneGraphManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FMAFeedbackGenerator] Cannot update scene graph: SceneGraphManager not available"));
        return;
    }
    
    SceneGraphManager->UpdatePickupItemCarrierStatus(ItemId, bIsCarried, CarrierId);
    UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Updated pickup item '%s' carrier status: bIsCarried=%s, CarrierId=%s"),
        *ItemId, bIsCarried ? TEXT("true") : TEXT("false"), *CarrierId);
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
    
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = CommandToSkillName(Command);
    
    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    
    switch (Command)
    {
        case EMACommand::Navigate:
            return GenerateNavigateFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Search:
            return GenerateSearchFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Follow:
            return GenerateFollowFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Charge:
            return GenerateChargeFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Place:
            return GeneratePlaceFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::TakeOff:
            return GenerateTakeOffFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Land:
            return GenerateLandFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::ReturnHome:
            return GenerateReturnHomeFeedback(Agent, SkillComp, bSuccess, Message);
        case EMACommand::Idle:
            return GenerateIdleFeedback(Agent, bSuccess, Message);
        default:
            Feedback.bSuccess = bSuccess;
            Feedback.Message = Message;
            return Feedback;
    }
}

//=============================================================================
// Navigate 反馈生成
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateNavigateFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Navigate");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        FVector Loc = Params.TargetLocation;
        Feedback.Data.Add(TEXT("target_x"), FString::Printf(TEXT("%.1f"), Loc.X));
        Feedback.Data.Add(TEXT("target_y"), FString::Printf(TEXT("%.1f"), Loc.Y));
        
        // 添加附近地标信息 (Requirements: 3.3, 3.4, 9.3, 9.4)
        if (!Context.NearbyLandmarkLabel.IsEmpty())
        {
            // 使用统一格式添加地标信息 (Requirements: 9.3, 9.4)
            AddEntityInfoToFeedback(Feedback, TEXT("nearby_landmark"), 
                Context.NearbyLandmarkLabel, FVector::ZeroVector, Context.NearbyLandmarkType);
            Feedback.Data.Add(TEXT("nearby_landmark_distance"), FString::Printf(TEXT("%.1f"), Context.NearbyLandmarkDistance));
        }
        
        //=========================================================================
        // 从场景图获取当前位置附近的地标信息 (Requirements: 9.1, 9.2)
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
        
        //=========================================================================
        // 更新场景图中机器人位置 (Requirements: 9.5)
        //=========================================================================
        if (bSuccess)
        {
            UpdateSceneGraphEntityPosition(Agent, Agent->AgentID, Agent->GetActorLocation(), true);
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
    
    return Feedback;
}

//=============================================================================
// Search 反馈生成
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateSearchFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Search");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        int32 FoundCount = Context.FoundObjects.Num();
        Feedback.Data.Add(TEXT("found_count"), FString::FromInt(FoundCount));
        
        if (FoundCount > 0)
        {
            //=========================================================================
            // 从场景图获取对象详细信息 (Requirements: 9.1, 9.2, 9.3, 9.4)
            //=========================================================================
            UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
            TArray<FMASceneGraphNode> AllNodes;
            if (SceneGraphManager)
            {
                AllNodes = SceneGraphManager->GetAllNodes();
            }
            
            for (int32 i = 0; i < FoundCount; ++i)
            {
                FString ObjectName = Context.FoundObjects[i];
                FVector ObjectLocation = Context.FoundLocations.IsValidIndex(i) ? Context.FoundLocations[i] : FVector::ZeroVector;
                FString ObjectType;
                FString ObjectLabel = ObjectName;
                
                // 尝试从场景图获取更详细的信息 (Requirements: 9.1)
                if (SceneGraphManager && AllNodes.Num() > 0)
                {
                    // 先尝试通过名称查找
                    FMASceneGraphNode Node = FMASceneGraphQuery::FindNodeByLabelString(AllNodes, ObjectName);
                    if (!Node.IsValid())
                    {
                        // 再尝试通过 ID 查找
                        Node = FMASceneGraphQuery::FindNodeById(AllNodes, ObjectName);
                    }
                    
                    if (Node.IsValid())
                    {
                        ObjectLabel = Node.Label.IsEmpty() ? Node.Id : Node.Label;
                        ObjectLocation = Node.Center;
                        ObjectType = Node.Type;
                        
                        // 添加节点特征信息
                        for (const auto& Feature : Node.Features)
                        {
                            Feedback.Data.Add(FString::Printf(TEXT("object_%d_%s"), i, *Feature.Key), Feature.Value);
                        }
                    }
                }
                
                // 使用统一格式添加对象信息 (Requirements: 9.3, 9.4)
                FString Prefix = FString::Printf(TEXT("object_%d"), i);
                Feedback.Data.Add(FString::Printf(TEXT("%s_name"), *Prefix), ObjectLabel);
                AddEntityInfoToFeedback(Feedback, Prefix, ObjectLabel, ObjectLocation, ObjectType);
            }
            
            // 添加对象属性 (从 ObjectAttributes 中提取)
            for (const auto& AttrPair : Context.ObjectAttributes)
            {
                Feedback.Data.Add(AttrPair.Key, AttrPair.Value);
            }
            
            // 构建找到的对象列表字符串 (用于消息)
            TArray<FString> ObjectNames;
            for (const FString& ObjName : Context.FoundObjects)
            {
                ObjectNames.Add(ObjName);
            }
            Feedback.Data.Add(TEXT("found_objects"), FString::Join(ObjectNames, TEXT(", ")));
        }
        
        //=========================================================================
        // 更新场景图中机器人位置 (Requirements: 9.5)
        //=========================================================================
        if (bSuccess)
        {
            UpdateSceneGraphEntityPosition(Agent, Agent->AgentID, Agent->GetActorLocation(), true);
        }
    }
    
    // 生成消息
    // Requirements: 4.7 - 如果未找到对象，指示 "search completed, no target found"
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
    
    return Feedback;
}

//=============================================================================
// Follow 反馈生成
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateFollowFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Follow");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加目标机器人信息 (Requirements: 5.4, 9.1, 9.2, 9.3, 9.4)
        //=========================================================================
        FString TargetRobotName = Context.FollowTargetRobotName;
        if (TargetRobotName.IsEmpty())
        {
            TargetRobotName = Context.TargetName;
        }
        
        if (!TargetRobotName.IsEmpty())
        {
            // 尝试从场景图获取目标机器人的详细信息 (Requirements: 9.1)
            FString RobotLabel, RobotAgentType;
            FVector RobotLocation;
            
            if (GetRobotInfoFromSceneGraph(Agent, TargetRobotName, RobotLabel, RobotLocation, RobotAgentType))
            {
                // 使用统一格式添加目标机器人信息 (Requirements: 9.3, 9.4)
                AddEntityInfoToFeedback(Feedback, TEXT("target_robot"), RobotLabel, RobotLocation, RobotAgentType);
            }
            else
            {
                // 回退：使用 Context 中的信息 (Requirements: 9.2)
                Feedback.Data.Add(TEXT("target_robot_label"), TargetRobotName);
            }
        }
        
        // 添加距离信息
        if (Context.FollowTargetDistance > 0.f)
        {
            Feedback.Data.Add(TEXT("distance"), FString::Printf(TEXT("%.1f"), Context.FollowTargetDistance));
        }
        
        // 添加目标是否找到的标志
        Feedback.Data.Add(TEXT("target_found"), Context.bFollowTargetFound ? TEXT("true") : TEXT("false"));
        
        //=========================================================================
        // 处理目标未找到的错误情况 (Requirements: 5.5)
        //=========================================================================
        if (!Context.bFollowTargetFound && !Context.FollowErrorReason.IsEmpty())
        {
            Feedback.Data.Add(TEXT("error_reason"), Context.FollowErrorReason);
        }
        
        //=========================================================================
        // 更新场景图中机器人位置 (Requirements: 9.5)
        //=========================================================================
        if (bSuccess)
        {
            UpdateSceneGraphEntityPosition(Agent, Agent->AgentID, Agent->GetActorLocation(), true);
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
            // 目标未找到的错误消息 (Requirements: 5.5)
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
            // 成功消息：包含目标机器人名称和距离 (Requirements: 5.4)
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
    
    return Feedback;
}

//=============================================================================
// Charge 反馈生成
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateChargeFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Charge");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加能量信息
        //=========================================================================
        Feedback.Data.Add(TEXT("energy_before"), FString::Printf(TEXT("%.1f%%"), Context.EnergyBefore));
        Feedback.Data.Add(TEXT("energy_after"), FString::Printf(TEXT("%.1f%%"), Context.EnergyAfter));
        
        //=========================================================================
        // 添加充电站信息 (Requirements: 7.3, 9.1, 9.2, 9.3, 9.4)
        //=========================================================================
        if (!Context.ChargingStationId.IsEmpty())
        {
            // 尝试从场景图获取充电站详细信息 (Requirements: 9.1)
            FString StationLabel;
            FVector StationLocation;
            
            if (GetChargingStationInfoFromSceneGraph(Agent, Context.ChargingStationId, StationLabel, StationLocation))
            {
                // 使用统一格式添加充电站信息 (Requirements: 9.3, 9.4)
                AddEntityInfoToFeedback(Feedback, TEXT("charging_station"), StationLabel, StationLocation, TEXT("charging_station"));
            }
            else
            {
                // 回退：使用 Context 中的信息 (Requirements: 9.2)
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
        // 处理充电站未找到的错误情况 (Requirements: 7.4)
        //=========================================================================
        if (!Context.bChargingStationFound && !Context.ChargeErrorReason.IsEmpty())
        {
            Feedback.Data.Add(TEXT("error_reason"), Context.ChargeErrorReason);
        }
        
        //=========================================================================
        // 更新场景图中机器人位置 (Requirements: 9.5)
        //=========================================================================
        if (bSuccess)
        {
            UpdateSceneGraphEntityPosition(Agent, Agent->AgentID, Agent->GetActorLocation(), true);
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
            // 充电站未找到的错误消息 (Requirements: 7.4)
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
            // 成功消息：包含充电站ID和能量等级 (Requirements: 7.3)
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
    
    return Feedback;
}

//=============================================================================
// Place 反馈生成
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::GeneratePlaceFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Place");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 从场景图获取对象信息 (Requirements: 6.7, 9.1, 9.2, 9.3, 9.4)
        //=========================================================================
        UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
        TArray<FMASceneGraphNode> AllNodes;
        if (SceneGraphManager)
        {
            AllNodes = SceneGraphManager->GetAllNodes();
        }
        
        // 获取 Object1 (放置的对象) 信息
        FString Object1NodeId = Context.ObjectAttributes.FindRef(TEXT("object1_node_id"));
        FString Object1Name = Context.PlacedObjectName;
        if (!Object1NodeId.IsEmpty() && SceneGraphManager)
        {
            FMASceneGraphNode Object1Node = FMASceneGraphQuery::FindNodeById(AllNodes, Object1NodeId);
            if (Object1Node.IsValid())
            {
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
        
        // 获取 Object2 (目标) 信息
        FString Object2NodeId = Context.ObjectAttributes.FindRef(TEXT("object2_node_id"));
        FString Object2Name = Context.PlaceTargetName;
        if (!Object2NodeId.IsEmpty() && SceneGraphManager)
        {
            FMASceneGraphNode Object2Node = FMASceneGraphQuery::FindNodeById(AllNodes, Object2NodeId);
            if (Object2Node.IsValid())
            {
                FString Label = Object2Node.Label.IsEmpty() ? Object2Node.Id : Object2Node.Label;
                AddEntityInfoToFeedback(Feedback, TEXT("target"), Label, Object2Node.Center, Object2Node.Type);
                
                // 如果是机器人，添加 agent_type
                if (Object2Node.IsRobot() && !Object2Node.AgentType.IsEmpty())
                {
                    Feedback.Data.Add(TEXT("target_agent_type"), Object2Node.AgentType);
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
        
        //=========================================================================
        // 更新场景图状态 (Requirements: 6.8, 9.5)
        //=========================================================================
        if (bSuccess && SceneGraphManager)
        {
            // 更新机器人位置
            UpdateSceneGraphEntityPosition(Agent, Agent->AgentID, Agent->GetActorLocation(), true);
            
            // 更新物品位置和携带状态
            if (!Object1NodeId.IsEmpty() && !Context.PlaceFinalLocation.IsZero())
            {
                // 更新物品位置
                SceneGraphManager->UpdatePickupItemPosition(Object1NodeId, Context.PlaceFinalLocation);
                
                // 判断是否放到了 UGV 上
                bool bPlacedOnRobot = false;
                FString CarrierId;
                if (!Object2NodeId.IsEmpty())
                {
                    FMASceneGraphNode Object2Node = FMASceneGraphQuery::FindNodeById(AllNodes, Object2NodeId);
                    if (Object2Node.IsValid() && Object2Node.IsRobot())
                    {
                        bPlacedOnRobot = true;
                        CarrierId = Object2NodeId;
                    }
                }
                
                // 更新携带状态
                SceneGraphManager->UpdatePickupItemCarrierStatus(Object1NodeId, bPlacedOnRobot, CarrierId);
                
                UE_LOG(LogTemp, Verbose, TEXT("[FMAFeedbackGenerator] Updated scene graph after Place: Item=%s, Location=(%f, %f, %f), Carried=%s, Carrier=%s"),
                    *Object1NodeId, Context.PlaceFinalLocation.X, Context.PlaceFinalLocation.Y, Context.PlaceFinalLocation.Z,
                    bPlacedOnRobot ? TEXT("true") : TEXT("false"), *CarrierId);
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
    
    return Feedback;
}

//=============================================================================
// TakeOff 反馈生成
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateTakeOffFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("TakeOff");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加高度信息 (Requirements: 8.4)
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
        // 添加空域信息 (附近建筑物) (Requirements: 8.4, 9.1, 9.3, 9.4)
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
            // 尝试从场景图获取附近建筑物信息 (Requirements: 9.1)
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
        
        //=========================================================================
        // 更新场景图中机器人位置 (Requirements: 9.5)
        //=========================================================================
        if (bSuccess)
        {
            UpdateSceneGraphEntityPosition(Agent, Agent->AgentID, Agent->GetActorLocation(), true);
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
    
    return Feedback;
}

//=============================================================================
// Land 反馈生成
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateLandFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Land");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加着陆位置信息 (Requirements: 8.5, 9.3, 9.4)
        //=========================================================================
        Feedback.Data.Add(TEXT("target_height"), FString::Printf(TEXT("%.1f"), Params.LandHeight));
        
        // 添加着陆位置坐标
        if (!Context.LandTargetLocation.IsZero())
        {
            AddEntityInfoToFeedback(Feedback, TEXT("land"), TEXT(""), Context.LandTargetLocation, TEXT(""));
        }
        
        //=========================================================================
        // 添加地面类型信息 (Requirements: 8.5, 9.1)
        //=========================================================================
        if (!Context.LandGroundType.IsEmpty())
        {
            Feedback.Data.Add(TEXT("ground_type"), Context.LandGroundType);
        }
        else
        {
            // 尝试从场景图获取地面类型 (Requirements: 9.1)
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
        
        // 添加附近地标信息 (Requirements: 9.1, 9.3, 9.4)
        if (!Context.LandNearbyLandmarkLabel.IsEmpty())
        {
            AddEntityInfoToFeedback(Feedback, TEXT("nearby_landmark"), 
                Context.LandNearbyLandmarkLabel, FVector::ZeroVector, TEXT(""));
        }
        else
        {
            // 尝试从场景图获取附近地标 (Requirements: 9.1)
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
        
        //=========================================================================
        // 更新场景图中机器人位置 (Requirements: 9.5)
        //=========================================================================
        if (bSuccess)
        {
            UpdateSceneGraphEntityPosition(Agent, Agent->AgentID, Agent->GetActorLocation(), true);
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
    
    return Feedback;
}

//=============================================================================
// ReturnHome 反馈生成
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateReturnHomeFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("ReturnHome");
    Feedback.bSuccess = bSuccess;
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        //=========================================================================
        // 添加家位置信息 (Requirements: 8.6, 9.3, 9.4)
        //=========================================================================
        AddEntityInfoToFeedback(Feedback, TEXT("home"), TEXT(""), Params.HomeLocation, TEXT(""));
        Feedback.Data.Add(TEXT("land_height"), FString::Printf(TEXT("%.1f"), Params.LandHeight));
        
        //=========================================================================
        // 添加家位置标签信息 (Requirements: 8.6, 9.1, 9.3)
        //=========================================================================
        if (!Context.HomeLandmarkLabel.IsEmpty())
        {
            Feedback.Data.Add(TEXT("home_landmark_label"), Context.HomeLandmarkLabel);
        }
        else
        {
            // 尝试从场景图获取家位置附近的地标 (Requirements: 9.1)
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
        
        //=========================================================================
        // 更新场景图中机器人位置 (Requirements: 9.5)
        //=========================================================================
        if (bSuccess)
        {
            UpdateSceneGraphEntityPosition(Agent, Agent->AgentID, Agent->GetActorLocation(), true);
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
    
    return Feedback;
}

//=============================================================================
// Idle 反馈生成
//=============================================================================

FMASkillExecutionFeedback FMAFeedbackGenerator::GenerateIdleFeedback(AMACharacter* Agent, bool bSuccess, const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = TEXT("Idle");
    Feedback.bSuccess = true;
    Feedback.Message = Message.IsEmpty() ? TEXT("Idle") : Message;
    return Feedback;
}

//=============================================================================
// 辅助方法
//=============================================================================

FString FMAFeedbackGenerator::CommandToSkillName(EMACommand Command)
{
    switch (Command)
    {
        case EMACommand::Idle: return TEXT("Idle");
        case EMACommand::Navigate: return TEXT("Navigate");
        case EMACommand::Follow: return TEXT("Follow");
        case EMACommand::Charge: return TEXT("Charge");
        case EMACommand::Search: return TEXT("Search");
        case EMACommand::Place: return TEXT("Place");
        case EMACommand::TakeOff: return TEXT("TakeOff");
        case EMACommand::Land: return TEXT("Land");
        case EMACommand::ReturnHome: return TEXT("ReturnHome");
        default: return TEXT("Unknown");
    }
}
