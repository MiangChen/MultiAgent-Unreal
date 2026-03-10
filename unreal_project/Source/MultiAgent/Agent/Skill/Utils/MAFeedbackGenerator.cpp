// MAFeedbackGenerator.cpp
// 反馈生成器实现

#include "MAFeedbackGenerator.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "../../../Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"
#include "Serialization/JsonSerializer.h"

// 辅助宏：cm 转 m
#define CM_TO_M(x) ((x) / 100.0f)

//=============================================================================
// 场景图节点 JSON 构建
//=============================================================================

TSharedPtr<FJsonObject> FMAFeedbackGenerator::BuildNodeJsonFromSceneGraph(const FMASceneGraphNode& Node)
{
    TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject());
    NodeObject->SetStringField(TEXT("id"), Node.Id);
    
    TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject());
    PropertiesObject->SetStringField(TEXT("category"), Node.Category);
    PropertiesObject->SetStringField(TEXT("type"), Node.Type);
    PropertiesObject->SetStringField(TEXT("label"), Node.Label);
    
    if (!Node.LocationLabel.IsEmpty())
        PropertiesObject->SetStringField(TEXT("location_label"), Node.LocationLabel);
    
    if (Node.IsPickupItem())
    {
        PropertiesObject->SetBoolField(TEXT("is_carried"), Node.bIsCarried);
        if (!Node.CarrierId.IsEmpty())
            PropertiesObject->SetStringField(TEXT("carrier_id"), Node.CarrierId);
    }
    
    if (Node.bIsDynamic)
        PropertiesObject->SetBoolField(TEXT("is_dynamic"), true);
    
    for (const auto& Feature : Node.Features)
        PropertiesObject->SetStringField(Feature.Key, Feature.Value);
    
    NodeObject->SetObjectField(TEXT("properties"), PropertiesObject);
    
    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    ShapeObject->SetStringField(TEXT("type"), Node.ShapeType.IsEmpty() ? TEXT("point") : Node.ShapeType);
    
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Z)));
    ShapeObject->SetArrayField(TEXT("center"), CenterArray);
    
    NodeObject->SetObjectField(TEXT("shape"), ShapeObject);
    return NodeObject;
}

TSharedPtr<FJsonObject> FMAFeedbackGenerator::BuildNodeJsonFromUE5Data(
    const FString& Id, const FString& Label, const FVector& Location,
    const FString& Type, const FString& Category, const TMap<FString, FString>& Features)
{
    TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject());
    NodeObject->SetStringField(TEXT("id"), Id);
    
    TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject());
    if (!Category.IsEmpty()) PropertiesObject->SetStringField(TEXT("category"), Category);
    if (!Type.IsEmpty()) PropertiesObject->SetStringField(TEXT("type"), Type);
    PropertiesObject->SetStringField(TEXT("label"), Label);
    
    for (const auto& Feature : Features)
        PropertiesObject->SetStringField(Feature.Key, Feature.Value);
    
    NodeObject->SetObjectField(TEXT("properties"), PropertiesObject);
    
    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    ShapeObject->SetStringField(TEXT("type"), TEXT("point"));
    
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
    ShapeObject->SetArrayField(TEXT("center"), CenterArray);
    
    NodeObject->SetObjectField(TEXT("shape"), ShapeObject);
    return NodeObject;
}

//=============================================================================
// 辅助方法
//=============================================================================

UMASceneGraphManager* FMAFeedbackGenerator::GetSceneGraphManager(AMACharacter* Agent)
{
    if (!Agent) return nullptr;
    UWorld* World = Agent->GetWorld();
    if (!World) return nullptr;
    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance) return nullptr;
    return GameInstance->GetSubsystem<UMASceneGraphManager>();
}

void FMAFeedbackGenerator::AddCommonFieldsToFeedback(FMASkillExecutionFeedback& Feedback, UMASkillComponent* SkillComp, AMACharacter* Agent)
{
    if (!SkillComp || !Agent) return;
    
    const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
    
    if (!Context.TaskId.IsEmpty())
        Feedback.Data.Add(TEXT("task_id"), Context.TaskId);
    
    // robot_id (数字ID) 和 robot_label (字符串标签)
    UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager(Agent);
    if (SceneGraphMgr)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr->GetAllNodes();
        // AgentID 可能是 "UAV-1" 这样的标签，需要转换为数字 ID
        FString RobotId = FMASceneGraphQueryUseCases::GetIdByLabel(AllNodes, Agent->AgentID);
        if (RobotId.IsEmpty())
            RobotId = Agent->AgentID;  // 回退
        Feedback.Data.Add(TEXT("robot_id"), RobotId);
        Feedback.Data.Add(TEXT("robot_label"), FMASceneGraphQueryUseCases::GetLabelById(AllNodes, RobotId));
    }
    else
    {
        Feedback.Data.Add(TEXT("robot_id"), Agent->AgentID);
        Feedback.Data.Add(TEXT("robot_label"), Agent->AgentLabel.IsEmpty() ? Agent->AgentID : Agent->AgentLabel);
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
    
    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = UMACommandManager::CommandToString(Command);
    Feedback.bSuccess = bSuccess;
    
    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    
    switch (Command)
    {
        case EMACommand::Navigate:    GenerateNavigateFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Search:      GenerateSearchFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Follow:      GenerateFollowFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Charge:      GenerateChargeFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Place:       GeneratePlaceFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::TakeOff:     GenerateTakeOffFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Land:        GenerateLandFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::ReturnHome:  GenerateReturnHomeFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::TakePhoto:   GenerateTakePhotoFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Broadcast:   GenerateBroadcastFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::HandleHazard: GenerateHandleHazardFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Guide:       GenerateGuideFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Idle:        GenerateIdleFeedback(Feedback, Agent, bSuccess, Message); break;
        default: Feedback.Message = Message; break;
    }
    
    return Feedback;
}


//=============================================================================
// Navigate 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateNavigateFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        FVector Loc = Params.TargetLocation;
        Feedback.Data.Add(TEXT("target_x"), FString::Printf(TEXT("%.1f"), Loc.X));
        Feedback.Data.Add(TEXT("target_y"), FString::Printf(TEXT("%.1f"), Loc.Y));
        
        if (!Context.NearbyLandmarkLabel.IsEmpty())
        {
            Feedback.Data.Add(TEXT("nearby_landmark_label"), Context.NearbyLandmarkLabel);
            Feedback.Data.Add(TEXT("nearby_landmark_distance"), FString::Printf(TEXT("%.1f"), CM_TO_M(Context.NearbyLandmarkDistance)));
        }
    }
    
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        
        if (bSuccess)
        {
            if (!Context.NearbyLandmarkLabel.IsEmpty())
                Feedback.Message = FString::Printf(TEXT("Navigation completed to (%.0f, %.0f), near %s"),
                    Params.TargetLocation.X, Params.TargetLocation.Y, *Context.NearbyLandmarkLabel);
            else
                Feedback.Message = FString::Printf(TEXT("Navigation completed to (%.0f, %.0f)"),
                    Params.TargetLocation.X, Params.TargetLocation.Y);
        }
        else
            Feedback.Message = TEXT("Navigation failed");
    }
    else
        Feedback.Message = bSuccess ? TEXT("Navigation completed") : TEXT("Navigation failed");
}

//=============================================================================
// Search 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateSearchFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        int32 FoundCount = Context.FoundObjects.Num();
        
        Feedback.Data.Add(TEXT("found_count"), FString::FromInt(FoundCount));
        Feedback.Data.Add(TEXT("found"), FoundCount > 0 ? TEXT("true") : TEXT("false"));
        
        if (!Context.SearchAreaToken.IsEmpty())
            Feedback.Data.Add(TEXT("area_token"), Context.SearchAreaToken);
        
        if (Context.SearchDurationSeconds > 0.f)
            Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), Context.SearchDurationSeconds));
        
        if (FoundCount > 0)
        {
            UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager(Agent);
            TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr ? SceneGraphMgr->GetAllNodes() : TArray<FMASceneGraphNode>();
            
            TArray<TSharedPtr<FJsonValue>> DiscoveredNodesArray;
            TArray<TSharedPtr<FJsonValue>> FoundIdsArray;
            
            for (int32 i = 0; i < FoundCount; ++i)
            {
                FString ObjectName = Context.FoundObjects[i];
                FVector ObjectLocation = Context.FoundLocations.IsValidIndex(i) ? Context.FoundLocations[i] : FVector::ZeroVector;
                
                TSharedPtr<FJsonObject> NodeJson;
                FString NodeId;
                
                if (AllNodes.Num() > 0)
                {
                    FMASceneGraphNode Node = FMASceneGraphQueryUseCases::FindNodeByIdOrLabel(AllNodes, ObjectName);
                    if (Node.IsValid())
                    {
                        NodeJson = BuildNodeJsonFromSceneGraph(Node);
                        NodeId = Node.Id;
                    }
                }
                
                if (!NodeJson.IsValid())
                {
                    NodeId = FString::Printf(TEXT("temp_%s_%d"), *ObjectName, i);
                    NodeJson = BuildNodeJsonFromUE5Data(NodeId, ObjectName, ObjectLocation, TEXT(""), TEXT(""), TMap<FString, FString>());
                }
                
                DiscoveredNodesArray.Add(MakeShareable(new FJsonValueObject(NodeJson)));
                FoundIdsArray.Add(MakeShareable(new FJsonValueString(NodeId)));
            }
            
            FString DiscoveredNodesJsonString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&DiscoveredNodesJsonString);
            FJsonSerializer::Serialize(DiscoveredNodesArray, Writer);
            Feedback.Data.Add(TEXT("discovered_nodes"), DiscoveredNodesJsonString);
            
            FString FoundIdsJsonString;
            TSharedRef<TJsonWriter<>> IdsWriter = TJsonWriterFactory<>::Create(&FoundIdsJsonString);
            FJsonSerializer::Serialize(FoundIdsArray, IdsWriter);
            Feedback.Data.Add(TEXT("found_ids"), FoundIdsJsonString);
            
            Feedback.Data.Add(TEXT("found_objects"), FString::Join(Context.FoundObjects, TEXT(", ")));
        }
    }
    
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        int32 FoundCount = Context.FoundObjects.Num();
        
        if (FoundCount > 0)
        {
            if (FoundCount == 1)
                Feedback.Message = FString::Printf(TEXT("Search completed, found %s"), *Context.FoundObjects[0]);
            else
                Feedback.Message = FString::Printf(TEXT("Search completed, found %d objects"), FoundCount);
        }
        else
            Feedback.Message = TEXT("Search completed, no target found");
    }
    else
        Feedback.Message = bSuccess ? TEXT("Search completed") : TEXT("Search failed");
}


//=============================================================================
// Follow 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateFollowFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager(Agent);
        TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr ? SceneGraphMgr->GetAllNodes() : TArray<FMASceneGraphNode>();
        
        // target_id 和 target_label
        if (!Context.FollowTargetId.IsEmpty())
        {
            Feedback.Data.Add(TEXT("target_id"), Context.FollowTargetId);
            FString TargetLabel = FMASceneGraphQueryUseCases::GetLabelById(AllNodes, Context.FollowTargetId);
            Feedback.Data.Add(TEXT("target_label"), TargetLabel);
        }
        else if (!Context.FollowTargetName.IsEmpty())
        {
            FString TargetId = FMASceneGraphQueryUseCases::GetIdByLabel(AllNodes, Context.FollowTargetName);
            if (!TargetId.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_id"), TargetId);
                Feedback.Data.Add(TEXT("target_label"), Context.FollowTargetName);
            }
            else
            {
                Feedback.Data.Add(TEXT("target_label"), Context.FollowTargetName);
            }
        }
        
        // duration_s
        if (Context.FollowDurationSeconds > 0.f)
            Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), Context.FollowDurationSeconds));
        
        // distance (cm -> m)
        if (Context.FollowTargetDistance > 0.f)
            Feedback.Data.Add(TEXT("distance"), FString::Printf(TEXT("%.1f"), CM_TO_M(Context.FollowTargetDistance)));
        
        Feedback.Data.Add(TEXT("target_found"), Context.bFollowTargetFound ? TEXT("true") : TEXT("false"));
        
        if (!Context.bFollowTargetFound && !Context.FollowErrorReason.IsEmpty())
            Feedback.Data.Add(TEXT("error_reason"), Context.FollowErrorReason);
    }
    
    // 生成消息
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        FString TargetLabel = Feedback.Data.FindRef(TEXT("target_label"));
        
        if (!Context.bFollowTargetFound)
        {
            Feedback.Message = Context.FollowErrorReason.IsEmpty() 
                ? TEXT("Follow failed: target not found")
                : FString::Printf(TEXT("Follow failed: %s"), *Context.FollowErrorReason);
            Feedback.bSuccess = false;
        }
        else if (bSuccess)
        {
            if (!TargetLabel.IsEmpty() && Context.FollowTargetDistance > 0.f)
                Feedback.Message = FString::Printf(TEXT("Follow completed: followed %s, final distance %.1fm"),
                    *TargetLabel, CM_TO_M(Context.FollowTargetDistance));
            else if (!TargetLabel.IsEmpty())
                Feedback.Message = FString::Printf(TEXT("Follow completed: followed %s"), *TargetLabel);
            else
                Feedback.Message = TEXT("Follow completed");
        }
        else
            Feedback.Message = TargetLabel.IsEmpty() ? TEXT("Follow failed") : FString::Printf(TEXT("Follow failed: could not follow %s"), *TargetLabel);
    }
    else
        Feedback.Message = bSuccess ? TEXT("Follow completed") : TEXT("Follow failed");
}

//=============================================================================
// Charge 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateChargeFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        Feedback.Data.Add(TEXT("energy_before"), FString::Printf(TEXT("%.1f%%"), Context.EnergyBefore));
        Feedback.Data.Add(TEXT("energy_after"), FString::Printf(TEXT("%.1f%%"), Context.EnergyAfter));
        
        if (!Context.ChargingStationId.IsEmpty())
        {
            UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager(Agent);
            TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr ? SceneGraphMgr->GetAllNodes() : TArray<FMASceneGraphNode>();
            
            Feedback.Data.Add(TEXT("station_id"), Context.ChargingStationId);
            FString StationLabel = FMASceneGraphQueryUseCases::GetLabelById(AllNodes, Context.ChargingStationId);
            Feedback.Data.Add(TEXT("station_label"), StationLabel);
        }
        
        Feedback.Data.Add(TEXT("station_found"), Context.bChargingStationFound ? TEXT("true") : TEXT("false"));
        
        if (!Context.bChargingStationFound && !Context.ChargeErrorReason.IsEmpty())
            Feedback.Data.Add(TEXT("error_reason"), Context.ChargeErrorReason);
    }
    
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        FString StationLabel = Feedback.Data.FindRef(TEXT("station_label"));
        
        if (!Context.bChargingStationFound)
        {
            Feedback.Message = Context.ChargeErrorReason.IsEmpty()
                ? TEXT("Charge failed: no charging station available")
                : FString::Printf(TEXT("Charge failed: %s"), *Context.ChargeErrorReason);
            Feedback.bSuccess = false;
        }
        else if (bSuccess)
        {
            if (!StationLabel.IsEmpty())
                Feedback.Message = FString::Printf(TEXT("Charge completed at %s, energy: %.1f%% -> %.1f%%"),
                    *StationLabel, Context.EnergyBefore, Context.EnergyAfter);
            else
                Feedback.Message = FString::Printf(TEXT("Charge completed, energy: %.1f%% -> %.1f%%"),
                    Context.EnergyBefore, Context.EnergyAfter);
        }
        else
            Feedback.Message = FString::Printf(TEXT("Charge interrupted, energy: %.1f%%"), Context.EnergyAfter);
    }
    else
        Feedback.Message = bSuccess ? TEXT("Charge completed") : TEXT("Charge interrupted");
}


//=============================================================================
// Place 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GeneratePlaceFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager(Agent);
        TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr ? SceneGraphMgr->GetAllNodes() : TArray<FMASceneGraphNode>();
        
        // object (被放置的物品)
        FString Object1NodeId = Context.ObjectAttributes.FindRef(TEXT("object1_node_id"));
        if (!Object1NodeId.IsEmpty())
        {
            Feedback.Data.Add(TEXT("object_id"), Object1NodeId);
            Feedback.Data.Add(TEXT("object_label"), FMASceneGraphQueryUseCases::GetLabelById(AllNodes, Object1NodeId));
        }
        else if (!Context.PlacedObjectName.IsEmpty())
            Feedback.Data.Add(TEXT("object_label"), Context.PlacedObjectName);
        
        // target (放置目标)
        FString Object2NodeId = Context.ObjectAttributes.FindRef(TEXT("object2_node_id"));
        if (!Object2NodeId.IsEmpty())
        {
            Feedback.Data.Add(TEXT("target_id"), Object2NodeId);
            Feedback.Data.Add(TEXT("target_label"), FMASceneGraphQueryUseCases::GetLabelById(AllNodes, Object2NodeId));
        }
        else if (!Context.PlaceTargetName.IsEmpty())
            Feedback.Data.Add(TEXT("target_label"), Context.PlaceTargetName);
        
        if (!Context.PlaceFinalLocation.IsZero())
            Feedback.Data.Add(TEXT("final_location"), FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), 
                Context.PlaceFinalLocation.X, Context.PlaceFinalLocation.Y, Context.PlaceFinalLocation.Z));
        
        if (!bSuccess && !Context.PlaceErrorReason.IsEmpty())
            Feedback.Data.Add(TEXT("error_reason"), Context.PlaceErrorReason);
    }
    
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        FString ObjectLabel = Feedback.Data.FindRef(TEXT("object_label"));
        FString TargetLabel = Feedback.Data.FindRef(TEXT("target_label"));
        
        if (bSuccess)
        {
            if (!ObjectLabel.IsEmpty() && !TargetLabel.IsEmpty())
                Feedback.Message = FString::Printf(TEXT("Place succeeded: Moved %s to %s"), *ObjectLabel, *TargetLabel);
            else
                Feedback.Message = TEXT("Place completed");
        }
        else
        {
            if (!Context.PlaceErrorReason.IsEmpty())
                Feedback.Message = FString::Printf(TEXT("Place failed: %s"), *Context.PlaceErrorReason);
            else
                Feedback.Message = TEXT("Place failed");
        }
    }
    else
        Feedback.Message = bSuccess ? TEXT("Place completed") : TEXT("Place failed");
}

//=============================================================================
// TakeOff 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateTakeOffFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        Feedback.Data.Add(TEXT("target_height"), FString::Printf(TEXT("%.1f"), CM_TO_M(Params.TakeOffHeight)));
        Feedback.Data.Add(TEXT("height_adjusted"), Context.bTakeOffHeightAdjusted ? TEXT("true") : TEXT("false"));
        
        if (!Context.TakeOffNearbyBuildingLabel.IsEmpty())
        {
            Feedback.Data.Add(TEXT("nearby_building_label"), Context.TakeOffNearbyBuildingLabel);
            Feedback.Data.Add(TEXT("nearby_building_height"), FString::Printf(TEXT("%.1f"), CM_TO_M(Context.TakeOffNearbyBuildingHeight)));
        }
    }
    
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            if (Context.bTakeOffHeightAdjusted && !Context.TakeOffNearbyBuildingLabel.IsEmpty())
                Feedback.Message = FString::Printf(TEXT("TakeOff completed at height %.0fm (adjusted above %s)"),
                    CM_TO_M(Params.TakeOffHeight), *Context.TakeOffNearbyBuildingLabel);
            else
                Feedback.Message = FString::Printf(TEXT("TakeOff completed at height %.0fm"), CM_TO_M(Params.TakeOffHeight));
        }
        else
            Feedback.Message = TEXT("TakeOff failed");
    }
    else
        Feedback.Message = bSuccess ? TEXT("TakeOff completed") : TEXT("TakeOff failed");
}

//=============================================================================
// Land 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateLandFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (!Context.LandGroundType.IsEmpty())
            Feedback.Data.Add(TEXT("ground_type"), Context.LandGroundType);
        
        if (!Context.LandNearbyLandmarkLabel.IsEmpty())
            Feedback.Data.Add(TEXT("nearby_landmark_label"), Context.LandNearbyLandmarkLabel);
        
        Feedback.Data.Add(TEXT("location_safe"), Context.bLandLocationSafe ? TEXT("true") : TEXT("false"));
    }
    
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            if (!Context.LandNearbyLandmarkLabel.IsEmpty())
                Feedback.Message = FString::Printf(TEXT("Land completed near %s"), *Context.LandNearbyLandmarkLabel);
            else
                Feedback.Message = TEXT("Land completed");
        }
        else
        {
            if (!Context.bLandLocationSafe)
                Feedback.Message = TEXT("Land failed: unsafe landing location");
            else
                Feedback.Message = TEXT("Land failed");
        }
    }
    else
        Feedback.Message = bSuccess ? TEXT("Land completed") : TEXT("Land failed");
}

//=============================================================================
// ReturnHome 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateReturnHomeFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        Feedback.Data.Add(TEXT("home_x"), FString::Printf(TEXT("%.1f"), Params.HomeLocation.X));
        Feedback.Data.Add(TEXT("home_y"), FString::Printf(TEXT("%.1f"), Params.HomeLocation.Y));
        
        if (!Context.HomeLandmarkLabel.IsEmpty())
            Feedback.Data.Add(TEXT("home_landmark_label"), Context.HomeLandmarkLabel);
    }
    
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess)
        {
            if (!Context.HomeLandmarkLabel.IsEmpty())
                Feedback.Message = FString::Printf(TEXT("ReturnHome completed at (%.0f, %.0f), near %s"),
                    Params.HomeLocation.X, Params.HomeLocation.Y, *Context.HomeLandmarkLabel);
            else
                Feedback.Message = FString::Printf(TEXT("ReturnHome completed at (%.0f, %.0f)"),
                    Params.HomeLocation.X, Params.HomeLocation.Y);
        }
        else
            Feedback.Message = TEXT("ReturnHome failed");
    }
    else
        Feedback.Message = bSuccess ? TEXT("ReturnHome completed") : TEXT("ReturnHome failed");
}


//=============================================================================
// TakePhoto 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateTakePhotoFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager(Agent);
        TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr ? SceneGraphMgr->GetAllNodes() : TArray<FMASceneGraphNode>();
        
        Feedback.Data.Add(TEXT("target_found"), Context.bPhotoTargetFound ? TEXT("true") : TEXT("false"));
        
        if (Context.bPhotoTargetFound)
        {
            if (!Context.PhotoTargetId.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_id"), Context.PhotoTargetId);
                Feedback.Data.Add(TEXT("target_label"), FMASceneGraphQueryUseCases::GetLabelById(AllNodes, Context.PhotoTargetId));
            }
            else if (!Context.PhotoTargetName.IsEmpty())
                Feedback.Data.Add(TEXT("target_label"), Context.PhotoTargetName);
            
            if (Context.PhotoRobotTargetDistance >= 0.f)
                Feedback.Data.Add(TEXT("robot_target_distance"), FString::Printf(TEXT("%.2f"), Context.PhotoRobotTargetDistance));
        }
    }
    
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        FString TargetLabel = Feedback.Data.FindRef(TEXT("target_label"));
        
        if (bSuccess)
        {
            if (Context.bPhotoTargetFound && !TargetLabel.IsEmpty())
                Feedback.Message = FString::Printf(TEXT("Photo taken successfully, found %s"), *TargetLabel);
            else if (Context.bPhotoTargetFound)
                Feedback.Message = TEXT("Photo taken successfully, found target");
            else
                Feedback.Message = TEXT("Photo taken successfully, but target not found");
        }
        else
            Feedback.Message = TEXT("Photo failed");
    }
    else
        Feedback.Message = bSuccess ? TEXT("Photo taken successfully") : TEXT("Photo failed");
}

//=============================================================================
// Broadcast 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateBroadcastFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (!Context.BroadcastMessage.IsEmpty())
            Feedback.Data.Add(TEXT("message"), Context.BroadcastMessage);
        
        Feedback.Data.Add(TEXT("target_found"), Context.bBroadcastTargetFound ? TEXT("true") : TEXT("false"));
        
        if (Context.bBroadcastTargetFound && !Context.BroadcastTargetName.IsEmpty())
            Feedback.Data.Add(TEXT("target_name"), Context.BroadcastTargetName);
        
        if (Context.BroadcastRobotTargetDistance >= 0.f)
            Feedback.Data.Add(TEXT("robot_target_distance"), FString::Printf(TEXT("%.2f"), Context.BroadcastRobotTargetDistance));
        
        if (Context.BroadcastDurationSeconds > 0.f)
            Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), Context.BroadcastDurationSeconds));
    }
    
    Feedback.Message = Message.IsEmpty() ? (bSuccess ? TEXT("Broadcast succeeded") : TEXT("Broadcast failed")) : Message;
}

//=============================================================================
// HandleHazard 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateHandleHazardFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager(Agent);
        TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr ? SceneGraphMgr->GetAllNodes() : TArray<FMASceneGraphNode>();
        
        Feedback.Data.Add(TEXT("target_found"), Context.bHazardTargetFound ? TEXT("true") : TEXT("false"));
        
        if (Context.bHazardTargetFound)
        {
            if (!Context.HazardTargetId.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_id"), Context.HazardTargetId);
                Feedback.Data.Add(TEXT("target_label"), FMASceneGraphQueryUseCases::GetLabelById(AllNodes, Context.HazardTargetId));
            }
            else if (!Context.HazardTargetName.IsEmpty())
                Feedback.Data.Add(TEXT("target_label"), Context.HazardTargetName);
        }
        
        if (Context.HazardHandleDurationSeconds > 0.f)
            Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), Context.HazardHandleDurationSeconds));
    }
    
    Feedback.Message = Message.IsEmpty() ? (bSuccess ? TEXT("Hazard handled successfully") : TEXT("Hazard handling failed")) : Message;
}

//=============================================================================
// Guide 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateGuideFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager(Agent);
        TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr ? SceneGraphMgr->GetAllNodes() : TArray<FMASceneGraphNode>();
        
        Feedback.Data.Add(TEXT("target_found"), Context.bGuideTargetFound ? TEXT("true") : TEXT("false"));
        
        if (Context.bGuideTargetFound)
        {
            if (!Context.GuideTargetId.IsEmpty())
            {
                Feedback.Data.Add(TEXT("target_id"), Context.GuideTargetId);
                Feedback.Data.Add(TEXT("target_label"), FMASceneGraphQueryUseCases::GetLabelById(AllNodes, Context.GuideTargetId));
            }
            else if (!Context.GuideTargetName.IsEmpty())
                Feedback.Data.Add(TEXT("target_label"), Context.GuideTargetName);
        }
        
        if (!Context.GuideDestination.IsZero())
            Feedback.Data.Add(TEXT("destination"), FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), 
                Context.GuideDestination.X, Context.GuideDestination.Y, Context.GuideDestination.Z));
        
        if (Context.GuideDurationSeconds > 0.f)
            Feedback.Data.Add(TEXT("duration_s"), FString::Printf(TEXT("%.1f"), Context.GuideDurationSeconds));
    }
    
    if (!Message.IsEmpty())
        Feedback.Message = Message;
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        
        if (bSuccess && !Context.GuideDestination.IsZero())
            Feedback.Message = FString::Printf(TEXT("Guide succeeded, target guided to (%.0f, %.0f, %.0f)"),
                Context.GuideDestination.X, Context.GuideDestination.Y, Context.GuideDestination.Z);
        else
            Feedback.Message = bSuccess ? TEXT("Guide succeeded") : TEXT("Guide failed");
    }
    else
        Feedback.Message = bSuccess ? TEXT("Guide succeeded") : TEXT("Guide failed");
}

//=============================================================================
// Idle 反馈生成
//=============================================================================

void FMAFeedbackGenerator::GenerateIdleFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, bool bSuccess, const FString& Message)
{
    if (UMASkillComponent* SkillComp = Agent->GetSkillComponent())
        AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);
    
    Feedback.Message = Message.IsEmpty() ? TEXT("Idle") : Message;
}
