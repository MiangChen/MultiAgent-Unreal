// MASkillParamsProcessor.cpp
// 技能参数处理器实现

#include "MASkillParamsProcessor.h"
#include "MAUESceneQuery.h"
#include "../MASkillComponent.h"
#include "../Impl/SK_Place.h"  // For EPlaceMode definition
#include "../../Character/MACharacter.h"
#include "../../../Core/Comm/MACommTypes.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Core/Manager/scene_graph_tools/MASceneGraphQuery.h"
#include "../../../Utils/MAGeometryUtils.h"
#include "../../../Environment/MAChargingStation.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

//=============================================================================
// 辅助方法实现
//=============================================================================

bool FMASkillParamsProcessor::MatchTargetObject(
    AMACharacter* Agent,
    const FString& ObjectId,
    const FMASemanticTarget& SemanticTarget,
    float SearchRadius,
    FString& OutFoundId,
    FString& OutFoundName,
    FVector& OutFoundLocation)
{
    if (!Agent || !Agent->GetWorld())
    {
        return false;
    }
    
    UWorld* World = Agent->GetWorld();
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    bool bFoundInSceneGraph = false;
    
    //=========================================================================
    // 优先级1：直接 object_id 匹配
    //=========================================================================
    if (!ObjectId.IsEmpty() && SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode TargetNode = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, ObjectId);
        
        if (TargetNode.IsValid())
        {
            OutFoundId = TargetNode.Id;
            OutFoundName = TargetNode.Label.IsEmpty() ? TargetNode.Id : TargetNode.Label;
            OutFoundLocation = TargetNode.Center;
            bFoundInSceneGraph = true;
            
            UE_LOG(LogTemp, Log, TEXT("[MatchTargetObject] %s: Found target by object_id '%s' at (%.0f, %.0f, %.0f)"),
                *Agent->AgentName, *ObjectId, OutFoundLocation.X, OutFoundLocation.Y, OutFoundLocation.Z);
            
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[MatchTargetObject] %s: object_id '%s' not found in scene graph"),
                *Agent->AgentName, *ObjectId);
        }
    }
    
    //=========================================================================
    // 优先级2：语义标签匹配（在指定半径内）
    //=========================================================================
    if (!bFoundInSceneGraph && !SemanticTarget.IsEmpty() && SceneGraphManager)
    {
        // 构建语义标签
        FMASemanticLabel TargetLabel;
        TargetLabel.Class = SemanticTarget.Class;
        TargetLabel.Type = SemanticTarget.Type;
        TargetLabel.Features = SemanticTarget.Features;
        
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        
        // 查找所有匹配的节点
        TArray<FMASceneGraphNode> MatchingNodes = FMASceneGraphQuery::FindAllNodesByLabel(AllNodes, TargetLabel);
        
        // 在匹配的节点中找到距离 Agent 最近且在搜索半径内的节点
        FMASceneGraphNode NearestNode;
        float MinDistanceSq = SearchRadius * SearchRadius;
        
        for (const FMASceneGraphNode& Node : MatchingNodes)
        {
            float DistanceSq = FVector::DistSquared(Agent->GetActorLocation(), Node.Center);
            if (DistanceSq < MinDistanceSq)
            {
                MinDistanceSq = DistanceSq;
                NearestNode = Node;
            }
        }
        
        if (NearestNode.IsValid())
        {
            OutFoundId = NearestNode.Id;
            OutFoundName = NearestNode.Label.IsEmpty() ? NearestNode.Id : NearestNode.Label;
            OutFoundLocation = NearestNode.Center;
            bFoundInSceneGraph = true;
            
            float Distance = FMath::Sqrt(MinDistanceSq);
            UE_LOG(LogTemp, Log, TEXT("[MatchTargetObject] %s: Found target by semantic label (Class=%s, Type=%s) at distance %.0f"),
                *Agent->AgentName, *SemanticTarget.Class, *SemanticTarget.Type, Distance);
            
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[MatchTargetObject] %s: No matching target found within radius %.0f (Class=%s, Type=%s)"),
                *Agent->AgentName, SearchRadius, *SemanticTarget.Class, *SemanticTarget.Type);
        }
    }
    
    //=========================================================================
    // 回退方案：UE5 场景查询
    //=========================================================================
    if (!bFoundInSceneGraph && !SemanticTarget.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("[MatchTargetObject] %s: Falling back to UE5 scene query"),
            *Agent->AgentName);
        
        // 构建 UE5 查询标签
        FMASemanticLabel UE5Label;
        UE5Label.Class = SemanticTarget.Class;
        UE5Label.Type = SemanticTarget.Type;
        UE5Label.Features = SemanticTarget.Features;
        
        FMAUESceneQueryResult Result = FMAUESceneQuery::FindNearestObject(World, UE5Label, Agent->GetActorLocation());
        
        if (Result.bFound)
        {
            OutFoundId = Result.Name;
            OutFoundName = Result.Name;
            OutFoundLocation = Result.Location;
            
            UE_LOG(LogTemp, Log, TEXT("[MatchTargetObject] %s: Found target '%s' via UE5 query at (%.0f, %.0f, %.0f)"),
                *Agent->AgentName, *Result.Name, Result.Location.X, Result.Location.Y, Result.Location.Z);
            
            return true;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MatchTargetObject] %s: Target not found"),
        *Agent->AgentName);
    return false;
}

void FMASkillParamsProcessor::ParseSemanticTargetFromJson(const FString& JsonStr, FMASemanticTarget& OutTarget)
{
    // 重置输出
    OutTarget = FMASemanticTarget();
    
    if (JsonStr.IsEmpty())
    {
        return;
    }
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ParseSemanticTargetFromJson] Failed to parse JSON: %s"), *JsonStr);
        return;
    }
    
    // 解析 class 字段
    JsonObject->TryGetStringField(TEXT("class"), OutTarget.Class);
    
    // 解析 type 字段
    JsonObject->TryGetStringField(TEXT("type"), OutTarget.Type);
    
    // 解析 features 字段 (键值对)
    const TSharedPtr<FJsonObject>* FeaturesObject;
    if (JsonObject->TryGetObjectField(TEXT("features"), FeaturesObject))
    {
        for (const auto& Pair : (*FeaturesObject)->Values)
        {
            FString FeatureValue;
            if (Pair.Value->TryGetString(FeatureValue))
            {
                OutTarget.Features.Add(Pair.Key, FeatureValue);
            }
        }
    }
    
    UE_LOG(LogTemp, Verbose, TEXT("[ParseSemanticTargetFromJson] Parsed: Class=%s, Type=%s, Features=%d"), 
        *OutTarget.Class, *OutTarget.Type, OutTarget.Features.Num());
}

EPlaceMode FMASkillParamsProcessor::DeterminePlaceMode(const FMASemanticTarget& surface_target)
{
    // 如果 type 或 features 中包含 "UGV" 或 "ugv"，则为装货模式
    if (surface_target.Type.Contains(TEXT("UGV"), ESearchCase::IgnoreCase) ||
        surface_target.Type.Contains(TEXT("ugv"), ESearchCase::IgnoreCase))
    {
        return EPlaceMode::LoadToUGV;
    }
    
    // 检查 features 中是否有 UGV 相关标识
    for (const auto& Pair : surface_target.Features)
    {
        if (Pair.Key.Contains(TEXT("UGV"), ESearchCase::IgnoreCase) ||
            Pair.Value.Contains(TEXT("UGV"), ESearchCase::IgnoreCase))
        {
            return EPlaceMode::LoadToUGV;
        }
    }
    
    // 检查 class 是否为 robot
    if (surface_target.Class.Equals(TEXT("robot"), ESearchCase::IgnoreCase))
    {
        return EPlaceMode::LoadToUGV;
    }
    // 如果 class 包含 "ground"，则为卸货模式
    if (surface_target.Class.Contains(TEXT("ground"), ESearchCase::IgnoreCase) ||
        surface_target.IsGround())
    {
        return EPlaceMode::UnloadToGround;
    }
    
    // 默认为堆叠模式
    return EPlaceMode::StackOnObject;
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
    
    // 设置 TaskId
    if (Cmd && !Cmd->Params.TaskId.IsEmpty())
    {
        SkillComp->GetFeedbackContextMutable().TaskId = Cmd->Params.TaskId;
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
    
    if (!Cmd->Params.bHasDestPosition) return;
    
    FVector TargetLocation = Cmd->Params.DestPosition;
    bool bIsFlying = (Agent->AgentType == EMAAgentType::UAV || Agent->AgentType == EMAAgentType::FixedWingUAV);
    bool bIsUGV = (Agent->AgentType == EMAAgentType::UGV);
    
    UWorld* World = Agent->GetWorld();
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Agent);
    
    //=========================================================================
    // 辅助 Lambda: 获取指定 X,Y 位置的地面高度
    //=========================================================================
    auto GetGroundHeightAt = [&](float X, float Y) -> float
    {
        FVector TraceStart(X, Y, 50000.f);
        FVector TraceEnd(X, Y, -10000.f);
        FHitResult HitResult;
        
        // 使用 Visibility 通道检测地面
        if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
        {
            return HitResult.Location.Z;
        }
        return 0.f;
    };
    
    //=========================================================================
    // UGV 特殊处理：只调整 Z 分量，保持 X、Y 不变
    //=========================================================================
    if (bIsUGV)
    {
        // 只调整 Z 到地面高度
        float GroundZ = GetGroundHeightAt(TargetLocation.X, TargetLocation.Y);
        TargetLocation.Z = GroundZ;
        
        UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s (UGV): Target (%.0f, %.0f, %.0f) - only Z adjusted to ground"),
            *Agent->AgentName, TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
        
        Params.TargetLocation = TargetLocation;
        return;
    }
    
    // 安全距离常量
    const float SafetyMargin = 500.f;      // 飞行器与障碍物的安全距离
    const float GroundOffset = 100.f;      // 地面机器人与障碍物边缘的距离
    
    //=========================================================================
    // 辅助 Lambda: 找到多边形边缘上距离点最近的位置
    //=========================================================================
    auto FindNearestPointOnPolygonEdge = [](const FVector2D& Point, const TArray<FVector>& Vertices) -> FVector2D
    {
        if (Vertices.Num() < 3) return Point;
        
        FVector2D NearestPoint = FVector2D(Vertices[0].X, Vertices[0].Y);
        float MinDistSq = TNumericLimits<float>::Max();
        
        int32 NumVertices = Vertices.Num();
        for (int32 i = 0; i < NumVertices; ++i)
        {
            int32 j = (i + 1) % NumVertices;
            FVector2D A(Vertices[i].X, Vertices[i].Y);
            FVector2D B(Vertices[j].X, Vertices[j].Y);
            
            // 计算点到线段的最近点
            FVector2D AB = B - A;
            float ABLenSq = AB.SizeSquared();
            
            FVector2D ClosestPoint;
            if (ABLenSq < KINDA_SMALL_NUMBER)
            {
                ClosestPoint = A;
            }
            else
            {
                float t = FMath::Clamp(FVector2D::DotProduct(Point - A, AB) / ABLenSq, 0.f, 1.f);
                ClosestPoint = A + t * AB;
            }
            
            float DistSq = FVector2D::DistSquared(Point, ClosestPoint);
            if (DistSq < MinDistSq)
            {
                MinDistSq = DistSq;
                NearestPoint = ClosestPoint;
            }
        }
        
        return NearestPoint;
    };
    
    //=========================================================================
    // 辅助 Lambda: 计算从多边形内部点向外推出的安全位置
    //=========================================================================
    auto PushPointOutsidePolygon = [&](const FVector2D& InsidePoint, const TArray<FVector>& Vertices, float Offset) -> FVector2D
    {
        FVector2D EdgePoint = FindNearestPointOnPolygonEdge(InsidePoint, Vertices);
        FVector2D Direction = EdgePoint - InsidePoint;
        
        if (Direction.IsNearlyZero())
        {
            // 如果点恰好在边上，使用边的法向量
            Direction = FVector2D(1.f, 0.f);
        }
        Direction.Normalize();
        
        // 从边缘点向外推出 Offset 距离
        return EdgePoint + Direction * Offset;
    };
    
    //=========================================================================
    // Step 1: 使用场景图查询验证目标点是否在建筑物内
    //=========================================================================
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    bool bAdjustedXY = false;
    
    if (SceneGraphManager)
    {
        // 检查目标点是否在建筑物内
        if (SceneGraphManager->IsPointInsideBuilding(TargetLocation))
        {
            UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Target point (%.0f, %.0f, %.0f) is inside a building, adjusting..."),
                *Agent->AgentName, TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
            
            TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
            TArray<FMASceneGraphNode> Buildings = FMASceneGraphQuery::GetAllBuildings(AllNodes);
            
            // 找到包含目标点的建筑物
            for (const FMASceneGraphNode& Building : Buildings)
            {
                TArray<FVector> Vertices;
                if (!FMASceneGraphQuery::ExtractPolygonVertices(Building, Vertices) || Vertices.Num() < 3)
                {
                    continue;
                }
                
                // 检查点是否在此建筑物多边形内
                if (FMAGeometryUtils::IsPointInPolygon2D(TargetLocation, Vertices))
                {
                    if (bIsFlying)
                    {
                        // 飞行器：调整 Z 高度到建筑物顶部以上
                        // 从 RawJson 中提取高度，如果没有则使用默认值
                        float BuildingHeight = 1000.f;
                        if (!Building.RawJson.IsEmpty())
                        {
                            TSharedPtr<FJsonObject> JsonObject;
                            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Building.RawJson);
                            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                            {
                                const TSharedPtr<FJsonObject>* ShapeObject;
                                if (JsonObject->TryGetObjectField(TEXT("shape"), ShapeObject))
                                {
                                    double Height = 0.0;
                                    if ((*ShapeObject)->TryGetNumberField(TEXT("height"), Height))
                                    {
                                        BuildingHeight = static_cast<float>(Height);
                                    }
                                }
                            }
                        }
                        float MinSafeAltitude = Building.Center.Z + BuildingHeight + SafetyMargin;
                        if (TargetLocation.Z < MinSafeAltitude)
                        {
                            TargetLocation.Z = MinSafeAltitude;
                        }
                        UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Flying robot - adjusted Z to %.0f above building %s"),
                            *Agent->AgentName, TargetLocation.Z, *Building.Label);
                    }
                    else
                    {
                        // 地面机器人：调整 X,Y 到建筑物边缘外侧
                        FVector2D InsidePoint(TargetLocation.X, TargetLocation.Y);
                        FVector2D SafePoint = PushPointOutsidePolygon(InsidePoint, Vertices, GroundOffset);
                        
                        TargetLocation.X = SafePoint.X;
                        TargetLocation.Y = SafePoint.Y;
                        bAdjustedXY = true;
                        
                        UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Ground robot - adjusted XY to (%.0f, %.0f) outside building %s"),
                            *Agent->AgentName, TargetLocation.X, TargetLocation.Y, *Building.Label);
                    }
                    break;
                }
            }
        }
    }
    
    // //=========================================================================
    // // Step 2: 回退到 UE5 射线检测（如果场景图未处理）
    // //=========================================================================
    // if (!bAdjustedXY)
    // {
    //     FVector TraceStart = FVector(TargetLocation.X, TargetLocation.Y, 50000.f);
    //     FVector TraceEnd = FVector(TargetLocation.X, TargetLocation.Y, -10000.f);
        
    //     FHitResult HitResult;
    //     if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
    //     {
    //         AActor* HitActor = HitResult.GetActor();
            
    //         if (HitActor)
    //         {
    //             FVector Origin, BoxExtent;
    //             HitActor->GetActorBounds(false, Origin, BoxExtent);
                
    //             UE_LOG(LogTemp, Verbose, TEXT("[ProcessNavigate] %s: Ray hit actor '%s', Origin=(%.1f, %.1f, %.1f), BoxExtent=(%.1f, %.1f, %.1f)"),
    //                 *Agent->AgentName, *HitActor->GetName(), Origin.X, Origin.Y, Origin.Z, BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
                
    //             // 检查是否是有高度的障碍物（不是地面）
    //             // 使用更严格的判断：障碍物高度应该明显大于地面厚度
    //             // 同时检查 Actor 是否是 Landscape 或地面类型
    //             bool bIsObstacle = BoxExtent.Z > 100.f;  // 半高度 > 100 意味着实际高度 > 200
                
    //             // 额外检查：如果 Actor 名称包含 "floor"、"ground"、"landscape" 则不是障碍物
    //             FString ActorName = HitActor->GetName().ToLower();
    //             if (ActorName.Contains(TEXT("floor")) || 
    //                 ActorName.Contains(TEXT("ground")) || 
    //                 ActorName.Contains(TEXT("landscape")) ||
    //                 ActorName.Contains(TEXT("terrain")))
    //             {
    //                 bIsObstacle = false;
    //             }
                
    //             if (bIsObstacle)
    //             {
    //                 bool bInsideX = FMath::Abs(TargetLocation.X - Origin.X) < BoxExtent.X;
    //                 bool bInsideY = FMath::Abs(TargetLocation.Y - Origin.Y) < BoxExtent.Y;
                    
    //                 if (bInsideX && bInsideY)
    //                 {
    //                     if (bIsFlying)
    //                     {
    //                         // 飞行器：调整 Z 高度
    //                         float ObstacleTopZ = Origin.Z + BoxExtent.Z;
    //                         float MinSafeAltitude = ObstacleTopZ + SafetyMargin;
    //                         if (TargetLocation.Z < MinSafeAltitude)
    //                         {
    //                             TargetLocation.Z = MinSafeAltitude;
    //                             UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Flying robot - adjusted Z to %.0f above obstacle"),
    //                                 *Agent->AgentName, TargetLocation.Z);
    //                         }
    //                     }
    //                     else
    //                     {
    //                         // 地面机器人：找到最近的边缘并推出
    //                         // 计算到四个边的距离，选择最近的边
    //                         float DistToMinX = FMath::Abs(TargetLocation.X - (Origin.X - BoxExtent.X));
    //                         float DistToMaxX = FMath::Abs(TargetLocation.X - (Origin.X + BoxExtent.X));
    //                         float DistToMinY = FMath::Abs(TargetLocation.Y - (Origin.Y - BoxExtent.Y));
    //                         float DistToMaxY = FMath::Abs(TargetLocation.Y - (Origin.Y + BoxExtent.Y));
                            
    //                         float MinDist = FMath::Min(FMath::Min(DistToMinX, DistToMaxX), FMath::Min(DistToMinY, DistToMaxY));
                            
    //                         if (MinDist == DistToMinX)
    //                         {
    //                             TargetLocation.X = Origin.X - BoxExtent.X - GroundOffset;
    //                         }
    //                         else if (MinDist == DistToMaxX)
    //                         {
    //                             TargetLocation.X = Origin.X + BoxExtent.X + GroundOffset;
    //                         }
    //                         else if (MinDist == DistToMinY)
    //                         {
    //                             TargetLocation.Y = Origin.Y - BoxExtent.Y - GroundOffset;
    //                         }
    //                         else
    //                         {
    //                             TargetLocation.Y = Origin.Y + BoxExtent.Y + GroundOffset;
    //                         }
                            
    //                         bAdjustedXY = true;
    //                         UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Ground robot - adjusted XY to (%.0f, %.0f) outside obstacle"),
    //                             *Agent->AgentName, TargetLocation.X, TargetLocation.Y);
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }
    
    //=========================================================================
    // Step 3: 地面机器人始终调整 Z 到实际地面高度
    //=========================================================================
    // if (!bIsFlying)
    // {
    //     float GroundZ = GetGroundHeightAt(TargetLocation.X, TargetLocation.Y);
    //     TargetLocation.Z = GroundZ;
    //     UE_LOG(LogTemp, Verbose, TEXT("[ProcessNavigate] %s: Ground robot - adjusted Z to ground height %.0f"),
    //         *Agent->AgentName, TargetLocation.Z);
    // }
    
    //=========================================================================
    // Step 4: 存储附近地标信息到反馈上下文（用于反馈生成）
    //=========================================================================
    if (SceneGraphManager)
    {
        FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(TargetLocation, 2000.f);
        if (NearestLandmark.IsValid())
        {
            Context.NearbyLandmarkLabel = NearestLandmark.Label;
            Context.NearbyLandmarkType = NearestLandmark.Type;
            Context.NearbyLandmarkDistance = FVector::Dist(TargetLocation, NearestLandmark.Center);
            
            UE_LOG(LogTemp, Verbose, TEXT("[ProcessNavigate] %s: Nearest landmark to target: %s (%s) at distance %.0f"),
                *Agent->AgentName, *NearestLandmark.Label, *NearestLandmark.Type, Context.NearbyLandmarkDistance);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Final target location (%.0f, %.0f, %.0f)"),
        *Agent->AgentName, TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
    
    Params.TargetLocation = TargetLocation;
}

void FMASkillParamsProcessor::ProcessSearch(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    //=========================================================================
    // Step 1: 解析搜索区域边界坐标
    //=========================================================================
    if (Cmd && Cmd->Params.SearchArea.Num() > 0)
    {
        Params.SearchBoundary.Empty();
        for (const FVector2D& Point : Cmd->Params.SearchArea)
        {
            Params.SearchBoundary.Add(FVector(Point.X, Point.Y, 0.0f));
        }
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: SearchArea has %d vertices"), 
            *Agent->AgentName, Params.SearchBoundary.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessSearch] %s: No SearchArea provided"), *Agent->AgentName);
    }
    
    //=========================================================================
    // Step 2: 解析目标语义标签 (class, type, features) - 使用通用解析函数
    //=========================================================================
    if (Cmd && !Cmd->Params.TargetJson.IsEmpty())
    {
        ParseSemanticTargetFromJson(Cmd->Params.TargetJson, Params.SearchTarget);
        
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Parsed target - Class=%s, Type=%s, Features=%d"),
            *Agent->AgentName, *Params.SearchTarget.Class, *Params.SearchTarget.Type, Params.SearchTarget.Features.Num());
    }
    
    // 从 TargetFeatures 补充特征
    if (Cmd && Cmd->Params.TargetFeatures.Num() > 0)
    {
        for (const auto& Pair : Cmd->Params.TargetFeatures)
        {
            if (!Params.SearchTarget.Features.Contains(Pair.Key))
            {
                Params.SearchTarget.Features.Add(Pair.Key, Pair.Value);
            }
        }
    }
    
    // 构建 FMASemanticLabel 用于场景查询
    FMASemanticLabel SearchLabel;
    SearchLabel.Class = Params.SearchTarget.Class;
    SearchLabel.Type = Params.SearchTarget.Type;
    SearchLabel.Features = Params.SearchTarget.Features;
    
    //=========================================================================
    // Step 3: 使用场景图查询匹配对象
    //=========================================================================
    UWorld* World = Agent->GetWorld();
    if (!World) return;
    
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    bool bFoundInSceneGraph = false;
    
    if (SceneGraphManager && Params.SearchBoundary.Num() >= 3)
    {
        // 使用场景图查询
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        TArray<FMASceneGraphNode> FoundNodes = FMASceneGraphQuery::FindNodesInBoundary(
            AllNodes, SearchLabel, Params.SearchBoundary);
        
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Scene graph query found %d nodes"),
            *Agent->AgentName, FoundNodes.Num());
        
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
                *Agent->AgentName, *ObjectName, Node.Center.X, Node.Center.Y, Node.Center.Z);
        }
        
        bFoundInSceneGraph = FoundNodes.Num() > 0;
    }
    
    //=========================================================================
    // Step 4: 回退到 UE5 场景查询 (如果场景图未找到)
    //=========================================================================
    if (!bFoundInSceneGraph && Params.SearchBoundary.Num() >= 3)
    {
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Falling back to UE5 scene query"),
            *Agent->AgentName);
        
        // 转换为 FMAUESceneQuery 使用的 FMASemanticLabel
        FMASemanticLabel UE5Label;
        UE5Label.Class = SearchLabel.Class;
        UE5Label.Type = SearchLabel.Type;
        UE5Label.Features = SearchLabel.Features;
        
        TArray<FMAUESceneQueryResult> Results = FMAUESceneQuery::FindObjectsInBoundary(
            World, UE5Label, Params.SearchBoundary);
        
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: UE5 scene query found %d objects"),
            *Agent->AgentName, Results.Num());
        
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
        *Agent->AgentName, Context.FoundObjects.Num());
}

void FMASkillParamsProcessor::ProcessFollow(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp) return;
    
    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    //=========================================================================
    // Step 1: 解析目标参数 - 使用通用解析函数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    
    if (Cmd)
    {
        // 从 TargetJson 解析语义标签
        if (!Cmd->Params.TargetJson.IsEmpty())
        {
            ParseSemanticTargetFromJson(Cmd->Params.TargetJson, Target);
        }
        // 兼容旧格式：从 TargetEntity 或 TargetToken 获取名称
        else if (!Cmd->Params.TargetEntity.IsEmpty())
        {
            // 构建简单的语义目标
            Target.Type = Cmd->Params.TargetEntity;
            ObjectId = Cmd->Params.TargetEntity;  // 尝试作为 ID 使用
        }
        else if (!Cmd->Params.TargetToken.IsEmpty())
        {
            Target.Type = Cmd->Params.TargetToken;
            ObjectId = Cmd->Params.TargetToken;  // 尝试作为 ID 使用
        }
    }
    
    //=========================================================================
    // Step 2: 匹配目标对象（使用通用匹配函数）
    //=========================================================================
    FString FoundId, FoundName;
    FVector FoundLocation;
    bool bFound = MatchTargetObject(Agent, ObjectId, Target, 1000.f, FoundId, FoundName, FoundLocation);
    
    //=========================================================================
    // Step 3: 保存到参数和反馈上下文
    //=========================================================================
    Context.bFollowTargetFound = bFound;
    Context.FollowTargetRobotName = FoundName;
    Context.TargetName = FoundName;
    Context.TargetLocation = FoundLocation;
    
    if (bFound)
    {
        Context.FollowTargetDistance = FVector::Dist(Agent->GetActorLocation(), FoundLocation);
        
        // 尝试获取 Actor 引用（用于跟随）
        // 首先尝试查找机器人
        AMACharacter* TargetRobot = FMAUESceneQuery::FindRobotByName(Agent->GetWorld(), FoundName);
        
        // 如果不是机器人，尝试通过名称查找其他 Actor
        if (!TargetRobot)
        {
            TArray<AActor*> AllActors;
            UGameplayStatics::GetAllActorsOfClass(Agent->GetWorld(), AActor::StaticClass(), AllActors);
            for (AActor* Actor : AllActors)
            {
                if (Actor && Actor->GetName().Equals(FoundName, ESearchCase::IgnoreCase))
                {
                    // 尝试转换为 AMACharacter
                    TargetRobot = Cast<AMACharacter>(Actor);
                    if (TargetRobot)
                    {
                        break;
                    }
                }
            }
        }
        
        if (TargetRobot)
        {
            Params.FollowTarget = TargetRobot;
            UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Found target '%s' at distance %.0f"),
                *Agent->AgentName, *FoundName, Context.FollowTargetDistance);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[ProcessFollow] %s: Target '%s' found in scene graph but no AMACharacter reference available"),
                *Agent->AgentName, *FoundName);
        }
    }
    else
    {
        Context.FollowErrorReason = FString::Printf(TEXT("Target not found"));
        UE_LOG(LogTemp, Warning, TEXT("[ProcessFollow] %s: Target not found"),
            *Agent->AgentName);
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
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    bool bFoundInSceneGraph = false;
    
    if (SceneGraphManager)
    {
        // 构建充电站语义标签
        FMASemanticLabel ChargingStationLabel = FMASemanticLabel::MakeChargingStation();
        
        // 获取所有节点并查找最近的充电站
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode NearestStation = FMASceneGraphQuery::FindNearestNode(
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
                *Agent->AgentName, *Context.ChargingStationId, 
                NearestStation.Center.X, NearestStation.Center.Y, NearestStation.Center.Z, Distance);
            
            bFoundInSceneGraph = true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[ProcessCharge] %s: No charging station found in scene graph"),
                *Agent->AgentName);
        }
    }
    
    //=========================================================================
    // Step 2: 回退到 UE5 场景查询 (如果场景图未找到)
    //=========================================================================
    if (!bFoundInSceneGraph)
    {
        UE_LOG(LogTemp, Log, TEXT("[ProcessCharge] %s: Falling back to UE5 scene query for charging station"),
            *Agent->AgentName);
        
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
                    *Agent->AgentName, *Context.ChargingStationId,
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
            *Agent->AgentName, *Context.ChargeErrorReason);
    }
}

void FMASkillParamsProcessor::ProcessPlace(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASearchResults& SearchResults = SkillComp->GetSearchResultsMutable();
    SearchResults.Reset();
    
    //=========================================================================
    // Step 1: 解析 target/surface_target 语义标签 JSON 到 FMASemanticTarget
    //=========================================================================
    if (Cmd)
    {
        // 解析 target JSON
        if (!Cmd->Params.Object1Json.IsEmpty())
        {
            ParseSemanticTargetFromJson(Cmd->Params.Object1Json, Params.PlaceObject1);
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Parsed target - Class=%s, Type=%s"), 
                *Agent->AgentName, *Params.PlaceObject1.Class, *Params.PlaceObject1.Type);
        }
        
        // 解析 surface_target JSON
        if (!Cmd->Params.Object2Json.IsEmpty())
        {
            ParseSemanticTargetFromJson(Cmd->Params.Object2Json, Params.PlaceObject2);
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Parsed surface_target - Class=%s, Type=%s"), 
                *Agent->AgentName, *Params.PlaceObject2.Class, *Params.PlaceObject2.Type);
        }
    }
    
    //=========================================================================
    // Step 2: 确定 PlaceMode
    // - LoadToUGV: surface_target 是 UGV 机器人
    // - UnloadToGround: surface_target 是 ground
    // - StackOnObject: surface_target 是其他物体
    //=========================================================================
    EPlaceMode PlaceMode = DeterminePlaceMode(Params.PlaceObject2);
    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: PlaceMode=%d"), *Agent->AgentName, (int32)PlaceMode);
    
    //=========================================================================
    // Step 3: 获取场景图管理器
    //=========================================================================
    UWorld* World = Agent->GetWorld();
    if (!World) return;
    
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
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
            *Agent->AgentName, AllNodes.Num(), RobotCount, PickupItemCount);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: SceneGraphManager is null!"), *Agent->AgentName);
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
        UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: target semantic label is empty"), *Agent->AgentName);
    }
    else
    {
        bool bFoundInSceneGraph = false;
        
        // 首先尝试使用场景图查询
        if (SceneGraphManager && AllNodes.Num() > 0)
        {
            FMASceneGraphNode Object1Node = FMASceneGraphQuery::FindNearestNode(AllNodes, Label1, Agent->GetActorLocation());
            
            if (Object1Node.IsValid() && Object1Node.IsPickupItem())
            {
                Context.PlacedObjectName = Object1Node.Label.IsEmpty() ? Object1Node.Id : Object1Node.Label;
                SearchResults.Object1Location = Object1Node.Center;
                
                // 存储场景图节点ID到Context，用于后续状态更新
                Context.ObjectAttributes.Add(TEXT("object1_node_id"), Object1Node.Id);
                
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found target '%s' in scene graph at (%.0f, %.0f, %.0f)"), 
                    *Agent->AgentName, *Context.PlacedObjectName, 
                    Object1Node.Center.X, Object1Node.Center.Y, Object1Node.Center.Z);
                
                // 关键修复：使用 UE5 场景查询获取实际的 Actor 引用
                // 场景图只提供位置信息，技能执行需要 Actor 引用
                FMAUESceneQueryResult ActorResult = FMAUESceneQuery::FindNearestObject(World, Label1, Agent->GetActorLocation());
                if (ActorResult.bFound && ActorResult.Actor)
                {
                    SearchResults.Object1Actor = ActorResult.Actor;
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Resolved Actor for target '%s'"), 
                        *Agent->AgentName, *Context.PlacedObjectName);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: Scene graph found target but UE5 Actor not found!"), 
                        *Agent->AgentName);
                }
                
                bFoundInSceneGraph = true;
            }
        }
        
        // 回退到 UE5 场景查询
        if (!bFoundInSceneGraph)
        {
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Falling back to UE5 scene query for target"), *Agent->AgentName);
            
            FMAUESceneQueryResult Result1 = FMAUESceneQuery::FindNearestObject(World, Label1, Agent->GetActorLocation());
            if (Result1.bFound)
            {
                Context.PlacedObjectName = Result1.Name;
                SearchResults.Object1Actor = Result1.Actor;
                SearchResults.Object1Location = Result1.Location;
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found target '%s' via UE5 query at (%.0f, %.0f, %.0f)"), 
                    *Agent->AgentName, *Result1.Name, Result1.Location.X, Result1.Location.Y, Result1.Location.Z);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: target not found in scene"), *Agent->AgentName);
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
                *Agent->AgentName, SearchResults.Object2Location.X, SearchResults.Object2Location.Y, SearchResults.Object2Location.Z);
            break;
        }
        
        case EPlaceMode::LoadToUGV:
        {
            // 装货到 UGV：根据 label 特征查找 UGV
            FString RobotName = Label2.Features.Contains(TEXT("label")) ? Label2.Features[TEXT("label")] : TEXT("");
            bool bFoundInSceneGraph = false;
            
            if (SceneGraphManager && AllNodes.Num() > 0 && !RobotName.IsEmpty())
            {
                // 使用场景图查询机器人
                FMASemanticLabel RobotLabel = FMASemanticLabel::MakeRobot(RobotName);
                FMASceneGraphNode RobotNode = FMASceneGraphQuery::FindNodeByLabel(AllNodes, RobotLabel);
                
                if (RobotNode.IsValid() && RobotNode.IsRobot())
                {
                    Context.PlaceTargetName = RobotNode.Label.IsEmpty() ? RobotNode.Id : RobotNode.Label;
                    SearchResults.Object2Location = RobotNode.Center;
                    
                    // 存储场景图节点ID到Context
                    Context.ObjectAttributes.Add(TEXT("object2_node_id"), RobotNode.Id);
                    
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found UGV '%s' in scene graph at (%.0f, %.0f, %.0f)"), 
                        *Agent->AgentName, *Context.PlaceTargetName, 
                        RobotNode.Center.X, RobotNode.Center.Y, RobotNode.Center.Z);
                    
                    // 关键修复：使用 UE5 场景查询获取实际的 Actor 引用
                    AMACharacter* Robot = FMAUESceneQuery::FindRobotByName(World, RobotName);
                    if (Robot)
                    {
                        SearchResults.Object2Actor = Robot;
                        UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Resolved Actor for UGV '%s'"), 
                            *Agent->AgentName, *RobotName);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: Scene graph found UGV but UE5 Actor not found!"), 
                            *Agent->AgentName);
                    }
                    
                    bFoundInSceneGraph = true;
                }
            }
            
            // 回退到 UE5 场景查询
            if (!bFoundInSceneGraph)
            {
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Falling back to UE5 scene query for UGV '%s'"), 
                    *Agent->AgentName, *RobotName);
                
                AMACharacter* Robot = FMAUESceneQuery::FindRobotByName(World, RobotName);
                if (Robot)
                {
                    Context.PlaceTargetName = Robot->AgentName;
                    SearchResults.Object2Actor = Robot;
                    SearchResults.Object2Location = Robot->GetActorLocation();
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found UGV '%s' via UE5 query at (%.0f, %.0f, %.0f)"), 
                        *Agent->AgentName, *Robot->AgentName, 
                        SearchResults.Object2Location.X, SearchResults.Object2Location.Y, SearchResults.Object2Location.Z);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: UGV '%s' not found"), *Agent->AgentName, *RobotName);
                }
            }
            break;
        }
        
        case EPlaceMode::StackOnObject:
        {
            // 堆叠到另一个物体：查找最近的匹配物体
            bool bFoundInSceneGraph = false;
            
            if (SceneGraphManager && AllNodes.Num() > 0)
            {
                FMASceneGraphNode Object2Node = FMASceneGraphQuery::FindNearestNode(AllNodes, Label2, Agent->GetActorLocation());
                
                if (Object2Node.IsValid() && Object2Node.IsPickupItem())
                {
                    Context.PlaceTargetName = Object2Node.Label.IsEmpty() ? Object2Node.Id : Object2Node.Label;
                    SearchResults.Object2Location = Object2Node.Center;
                    
                    // 存储场景图节点ID到Context
                    Context.ObjectAttributes.Add(TEXT("object2_node_id"), Object2Node.Id);
                    
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found surface_target '%s' in scene graph at (%.0f, %.0f, %.0f)"), 
                        *Agent->AgentName, *Context.PlaceTargetName, 
                        Object2Node.Center.X, Object2Node.Center.Y, Object2Node.Center.Z);
                    
                    // 关键修复：使用 UE5 场景查询获取实际的 Actor 引用
                    FMAUESceneQueryResult ActorResult = FMAUESceneQuery::FindNearestObject(World, Label2, Agent->GetActorLocation());
                    if (ActorResult.bFound && ActorResult.Actor)
                    {
                        SearchResults.Object2Actor = ActorResult.Actor;
                        UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Resolved Actor for surface_target '%s'"), 
                            *Agent->AgentName, *Context.PlaceTargetName);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: Scene graph found surface_target but UE5 Actor not found!"), 
                            *Agent->AgentName);
                    }
                    
                    bFoundInSceneGraph = true;
                }
            }
            
            // 回退到 UE5 场景查询
            if (!bFoundInSceneGraph)
            {
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Falling back to UE5 scene query for surface_target"), *Agent->AgentName);
                
                FMAUESceneQueryResult Result2 = FMAUESceneQuery::FindNearestObject(World, Label2, Agent->GetActorLocation());
                if (Result2.bFound)
                {
                    Context.PlaceTargetName = Result2.Name;
                    SearchResults.Object2Actor = Result2.Actor;
                    SearchResults.Object2Location = Result2.Location;
                    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found surface_target '%s' via UE5 query at (%.0f, %.0f, %.0f)"), 
                        *Agent->AgentName, *Result2.Name, Result2.Location.X, Result2.Location.Y, Result2.Location.Z);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: surface_target not found in scene"), *Agent->AgentName);
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
    
    // 默认起飞高度 1000cm (10m)
    float RequestedHeight = 1000.f;
    
    // 如果有传入参数，覆盖默认值
    if (Cmd && Cmd->Params.bHasDestPosition && Cmd->Params.DestPosition.Z > 0.f)
    {
        RequestedHeight = Cmd->Params.DestPosition.Z;
    }
    
    //=========================================================================
    // Step 1: 使用场景图验证目标高度
    // 确保高度高于附近建筑物
    //=========================================================================
    UWorld* World = Agent->GetWorld();
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    const float SafetyMargin = 500.f;  // 安全距离 5m
    const float SearchRadius = 3000.f;  // 搜索半径 30m
    float MinSafeHeight = RequestedHeight;
    float MaxBuildingHeight = 0.f;
    FString TallestBuildingLabel;
    
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        TArray<FMASceneGraphNode> Buildings = FMASceneGraphQuery::GetAllBuildings(AllNodes);
        
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
                *Agent->AgentName, *TallestBuildingLabel, MaxBuildingHeight, MinSafeHeight);
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
            *Agent->AgentName, RequestedHeight, FinalHeight, *TallestBuildingLabel);
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
        *Agent->AgentName, FinalHeight, bHeightAdjusted ? TEXT("true") : TEXT("false"));
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
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        
        // 检查当前位置是否在建筑物内
        if (SceneGraphManager->IsPointInsideBuilding(LandLocation))
        {
            bLocationSafe = false;
            GroundType = TEXT("building_interior");
            
            UE_LOG(LogTemp, Warning, TEXT("[ProcessLand] %s: Current position is inside a building, searching for safe landing zone"),
                *Agent->AgentName);
            
            // 查找最近的安全着陆区（道路或路口）
            TArray<FMASceneGraphNode> Roads = FMASceneGraphQuery::GetAllRoads(AllNodes);
            TArray<FMASceneGraphNode> Intersections = FMASceneGraphQuery::GetAllIntersections(AllNodes);
            
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
                    *Agent->AgentName, *NearestSafeNode.Label, *GroundType, MinDistance);
            }
        }
        else
        {
            // 当前位置安全，确定地面类型
            // 检查是否在道路上
            TArray<FMASceneGraphNode> Roads = FMASceneGraphQuery::GetAllRoads(AllNodes);
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
                TArray<FMASceneGraphNode> Intersections = FMASceneGraphQuery::GetAllIntersections(AllNodes);
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
    // Step 2: 射线检测地面高度
    //=========================================================================
    if (Cmd && Cmd->Params.bHasDestPosition)
    {
        LandHeight = Cmd->Params.DestPosition.Z;
        LandLocation = FVector(Cmd->Params.DestPosition.X, Cmd->Params.DestPosition.Y, LandHeight);
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
        *Agent->AgentName, LandLocation.X, LandLocation.Y, LandHeight, *GroundType, bLocationSafe ? TEXT("true") : TEXT("false"));
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
    // Step 1: 如果有传入 HomeLocation，使用传入值
    //=========================================================================
    if (Cmd && Cmd->Params.bHasDestPosition)
    {
        HomeLocation = Cmd->Params.DestPosition;
    }
    
    //=========================================================================
    // Step 2: 查找家位置附近的地标
    //=========================================================================
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    if (SceneGraphManager)
    {
        FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(HomeLocation, 2000.f);
        if (NearestLandmark.IsValid())
        {
            HomeLandmark = NearestLandmark.Label;
            
            UE_LOG(LogTemp, Verbose, TEXT("[ProcessReturnHome] %s: Home location near landmark '%s'"),
                *Agent->AgentName, *HomeLandmark);
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
        *Agent->AgentName, HomeLocation.X, HomeLocation.Y, HomeLocation.Z, LandHeight, *HomeLandmark);
}


void FMASkillParamsProcessor::ProcessTakePhoto(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    //=========================================================================
    // Step 1: 解析目标参数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    
    // 解析 RawParamsJson
    TSharedPtr<FJsonObject> ParamsJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Cmd->Params.RawParamsJson);
    if (FJsonSerializer::Deserialize(Reader, ParamsJson) && ParamsJson.IsValid())
    {
        // 从 Params 中提取 object_id
        if (ParamsJson->HasField(TEXT("object_id")))
        {
            ObjectId = ParamsJson->GetStringField(TEXT("object_id"));
        }
        
        // 从 Params 中提取 target JSON
        if (ParamsJson->HasField(TEXT("target")))
        {
            const TSharedPtr<FJsonObject>* TargetObj;
            if (ParamsJson->TryGetObjectField(TEXT("target"), TargetObj))
            {
                // 将 JSON 对象转换为字符串
                FString TargetJsonStr;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&TargetJsonStr);
                FJsonSerializer::Serialize((*TargetObj).ToSharedRef(), Writer);
                ParseSemanticTargetFromJson(TargetJsonStr, Target);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: object_id='%s', target(Class=%s, Type=%s)"),
        *Agent->AgentName, *ObjectId, *Target.Class, *Target.Type);
    
    //=========================================================================
    // Step 2: 匹配目标对象
    //=========================================================================
    FString FoundId, FoundName;
    FVector FoundLocation;
    bool bFound = MatchTargetObject(Agent, ObjectId, Target, 1000.f, FoundId, FoundName, FoundLocation);
    
    //=========================================================================
    // Step 3: 保存到参数和反馈上下文
    //=========================================================================
    Params.CommonTargetObjectId = FoundId;
    Params.CommonTarget = Target;
    
    Context.bPhotoTargetFound = bFound;
    Context.PhotoTargetName = FoundName;
    Context.PhotoTargetId = FoundId;
    Context.TargetLocation = FoundLocation;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: Target found=%s, name='%s', id='%s'"),
        *Agent->AgentName, bFound ? TEXT("true") : TEXT("false"), *FoundName, *FoundId);
}

void FMASkillParamsProcessor::ProcessBroadcast(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    //=========================================================================
    // Step 1: 解析目标参数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    FString Message;
    
    // 解析 RawParamsJson
    TSharedPtr<FJsonObject> ParamsJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Cmd->Params.RawParamsJson);
    if (FJsonSerializer::Deserialize(Reader, ParamsJson) && ParamsJson.IsValid())
    {
        // 从 Params 中提取 object_id
        if (ParamsJson->HasField(TEXT("object_id")))
        {
            ObjectId = ParamsJson->GetStringField(TEXT("object_id"));
        }
        
        // 从 Params 中提取 target JSON
        if (ParamsJson->HasField(TEXT("target")))
        {
            const TSharedPtr<FJsonObject>* TargetObj;
            if (ParamsJson->TryGetObjectField(TEXT("target"), TargetObj))
            {
                // 将 JSON 对象转换为字符串
                FString TargetJsonStr;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&TargetJsonStr);
                FJsonSerializer::Serialize((*TargetObj).ToSharedRef(), Writer);
                ParseSemanticTargetFromJson(TargetJsonStr, Target);
            }
        }
        
        // 从 Params 中提取 message 字段
        if (ParamsJson->HasField(TEXT("message")))
        {
            Message = ParamsJson->GetStringField(TEXT("message"));
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: object_id='%s', target(Class=%s, Type=%s), message='%s'"),
        *Agent->AgentName, *ObjectId, *Target.Class, *Target.Type, *Message);
    
    //=========================================================================
    // Step 2: 匹配目标对象
    //=========================================================================
    FString FoundId, FoundName;
    FVector FoundLocation;
    bool bFound = MatchTargetObject(Agent, ObjectId, Target, 1000.f, FoundId, FoundName, FoundLocation);
    
    //=========================================================================
    // Step 3: 保存到参数和反馈上下文
    //=========================================================================
    Params.CommonTargetObjectId = FoundId;
    Params.CommonTarget = Target;
    Params.BroadcastMessage = Message;
    
    Context.bBroadcastTargetFound = bFound;
    Context.BroadcastTargetName = FoundName;
    Context.BroadcastMessage = Message;
    Context.TargetLocation = FoundLocation;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: Target found=%s, name='%s', message='%s'"),
        *Agent->AgentName, bFound ? TEXT("true") : TEXT("false"), *FoundName, *Message);
}

void FMASkillParamsProcessor::ProcessHandleHazard(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    //=========================================================================
    // Step 1: 解析目标参数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    
    // 解析 RawParamsJson
    TSharedPtr<FJsonObject> ParamsJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Cmd->Params.RawParamsJson);
    if (FJsonSerializer::Deserialize(Reader, ParamsJson) && ParamsJson.IsValid())
    {
        // 从 Params 中提取 object_id
        if (ParamsJson->HasField(TEXT("object_id")))
        {
            ObjectId = ParamsJson->GetStringField(TEXT("object_id"));
        }
        
        // 从 Params 中提取 target JSON
        if (ParamsJson->HasField(TEXT("target")))
        {
            const TSharedPtr<FJsonObject>* TargetObj;
            if (ParamsJson->TryGetObjectField(TEXT("target"), TargetObj))
            {
                // 将 JSON 对象转换为字符串
                FString TargetJsonStr;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&TargetJsonStr);
                FJsonSerializer::Serialize((*TargetObj).ToSharedRef(), Writer);
                ParseSemanticTargetFromJson(TargetJsonStr, Target);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessHandleHazard] %s: object_id='%s', target(Class=%s, Type=%s)"),
        *Agent->AgentName, *ObjectId, *Target.Class, *Target.Type);
    
    //=========================================================================
    // Step 2: 匹配目标对象
    //=========================================================================
    FString FoundId, FoundName;
    FVector FoundLocation;
    bool bFound = MatchTargetObject(Agent, ObjectId, Target, 1000.f, FoundId, FoundName, FoundLocation);
    
    //=========================================================================
    // Step 3: 保存到参数和反馈上下文
    //=========================================================================
    Params.CommonTargetObjectId = FoundId;
    Params.CommonTarget = Target;
    
    Context.bHazardTargetFound = bFound;
    Context.HazardTargetName = FoundName;
    Context.HazardTargetId = FoundId;
    Context.TargetLocation = FoundLocation;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessHandleHazard] %s: Target found=%s, name='%s', id='%s'"),
        *Agent->AgentName, bFound ? TEXT("true") : TEXT("false"), *FoundName, *FoundId);
}

void FMASkillParamsProcessor::ProcessGuide(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    
    //=========================================================================
    // Step 1: 解析目标参数
    //=========================================================================
    FString ObjectId;
    FMASemanticTarget Target;
    FVector Destination = FVector::ZeroVector;
    
    // 解析 RawParamsJson
    TSharedPtr<FJsonObject> ParamsJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Cmd->Params.RawParamsJson);
    if (FJsonSerializer::Deserialize(Reader, ParamsJson) && ParamsJson.IsValid())
    {
        // 从 Params 中提取 object_id
        if (ParamsJson->HasField(TEXT("object_id")))
        {
            ObjectId = ParamsJson->GetStringField(TEXT("object_id"));
        }
        
        // 从 Params 中提取 target JSON
        if (ParamsJson->HasField(TEXT("target")))
        {
            const TSharedPtr<FJsonObject>* TargetObj;
            if (ParamsJson->TryGetObjectField(TEXT("target"), TargetObj))
            {
                // 将 JSON 对象转换为字符串
                FString TargetJsonStr;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&TargetJsonStr);
                FJsonSerializer::Serialize((*TargetObj).ToSharedRef(), Writer);
                ParseSemanticTargetFromJson(TargetJsonStr, Target);
            }
        }
    }
    
    // 从 Params 中提取 destination 坐标（格式与 Navigate 相同）
    if (Cmd->Params.bHasDestPosition)
    {
        Destination = Cmd->Params.DestPosition;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessGuide] %s: object_id='%s', target(Class=%s, Type=%s), destination=(%.0f, %.0f, %.0f)"),
        *Agent->AgentName, *ObjectId, *Target.Class, *Target.Type, Destination.X, Destination.Y, Destination.Z);
    
    //=========================================================================
    // Step 2: 匹配目标对象
    //=========================================================================
    FString FoundId, FoundName;
    FVector FoundLocation;
    bool bFound = MatchTargetObject(Agent, ObjectId, Target, 1000.f, FoundId, FoundName, FoundLocation);
    
    //=========================================================================
    // Step 3: 如果找到目标，获取 Actor 引用
    //=========================================================================
    TWeakObjectPtr<AActor> TargetActor;
    
    if (bFound && !FoundId.IsEmpty())
    {
        // 尝试通过名称查找 Actor（主要用于机器人）
        if (!FoundName.IsEmpty())
        {
            AMACharacter* TargetRobot = FMAUESceneQuery::FindRobotByName(Agent->GetWorld(), FoundName);
            if (TargetRobot)
            {
                TargetActor = TargetRobot;
                UE_LOG(LogTemp, Log, TEXT("[ProcessGuide] %s: Found target Actor '%s'"),
                    *Agent->AgentName, *FoundName);
            }
        }
        
        // 如果没有找到 Actor，尝试通过 UE5 场景查询
        if (!TargetActor.IsValid() && !Target.IsEmpty())
        {
            FMASemanticLabel UE5Label;
            UE5Label.Class = Target.Class;
            UE5Label.Type = Target.Type;
            UE5Label.Features = Target.Features;
            
            FMAUESceneQueryResult Result = FMAUESceneQuery::FindNearestObject(Agent->GetWorld(), UE5Label, Agent->GetActorLocation());
            if (Result.bFound && Result.Actor != nullptr)
            {
                TargetActor = Result.Actor;
                UE_LOG(LogTemp, Log, TEXT("[ProcessGuide] %s: Found target Actor via UE5 query"),
                    *Agent->AgentName);
            }
        }
    }
    
    //=========================================================================
    // Step 4: 保存到参数和反馈上下文
    //=========================================================================
    Params.GuideTargetObjectId = FoundId;
    Params.GuideTarget = Target;
    Params.GuideDestination = Destination;
    Params.GuideTargetActor = TargetActor;
    
    Context.bGuideTargetFound = bFound;
    Context.GuideTargetName = FoundName;
    Context.GuideTargetId = FoundId;
    Context.GuideDestination = Destination;
    Context.TargetLocation = FoundLocation;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessGuide] %s: Target found=%s, name='%s', Actor=%s, destination=(%.0f, %.0f, %.0f)"),
        *Agent->AgentName, bFound ? TEXT("true") : TEXT("false"), *FoundName, 
        TargetActor.IsValid() ? TEXT("valid") : TEXT("null"), Destination.X, Destination.Y, Destination.Z);
}
