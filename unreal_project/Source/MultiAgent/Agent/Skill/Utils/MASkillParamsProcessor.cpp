// MASkillParamsProcessor.cpp
// 技能参数处理器实现

#include "MASkillParamsProcessor.h"
#include "MASceneQuery.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Core/Comm/MACommTypes.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Utils/MAWorldQuery.h"

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
    
    // 查找 Object1
    FMASemanticLabel Label1;
    Label1.Class = Params.PlaceObject1.Class;
    Label1.Type = Params.PlaceObject1.Type;
    Label1.Features = Params.PlaceObject1.Features;
    
    FMASceneQueryResult Result1 = FMASceneQuery::FindNearestObject(Agent->GetWorld(), Label1, Agent->GetActorLocation());
    if (Result1.bFound)
    {
        Context.PlacedObjectName = Result1.Name;
        SearchResults.Object1Actor = Result1.Actor;
        SearchResults.Object1Location = Result1.Location;
    }
    
    // 查找 Object2
    FMASemanticLabel Label2;
    Label2.Class = Params.PlaceObject2.Class;
    Label2.Type = Params.PlaceObject2.Type;
    Label2.Features = Params.PlaceObject2.Features;
    
    if (Label2.IsGround())
    {
        Context.PlaceTargetName = TEXT("ground");
        SearchResults.Object2Location = Agent->GetActorLocation();
    }
    else if (Label2.IsRobot())
    {
        FString RobotName = Label2.Features.Contains(TEXT("name")) ? Label2.Features[TEXT("name")] : TEXT("");
        AMACharacter* Robot = FMASceneQuery::FindRobotByName(Agent->GetWorld(), RobotName);
        if (Robot)
        {
            Context.PlaceTargetName = Robot->AgentName;
            SearchResults.Object2Actor = Robot;
            SearchResults.Object2Location = Robot->GetActorLocation();
        }
    }
    else
    {
        FMASceneQueryResult Result2 = FMASceneQuery::FindNearestObject(Agent->GetWorld(), Label2, Agent->GetActorLocation());
        if (Result2.bFound)
        {
            Context.PlaceTargetName = Result2.Name;
            SearchResults.Object2Actor = Result2.Actor;
            SearchResults.Object2Location = Result2.Location;
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
