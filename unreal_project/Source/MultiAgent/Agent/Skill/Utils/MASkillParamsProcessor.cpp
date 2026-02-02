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
#include "../../../Environment/Entity/MAChargingStation.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
                *Agent->AgentLabel, *ObjectId, OutFoundLocation.X, OutFoundLocation.Y, OutFoundLocation.Z);
            
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[MatchTargetObject] %s: object_id '%s' not found in scene graph"),
                *Agent->AgentLabel, *ObjectId);
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
                *Agent->AgentLabel, *SemanticTarget.Class, *SemanticTarget.Type, Distance);
            
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[MatchTargetObject] %s: No matching target found within radius %.0f (Class=%s, Type=%s)"),
                *Agent->AgentLabel, SearchRadius, *SemanticTarget.Class, *SemanticTarget.Type);
        }
    }
    
    //=========================================================================
    // 回退方案：UE5 场景查询
    //=========================================================================
    if (!bFoundInSceneGraph && !SemanticTarget.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("[MatchTargetObject] %s: Falling back to UE5 scene query"),
            *Agent->AgentLabel);
        
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
                *Agent->AgentLabel, *Result.Name, Result.Location.X, Result.Location.Y, Result.Location.Z);
            
            return true;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MatchTargetObject] %s: Target not found"),
        *Agent->AgentLabel);
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
    
    //=========================================================================
    // 特殊处理: event 类型转换
    // 输入格式: {"class": "event", "event_type": "illegal_parking", "conf_ge": 0.85, "persist_ge_s": 2.0}
    // 转换为:   {"class": "prop", "type": "vehicle", "features": {"illegal_parking": "true"}, ...}
    //=========================================================================
    if (OutTarget.Class.Equals(TEXT("event"), ESearchCase::IgnoreCase))
    {
        FString EventType;
        if (JsonObject->TryGetStringField(TEXT("event_type"), EventType))
        {
            // 转换 class 和 type
            OutTarget.Class = TEXT("prop");
            OutTarget.Type = TEXT("vehicle");
            
            // 将 event_type 作为 feature，值为 "true"
            OutTarget.Features.Add(EventType, TEXT("true"));
            
            UE_LOG(LogTemp, Log, TEXT("[ParseSemanticTargetFromJson] Converted event type '%s' to prop/vehicle with feature"), *EventType);
        }
    }
    else
    {
        // 标准格式: 解析 features 字段 (键值对)
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
                else if (Pair.Value->Type == EJson::Boolean)
                {
                    // 支持布尔类型的 feature 值
                    OutTarget.Features.Add(Pair.Key, Pair.Value->AsBool() ? TEXT("true") : TEXT("false"));
                }
                else if (Pair.Value->Type == EJson::Number)
                {
                    // 支持数字类型的 feature 值
                    OutTarget.Features.Add(Pair.Key, FString::Printf(TEXT("%g"), Pair.Value->AsNumber()));
                }
            }
        }
        
        // 也支持 "feature" 单数形式（兼容性）
        const TSharedPtr<FJsonObject>* FeatureObject;
        if (JsonObject->TryGetObjectField(TEXT("feature"), FeatureObject))
        {
            for (const auto& Pair : (*FeatureObject)->Values)
            {
                FString FeatureValue;
                if (Pair.Value->TryGetString(FeatureValue))
                {
                    if (!OutTarget.Features.Contains(Pair.Key))
                    {
                        OutTarget.Features.Add(Pair.Key, FeatureValue);
                    }
                }
                else if (Pair.Value->Type == EJson::Boolean)
                {
                    if (!OutTarget.Features.Contains(Pair.Key))
                    {
                        OutTarget.Features.Add(Pair.Key, Pair.Value->AsBool() ? TEXT("true") : TEXT("false"));
                    }
                }
                else if (Pair.Value->Type == EJson::Number)
                {
                    if (!OutTarget.Features.Contains(Pair.Key))
                    {
                        OutTarget.Features.Add(Pair.Key, FString::Printf(TEXT("%g"), Pair.Value->AsNumber()));
                    }
                }
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
    
    // //=========================================================================
    // // UGV 特殊处理：只调整 Z 分量，保持 X、Y 不变
    // //=========================================================================
    // if (bIsUGV)
    // {
    //     // 只调整 Z 到地面高度
    //     float GroundZ = GetGroundHeightAt(TargetLocation.X, TargetLocation.Y);
    //     TargetLocation.Z = GroundZ;
        
    //     UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s (UGV): Target (%.0f, %.0f, %.0f) - only Z adjusted to ground"),
    //         *Agent->AgentLabel, TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
        
    //     Params.TargetLocation = TargetLocation;
    //     return;
    // }
    
    // 安全距离常量
    const float SafetyMargin = 500.f;      // 飞行器与障碍物的安全距离
    const float GroundOffset = 500.f;      // 地面机器人与障碍物边缘的距离
    
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
                *Agent->AgentLabel, TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
            
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
                            *Agent->AgentLabel, TargetLocation.Z, *Building.Label);
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
                            *Agent->AgentLabel, TargetLocation.X, TargetLocation.Y, *Building.Label);
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
    //                 *Agent->AgentLabel, *HitActor->GetName(), Origin.X, Origin.Y, Origin.Z, BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
                
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
    //                                 *Agent->AgentLabel, TargetLocation.Z);
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
    //                             *Agent->AgentLabel, TargetLocation.X, TargetLocation.Y);
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
    //         *Agent->AgentLabel, TargetLocation.Z);
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
                *Agent->AgentLabel, *NearestLandmark.Label, *NearestLandmark.Type, Context.NearbyLandmarkDistance);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Final target location (%.0f, %.0f, %.0f)"),
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
        ParseSemanticTargetFromJson(TargetJsonStr, Params.SearchTarget);
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Parsed target - Class=%s, Type=%s, Features=%d"),
            *Agent->AgentLabel, *Params.SearchTarget.Class, *Params.SearchTarget.Type, Params.SearchTarget.Features.Num());
    }
    
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
    FMASearchResults& SearchResults = SkillComp->GetSearchResultsMutable();
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
            ParseSemanticTargetFromJson(TargetJsonStr, Params.PlaceObject1);
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Parsed target - Class=%s, Type=%s"), 
                *Agent->AgentLabel, *Params.PlaceObject1.Class, *Params.PlaceObject1.Type);
        }
        
        // 解析 surface_target JSON
        FString SurfaceTargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("surface_target"), SurfaceTargetJsonStr))
        {
            ParseSemanticTargetFromJson(SurfaceTargetJsonStr, Params.PlaceObject2);
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
    EPlaceMode PlaceMode = DeterminePlaceMode(Params.PlaceObject2);
    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: PlaceMode=%d"), *Agent->AgentLabel, (int32)PlaceMode);
    
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
            FMASceneGraphNode Object1Node = FMASceneGraphQuery::FindNodeByLabel(AllNodes, Label1);
            
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
                FMASceneGraphNode RobotNode = FMASceneGraphQuery::FindNodeByLabel(AllNodes, RobotSemanticLabel);
                
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
                FMASceneGraphNode Object2Node = FMASceneGraphQuery::FindNodeByLabel(AllNodes, Label2);
                
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
                *Agent->AgentLabel);
            
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
                    *Agent->AgentLabel, *NearestSafeNode.Label, *GroundType, MinDistance);
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
    UGameInstance* GameInstance = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    
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
    UWorld* World = Agent->GetWorld();
    
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
            ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
    }
    
    // 构建语义标签
    FMASemanticLabel TargetLabel;
    TargetLabel.Class = Target.Class;
    TargetLabel.Type = Target.Type;
    TargetLabel.Features = Target.Features;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: object_id='%s', target(Class=%s, Type=%s)"),
        *Agent->AgentLabel, *ObjectId, *Target.Class, *Target.Type);
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> ObjectId -> 场景图语义标签
    //=========================================================================
    TWeakObjectPtr<AActor> TargetActor;
    FString FoundName;
    FString FoundId;
    FVector FoundLocation = FVector::ZeroVector;
    
    // 方式1: 通过语义标签直接查找 UE5 Actor
    if (!TargetLabel.IsEmpty())
    {
        FMAUESceneQueryResult Result = FMAUESceneQuery::FindObjectByLabel(World, TargetLabel);
        if (Result.bFound && Result.Actor)
        {
            TargetActor = Result.Actor;
            FoundName = Result.Name;
            FoundLocation = Result.Location;
            UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: Found target '%s' by semantic label (UE5)"),
                *Agent->AgentLabel, *FoundName);
        }
    }
    
    // 方式2: 通过场景图查找节点，再获取 Actor
    if (!TargetActor.IsValid())
    {
        UGameInstance* GameInstance = World->GetGameInstance();
        UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
        
        if (SceneGraphManager)
        {
            TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
            FMASceneGraphNode TargetNode;
            
            // 优先用 ObjectId 查找
            if (!ObjectId.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, ObjectId);
            }
            // 回退用语义标签查找
            if (!TargetNode.IsValid() && !TargetLabel.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByLabel(AllNodes, TargetLabel);
            }
            
            if (TargetNode.IsValid())
            {
                FoundName = TargetNode.Label.IsEmpty() ? TargetNode.Id : TargetNode.Label;
                FoundId = TargetNode.Id;
                FoundLocation = TargetNode.Center;
                
                // 通过 GUID 查找 Actor
                if (!TargetNode.Guid.IsEmpty())
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.Guid);
                }
                else if (TargetNode.GuidArray.Num() > 0)
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.GuidArray[0]);
                }
                
                if (TargetActor.IsValid())
                {
                    UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: Found target '%s' by scene graph"),
                        *Agent->AgentLabel, *FoundName);
                }
            }
        }
    }
    
    //=========================================================================
    // Step 3: 计算机器人与目标的水平距离（米）
    //=========================================================================
    float RobotTargetDistanceM = -1.f;
    if (TargetActor.IsValid() || !FoundLocation.IsZero())
    {
        FVector TargetLoc = TargetActor.IsValid() ? TargetActor->GetActorLocation() : FoundLocation;
        FVector RobotLocation = Agent->GetActorLocation();
        float DistanceCm = FVector::Dist2D(RobotLocation, TargetLoc);
        RobotTargetDistanceM = DistanceCm / 100.f;
    }
    
    //=========================================================================
    // Step 4: 保存到参数和反馈上下文
    //=========================================================================
    Params.CommonTargetObjectId = FoundId;
    Params.CommonTarget = Target;
    Params.PhotoTargetActor = TargetActor;
    
    Context.bPhotoTargetFound = TargetActor.IsValid() || !FoundLocation.IsZero();
    Context.PhotoTargetName = FoundName;
    Context.PhotoTargetId = FoundId;
    Context.TargetLocation = FoundLocation.IsZero() && TargetActor.IsValid() ? TargetActor->GetActorLocation() : FoundLocation;
    Context.PhotoRobotTargetDistance = RobotTargetDistanceM;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: Target found=%s, name='%s', id='%s', distance=%.2fm, Actor=%s"),
        *Agent->AgentLabel, 
        Context.bPhotoTargetFound ? TEXT("true") : TEXT("false"), 
        *FoundName, *FoundId, RobotTargetDistanceM,
        TargetActor.IsValid() ? TEXT("valid") : TEXT("null"));
}

void FMASkillParamsProcessor::ProcessBroadcast(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    UWorld* World = Agent->GetWorld();
    
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
            ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
        
        ParamsJson->TryGetStringField(TEXT("message"), Message);
    }
    
    // 构建语义标签
    FMASemanticLabel TargetLabel;
    TargetLabel.Class = Target.Class;
    TargetLabel.Type = Target.Type;
    TargetLabel.Features = Target.Features;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: object_id='%s', target(Class=%s, Type=%s), message='%s'"),
        *Agent->AgentLabel, *ObjectId, *Target.Class, *Target.Type, *Message);
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> ObjectId -> 场景图语义标签
    //=========================================================================
    TWeakObjectPtr<AActor> TargetActor;
    FString FoundName;
    FString FoundId;
    FVector FoundLocation = FVector::ZeroVector;
    
    // 方式1: 通过语义标签直接查找 UE5 Actor
    if (!TargetLabel.IsEmpty())
    {
        FMAUESceneQueryResult Result = FMAUESceneQuery::FindObjectByLabel(World, TargetLabel);
        if (Result.bFound && Result.Actor)
        {
            TargetActor = Result.Actor;
            FoundName = Result.Name;
            FoundLocation = Result.Location;
            UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: Found target '%s' by semantic label (UE5)"),
                *Agent->AgentLabel, *FoundName);
        }
    }
    
    // 方式2: 通过场景图查找节点，再获取 Actor
    if (!TargetActor.IsValid())
    {
        UGameInstance* GameInstance = World->GetGameInstance();
        UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
        
        if (SceneGraphManager)
        {
            TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
            FMASceneGraphNode TargetNode;
            
            // 优先用 ObjectId 查找
            if (!ObjectId.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, ObjectId);
            }
            // 回退用语义标签查找
            if (!TargetNode.IsValid() && !TargetLabel.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByLabel(AllNodes, TargetLabel);
            }
            
            if (TargetNode.IsValid())
            {
                FoundName = TargetNode.Label.IsEmpty() ? TargetNode.Id : TargetNode.Label;
                FoundId = TargetNode.Id;
                FoundLocation = TargetNode.Center;
                
                // 通过 GUID 查找 Actor
                if (!TargetNode.Guid.IsEmpty())
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.Guid);
                }
                else if (TargetNode.GuidArray.Num() > 0)
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.GuidArray[0]);
                }
                
                if (TargetActor.IsValid())
                {
                    UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: Found target '%s' by scene graph"),
                        *Agent->AgentLabel, *FoundName);
                }
            }
        }
    }
    
    //=========================================================================
    // Step 3: 计算机器人与目标的水平距离（米）
    //=========================================================================
    float RobotTargetDistanceM = -1.f;
    if (TargetActor.IsValid() || !FoundLocation.IsZero())
    {
        FVector TargetLoc = TargetActor.IsValid() ? TargetActor->GetActorLocation() : FoundLocation;
        FVector RobotLocation = Agent->GetActorLocation();
        float DistanceCm = FVector::Dist2D(RobotLocation, TargetLoc);
        RobotTargetDistanceM = DistanceCm / 100.f;
    }
    
    //=========================================================================
    // Step 4: 保存到参数和反馈上下文
    //=========================================================================
    Params.CommonTargetObjectId = FoundId;
    Params.CommonTarget = Target;
    Params.BroadcastMessage = Message;
    Params.BroadcastTargetActor = TargetActor;
    
    Context.bBroadcastTargetFound = TargetActor.IsValid() || !FoundLocation.IsZero();
    Context.BroadcastTargetName = FoundName;
    Context.BroadcastMessage = Message;
    Context.TargetLocation = FoundLocation.IsZero() && TargetActor.IsValid() ? TargetActor->GetActorLocation() : FoundLocation;
    Context.BroadcastRobotTargetDistance = RobotTargetDistanceM;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: Target found=%s, name='%s', message='%s', distance=%.2fm, Actor=%s"),
        *Agent->AgentLabel, 
        Context.bBroadcastTargetFound ? TEXT("true") : TEXT("false"), 
        *FoundName, *Message, RobotTargetDistanceM,
        TargetActor.IsValid() ? TEXT("valid") : TEXT("null"));
}

void FMASkillParamsProcessor::ProcessHandleHazard(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    UWorld* World = Agent->GetWorld();
    
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
            ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
    }
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> GUID -> 场景图节点
    //=========================================================================
    TWeakObjectPtr<AActor> TargetActor;
    FString FoundId;
    FString FoundName;
    FVector FoundLocation = FVector::ZeroVector;
    
    // 构建语义标签
    FMASemanticLabel TargetLabel;
    TargetLabel.Class = Target.Class;
    TargetLabel.Type = Target.Type;
    TargetLabel.Features = Target.Features;
    
    // 方式1: 通过语义标签查找 Actor
    if (!TargetLabel.IsEmpty())
    {
        FMAUESceneQueryResult Result = FMAUESceneQuery::FindObjectByLabel(World, TargetLabel);
        if (Result.bFound && Result.Actor)
        {
            TargetActor = Result.Actor;
            FoundName = Result.Name;
            FoundLocation = Result.Location;
            UE_LOG(LogTemp, Log, TEXT("[ProcessHandleHazard] %s: Found target '%s' by semantic label"),
                *Agent->AgentLabel, *FoundName);
        }
    }
    
    // 方式2: 通过场景图查找节点，再获取 Actor
    if (!TargetActor.IsValid())
    {
        UGameInstance* GameInstance = World->GetGameInstance();
        UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
        
        if (SceneGraphManager)
        {
            TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
            FMASceneGraphNode TargetNode;
            
            // 优先用 ObjectId 查找
            if (!ObjectId.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, ObjectId);
            }
            // 回退用语义标签查找
            if (!TargetNode.IsValid() && !TargetLabel.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByLabel(AllNodes, TargetLabel);
            }
            
            if (TargetNode.IsValid())
            {
                FoundId = TargetNode.Id;
                FoundName = TargetNode.Label.IsEmpty() ? TargetNode.Id : TargetNode.Label;
                FoundLocation = TargetNode.Center;
                
                // 通过 GUID 查找 Actor
                if (!TargetNode.Guid.IsEmpty())
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.Guid);
                }
                else if (TargetNode.GuidArray.Num() > 0)
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.GuidArray[0]);
                }
                
                if (TargetActor.IsValid())
                {
                    UE_LOG(LogTemp, Log, TEXT("[ProcessHandleHazard] %s: Found target '%s' by scene graph"),
                        *Agent->AgentLabel, *FoundName);
                }
            }
        }
    }
    
    //=========================================================================
    // Step 3: 保存结果
    //=========================================================================
    Params.CommonTargetObjectId = FoundId;
    Params.CommonTarget = Target;
    Params.HazardTargetActor = TargetActor;
    
    Context.bHazardTargetFound = TargetActor.IsValid() || !FoundLocation.IsZero();
    Context.HazardTargetName = FoundName;
    Context.HazardTargetId = FoundId;
    Context.TargetLocation = FoundLocation;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessHandleHazard] %s: Target found=%s, name='%s', Actor=%s"),
        *Agent->AgentLabel, Context.bHazardTargetFound ? TEXT("true") : TEXT("false"), 
        *FoundName, TargetActor.IsValid() ? TEXT("valid") : TEXT("null"));
}

void FMASkillParamsProcessor::ProcessGuide(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    UWorld* World = Agent->GetWorld();
    
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
            ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
        MAParamsHelper::ExtractDestPosition(ParamsJson, Destination);
    }
    
    // 构建语义标签
    FMASemanticLabel TargetLabel;
    TargetLabel.Class = Target.Class;
    TargetLabel.Type = Target.Type;
    TargetLabel.Features = Target.Features;
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> ObjectId -> 场景图语义标签
    //=========================================================================
    TWeakObjectPtr<AActor> TargetActor;
    FString FoundName;
    FString FoundId;
    FVector FoundLocation = FVector::ZeroVector;
    
    // 方式1: 通过语义标签直接查找 UE5 Actor
    if (!TargetLabel.IsEmpty())
    {
        FMAUESceneQueryResult Result = FMAUESceneQuery::FindObjectByLabel(World, TargetLabel);
        if (Result.bFound && Result.Actor)
        {
            TargetActor = Result.Actor;
            FoundName = Result.Name;
            FoundLocation = Result.Location;
            UE_LOG(LogTemp, Log, TEXT("[ProcessGuide] %s: Found target '%s' by semantic label (UE5)"),
                *Agent->AgentLabel, *FoundName);
        }
    }
    
    // 方式2: 通过场景图查找节点，再获取 Actor
    if (!TargetActor.IsValid())
    {
        UGameInstance* GameInstance = World->GetGameInstance();
        UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
        
        if (SceneGraphManager)
        {
            TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
            FMASceneGraphNode TargetNode;
            
            // 优先用 ObjectId 查找
            if (!ObjectId.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, ObjectId);
            }
            // 回退用语义标签查找
            if (!TargetNode.IsValid() && !TargetLabel.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByLabel(AllNodes, TargetLabel);
            }
            
            if (TargetNode.IsValid())
            {
                FoundName = TargetNode.Label.IsEmpty() ? TargetNode.Id : TargetNode.Label;
                FoundId = TargetNode.Id;
                FoundLocation = TargetNode.Center;
                
                // 通过 GUID 查找 Actor
                if (!TargetNode.Guid.IsEmpty())
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.Guid);
                }
                else if (TargetNode.GuidArray.Num() > 0)
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.GuidArray[0]);
                }
                
                if (TargetActor.IsValid())
                {
                    UE_LOG(LogTemp, Log, TEXT("[ProcessGuide] %s: Found target '%s' by scene graph"),
                        *Agent->AgentLabel, *FoundName);
                }
            }
        }
    }
    
    //=========================================================================
    // Step 3: 保存结果
    //=========================================================================
    Context.bGuideTargetFound = TargetActor.IsValid();
    Context.GuideTargetName = FoundName;
    Context.GuideTargetId = FoundId;
    Context.GuideDestination = Destination;
    Context.TargetLocation = FoundLocation;
    
    Params.GuideTargetObjectId = FoundId;
    Params.GuideTarget = Target;
    Params.GuideDestination = Destination;
    Params.GuideTargetActor = TargetActor;
    
    UE_LOG(LogTemp, Log, TEXT("[ProcessGuide] %s: Target found=%s, name='%s', destination=(%.0f, %.0f, %.0f)"),
        *Agent->AgentLabel, TargetActor.IsValid() ? TEXT("true") : TEXT("false"), *FoundName,
        Destination.X, Destination.Y, Destination.Z);
}

void FMASkillParamsProcessor::ProcessFollow(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp || !Cmd) return;
    
    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;
    
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    UWorld* World = Agent->GetWorld();
    
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
            ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
    }
    
    // 构建语义标签
    FMASemanticLabel TargetLabel;
    TargetLabel.Class = Target.Class;
    TargetLabel.Type = Target.Type;
    TargetLabel.Features = Target.Features;
    
    //=========================================================================
    // Step 2: 查找目标 Actor
    // 优先级: 语义标签 -> ObjectId -> 场景图语义标签
    //=========================================================================
    AActor* TargetActor = nullptr;
    FString FoundName;
    FVector FoundLocation = FVector::ZeroVector;
    
    // 方式1: 通过语义标签直接查找 UE5 Actor
    if (!TargetLabel.IsEmpty())
    {
        FMAUESceneQueryResult Result = FMAUESceneQuery::FindObjectByLabel(World, TargetLabel);
        if (Result.bFound && Result.Actor)
        {
            TargetActor = Result.Actor;
            FoundName = Result.Name;
            FoundLocation = Result.Location;
            UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Found target '%s' by semantic label (UE5)"),
                *Agent->AgentLabel, *FoundName);
        }
    }
    
    // 方式2: 通过场景图查找节点，再获取 Actor
    if (!TargetActor)
    {
        UGameInstance* GameInstance = World->GetGameInstance();
        UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
        
        if (SceneGraphManager)
        {
            TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
            FMASceneGraphNode TargetNode;
            
            // 优先用 ObjectId 查找
            if (!ObjectId.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, ObjectId);
            }
            // 回退用语义标签查找
            if (!TargetNode.IsValid() && !TargetLabel.IsEmpty())
            {
                TargetNode = FMASceneGraphQuery::FindNodeByLabel(AllNodes, TargetLabel);
            }
            
            if (TargetNode.IsValid())
            {
                FoundName = TargetNode.Label.IsEmpty() ? TargetNode.Id : TargetNode.Label;
                FoundLocation = TargetNode.Center;
                
                // 通过 GUID 查找 Actor
                if (!TargetNode.Guid.IsEmpty())
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.Guid);
                }
                else if (TargetNode.GuidArray.Num() > 0)
                {
                    TargetActor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.GuidArray[0]);
                }
                
                if (TargetActor)
                {
                    UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Found target '%s' by scene graph"),
                        *Agent->AgentLabel, *FoundName);
                }
            }
        }
    }
    
    //=========================================================================
    // Step 3: 保存结果
    //=========================================================================
    Context.bFollowTargetFound = (TargetActor != nullptr);
    Context.FollowTargetName = FoundName;
    Context.FollowTargetId = ObjectId;
    Context.TargetName = FoundName;
    Context.TargetLocation = FoundLocation;
    
    if (TargetActor)
    {
        Params.FollowTarget = TargetActor;
        Context.FollowTargetDistance = FVector::Dist(Agent->GetActorLocation(), TargetActor->GetActorLocation());
        UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Target '%s' at distance %.0f"),
            *Agent->AgentLabel, *FoundName, Context.FollowTargetDistance);
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