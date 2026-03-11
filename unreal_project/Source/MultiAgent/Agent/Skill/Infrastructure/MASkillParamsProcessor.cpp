// MASkillParamsProcessor.cpp
// 技能参数处理器实现

#include "MASkillParamsProcessor.h"
#include "Agent/Skill/Infrastructure/MASkillTargetResolver.h"
#include "Agent/Skill/Infrastructure/MASkillRuntimeBridge.h"
#include "Agent/Skill/Infrastructure/MAUESceneQuery.h"
#include "Agent/Skill/Domain/MASkillTypes.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Core/Comm/Domain/MACommTypes.h"
#include "Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"
#include "Utils/MAGeometryUtils.h"
#include "Environment/Entity/MAChargingStation.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/OverlapResult.h"

//=============================================================================
// JSON 解析辅助函数
//=============================================================================

namespace MAParamsHelper
{
    /** 解析 RawParamsJson 为 JsonObject */
    bool ParseRawParams(const FString& RawParamsJson, TSharedPtr<FJsonObject>& OutJson)
    {
        if (RawParamsJson.IsEmpty())
        {
            return false;
        }
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawParamsJson);
        return FJsonSerializer::Deserialize(Reader, OutJson) && OutJson.IsValid();
    }

    /** 从 JsonObject 提取 dest 坐标 */
    bool ExtractDestPosition(const TSharedPtr<FJsonObject>& Json, FVector& OutPosition)
    {
        const TSharedPtr<FJsonObject>* DestObject;
        if (Json->TryGetObjectField(TEXT("dest"), DestObject))
        {
            double X = 0, Y = 0, Z = 0;
            (*DestObject)->TryGetNumberField(TEXT("x"), X);
            (*DestObject)->TryGetNumberField(TEXT("y"), Y);
            (*DestObject)->TryGetNumberField(TEXT("z"), Z);
            OutPosition = FVector(X, Y, Z);
            return true;
        }
        return false;
    }

    /** 从 JsonObject 提取 search_area 多边形 */
    bool ExtractSearchArea(const TSharedPtr<FJsonObject>& Json, TArray<FVector>& OutBoundary)
    {
        OutBoundary.Empty();
        
        // 格式1: { "area": { "coords": [[x,y], ...] } }
        const TSharedPtr<FJsonObject>* AreaObject;
        if (Json->TryGetObjectField(TEXT("area"), AreaObject))
        {
            const TArray<TSharedPtr<FJsonValue>>* CoordsArray;
            if ((*AreaObject)->TryGetArrayField(TEXT("coords"), CoordsArray))
            {
                for (const auto& CoordValue : *CoordsArray)
                {
                    const TArray<TSharedPtr<FJsonValue>>* PointArray;
                    if (CoordValue->TryGetArray(PointArray) && PointArray->Num() >= 2)
                    {
                        double PX = (*PointArray)[0]->AsNumber();
                        double PY = (*PointArray)[1]->AsNumber();
                        OutBoundary.Add(FVector(PX, PY, 0.0f));
                    }
                }
            }
        }
        
        // 格式2: { "search_area": [[x,y], ...] }
        if (OutBoundary.Num() == 0)
        {
            const TArray<TSharedPtr<FJsonValue>>* SearchAreaArray;
            if (Json->TryGetArrayField(TEXT("search_area"), SearchAreaArray))
            {
                for (const auto& CoordValue : *SearchAreaArray)
                {
                    const TArray<TSharedPtr<FJsonValue>>* PointArray;
                    if (CoordValue->TryGetArray(PointArray) && PointArray->Num() >= 2)
                    {
                        double PX = (*PointArray)[0]->AsNumber();
                        double PY = (*PointArray)[1]->AsNumber();
                        OutBoundary.Add(FVector(PX, PY, 0.0f));
                    }
                }
            }
        }
        
        return OutBoundary.Num() >= 3;
    }

    /** 从 JsonObject 提取 target 并序列化为 JSON 字符串 */
    bool ExtractTargetJson(const TSharedPtr<FJsonObject>& Json, const FString& FieldName, FString& OutTargetJson)
    {
        const TSharedPtr<FJsonObject>* TargetObj;
        if (Json->TryGetObjectField(FieldName, TargetObj))
        {
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutTargetJson);
            FJsonSerializer::Serialize((*TargetObj).ToSharedRef(), Writer);
            return true;
        }
        return false;
    }

    /** 从 JsonObject 提取 task_id */
    FString ExtractTaskId(const TSharedPtr<FJsonObject>& Json)
    {
        FString TaskId;
        Json->TryGetStringField(TEXT("task_id"), TaskId);
        return TaskId;
    }

    /** 从 JsonObject 提取 object_id */
    FString ExtractObjectId(const TSharedPtr<FJsonObject>& Json)
    {
        FString ObjectId;
        Json->TryGetStringField(TEXT("object_id"), ObjectId);
        return ObjectId;
    }
}

namespace
{
float ComputeHorizontalDistanceMeters(const AMACharacter* Agent, const FMAResolvedSkillTarget& Target)
{
    if (!Agent || !Target.bFound)
    {
        return -1.f;
    }

    const FVector TargetLocation = Target.Actor.IsValid() ? Target.Actor->GetActorLocation() : Target.Location;
    return FVector::Dist2D(Agent->GetActorLocation(), TargetLocation) / 100.f;
}
}

//=============================================================================
// 主处理入口
//=============================================================================

void FMASkillParamsProcessor::Process(AMACharacter* Agent, EMACommand Command, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent) return;
    
    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    if (!SkillComp) return;
    
    // 重置反馈上下文
    SkillComp->ResetFeedbackContext();
    SkillComp->ResetSkillRuntimeTargets();
    SkillComp->ResetSearchResults();
    
    // 设置 TaskId (从 RawParamsJson 提取)
    if (Cmd && !Cmd->Params.RawParamsJson.IsEmpty())
    {
        TSharedPtr<FJsonObject> ParamsJson;
        if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
        {
            FString TaskId = MAParamsHelper::ExtractTaskId(ParamsJson);
            if (!TaskId.IsEmpty())
            {
                SkillComp->GetFeedbackContextMutable().TaskId = TaskId;
            }
        }
    }
    
    // 根据指令类型分发处理
    switch (Command)
    {
        case EMACommand::Navigate:
            ProcessNavigate(SkillComp, Cmd);
            break;
        case EMACommand::Search:
            ProcessSearch(Agent, SkillComp, Cmd);
            break;
        case EMACommand::Follow:
            ProcessFollow(SkillComp, Cmd);
            break;
        case EMACommand::Charge:
            ProcessCharge(SkillComp, Cmd);
            break;
        case EMACommand::Place:
            ProcessPlace(Agent, SkillComp, Cmd);
            break;
        case EMACommand::TakeOff:
            ProcessTakeOff(SkillComp, Cmd);
            break;
        case EMACommand::Land:
            ProcessLand(SkillComp, Cmd);
            break;
        case EMACommand::ReturnHome:
            ProcessReturnHome(SkillComp, Cmd);
            break;
        case EMACommand::TakePhoto:
            ProcessTakePhoto(Agent, SkillComp, Cmd);
            break;
        case EMACommand::Broadcast:
            ProcessBroadcast(Agent, SkillComp, Cmd);
            break;
        case EMACommand::HandleHazard:
            ProcessHandleHazard(Agent, SkillComp, Cmd);
            break;
        case EMACommand::Guide:
            ProcessGuide(Agent, SkillComp, Cmd);
            break;
        default:
            break;
    }
}

void FMASkillParamsProcessor::ProcessNavigate(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Cmd) return;
    
    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    //=========================================================================
    // Step 1: 从 RawParamsJson 解析 dest 坐标
    //=========================================================================
    TSharedPtr<FJsonObject> ParamsJson;
    if (!MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessNavigate] %s: Failed to parse RawParamsJson"), *Agent->AgentLabel);
        return;
    }
    
    FVector TargetLocation;
    if (!MAParamsHelper::ExtractDestPosition(ParamsJson, TargetLocation))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessNavigate] %s: No dest position in params"), *Agent->AgentLabel);
        return;
    }
    
    const FVector OriginalTarget = TargetLocation;
    const bool bIsFlying = (Agent->AgentType == EMAAgentType::UAV || Agent->AgentType == EMAAgentType::FixedWingUAV);
    
    UWorld* World = Agent->GetWorld();
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Agent);
    
    //=========================================================================
    // Step 2: 将目标点从碰撞体内部推出
    //=========================================================================
    // 使用球体重叠检测，如果目标点在任何碰撞体内部，
    // 利用引擎穿透检测将目标点推出碰撞体
    // 策略：一次性收集所有重叠碰撞体，累加所有推出向量，一步到位
    // 地面机器人：仅在 XY 平面推出（推到障碍物旁边，而非顶部）
    // 飞行机器人：使用完整 3D 推出
    const float ProbeRadius = 50.f;
    const float PushMargin = 200.f;     // 碰撞体边界外的安全余量
    
    {
        TArray<FOverlapResult> Overlaps;
        if (World->OverlapMultiByChannel(
                Overlaps, TargetLocation, FQuat::Identity,
                ECC_WorldStatic, FCollisionShape::MakeSphere(ProbeRadius), QueryParams))
        {
            // 累加所有碰撞体的推出向量
            FVector AccumulatedPush = FVector::ZeroVector;
            
            for (const FOverlapResult& Overlap : Overlaps)
            {
                UPrimitiveComponent* Comp = Overlap.GetComponent();
                if (!Comp) continue;
                
                FMTDResult MTD;
                if (!Comp->ComputePenetration(MTD, FCollisionShape::MakeSphere(ProbeRadius), TargetLocation, FQuat::Identity))
                {
                    continue;
                }
                
                FVector Push = MTD.Direction * (MTD.Distance + PushMargin);
                
                if (!bIsFlying)
                {
                    Push.Z = 0.f;
                    if (Push.IsNearlyZero())
                    {
                        // MTD 几乎纯垂直，回退到从碰撞体中心向外推
                        FVector Away = TargetLocation - Comp->GetComponentLocation();
                        Away.Z = 0.f;
                        if (Away.IsNearlyZero())
                        {
                            Away = FVector(1.f, 0.f, 0.f);
                        }
                        Push = Away.GetSafeNormal() * (MTD.Distance + PushMargin);
                    }
                }
                
                AccumulatedPush += Push;
            }
            
            if (!AccumulatedPush.IsNearlyZero())
            {
                TargetLocation += AccumulatedPush;
                
                UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Pushed out of collider, offset (%.0f, %.0f, %.0f)"),
                    *Agent->AgentLabel, AccumulatedPush.X, AccumulatedPush.Y, AccumulatedPush.Z);
            }
        }
    }
    
    //=========================================================================
    // Step 3: 高度修正
    //=========================================================================
    if (bIsFlying)
    {
        // 飞行机器人：确保不低于最低飞行高度
        float MinAltitude = 800.f;
        if (UMANavigationService* NavService = Agent->GetNavigationService())
        {
            MinAltitude = NavService->MinFlightAltitude;
        }
        if (TargetLocation.Z < MinAltitude)
        {
            TargetLocation.Z = MinAltitude;
        }
    }
    else
    {
        // 地面机器人：射线检测地面高度
        // 使用 ECC_WorldStatic 通道 + 忽略所有 Pawn 类型 Actor
        FCollisionQueryParams GroundQuery;
        GroundQuery.AddIgnoredActor(Agent);
        // 收集场景中所有 Pawn 并忽略（车辆、行人等都继承自 ACharacter/APawn）
        TArray<AActor*> AllPawns;
        UGameplayStatics::GetAllActorsOfClass(World, APawn::StaticClass(), AllPawns);
        GroundQuery.AddIgnoredActors(AllPawns);
        
        FHitResult HitResult;
        FVector TraceStart(TargetLocation.X, TargetLocation.Y, 50000.f);
        FVector TraceEnd(TargetLocation.X, TargetLocation.Y, -10000.f);
        
        if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, GroundQuery))
        {
            TargetLocation.Z = HitResult.Location.Z;
        }
    }
    
    if (!TargetLocation.Equals(OriginalTarget, 1.f))
    {
        UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Adjusted (%.0f,%.0f,%.0f) -> (%.0f,%.0f,%.0f)"),
            *Agent->AgentLabel,
            OriginalTarget.X, OriginalTarget.Y, OriginalTarget.Z,
            TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
    }
    
    //=========================================================================
    // Step 4: 存储附近地标信息到反馈上下文（用于反馈生成）
    //=========================================================================
    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
    
    if (SceneGraphManager)
    {
        FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(TargetLocation, 2000.f);
        if (NearestLandmark.IsValid())
        {
            Context.NearbyLandmarkLabel = NearestLandmark.Label;
            Context.NearbyLandmarkType = NearestLandmark.Type;
            Context.NearbyLandmarkDistance = FVector::Dist(TargetLocation, NearestLandmark.Center);
            
            UE_LOG(LogTemp, Verbose, TEXT("[ProcessNavigate] %s: Nearest landmark: %s (%s) dist=%.0f"),
                *Agent->AgentLabel, *NearestLandmark.Label, *NearestLandmark.Type, Context.NearbyLandmarkDistance);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Final target (%.0f, %.0f, %.0f)"),
        *Agent->AgentLabel, TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
    
    Params.TargetLocation = TargetLocation;
}

void FMASkillParamsProcessor::ProcessSearch(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    //=========================================================================
    // Step 1: 从 RawParamsJson 解析参数
    //=========================================================================
    TSharedPtr<FJsonObject> ParamsJson;
    if (!MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessSearch] %s: Failed to parse RawParamsJson"), *Agent->AgentLabel);
        return;
    }
    
    // 解析搜索区域边界坐标
    if (MAParamsHelper::ExtractSearchArea(ParamsJson, Params.SearchBoundary))
    {
        // 根据区域面积计算自适应扫描宽度
        Params.SearchScanWidth = FMAGeometryUtils::CalculateAdaptiveScanWidth(Params.SearchBoundary, 200.f, 1600.f);
        
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: SearchArea has %d vertices, ScanWidth=%.0f"), 
            *Agent->AgentLabel, Params.SearchBoundary.Num(), Params.SearchScanWidth);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessSearch] %s: No SearchArea provided"), *Agent->AgentLabel);
    }
    
    // 解析目标语义标签
    FString TargetJsonStr;
    if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
    {
        FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Params.SearchTarget);
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Parsed target - Class=%s, Type=%s, Features=%d"),
            *Agent->AgentLabel, *Params.SearchTarget.Class, *Params.SearchTarget.Type, Params.SearchTarget.Features.Num());
    }
    
    // 解析 goal_type 参数 (搜索模式)
    FString GoalType;
    if (ParamsJson->TryGetStringField(TEXT("goal_type"), GoalType))
    {
        if (GoalType.Equals(TEXT("patrol"), ESearchCase::IgnoreCase))
        {
            Params.SearchMode = ESearchMode::Patrol;
        }
        else
        {
            Params.SearchMode = ESearchMode::Coverage;
        }
    }
    else
    {
        Params.SearchMode = ESearchMode::Coverage;  // 默认为覆盖搜索模式
    }
    
    // 解析 wait_time 参数 (巡逻模式等待时间)
    double WaitTime = 2.0;
    if (ParamsJson->TryGetNumberField(TEXT("wait_time"), WaitTime))
    {
        Params.PatrolWaitTime = static_cast<float>(WaitTime);
    }
    
    // 解析 patrol_cycles 参数 (巡逻模式循环次数限制)
    int32 PatrolCycles = 1;  // 默认1次
    if (ParamsJson->TryGetNumberField(TEXT("patrol_cycles"), WaitTime))  // 复用 WaitTime 变量
    {
        PatrolCycles = static_cast<int32>(WaitTime);
        if (PatrolCycles < 1) PatrolCycles = 1;  // 至少1次
    }
    Params.PatrolCycleLimit = PatrolCycles;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: SearchMode=%s, PatrolWaitTime=%.1f, PatrolCycleLimit=%d"),
        *Agent->AgentLabel, 
        Params.SearchMode == ESearchMode::Patrol ? TEXT("Patrol") : TEXT("Coverage"),
        Params.PatrolWaitTime,
        Params.PatrolCycleLimit);
    
    // 构建 FMASemanticLabel 用于场景查询
    FMASemanticLabel SearchLabel;
    SearchLabel.Class = Params.SearchTarget.Class;
    SearchLabel.Type = Params.SearchTarget.Type;
    SearchLabel.Features = Params.SearchTarget.Features;
    
    //=========================================================================
    // Step 2: 使用场景图查询匹配对象
    //=========================================================================
    UWorld* World = Agent->GetWorld();
    if (!World) return;
    
    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
    
    bool bFoundInSceneGraph = false;
    
    if (SceneGraphManager && Params.SearchBoundary.Num() >= 3)
    {
        // 使用场景图查询
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        TArray<FMASceneGraphNode> FoundNodes = FMASceneGraphQueryUseCases::FindNodesInBoundary(
            AllNodes, SearchLabel, Params.SearchBoundary);
        
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Scene graph query found %d nodes"),
            *Agent->AgentLabel, FoundNodes.Num());
        
        // 填充反馈上下文
        for (const FMASceneGraphNode& Node : FoundNodes)
        {
            // 使用 Label 或 Id 作为对象名称
            FString ObjectName = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            Context.FoundObjects.Add(ObjectName);
            Context.FoundLocations.Add(Node.Center);
            
            // 添加对象属性到 ObjectAttributes
            // 格式: "object_N_attr" -> "value"
            int32 Index = Context.FoundObjects.Num() - 1;
            
            // 添加类型信息
            if (!Node.Type.IsEmpty())
            {
                Context.ObjectAttributes.Add(FString::Printf(TEXT("object_%d_type"), Index), Node.Type);
            }
            
            // 添加类别信息
            if (!Node.Category.IsEmpty())
            {
                Context.ObjectAttributes.Add(FString::Printf(TEXT("object_%d_category"), Index), Node.Category);
            }
            
            // 添加特征信息
            for (const auto& Feature : Node.Features)
            {
                Context.ObjectAttributes.Add(
                    FString::Printf(TEXT("object_%d_%s"), Index, *Feature.Key), 
                    Feature.Value);
            }
            
            UE_LOG(LogTemp, Verbose, TEXT("[ProcessSearch] %s: Found object '%s' at (%.0f, %.0f, %.0f)"),
                *Agent->AgentLabel, *ObjectName, Node.Center.X, Node.Center.Y, Node.Center.Z);
        }
        
        bFoundInSceneGraph = FoundNodes.Num() > 0;
    }
    
    //=========================================================================
    // Step 4: 回退到 UE5 场景查询 (如果场景图未找到)
    //=========================================================================
    if (!bFoundInSceneGraph && Params.SearchBoundary.Num() >= 3)
    {
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Falling back to UE5 scene query"),
            *Agent->AgentLabel);
        
        // 转换为 FMAUESceneQuery 使用的 FMASemanticLabel
        FMASemanticLabel UE5Label;
        UE5Label.Class = SearchLabel.Class;
        UE5Label.Type = SearchLabel.Type;
        UE5Label.Features = SearchLabel.Features;
        
        TArray<FMAUESceneQueryResult> Results = FMAUESceneQuery::FindObjectsInBoundary(
            World, UE5Label, Params.SearchBoundary);
        
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: UE5 scene query found %d objects"),
            *Agent->AgentLabel, Results.Num());
        
        for (const FMAUESceneQueryResult& Result : Results)
        {
            Context.FoundObjects.Add(Result.Name);
            Context.FoundLocations.Add(Result.Location);
            
            // 添加从 UE5 查询获取的属性
            int32 Index = Context.FoundObjects.Num() - 1;
            if (!Result.Label.Type.IsEmpty())
            {
                Context.ObjectAttributes.Add(FString::Printf(TEXT("object_%d_type"), Index), Result.Label.Type);
            }
            for (const auto& Feature : Result.Label.Features)
            {
                Context.ObjectAttributes.Add(
                    FString::Printf(TEXT("object_%d_%s"), Index, *Feature.Key), 
                    Feature.Value);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Total found %d objects"),
        *Agent->AgentLabel, Context.FoundObjects.Num());
}

void FMASkillParamsProcessor::ProcessPlace(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASearchRuntimeResults& SearchResults = SkillComp->GetSearchResultsMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();
    SearchResults.Reset();
    
    //=========================================================================
    // Step 1: 从 RawParamsJson 解析 target/surface_target 语义标签
    //=========================================================================
    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        // 解析 target JSON
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Params.PlaceObject1);
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Parsed target - Class=%s, Type=%s"), 
                *Agent->AgentLabel, *Params.PlaceObject1.Class, *Params.PlaceObject1.Type);
        }
        
        // 解析 surface_target JSON
        FString SurfaceTargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("surface_target"), SurfaceTargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(SurfaceTargetJsonStr, Params.PlaceObject2);
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Parsed surface_target - Class=%s, Type=%s"), 
                *Agent->AgentLabel, *Params.PlaceObject2.Class, *Params.PlaceObject2.Type);
        }
    }
    
    //=========================================================================
    // Step 2: 确定 PlaceMode
    // - LoadToUGV: surface_target 是 UGV 机器人
    // - UnloadToGround: surface_target 是 ground
    // - StackOnObject: surface_target 是其他物体
    //=========================================================================
    EPlaceMode PlaceMode = FMASkillTargetResolver::DeterminePlaceMode(Params.PlaceObject2);
    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: PlaceMode=%d"), *Agent->AgentLabel, (int32)PlaceMode);
    
    //=========================================================================
    // Step 3: 获取场景图管理器
    //=========================================================================
    UWorld* World = Agent->GetWorld();
    if (!World) return;
    
    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
    
    TArray<FMASceneGraphNode> AllNodes;
    if (SceneGraphManager)
    {
        AllNodes = SceneGraphManager->GetAllNodes();
        
        // 调试: 统计动态节点数量
        int32 RobotCount = 0;
        int32 PickupItemCount = 0;
        for (const FMASceneGraphNode& Node : AllNodes)
        {
            if (Node.IsRobot()) RobotCount++;
            if (Node.IsPickupItem()) PickupItemCount++;
        }
        UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Scene graph has %d total nodes (%d robots, %d pickup items)"),
            *Agent->AgentLabel, AllNodes.Num(), RobotCount, PickupItemCount);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: SceneGraphManager is null!"), *Agent->AgentLabel);
    }
    
    //=========================================================================
    // Step 4: 查找 target
    // 使用 MASceneGraphQuery 替换 FMAUESceneQuery
    //=========================================================================
    FMASemanticLabel Label1;
    Label1.Class = Params.PlaceObject1.Class;
    Label1.Type = Params.PlaceObject1.Type;
    Label1.Features = Params.PlaceObject1.Features;
    
    if (Label1.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: target semantic label is empty"), *Agent->AgentLabel);
    }
    else
    {
        bool bFoundInSceneGraph = false;
        
        // 首先尝试使用场景图查询
        if (SceneGraphManager && AllNodes.Num() > 0)
        {
            FMASceneGraphNode Object1Node = FMASceneGraphQueryUseCases::FindNodeByLabel(AllNodes, Label1);
            
            if (Object1Node.IsValid() && Object1Node.IsPickupItem())
            {
                Context.PlacedObjectName = Object1Node.Label.IsEmpty() ? Object1Node.Id : Object1Node.Label;
                SearchResults.Object1Location = Object1Node.Center;
                
                // 存储场景图节点ID到Context，用于后续状态更新
                Context.ObjectAttributes.Add(TEXT("object1_node_id"), Object1Node.Id);
                
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found target '%s' in scene graph at (%.0f, %.0f, %.0f)"), 
                    *Agent->AgentLabel, *Context.PlacedObjectName, 
                    Object1Node.Center.X, Object1Node.Center.Y, Object1Node.Center.Z);
                
                // 使用 UE5 场景查询获取实际的 Actor 引用
                // 场景图只提供位置信息，技能执行需要 Actor 引用
                FMAUESceneQueryResult ActorResult = FMAUESceneQuery::FindObjectByLabel(World, Label1);
                if (ActorResult.bFound && ActorResult.Actor)
                {
                    SearchResults.Object1Actor = ActorResult.Actor;
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Resolved Actor for target '%s'"), 
                        *Agent->AgentLabel, *Context.PlacedObjectName);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: Scene graph found target but UE5 Actor not found!"), 
                        *Agent->AgentLabel);
                }
                
                bFoundInSceneGraph = true;
            }
        }
        
        // 回退到 UE5 场景查询
        if (!bFoundInSceneGraph)
        {
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Falling back to UE5 scene query for target"), *Agent->AgentLabel);
            
            FMAUESceneQueryResult Result1 = FMAUESceneQuery::FindObjectByLabel(World, Label1);
            if (Result1.bFound)
            {
                Context.PlacedObjectName = Result1.Name;
                SearchResults.Object1Actor = Result1.Actor;
                SearchResults.Object1Location = Result1.Location;
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found target '%s' via UE5 query at (%.0f, %.0f, %.0f)"), 
                    *Agent->AgentLabel, *Result1.Name, Result1.Location.X, Result1.Location.Y, Result1.Location.Z);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: target not found in scene"), *Agent->AgentLabel);
            }
        }
    }
    
    //=========================================================================
    // Step 5: 查找 surface_target
    // 使用 MASceneGraphQuery 替换 FMAUESceneQuery
    //=========================================================================
    FMASemanticLabel Label2;
    Label2.Class = Params.PlaceObject2.Class;
    Label2.Type = Params.PlaceObject2.Type;
    Label2.Features = Params.PlaceObject2.Features;
    
    switch (PlaceMode)
    {
        case EPlaceMode::UnloadToGround:
        {
            // 卸货到地面：使用 Humanoid 当前位置作为放置位置
            Context.PlaceTargetName = TEXT("ground");
            SearchResults.Object2Location = Agent->GetActorLocation();
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Target is ground at (%.0f, %.0f, %.0f)"), 
                *Agent->AgentLabel, SearchResults.Object2Location.X, SearchResults.Object2Location.Y, SearchResults.Object2Location.Z);
            break;
        }
        
        case EPlaceMode::LoadToUGV:
        {
            // 装货到 UGV：根据 label 特征查找 UGV
            FString TargetRobotLabel = Label2.Features.Contains(TEXT("label")) ? Label2.Features[TEXT("label")] : TEXT("");
            bool bFoundInSceneGraph = false;
            
            if (SceneGraphManager && AllNodes.Num() > 0 && !TargetRobotLabel.IsEmpty())
            {
                // 使用场景图查询机器人
                FMASemanticLabel RobotSemanticLabel = FMASemanticLabel::MakeRobot(TargetRobotLabel);
                FMASceneGraphNode RobotNode = FMASceneGraphQueryUseCases::FindNodeByLabel(AllNodes, RobotSemanticLabel);
                
                if (RobotNode.IsValid() && RobotNode.IsRobot())
                {
                    Context.PlaceTargetName = RobotNode.Label.IsEmpty() ? RobotNode.Id : RobotNode.Label;
                    SearchResults.Object2Location = RobotNode.Center;
                    
                    // 存储场景图节点ID到Context
                    Context.ObjectAttributes.Add(TEXT("object2_node_id"), RobotNode.Id);
                    
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found UGV '%s' in scene graph at (%.0f, %.0f, %.0f)"), 
                        *Agent->AgentLabel, *Context.PlaceTargetName, 
                        RobotNode.Center.X, RobotNode.Center.Y, RobotNode.Center.Z);
                    
                    // 使用 UE5 场景查询获取实际的 Actor 引用
                    AMACharacter* Robot = FMAUESceneQuery::FindRobotByLabel(World, TargetRobotLabel);
                    if (Robot)
                    {
                        SearchResults.Object2Actor = Robot;
                        UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Resolved Actor for UGV '%s'"), 
                            *Agent->AgentLabel, *TargetRobotLabel);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: Scene graph found UGV but UE5 Actor not found!"), 
                            *Agent->AgentLabel);
                    }
                    
                    bFoundInSceneGraph = true;
                }
            }
            
            // 回退到 UE5 场景查询
            if (!bFoundInSceneGraph)
            {
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Falling back to UE5 scene query for UGV '%s'"), 
                    *Agent->AgentLabel, *TargetRobotLabel);
                
                AMACharacter* Robot = FMAUESceneQuery::FindRobotByLabel(World, TargetRobotLabel);
                if (Robot)
                {
                    Context.PlaceTargetName = Robot->AgentLabel;
                    SearchResults.Object2Actor = Robot;
                    SearchResults.Object2Location = Robot->GetActorLocation();
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found UGV '%s' via UE5 query at (%.0f, %.0f, %.0f)"), 
                        *Agent->AgentLabel, *Robot->AgentLabel, 
                        SearchResults.Object2Location.X, SearchResults.Object2Location.Y, SearchResults.Object2Location.Z);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: UGV '%s' not found"), *Agent->AgentLabel, *TargetRobotLabel);
                }
            }
            break;
        }
        
        case EPlaceMode::StackOnObject:
        {
            // 堆叠到另一个物体：查找匹配物体
            bool bFoundInSceneGraph = false;
            
            if (SceneGraphManager && AllNodes.Num() > 0)
            {
                FMASceneGraphNode Object2Node = FMASceneGraphQueryUseCases::FindNodeByLabel(AllNodes, Label2);
                
                if (Object2Node.IsValid() && Object2Node.IsPickupItem())
                {
                    Context.PlaceTargetName = Object2Node.Label.IsEmpty() ? Object2Node.Id : Object2Node.Label;
                    SearchResults.Object2Location = Object2Node.Center;
                    
                    // 存储场景图节点ID到Context
                    Context.ObjectAttributes.Add(TEXT("object2_node_id"), Object2Node.Id);
                    
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found surface_target '%s' in scene graph at (%.0f, %.0f, %.0f)"), 
                        *Agent->AgentLabel, *Context.PlaceTargetName, 
                        Object2Node.Center.X, Object2Node.Center.Y, Object2Node.Center.Z);
                    
                    // 使用 UE5 场景查询获取实际的 Actor 引用
                    FMAUESceneQueryResult ActorResult = FMAUESceneQuery::FindObjectByLabel(World, Label2);
                    if (ActorResult.bFound && ActorResult.Actor)
                    {
                        SearchResults.Object2Actor = ActorResult.Actor;
                        UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Resolved Actor for surface_target '%s'"), 
                            *Agent->AgentLabel, *Context.PlaceTargetName);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: Scene graph found surface_target but UE5 Actor not found!"), 
                            *Agent->AgentLabel);
                    }
                    
                    bFoundInSceneGraph = true;
                }
            }
            
            // 回退到 UE5 场景查询
            if (!bFoundInSceneGraph)
            {
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Falling back to UE5 scene query for surface_target"), *Agent->AgentLabel);
                
                FMAUESceneQueryResult Result2 = FMAUESceneQuery::FindObjectByLabel(World, Label2);
                if (Result2.bFound)
                {
                    Context.PlaceTargetName = Result2.Name;
                    SearchResults.Object2Actor = Result2.Actor;
                    SearchResults.Object2Location = Result2.Location;
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found surface_target '%s' via UE5 query at (%.0f, %.0f, %.0f)"), 
                        *Agent->AgentLabel, *Result2.Name, Result2.Location.X, Result2.Location.Y, Result2.Location.Z);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: surface_target not found in scene"), *Agent->AgentLabel);
                }
            }
            break;
        }
    }
}

void FMASkillParamsProcessor::ProcessTakeOff(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp) return;
    
    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    // 使用 SkillParams 中配置的默认起飞高度（默认 2200cm = 22m）
    float RequestedHeight = Params.TakeOffHeight;
    
    // 从 RawParamsJson 解析高度参数
    if (Cmd)
    {
        TSharedPtr<FJsonObject> ParamsJson;
        if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
        {
            FVector DestPosition;
            if (MAParamsHelper::ExtractDestPosition(ParamsJson, DestPosition) && DestPosition.Z > 0.f)
            {
                RequestedHeight = DestPosition.Z;
            }
            
            // 也支持直接的 height 字段
            double Height = 0.0;
            if (ParamsJson->TryGetNumberField(TEXT("height"), Height) && Height > 0.0)
            {
                RequestedHeight = static_cast<float>(Height);
            }
        }
    }
    
    //=========================================================================
    // Step 1: 使用场景图验证目标高度
    // 确保高度高于附近建筑物
    //=========================================================================
    UWorld* World = Agent->GetWorld();
    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
    
    const float SafetyMargin = 500.f;  // 安全距离 5m
    const float SearchRadius = 3000.f;  // 搜索半径 30m
    float MinSafeHeight = RequestedHeight;
    float MaxBuildingHeight = 0.f;
    FString TallestBuildingLabel;
    
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        TArray<FMASceneGraphNode> Buildings = FMASceneGraphQueryUseCases::GetAllBuildings(AllNodes);
        
        FVector AgentLocation = Agent->GetActorLocation();
        
        // 查找附近建筑物的最高点
        for (const FMASceneGraphNode& Building : Buildings)
        {
            // 计算到建筑物中心的距离
            float Distance = FVector::Dist2D(AgentLocation, Building.Center);
            
            if (Distance <= SearchRadius)
            {
                // 从建筑物节点提取高度信息
                float BuildingHeight = 0.f;
                
                // 尝试从 RawJson 中提取高度
                if (!Building.RawJson.IsEmpty())
                {
                    TSharedPtr<FJsonObject> JsonObject;
                    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Building.RawJson);
                    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                    {
                        const TSharedPtr<FJsonObject>* PropertiesObject;
                        if (JsonObject->TryGetObjectField(TEXT("properties"), PropertiesObject))
                        {
                            // 尝试获取 height 字段
                            double Height = 0.0;
                            if ((*PropertiesObject)->TryGetNumberField(TEXT("height"), Height))
                            {
                                BuildingHeight = static_cast<float>(Height);
                            }
                        }
                        
                        // 如果没有 height 字段，尝试从 shape 中估算
                        if (BuildingHeight <= 0.f)
                        {
                            const TSharedPtr<FJsonObject>* ShapeObject;
                            if (JsonObject->TryGetObjectField(TEXT("shape"), ShapeObject))
                            {
                                // 检查是否有 z_max 或 height 字段
                                double ZMax = 0.0;
                                if ((*ShapeObject)->TryGetNumberField(TEXT("z_max"), ZMax))
                                {
                                    BuildingHeight = static_cast<float>(ZMax);
                                }
                            }
                        }
                    }
                }
                
                // 如果仍然没有高度信息，使用默认估算值
                if (BuildingHeight <= 0.f)
                {
                    // 默认建筑物高度估算为 1000cm (10m)
                    BuildingHeight = 1000.f;
                }
                
                // 更新最高建筑物
                if (BuildingHeight > MaxBuildingHeight)
                {
                    MaxBuildingHeight = BuildingHeight;
                    TallestBuildingLabel = Building.Label;
                }
            }
        }
        
        // 计算最小安全高度
        if (MaxBuildingHeight > 0.f)
        {
            MinSafeHeight = MaxBuildingHeight + SafetyMargin;
            
            UE_LOG(LogTemp, Log, TEXT("[ProcessTakeOff] %s: Nearby building '%s' height=%.0f, min safe height=%.0f"),
                *Agent->AgentLabel, *TallestBuildingLabel, MaxBuildingHeight, MinSafeHeight);
        }
    }
    
    //=========================================================================
    // Step 2: 调整目标高度（如果需要）
    //=========================================================================
    float FinalHeight = RequestedHeight;
    bool bHeightAdjusted = false;
    
    if (RequestedHeight < MinSafeHeight)
    {
        FinalHeight = MinSafeHeight;
        bHeightAdjusted = true;
        
        UE_LOG(LogTemp, Log, TEXT("[ProcessTakeOff] %s: Adjusted height from %.0f to %.0f (above building '%s')"),
            *Agent->AgentLabel, RequestedHeight, FinalHeight, *TallestBuildingLabel);
    }
    
    //=========================================================================
    // Step 3: 存储到参数和反馈上下文
    //=========================================================================
    Params.TakeOffHeight = FinalHeight;
    
    // 存储到反馈上下文
    Context.TakeOffTargetHeight = FinalHeight;
    Context.TakeOffMinSafeHeight = MinSafeHeight;
    Context.TakeOffNearbyBuildingLabel = TallestBuildingLabel;
    Context.TakeOffNearbyBuildingHeight = MaxBuildingHeight;
    Context.bTakeOffHeightAdjusted = bHeightAdjusted;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessTakeOff] %s: Final takeoff height=%.0f, adjusted=%s"),
        *Agent->AgentLabel, FinalHeight, bHeightAdjusted ? TEXT("true") : TEXT("false"));
}

void FMASkillParamsProcessor::ProcessLand(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp) return;
    
    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    UWorld* World = Agent->GetWorld();
    FVector AgentLocation = Agent->GetActorLocation();
    FVector LandLocation = FVector(AgentLocation.X, AgentLocation.Y, 0.f);
    float LandHeight = 0.f;
    FString GroundType = TEXT("unknown");
    FString NearbyLandmark;
    bool bLocationSafe = true;
    
    //=========================================================================
    // Step 1: 使用场景图查找安全着陆区
    //=========================================================================
    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
    
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        
        // 检查当前位置是否在建筑物内
        if (SceneGraphManager->IsPointInsideBuilding(LandLocation))
        {
            bLocationSafe = false;
            GroundType = TEXT("building_interior");
            
            UE_LOG(LogTemp, Warning, TEXT("[ProcessLand] %s: Current position is inside a building, searching for safe landing zone"),
                *Agent->AgentLabel);
            
            // 查找最近的安全着陆区（道路或路口）
            TArray<FMASceneGraphNode> Roads = FMASceneGraphQueryUseCases::GetAllRoads(AllNodes);
            TArray<FMASceneGraphNode> Intersections = FMASceneGraphQueryUseCases::GetAllIntersections(AllNodes);
            
            float MinDistance = TNumericLimits<float>::Max();
            FMASceneGraphNode NearestSafeNode;
            
            // 检查道路
            for (const FMASceneGraphNode& Road : Roads)
            {
                float Distance = FVector::Dist2D(AgentLocation, Road.Center);
                if (Distance < MinDistance)
                {
                    MinDistance = Distance;
                    NearestSafeNode = Road;
                }
            }
            
            // 检查路口
            for (const FMASceneGraphNode& Intersection : Intersections)
            {
                float Distance = FVector::Dist2D(AgentLocation, Intersection.Center);
                if (Distance < MinDistance)
                {
                    MinDistance = Distance;
                    NearestSafeNode = Intersection;
                }
            }
            
            // 如果找到安全着陆区，更新着陆位置
            if (NearestSafeNode.IsValid())
            {
                LandLocation = NearestSafeNode.Center;
                bLocationSafe = true;
                
                if (NearestSafeNode.IsRoad())
                {
                    GroundType = TEXT("road");
                }
                else if (NearestSafeNode.IsIntersection())
                {
                    GroundType = TEXT("intersection");
                }
                
                NearbyLandmark = NearestSafeNode.Label;
                
                UE_LOG(LogTemp, Log, TEXT("[ProcessLand] %s: Found safe landing zone at %s (%s), distance=%.0f"),
                    *Agent->AgentLabel, *NearestSafeNode.Label, *GroundType, MinDistance);
            }
        }
        else
        {
            // 当前位置安全，确定地面类型
            // 检查是否在道路上
            TArray<FMASceneGraphNode> Roads = FMASceneGraphQueryUseCases::GetAllRoads(AllNodes);
            for (const FMASceneGraphNode& Road : Roads)
            {
                // 简单检查：如果距离道路中心较近，认为在道路上
                float Distance = FVector::Dist2D(AgentLocation, Road.Center);
                if (Distance < 500.f)  // 5m 范围内
                {
                    GroundType = TEXT("road");
                    NearbyLandmark = Road.Label;
                    break;
                }
            }
            
            // 检查是否在路口
            if (GroundType == TEXT("unknown"))
            {
                TArray<FMASceneGraphNode> Intersections = FMASceneGraphQueryUseCases::GetAllIntersections(AllNodes);
                for (const FMASceneGraphNode& Intersection : Intersections)
                {
                    float Distance = FVector::Dist2D(AgentLocation, Intersection.Center);
                    if (Distance < 500.f)
                    {
                        GroundType = TEXT("intersection");
                        NearbyLandmark = Intersection.Label;
                        break;
                    }
                }
            }
            
            // 如果仍然未知，查找最近的地标
            if (GroundType == TEXT("unknown"))
            {
                FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(LandLocation, 2000.f);
                if (NearestLandmark.IsValid())
                {
                    NearbyLandmark = NearestLandmark.Label;
                    GroundType = TEXT("open_area");
                }
            }
        }
    }
    
    //=========================================================================
    // Step 2: 从 RawParamsJson 解析着陆位置
    //=========================================================================
    bool bHasDestPosition = false;
    FVector DestPosition;
    
    if (Cmd)
    {
        TSharedPtr<FJsonObject> ParamsJson;
        if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
        {
            if (MAParamsHelper::ExtractDestPosition(ParamsJson, DestPosition))
            {
                bHasDestPosition = true;
            }
        }
    }
    
    if (bHasDestPosition)
    {
        LandHeight = DestPosition.Z;
        LandLocation = FVector(DestPosition.X, DestPosition.Y, LandHeight);
    }
    else
    {
        // 射线检测当前位置下方的地面高度
        FVector Start = FVector(LandLocation.X, LandLocation.Y, AgentLocation.Z);
        FVector End = FVector(LandLocation.X, LandLocation.Y, -10000.f);
        
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Agent);
        
        if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
        {
            LandHeight = HitResult.Location.Z;
        }
    }
    
    //=========================================================================
    // Step 3: 存储到参数和反馈上下文
    //=========================================================================
    Params.LandHeight = LandHeight;
    
    // 存储到反馈上下文
    Context.LandTargetLocation = FVector(LandLocation.X, LandLocation.Y, LandHeight);
    Context.LandGroundType = GroundType;
    Context.LandNearbyLandmarkLabel = NearbyLandmark;
    Context.bLandLocationSafe = bLocationSafe;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessLand] %s: Land location=(%.0f, %.0f, %.0f), ground_type=%s, safe=%s"),
        *Agent->AgentLabel, LandLocation.X, LandLocation.Y, LandHeight, *GroundType, bLocationSafe ? TEXT("true") : TEXT("false"));
}

void FMASkillParamsProcessor::ProcessReturnHome(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp) return;
    
    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    UWorld* World = Agent->GetWorld();
    FVector HomeLocation = Agent->InitialLocation;
    FString HomeLandmark;
    
    //=========================================================================
    // Step 1: 从 RawParamsJson 解析 HomeLocation
    //=========================================================================
    if (Cmd)
    {
        TSharedPtr<FJsonObject> ParamsJson;
        if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
        {
            FVector DestPosition;
            if (MAParamsHelper::ExtractDestPosition(ParamsJson, DestPosition))
            {
                HomeLocation = DestPosition;
            }
        }
    }
    
    //=========================================================================
    // Step 2: 查找家位置附近的地标
    //=========================================================================
    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
    
    if (SceneGraphManager)
    {
        FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(HomeLocation, 2000.f);
        if (NearestLandmark.IsValid())
        {
            HomeLandmark = NearestLandmark.Label;
            
            UE_LOG(LogTemp, Verbose, TEXT("[ProcessReturnHome] %s: Home location near landmark '%s'"),
                *Agent->AgentLabel, *HomeLandmark);
        }
    }
    
    //=========================================================================
    // Step 3: 射线检测 HomeLocation 下方的地面高度
    //=========================================================================
    float LandHeight = 50.f;  // 默认 50cm
    
    // 从高空开始向下检测，确保能检测到地面
    FVector Start = FVector(HomeLocation.X, HomeLocation.Y, 50000.f);
    FVector End = FVector(HomeLocation.X, HomeLocation.Y, -10000.f);
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Agent);
    
    if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
    {
        // 地面高度 + 50cm 偏移（避免穿模）
        LandHeight = HitResult.Location.Z + 50.f;
    }
    
    //=========================================================================
    // Step 4: 存储到参数和反馈上下文
    //=========================================================================
    Params.HomeLocation = HomeLocation;
    Params.LandHeight = LandHeight;
    
    // 存储到反馈上下文
    Context.HomeLocationFromSceneGraph = HomeLocation;
    Context.HomeLandmarkLabel = HomeLandmark;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessReturnHome] %s: Home location=(%.0f, %.0f, %.0f), land_height=%.0f, landmark=%s"),
        *Agent->AgentLabel, HomeLocation.X, HomeLocation.Y, HomeLocation.Z, LandHeight, *HomeLandmark);
}


void FMASkillParamsProcessor::ProcessTakePhoto(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();
    
    //=========================================================================
    // Step 1: 从 RawParamsJson 解析目标参数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    
    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);
        
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: object_id='%s', target(Class=%s, Type=%s)"),
        *Agent->AgentLabel, *ObjectId, *Target.Class, *Target.Type);
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> ObjectId -> 场景图语义标签
    //=========================================================================
    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessTakePhoto"));
    
    //=========================================================================
    // Step 3: 计算机器人与目标的水平距离（米）
    //=========================================================================
    const float RobotTargetDistanceM = ComputeHorizontalDistanceMeters(Agent, ResolvedTarget);
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;
    
    //=========================================================================
    // Step 4: 保存到参数和反馈上下文
    //=========================================================================
    Params.CommonTargetObjectId = ResolvedTarget.Id;
    Params.CommonTarget = Target;
    RuntimeTargets.PhotoTargetActor = ResolvedTarget.Actor;
    
    // 大范围目标（intersection/building/area）使用更大的 FOV
    static const TSet<FString> WideViewTypes = { TEXT("intersection"), TEXT("building"), TEXT("area") };
    if (WideViewTypes.Contains(Target.Type.ToLower()))
    {
        Params.PhotoFOVOverride = 110.f;
    }
    
    Context.bPhotoTargetFound = ResolvedTarget.bFound;
    Context.PhotoTargetName = ResolvedTarget.Name;
    Context.PhotoTargetId = ResolvedTarget.Id;
    Context.TargetLocation = TargetLocation;
    Context.PhotoRobotTargetDistance = RobotTargetDistanceM;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: Target found=%s, name='%s', id='%s', distance=%.2fm, Actor=%s"),
        *Agent->AgentLabel, 
        Context.bPhotoTargetFound ? TEXT("true") : TEXT("false"), 
        *ResolvedTarget.Name, *ResolvedTarget.Id, RobotTargetDistanceM,
        ResolvedTarget.Actor.IsValid() ? TEXT("valid") : TEXT("null"));
}

void FMASkillParamsProcessor::ProcessBroadcast(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();
    
    //=========================================================================
    // Step 1: 从 RawParamsJson 解析目标参数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    FString Message;
    
    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);
        
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
        
        ParamsJson->TryGetStringField(TEXT("message"), Message);
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: object_id='%s', target(Class=%s, Type=%s), message='%s'"),
        *Agent->AgentLabel, *ObjectId, *Target.Class, *Target.Type, *Message);
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> ObjectId -> 场景图语义标签
    //=========================================================================
    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessBroadcast"));
    
    //=========================================================================
    // Step 3: 计算机器人与目标的水平距离（米）
    //=========================================================================
    const float RobotTargetDistanceM = ComputeHorizontalDistanceMeters(Agent, ResolvedTarget);
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;
    
    //=========================================================================
    // Step 4: 保存到参数和反馈上下文
    //=========================================================================
    Params.CommonTargetObjectId = ResolvedTarget.Id;
    Params.CommonTarget = Target;
    Params.BroadcastMessage = Message;
    RuntimeTargets.BroadcastTargetActor = ResolvedTarget.Actor;
    
    Context.bBroadcastTargetFound = ResolvedTarget.bFound;
    Context.BroadcastTargetName = ResolvedTarget.Name;
    Context.BroadcastMessage = Message;
    Context.TargetLocation = TargetLocation;
    Context.BroadcastRobotTargetDistance = RobotTargetDistanceM;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: Target found=%s, name='%s', message='%s', distance=%.2fm, Actor=%s"),
        *Agent->AgentLabel, 
        Context.bBroadcastTargetFound ? TEXT("true") : TEXT("false"), 
        *ResolvedTarget.Name, *Message, RobotTargetDistanceM,
        ResolvedTarget.Actor.IsValid() ? TEXT("valid") : TEXT("null"));
}

void FMASkillParamsProcessor::ProcessHandleHazard(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();
    
    //=========================================================================
    // Step 1: 解析参数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    
    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
    }
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> GUID -> 场景图节点
    //=========================================================================
    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessHandleHazard"));
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;
    
    //=========================================================================
    // Step 3: 保存结果
    //=========================================================================
    Params.CommonTargetObjectId = ResolvedTarget.Id;
    Params.CommonTarget = Target;
    RuntimeTargets.HazardTargetActor = ResolvedTarget.Actor;
    
    Context.bHazardTargetFound = ResolvedTarget.bFound;
    Context.HazardTargetName = ResolvedTarget.Name;
    Context.HazardTargetId = ResolvedTarget.Id;
    Context.TargetLocation = TargetLocation;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessHandleHazard] %s: Target found=%s, name='%s', Actor=%s"),
        *Agent->AgentLabel, Context.bHazardTargetFound ? TEXT("true") : TEXT("false"), 
        *ResolvedTarget.Name, ResolvedTarget.Actor.IsValid() ? TEXT("valid") : TEXT("null"));
}

void FMASkillParamsProcessor::ProcessGuide(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();
    
    //=========================================================================
    // Step 1: 解析参数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    FVector Destination = FVector::ZeroVector;
    
    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
        MAParamsHelper::ExtractDestPosition(ParamsJson, Destination);
    }
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> ObjectId -> 场景图语义标签
    //=========================================================================
    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessGuide"));
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;
    
    //=========================================================================
    // Step 3: 保存结果
    //=========================================================================
    Context.bGuideTargetFound = ResolvedTarget.Actor.IsValid();
    Context.GuideTargetName = ResolvedTarget.Name;
    Context.GuideTargetId = ResolvedTarget.Id;
    Context.GuideDestination = Destination;
    Context.TargetLocation = TargetLocation;
    
    Params.GuideTargetObjectId = ResolvedTarget.Id;
    Params.GuideTarget = Target;
    Params.GuideDestination = Destination;
    RuntimeTargets.GuideTargetActor = ResolvedTarget.Actor;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessGuide] %s: Target found=%s, name='%s', destination=(%.0f, %.0f, %.0f)"),
        *Agent->AgentLabel, ResolvedTarget.Actor.IsValid() ? TEXT("true") : TEXT("false"), *ResolvedTarget.Name,
        Destination.X, Destination.Y, Destination.Z);
}

void FMASkillParamsProcessor::ProcessFollow(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp || !Cmd) return;
    
    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;
    
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();
    
    //=========================================================================
    // Step 1: 解析参数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    
    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
    }
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> ObjectId -> 场景图语义标签
    //=========================================================================
    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessFollow"));
    AActor* TargetActor = ResolvedTarget.Actor.Get();
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;
    
    //=========================================================================
    // Step 3: 保存结果
    //=========================================================================
    Context.bFollowTargetFound = (TargetActor != nullptr);
    Context.FollowTargetName = ResolvedTarget.Name;
    Context.FollowTargetId = ObjectId;
    Context.TargetName = ResolvedTarget.Name;
    Context.TargetLocation = TargetLocation;
    
    if (TargetActor)
    {
        RuntimeTargets.FollowTarget = TargetActor;
        Context.FollowTargetDistance = FVector::Dist(Agent->GetActorLocation(), TargetActor->GetActorLocation());
        UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Target '%s' at distance %.0f"),
            *Agent->AgentLabel, *ResolvedTarget.Name, Context.FollowTargetDistance);
    }
    else
    {
        Context.FollowErrorReason = TEXT("Target Actor not found");
        UE_LOG(LogTemp, Warning, TEXT("[ProcessFollow] %s: Target not found (ObjectId=%s, Class=%s)"),
            *Agent->AgentLabel, *ObjectId, *Target.Class);
    }
}

void FMASkillParamsProcessor::ProcessCharge(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp) return;
    
    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    // 记录充电前的能量
    Context.EnergyBefore = SkillComp->GetEnergyPercent();
    
    //=========================================================================
    // Step 1: 使用场景图查询查找最近的充电站
    //=========================================================================
    UWorld* World = Agent->GetWorld();
    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
    
    bool bFoundInSceneGraph = false;
    
    if (SceneGraphManager)
    {
        // 构建充电站语义标签
        FMASemanticLabel ChargingStationLabel = FMASemanticLabel::MakeChargingStation();
        
        // 获取所有节点并查找最近的充电站
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode NearestStation = FMASceneGraphQueryUseCases::FindNearestNode(
            AllNodes, ChargingStationLabel, Agent->GetActorLocation());
        
        if (NearestStation.IsValid() && NearestStation.IsChargingStation())
        {
            // 找到充电站，缓存位置和ID
            Params.ChargingStationLocation = NearestStation.Center;
            Params.ChargingStationId = NearestStation.Id;
            Params.bChargingStationFound = true;
            
            // 同步到反馈上下文
            Context.ChargingStationId = NearestStation.Label.IsEmpty() ? NearestStation.Id : NearestStation.Label;
            Context.ChargingStationLocation = NearestStation.Center;
            Context.bChargingStationFound = true;
            
            float Distance = FVector::Dist(Agent->GetActorLocation(), NearestStation.Center);
            
            UE_LOG(LogTemp, Log, TEXT("[ProcessCharge] %s: Found charging station '%s' in scene graph at (%.0f, %.0f, %.0f), distance %.0f"),
                *Agent->AgentLabel, *Context.ChargingStationId, 
                NearestStation.Center.X, NearestStation.Center.Y, NearestStation.Center.Z, Distance);
            
            bFoundInSceneGraph = true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[ProcessCharge] %s: No charging station found in scene graph"),
                *Agent->AgentLabel);
        }
    }
    
    //=========================================================================
    // Step 2: 回退到 UE5 场景查询 (如果场景图未找到)
    //=========================================================================
    if (!bFoundInSceneGraph)
    {
        UE_LOG(LogTemp, Log, TEXT("[ProcessCharge] %s: Falling back to UE5 scene query for charging station"),
            *Agent->AgentLabel);
        
        // 使用 UE5 场景查询查找充电站 Actor
        // 查找所有 AMAChargingStation 类型的 Actor
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(World, AMAChargingStation::StaticClass(), FoundActors);
        
        if (FoundActors.Num() > 0)
        {
            // 找到最近的充电站
            AActor* NearestActor = nullptr;
            float MinDistanceSq = TNumericLimits<float>::Max();
            
            for (AActor* Actor : FoundActors)
            {
                float DistanceSq = FVector::DistSquared(Agent->GetActorLocation(), Actor->GetActorLocation());
                if (DistanceSq < MinDistanceSq)
                {
                    MinDistanceSq = DistanceSq;
                    NearestActor = Actor;
                }
            }
            
            if (NearestActor)
            {
                Params.ChargingStationLocation = NearestActor->GetActorLocation();
                Params.ChargingStationId = NearestActor->GetName();
                Params.bChargingStationFound = true;
                
                Context.ChargingStationId = NearestActor->GetName();
                Context.ChargingStationLocation = NearestActor->GetActorLocation();
                Context.bChargingStationFound = true;
                
                UE_LOG(LogTemp, Log, TEXT("[ProcessCharge] %s: Found charging station '%s' via UE5 query at (%.0f, %.0f, %.0f)"),
                    *Agent->AgentLabel, *Context.ChargingStationId,
                    Params.ChargingStationLocation.X, Params.ChargingStationLocation.Y, Params.ChargingStationLocation.Z);
            }
        }
    }
    
    //=========================================================================
    // Step 3: 处理未找到充电站的情况
    //=========================================================================
    if (!Params.bChargingStationFound)
    {
        Context.bChargingStationFound = false;
        Context.ChargeErrorReason = TEXT("no charging station available");
        
        UE_LOG(LogTemp, Warning, TEXT("[ProcessCharge] %s: %s"),
            *Agent->AgentLabel, *Context.ChargeErrorReason);
    }
}
