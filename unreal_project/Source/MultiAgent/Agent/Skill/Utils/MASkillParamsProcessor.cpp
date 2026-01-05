// MASkillParamsProcessor.cpp
// 技能参数处理器实现

#include "MASkillParamsProcessor.h"
#include "MASceneQuery.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Core/Comm/MACommTypes.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Core/Manager/MASceneGraphQuery.h"
#include "../../../Environment/MAChargingStation.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

//=============================================================================
// Place 技能辅助方法实现
//=============================================================================

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
                if (FMASceneGraphQuery::IsPointInPolygon2D(TargetLocation, Vertices))
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
    // Step 2: 解析目标语义标签 (class, type, features)
    //=========================================================================
    FMASemanticLabel SearchLabel;
    
    // 从 TargetJson 解析语义标签 (如果提供)
    if (Cmd && !Cmd->Params.TargetJson.IsEmpty())
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Cmd->Params.TargetJson);
        
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            JsonObject->TryGetStringField(TEXT("class"), SearchLabel.Class);
            JsonObject->TryGetStringField(TEXT("type"), SearchLabel.Type);
            
            const TSharedPtr<FJsonObject>* FeaturesObject;
            if (JsonObject->TryGetObjectField(TEXT("features"), FeaturesObject))
            {
                for (const auto& Pair : (*FeaturesObject)->Values)
                {
                    FString FeatureValue;
                    if (Pair.Value->TryGetString(FeatureValue))
                    {
                        SearchLabel.Features.Add(Pair.Key, FeatureValue);
                    }
                }
            }
            
            UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Parsed target label - Class=%s, Type=%s, Features=%d"),
                *Agent->AgentName, *SearchLabel.Class, *SearchLabel.Type, SearchLabel.Features.Num());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[ProcessSearch] %s: Failed to parse TargetJson: %s"),
                *Agent->AgentName, *Cmd->Params.TargetJson);
        }
    }
    
    // 从 TargetFeatures 补充特征 (向后兼容)
    if (Cmd && Cmd->Params.TargetFeatures.Num() > 0)
    {
        for (const auto& Pair : Cmd->Params.TargetFeatures)
        {
            if (!SearchLabel.Features.Contains(Pair.Key))
            {
                SearchLabel.Features.Add(Pair.Key, Pair.Value);
            }
        }
    }
    
    // 存储到 Params 供后续使用
    Params.SearchTarget.Class = SearchLabel.Class;
    Params.SearchTarget.Type = SearchLabel.Type;
    Params.SearchTarget.Features = SearchLabel.Features;
    
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
        
        // 转换为 FMASceneQuery 使用的 FMASemanticLabel
        FMASemanticLabel UE5Label;
        UE5Label.Class = SearchLabel.Class;
        UE5Label.Type = SearchLabel.Type;
        UE5Label.Features = SearchLabel.Features;
        
        TArray<FMASceneQueryResult> Results = FMASceneQuery::FindObjectsInBoundary(
            World, UE5Label, Params.SearchBoundary);
        
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: UE5 scene query found %d objects"),
            *Agent->AgentName, Results.Num());
        
        for (const FMASceneQueryResult& Result : Results)
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
    // Step 1: 解析目标机器人名称
    //=========================================================================
    FString TargetRobotName;
    
    // 从 Cmd 参数中获取目标机器人名称
    if (Cmd)
    {
        // 优先从 TargetEntity 获取
        if (!Cmd->Params.TargetEntity.IsEmpty())
        {
            TargetRobotName = Cmd->Params.TargetEntity;
        }
        // 其次从 TargetToken 获取
        else if (!Cmd->Params.TargetToken.IsEmpty())
        {
            TargetRobotName = Cmd->Params.TargetToken;
        }
        // 最后尝试从 TargetJson 解析
        else if (!Cmd->Params.TargetJson.IsEmpty())
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Cmd->Params.TargetJson);
            
            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                const TSharedPtr<FJsonObject>* FeaturesObject;
                if (JsonObject->TryGetObjectField(TEXT("features"), FeaturesObject))
                {
                    // 优先读取 label，向后兼容 name
                    if (!(*FeaturesObject)->TryGetStringField(TEXT("label"), TargetRobotName))
                    {
                        (*FeaturesObject)->TryGetStringField(TEXT("name"), TargetRobotName);
                    }
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Target robot name = '%s'"),
        *Agent->AgentName, *TargetRobotName);
    
    //=========================================================================
    // Step 2: 使用场景图查询查找目标机器人
    //=========================================================================
    UWorld* World = Agent->GetWorld();
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    AMACharacter* TargetRobot = nullptr;
    bool bFoundInSceneGraph = false;
    
    if (SceneGraphManager && !TargetRobotName.IsEmpty())
    {
        // 构建语义标签查询机器人
        FMASemanticLabel RobotLabel = FMASemanticLabel::MakeRobot(TargetRobotName);
        
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode RobotNode = FMASceneGraphQuery::FindNodeByLabel(AllNodes, RobotLabel);
        
        if (RobotNode.IsValid() && RobotNode.IsRobot())
        {
            UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Found robot '%s' in scene graph at (%.0f, %.0f, %.0f)"),
                *Agent->AgentName, *RobotNode.Label, RobotNode.Center.X, RobotNode.Center.Y, RobotNode.Center.Z);
            
            // 缓存到反馈上下文
            Context.FollowTargetRobotName = RobotNode.Label.IsEmpty() ? RobotNode.Id : RobotNode.Label;
            Context.bFollowTargetFound = true;
            Context.TargetName = Context.FollowTargetRobotName;
            
            // 计算初始距离
            Context.FollowTargetDistance = FVector::Dist(Agent->GetActorLocation(), RobotNode.Center);
            
            bFoundInSceneGraph = true;
            
            // 尝试从 UE5 场景中获取实际的 Actor 引用
            TargetRobot = FMASceneQuery::FindRobotByName(World, TargetRobotName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[ProcessFollow] %s: Robot '%s' not found in scene graph"),
                *Agent->AgentName, *TargetRobotName);
        }
    }
    
    //=========================================================================
    // Step 3: 回退到 UE5 场景查询 (如果场景图未找到)
    //=========================================================================
    if (!bFoundInSceneGraph && !TargetRobotName.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Falling back to UE5 scene query for robot '%s'"),
            *Agent->AgentName, *TargetRobotName);
        
        TargetRobot = FMASceneQuery::FindRobotByName(World, TargetRobotName);
        
        if (TargetRobot)
        {
            Context.FollowTargetRobotName = TargetRobot->AgentName;
            Context.bFollowTargetFound = true;
            Context.TargetName = TargetRobot->AgentName;
            Context.FollowTargetDistance = FVector::Dist(Agent->GetActorLocation(), TargetRobot->GetActorLocation());
            
            UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Found robot '%s' via UE5 query at distance %.0f"),
                *Agent->AgentName, *TargetRobot->AgentName, Context.FollowTargetDistance);
        }
    }
    
    //=========================================================================
    // Step 4: 处理未找到目标的情况
    //=========================================================================
    if (!TargetRobot)
    {
        Context.bFollowTargetFound = false;
        Context.FollowErrorReason = FString::Printf(TEXT("target robot '%s' not found"), *TargetRobotName);
        
        UE_LOG(LogTemp, Warning, TEXT("[ProcessFollow] %s: %s"),
            *Agent->AgentName, *Context.FollowErrorReason);
    }
    else
    {
        // 设置技能参数
        Params.FollowTarget = TargetRobot;
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
    // 使用 MASceneGraphQuery 替换 FMASceneQuery
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
                
                // 尝试从 UE5 场景中获取实际的 Actor 引用
                FMASceneQueryResult UE5Result = FMASceneQuery::FindNearestObject(World, Label1, Agent->GetActorLocation());
                if (UE5Result.bFound)
                {
                    SearchResults.Object1Actor = UE5Result.Actor;
                }
                
                bFoundInSceneGraph = true;
            }
        }
        
        // 回退到 UE5 场景查询
        if (!bFoundInSceneGraph)
        {
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Falling back to UE5 scene query for target"), *Agent->AgentName);
            
            FMASceneQueryResult Result1 = FMASceneQuery::FindNearestObject(World, Label1, Agent->GetActorLocation());
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
    // 使用 MASceneGraphQuery 替换 FMASceneQuery
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
                    
                    // 尝试从 UE5 场景中获取实际的 Actor 引用
                    AMACharacter* Robot = FMASceneQuery::FindRobotByName(World, RobotName);
                    if (Robot)
                    {
                        SearchResults.Object2Actor = Robot;
                    }
                    
                    bFoundInSceneGraph = true;
                }
            }
            
            // 回退到 UE5 场景查询
            if (!bFoundInSceneGraph)
            {
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Falling back to UE5 scene query for UGV '%s'"), 
                    *Agent->AgentName, *RobotName);
                
                AMACharacter* Robot = FMASceneQuery::FindRobotByName(World, RobotName);
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
                    
                    // 尝试从 UE5 场景中获取实际的 Actor 引用
                    FMASceneQueryResult UE5Result = FMASceneQuery::FindNearestObject(World, Label2, Agent->GetActorLocation());
                    if (UE5Result.bFound)
                    {
                        SearchResults.Object2Actor = UE5Result.Actor;
                    }
                    
                    bFoundInSceneGraph = true;
                }
            }
            
            // 回退到 UE5 场景查询
            if (!bFoundInSceneGraph)
            {
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Falling back to UE5 scene query for surface_target"), *Agent->AgentName);
                
                FMASceneQueryResult Result2 = FMASceneQuery::FindNearestObject(World, Label2, Agent->GetActorLocation());
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
    bool bFromSceneGraph = false;
    FString HomeLandmark;
    
    //=========================================================================
    // Step 1: 从场景图获取机器人初始位置
    //=========================================================================
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        
        // 使用机器人名称查找场景图中的机器人节点
        FMASemanticLabel RobotLabel = FMASemanticLabel::MakeRobot(Agent->AgentName);
        FMASceneGraphNode RobotNode = FMASceneGraphQuery::FindNodeByLabel(AllNodes, RobotLabel);
        
        if (RobotNode.IsValid() && RobotNode.IsRobot())
        {
            // 尝试从节点的 Features 中获取初始位置
            // 格式: features["initial_x"], features["initial_y"], features["initial_z"]
            FString InitialX = RobotNode.Features.FindRef(TEXT("initial_x"));
            FString InitialY = RobotNode.Features.FindRef(TEXT("initial_y"));
            FString InitialZ = RobotNode.Features.FindRef(TEXT("initial_z"));
            
            if (!InitialX.IsEmpty() && !InitialY.IsEmpty())
            {
                HomeLocation.X = FCString::Atof(*InitialX);
                HomeLocation.Y = FCString::Atof(*InitialY);
                HomeLocation.Z = InitialZ.IsEmpty() ? 0.f : FCString::Atof(*InitialZ);
                bFromSceneGraph = true;
                
                UE_LOG(LogTemp, Log, TEXT("[ProcessReturnHome] %s: Got initial position from scene graph: (%.0f, %.0f, %.0f)"),
                    *Agent->AgentName, HomeLocation.X, HomeLocation.Y, HomeLocation.Z);
            }
            else
            {
                // 如果没有初始位置特征，使用节点的 Center 作为参考
                // 但优先使用 Agent 的 InitialLocation
                UE_LOG(LogTemp, Verbose, TEXT("[ProcessReturnHome] %s: No initial position in scene graph, using Agent.InitialLocation"),
                    *Agent->AgentName);
            }
        }
        
        // 查找家位置附近的地标
        FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(HomeLocation, 2000.f);
        if (NearestLandmark.IsValid())
        {
            HomeLandmark = NearestLandmark.Label;
            
            UE_LOG(LogTemp, Verbose, TEXT("[ProcessReturnHome] %s: Home location near landmark '%s'"),
                *Agent->AgentName, *HomeLandmark);
        }
    }
    
    //=========================================================================
    // Step 2: 如果有传入 HomeLocation，使用传入值
    //=========================================================================
    if (Cmd && Cmd->Params.bHasDestPosition)
    {
        HomeLocation = Cmd->Params.DestPosition;
        bFromSceneGraph = false;  // 使用传入值覆盖
        
        // 重新查找附近地标
        if (SceneGraphManager)
        {
            FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(HomeLocation, 2000.f);
            if (NearestLandmark.IsValid())
            {
                HomeLandmark = NearestLandmark.Label;
            }
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
    Context.bHomeLocationFromSceneGraph = bFromSceneGraph;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessReturnHome] %s: Home location=(%.0f, %.0f, %.0f), land_height=%.0f, from_scene_graph=%s, landmark=%s"),
        *Agent->AgentName, HomeLocation.X, HomeLocation.Y, HomeLocation.Z, LandHeight, 
        bFromSceneGraph ? TEXT("true") : TEXT("false"), *HomeLandmark);
}
