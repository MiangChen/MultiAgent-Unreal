// MASkillParamsProcessor.cpp
// 技能参数处理器实现

#include "MASkillParamsProcessor.h"
#include "MASceneQuery.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Core/Comm/MACommTypes.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Utils/MAWorldQuery.h"
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

EPlaceMode FMASkillParamsProcessor::DeterminePlaceMode(const FMASemanticTarget& Object2)
{
    // Requirements: 1.3 - 检查是否为 UGV 机器人
    // 如果 type 或 features 中包含 "UGV" 或 "ugv"，则为装货模式
    if (Object2.Type.Contains(TEXT("UGV"), ESearchCase::IgnoreCase) ||
        Object2.Type.Contains(TEXT("ugv"), ESearchCase::IgnoreCase))
    {
        return EPlaceMode::LoadToUGV;
    }
    
    // 检查 features 中是否有 UGV 相关标识
    for (const auto& Pair : Object2.Features)
    {
        if (Pair.Key.Contains(TEXT("UGV"), ESearchCase::IgnoreCase) ||
            Pair.Value.Contains(TEXT("UGV"), ESearchCase::IgnoreCase))
        {
            return EPlaceMode::LoadToUGV;
        }
    }
    
    // 检查 class 是否为 robot
    if (Object2.Class.Equals(TEXT("robot"), ESearchCase::IgnoreCase))
    {
        return EPlaceMode::LoadToUGV;
    }
    
    // Requirements: 1.4 - 检查是否为地面放置模式
    // 如果 class 包含 "ground"，则为卸货模式
    if (Object2.Class.Contains(TEXT("ground"), ESearchCase::IgnoreCase) ||
        Object2.IsGround())
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
    
    if (!Cmd->Params.bHasDestPosition) return;
    
    FVector TargetLocation = Cmd->Params.DestPosition;
    bool bIsFlying = (Agent->AgentType == EMAAgentType::UAV || Agent->AgentType == EMAAgentType::FixedWingUAV);
    
    // 安全距离常量
    const float SafetyMargin = 500.f;      // 飞行器与障碍物的安全距离
    const float GroundOffset = 200.f;      // 地面机器人与障碍物边缘的距离
    
    // ========== 方式1: 如果指定了目标实体，使用实体信息精确调整 ==========
    if (!Cmd->Params.TargetEntity.IsEmpty())
    {
        FMAEntityNode TargetEntity = FMAWorldQuery::GetEntityByName(Agent->GetWorld(), Cmd->Params.TargetEntity);
        
        if (!TargetEntity.Id.IsEmpty())
        {
            float EntityX = FCString::Atof(*TargetEntity.Properties.FindRef(TEXT("x")));
            float EntityY = FCString::Atof(*TargetEntity.Properties.FindRef(TEXT("y")));
            float EntityZ = FCString::Atof(*TargetEntity.Properties.FindRef(TEXT("z")));
            float SizeX = FCString::Atof(*TargetEntity.Properties.FindRef(TEXT("size_x")));
            float SizeY = FCString::Atof(*TargetEntity.Properties.FindRef(TEXT("size_y")));
            float SizeZ = FCString::Atof(*TargetEntity.Properties.FindRef(TEXT("size_z")));
            
            float EntityTopZ = EntityZ + SizeZ / 2.f;
            
            if (bIsFlying)
            {
                // 飞行器：确保高度高于实体顶部 + 安全距离
                float MinAltitude = EntityTopZ + SafetyMargin;
                if (TargetLocation.Z < MinAltitude)
                {
                    TargetLocation.Z = MinAltitude;
                }
            }
            else
            {
                // 地面机器人：调整到实体边缘外侧
                float EdgeOffset = FMath::Max(SizeX, SizeY) / 2.f + GroundOffset;
                
                FVector2D ToTarget(TargetLocation.X - EntityX, TargetLocation.Y - EntityY);
                if (ToTarget.IsNearlyZero())
                {
                    ToTarget = FVector2D(1.f, 0.f);
                }
                ToTarget.Normalize();
                
                TargetLocation.X = EntityX + ToTarget.X * EdgeOffset;
                TargetLocation.Y = EntityY + ToTarget.Y * EdgeOffset;
                TargetLocation.Z = 0.f;
            }
            
            Params.TargetLocation = TargetLocation;
            return;
        }
    }
    
    // ========== 方式2: 通用射线检测，确保目标点不在障碍物内部 ==========
    UWorld* World = Agent->GetWorld();
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Agent);
    
    if (bIsFlying)
    {
        // 飞行器：从高空向下射线检测，找出目标点 (x,y) 处的障碍物顶部
        FVector TraceStart = FVector(TargetLocation.X, TargetLocation.Y, 50000.f);
        FVector TraceEnd = FVector(TargetLocation.X, TargetLocation.Y, -10000.f);
        
        FHitResult HitResult;
        if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
        {
            // 找到障碍物，检查目标 Z 是否在障碍物内部或太接近
            float ObstacleTopZ = HitResult.Location.Z;
            float MinSafeAltitude = ObstacleTopZ + SafetyMargin;
            
            if (TargetLocation.Z < MinSafeAltitude)
            {
                TargetLocation.Z = MinSafeAltitude;
            }
        }
        // 如果没有检测到障碍物，保持原目标高度
    }
    else
    {
        // 地面机器人：检测目标点是否在障碍物内部
        // 从目标点上方向下检测
        FVector TraceStart = FVector(TargetLocation.X, TargetLocation.Y, 50000.f);
        FVector TraceEnd = FVector(TargetLocation.X, TargetLocation.Y, -10000.f);
        
        FHitResult HitResult;
        if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
        {
            AActor* HitActor = HitResult.GetActor();
            
            // 检查是否击中了非地面的障碍物（通过检测 Actor 的边界盒）
            if (HitActor)
            {
                FVector Origin, BoxExtent;
                HitActor->GetActorBounds(false, Origin, BoxExtent);
                
                // 如果目标点在障碍物的 XY 范围内，需要调整
                float HalfSizeX = BoxExtent.X;
                float HalfSizeY = BoxExtent.Y;
                
                bool bInsideX = FMath::Abs(TargetLocation.X - Origin.X) < HalfSizeX;
                bool bInsideY = FMath::Abs(TargetLocation.Y - Origin.Y) < HalfSizeY;
                
                if (bInsideX && bInsideY && BoxExtent.Z > 50.f)  // 排除地面（高度很小的物体）
                {
                    // 目标点在障碍物内部，调整到最近的边缘外侧
                    FVector AgentLoc = Agent->GetActorLocation();
                    FVector2D FromAgent(TargetLocation.X - AgentLoc.X, TargetLocation.Y - AgentLoc.Y);
                    
                    if (FromAgent.IsNearlyZero())
                    {
                        FromAgent = FVector2D(1.f, 0.f);
                    }
                    FromAgent.Normalize();
                    
                    // 计算从障碍物中心到边缘的距离
                    float EdgeOffset = FMath::Max(HalfSizeX, HalfSizeY) + GroundOffset;
                    
                    // 将目标调整到障碍物边缘外侧（沿着从 Agent 到目标的方向）
                    TargetLocation.X = Origin.X + FromAgent.X * EdgeOffset;
                    TargetLocation.Y = Origin.Y + FromAgent.Y * EdgeOffset;
                    TargetLocation.Z = 0.f;  // 地面高度，导航系统会自动调整
                }
            }
        }
    }
    
    Params.TargetLocation = TargetLocation;
}

void FMASkillParamsProcessor::ProcessSearch(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    
    // 从通信层参数设置搜索区域
    if (Cmd && Cmd->Params.SearchArea.Num() > 0)
    {
        Params.SearchBoundary.Empty();
        for (const FVector2D& Point : Cmd->Params.SearchArea)
        {
            Params.SearchBoundary.Add(FVector(Point.X, Point.Y, 0.0f));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessSearch] %s: No SearchArea provided"), *Agent->AgentName);
    }
    
    // 从通信层参数设置目标特征
    if (Cmd && Cmd->Params.TargetFeatures.Num() > 0)
    {
        Params.SearchTarget.Features = Cmd->Params.TargetFeatures;
    }
    
    // 执行场景查询
    FMASemanticLabel Label;
    Label.Class = Params.SearchTarget.Class;
    Label.Type = Params.SearchTarget.Type;
    Label.Features = Params.SearchTarget.Features;
    
    TArray<FMASceneQueryResult> Results = FMASceneQuery::FindObjectsInBoundary(
        Agent->GetWorld(), Label, Params.SearchBoundary);
    
    // 填充反馈上下文
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    for (const FMASceneQueryResult& Result : Results)
    {
        Context.FoundObjects.Add(Result.Name);
        Context.FoundLocations.Add(Result.Location);
    }
}

void FMASkillParamsProcessor::ProcessFollow(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    // Follow 参数处理（目前为空，后续可扩展）
}

void FMASkillParamsProcessor::ProcessCharge(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    // Charge 参数处理（目前为空，后续可扩展）
}

void FMASkillParamsProcessor::ProcessPlace(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASearchResults& SearchResults = SkillComp->GetSearchResultsMutable();
    SearchResults.Reset();
    
    //=========================================================================
    // Step 1: 解析 object1/object2 语义标签 JSON 到 FMASemanticTarget
    // Requirements: 1.1, 1.2
    //=========================================================================
    if (Cmd)
    {
        // 解析 Object1 JSON
        if (!Cmd->Params.Object1Json.IsEmpty())
        {
            ParseSemanticTargetFromJson(Cmd->Params.Object1Json, Params.PlaceObject1);
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Parsed Object1 - Class=%s, Type=%s"), 
                *Agent->AgentName, *Params.PlaceObject1.Class, *Params.PlaceObject1.Type);
        }
        
        // 解析 Object2 JSON
        if (!Cmd->Params.Object2Json.IsEmpty())
        {
            ParseSemanticTargetFromJson(Cmd->Params.Object2Json, Params.PlaceObject2);
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Parsed Object2 - Class=%s, Type=%s"), 
                *Agent->AgentName, *Params.PlaceObject2.Class, *Params.PlaceObject2.Type);
        }
    }
    
    //=========================================================================
    // Step 2: 确定 PlaceMode (Requirements: 1.3, 1.4)
    // - LoadToUGV: object2 是 UGV 机器人
    // - UnloadToGround: object2 是 ground
    // - StackOnObject: object2 是其他物体
    //=========================================================================
    EPlaceMode PlaceMode = DeterminePlaceMode(Params.PlaceObject2);
    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: PlaceMode=%d"), *Agent->AgentName, (int32)PlaceMode);
    
    //=========================================================================
    // Step 3: 查找 Object1 (Requirements: 2.1, 2.5, 2.6)
    //=========================================================================
    FMASemanticLabel Label1;
    Label1.Class = Params.PlaceObject1.Class;
    Label1.Type = Params.PlaceObject1.Type;
    Label1.Features = Params.PlaceObject1.Features;
    
    if (Label1.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: Object1 semantic label is empty"), *Agent->AgentName);
    }
    else
    {
        FMASceneQueryResult Result1 = FMASceneQuery::FindNearestObject(Agent->GetWorld(), Label1, Agent->GetActorLocation());
        if (Result1.bFound)
        {
            Context.PlacedObjectName = Result1.Name;
            SearchResults.Object1Actor = Result1.Actor;
            SearchResults.Object1Location = Result1.Location;
            UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found Object1 '%s' at (%.0f, %.0f, %.0f)"), 
                *Agent->AgentName, *Result1.Name, Result1.Location.X, Result1.Location.Y, Result1.Location.Z);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: Object1 not found in scene"), *Agent->AgentName);
        }
    }
    
    //=========================================================================
    // Step 4: 查找 Object2 (Requirements: 2.2, 2.3, 2.4)
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
            // 装货到 UGV：根据 name 特征查找 UGV
            FString RobotName = Label2.Features.Contains(TEXT("name")) ? Label2.Features[TEXT("name")] : TEXT("");
            AMACharacter* Robot = FMASceneQuery::FindRobotByName(Agent->GetWorld(), RobotName);
            if (Robot)
            {
                Context.PlaceTargetName = Robot->AgentName;
                SearchResults.Object2Actor = Robot;
                SearchResults.Object2Location = Robot->GetActorLocation();
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found UGV '%s' at (%.0f, %.0f, %.0f)"), 
                    *Agent->AgentName, *Robot->AgentName, SearchResults.Object2Location.X, SearchResults.Object2Location.Y, SearchResults.Object2Location.Z);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: UGV '%s' not found"), *Agent->AgentName, *RobotName);
            }
            break;
        }
        
        case EPlaceMode::StackOnObject:
        {
            // 堆叠到另一个物体：查找最近的匹配物体
            FMASceneQueryResult Result2 = FMASceneQuery::FindNearestObject(Agent->GetWorld(), Label2, Agent->GetActorLocation());
            if (Result2.bFound)
            {
                Context.PlaceTargetName = Result2.Name;
                SearchResults.Object2Actor = Result2.Actor;
                SearchResults.Object2Location = Result2.Location;
                UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: Found Object2 '%s' at (%.0f, %.0f, %.0f)"), 
                    *Agent->AgentName, *Result2.Name, Result2.Location.X, Result2.Location.Y, Result2.Location.Z);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[ProcessPlace] %s: Object2 not found in scene"), *Agent->AgentName);
            }
            break;
        }
    }
}

void FMASkillParamsProcessor::ProcessTakeOff(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    
    // 默认起飞高度 1000cm (10m)
    Params.TakeOffHeight = 1000.f;
    
    // 如果有传入参数，覆盖默认值
    if (Cmd && Cmd->Params.bHasDestPosition && Cmd->Params.DestPosition.Z > 0.f)
    {
        Params.TakeOffHeight = Cmd->Params.DestPosition.Z;
    }
}

void FMASkillParamsProcessor::ProcessLand(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    
    // 默认降落高度（地面）
    Params.LandHeight = 0.f;
    
    // 如果有传入参数，使用传入值
    if (Cmd && Cmd->Params.bHasDestPosition)
    {
        Params.LandHeight = Cmd->Params.DestPosition.Z;
    }
    else
    {
        // 射线检测当前位置下方的地面高度
        AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
        if (Agent && Agent->GetWorld())
        {
            FVector Start = Agent->GetActorLocation();
            FVector End = Start - FVector(0.f, 0.f, 50000.f);  // 向下 500m
            
            FHitResult HitResult;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(Agent);
            
            if (Agent->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
            {
                Params.LandHeight = HitResult.Location.Z;
            }
        }
    }
}

void FMASkillParamsProcessor::ProcessReturnHome(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    
    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent) return;
    
    // 如果有传入 HomeLocation，使用传入值
    if (Cmd && Cmd->Params.bHasDestPosition)
    {
        Params.HomeLocation = Cmd->Params.DestPosition;
    }
    else
    {
        // 使用 Agent 的初始位置作为 HomeLocation
        Params.HomeLocation = Agent->InitialLocation;
    }
    
    // 射线检测 HomeLocation 下方的地面高度
    if (Agent->GetWorld())
    {
        // 从高空开始向下检测，确保能检测到地面
        FVector Start = FVector(Params.HomeLocation.X, Params.HomeLocation.Y, 50000.f);
        FVector End = FVector(Params.HomeLocation.X, Params.HomeLocation.Y, -10000.f);
        
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Agent);
        
        if (Agent->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
        {
            // 地面高度 + 50cm 偏移（避免穿模）
            Params.LandHeight = HitResult.Location.Z + 50.f;
        }
        else
        {
            Params.LandHeight = 50.f;  // 默认 50cm
        }
    }
}
