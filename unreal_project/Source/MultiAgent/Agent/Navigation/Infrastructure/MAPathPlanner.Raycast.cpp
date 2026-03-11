// MAPathPlanner.Raycast.cpp
// Multi-layer raycast planner implementation

#include "MAPathPlanner.h"
#include "MAPathPlannerInternal.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

FMAMultiLayerRaycast::FMAMultiLayerRaycast(const FMAPathPlannerConfig& InConfig)
    : Config(InConfig)
    , LastDirection(FVector::ForwardVector)
    , bHasValidPath(true)
    , OscillationLockFrames(0)
    , SmoothedDirection(FVector::ZeroVector)
{
    // 验证并限制层数在 2-5 范围内
    Config.RaycastLayerCount = FMath::Clamp(Config.RaycastLayerCount, 2, 5);
    
    RecentDirections.Reserve(MaxRecentDirections);
}

FVector FMAMultiLayerRaycast::CalculateDirection(
    UWorld* World,
    const FVector& CurrentLocation,
    const FVector& TargetLocation,
    AActor* IgnoreActor)
{
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("FMAMultiLayerRaycast::CalculateDirection - World is null"));
        return FVector::ZeroVector;
    }
    
    // 计算朝向目标的方向
    FVector ToTarget = TargetLocation - CurrentLocation;
    ToTarget.Z = 0.f;  // 忽略高度差
    
    if (ToTarget.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }
    
    FVector TargetDirection = ToTarget.GetSafeNormal();
    FVector BestDirection = TargetDirection;
    float BestScore = -FLT_MAX;
    
    // 如果处于振荡锁定状态，继续使用上一次的方向
    if (OscillationLockFrames > 0)
    {
        OscillationLockFrames--;
        return SmoothedDirection.IsNearlyZero() ? LastDirection : SmoothedDirection;
    }
    
    // 步骤 1: 先检查直线路径是否畅通
    bool bDirectPathClear = IsDirectionClear(World, CurrentLocation, TargetDirection, 
                                              Config.RaycastLayerDistance, IgnoreActor);
    bool bDirectPathHasGround = HasGroundAhead(World, CurrentLocation, TargetDirection,
                                                Config.RaycastLayerDistance, IgnoreActor);
    
    if (bDirectPathClear && bDirectPathHasGround)
    {
        // 直线路径畅通，评估得分
        BestScore = EvaluateDirection(World, CurrentLocation, TargetLocation, TargetDirection, IgnoreActor);
        BestDirection = TargetDirection;
    }
    
    // 步骤 2: 如果直线路径被阻挡，扫描多方向
    if (!bDirectPathClear || !bDirectPathHasGround || BestScore < 0.f)
    {
        // 扫描 -ScanAngleRange 到 +ScanAngleRange 范围内的方向
        for (float Angle = -Config.ScanAngleRange; Angle <= Config.ScanAngleRange; Angle += Config.ScanAngleStep)
        {
            // 跳过直线方向（已经检查过）
            if (FMath::Abs(Angle) < Config.ScanAngleStep * 0.5f)
            {
                continue;
            }
            
            // 计算候选方向
            FVector CandidateDirection = TargetDirection.RotateAngleAxis(Angle, FVector::UpVector);
            
            // 检查方向是否畅通
            if (!IsDirectionClear(World, CurrentLocation, CandidateDirection, 
                                  Config.RaycastLayerDistance, IgnoreActor))
            {
                continue;
            }
            
            // 检查地面连续性
            if (!HasGroundAhead(World, CurrentLocation, CandidateDirection,
                               Config.RaycastLayerDistance, IgnoreActor))
            {
                continue;
            }
            
            // 评估方向得分
            float Score = EvaluateDirection(World, CurrentLocation, TargetLocation, 
                                           CandidateDirection, IgnoreActor);
            
            if (Score > BestScore)
            {
                BestScore = Score;
                BestDirection = CandidateDirection;
            }
        }
    }
    
    // 步骤 3: 如果所有方向都被阻挡，返回后退方向
    if (BestScore < -1000.f)
    {
        bHasValidPath = false;
        BestDirection = -TargetDirection;  // 后退
        UE_LOG(LogTemp, Warning, TEXT("FMAMultiLayerRaycast: All directions blocked, moving backward"));
    }
    else
    {
        bHasValidPath = true;
    }
    
    // 记录方向历史（用于振荡检测）
    RecentDirections.Add(BestDirection);
    if (RecentDirections.Num() > MaxRecentDirections)
    {
        RecentDirections.RemoveAt(0);
    }
    
    // 检测振荡
    if (IsOscillating())
    {
        OscillationLockFrames = OscillationLockDuration;
        UE_LOG(LogTemp, Warning, TEXT("FMAMultiLayerRaycast: Oscillation detected, locking direction"));
        // 使用平滑后的方向
        return SmoothedDirection.IsNearlyZero() ? LastDirection : SmoothedDirection;
    }
    
    // 方向平滑（指数移动平均）
    if (SmoothedDirection.IsNearlyZero())
    {
        SmoothedDirection = BestDirection;
    }
    else
    {
        SmoothedDirection = FMath::Lerp(SmoothedDirection, BestDirection, DirectionSmoothingFactor);
        SmoothedDirection.Normalize();
    }
    
    LastDirection = BestDirection;
    
    return SmoothedDirection;
}

bool FMAMultiLayerRaycast::HasValidPath() const
{
    return bHasValidPath;
}

void FMAMultiLayerRaycast::Reset()
{
    LastDirection = FVector::ForwardVector;
    RecentDirections.Empty();
    bHasValidPath = true;
    OscillationLockFrames = 0;
    SmoothedDirection = FVector::ZeroVector;
}

void FMAMultiLayerRaycast::SetConfig(const FMAPathPlannerConfig& NewConfig)
{
    Config = NewConfig;
    
    // 验证并限制层数在 2-5 范围内
    Config.RaycastLayerCount = FMath::Clamp(Config.RaycastLayerCount, 2, 5);
    
    // 验证其他参数
    Config.RaycastLayerDistance = FMath::Max(Config.RaycastLayerDistance, 50.f);
    Config.ScanAngleRange = FMath::Clamp(Config.ScanAngleRange, 30.f, 180.f);
    Config.ScanAngleStep = FMath::Clamp(Config.ScanAngleStep, 5.f, 45.f);
    Config.AgentRadius = FMath::Max(Config.AgentRadius, 10.f);
}

bool FMAMultiLayerRaycast::IsDirectionClear(
    UWorld* World, 
    const FVector& Start, 
    const FVector& Direction, 
    float Distance, 
    AActor* IgnoreActor)
{
    if (!World || Direction.IsNearlyZero())
    {
        return false;
    }
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(IgnoreActor);
    QueryParams.bTraceComplex = false;
    
    FHitResult HitResult;
    FVector EndPoint = Start + Direction * Distance;
    
    // 多高度射线检测（脚踝、膝盖、腰部）
    for (float Height : MAPathPlannerInternal::GetRaycastHeights())
    {
        FVector RayStart = Start + FVector(0.f, 0.f, Height);
        FVector RayEnd = EndPoint + FVector(0.f, 0.f, Height);
        
        bool bHit = World->LineTraceSingleByChannel(
            HitResult,
            RayStart,
            RayEnd,
            ECC_Visibility,
            QueryParams
        );
        
        if (bHit)
        {
            // 任何高度检测到障碍物，方向不畅通
            return false;
        }
    }
    
    return true;
}

bool FMAMultiLayerRaycast::HasGroundAhead(
    UWorld* World, 
    const FVector& Start, 
    const FVector& Direction,
    float Distance, 
    AActor* IgnoreActor)
{
    if (!World || Direction.IsNearlyZero())
    {
        return false;
    }
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(IgnoreActor);
    QueryParams.bTraceComplex = false;
    
    FHitResult HitResult;
    
    // 计算前方检测点
    FVector CheckPoint = Start + Direction * Distance;
    
    // 从检测点上方向下发射射线，检查地面连续性
    FVector RayStart = CheckPoint + FVector(0.f, 0.f, MAPathPlannerInternal::CliffCheckHeight);
    FVector RayEnd = CheckPoint - FVector(0.f, 0.f, MAPathPlannerInternal::CliffCheckDepth);
    
    bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        RayStart,
        RayEnd,
        ECC_Visibility,
        QueryParams
    );
    
    if (!bHit)
    {
        // 没有检测到地面，可能是悬崖
        return false;
    }
    
    // 检查高度差是否在允许范围内
    float HeightDiff = FMath::Abs(HitResult.ImpactPoint.Z - Start.Z);
    if (HeightDiff > MAPathPlannerInternal::MaxAllowedHeightDiff)
    {
        // 高度差过大，可能是悬崖或陡坡
        return false;
    }
    
    return true;
}

float FMAMultiLayerRaycast::EvaluateDirection(
    UWorld* World, 
    const FVector& CurrentLoc, 
    const FVector& TargetLoc,
    const FVector& Direction, 
    AActor* IgnoreActor)
{
    if (!World || Direction.IsNearlyZero())
    {
        return -FLT_MAX;
    }
    
    float Score = 0.f;
    
    // 计算朝向目标的方向
    FVector ToTarget = TargetLoc - CurrentLoc;
    ToTarget.Z = 0.f;
    FVector TargetDirection = ToTarget.GetSafeNormal();
    float DistanceToTarget = ToTarget.Size();
    
    // 评分因子 1: 角度偏差（越小越好）
    // 与目标方向的夹角，范围 0-180 度
    float AngleDeviation = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, TargetDirection)));
    // 角度得分：0度 = 100分，180度 = -100分
    float AngleScore = 100.f - (AngleDeviation / 180.f) * 200.f;
    Score += AngleScore * 1.0f;  // 权重 1.0
    
    // 评分因子 2: 多层检测（越远越好）
    // 检查多层，每层都畅通则得分更高
    float LayerScore = 0.f;
    int32 ClearLayers = 0;
    
    for (int32 Layer = 1; Layer <= Config.RaycastLayerCount; ++Layer)
    {
        float LayerDistance = Config.RaycastLayerDistance * Layer;
        
        // 检查该层是否畅通
        bool bLayerClear = IsDirectionClear(World, CurrentLoc, Direction, LayerDistance, IgnoreActor);
        bool bLayerHasGround = HasGroundAhead(World, CurrentLoc, Direction, LayerDistance, IgnoreActor);
        
        if (bLayerClear && bLayerHasGround)
        {
            ClearLayers++;
            // 越远的层畅通，得分越高
            LayerScore += (float)Layer * 20.f;
        }
        else
        {
            // 遇到阻挡，停止检查更远的层
            break;
        }
    }
    
    // 如果第一层都不畅通，给予很低的分数
    if (ClearLayers == 0)
    {
        return -2000.f;
    }
    
    Score += LayerScore * 0.8f;  // 权重 0.8
    
    // 评分因子 3: 距离改善（移动后是否更接近目标）
    FVector NewPosition = CurrentLoc + Direction * Config.RaycastLayerDistance;
    float NewDistanceToTarget = (TargetLoc - NewPosition).Size2D();
    float DistanceImprovement = DistanceToTarget - NewDistanceToTarget;
    // 距离改善得分：每接近 100 单位得 10 分
    float DistanceScore = (DistanceImprovement / 100.f) * 10.f;
    Score += DistanceScore * 0.5f;  // 权重 0.5
    
    return Score;
}

bool FMAMultiLayerRaycast::IsOscillating() const
{
    // 需要足够的历史数据才能检测振荡
    if (RecentDirections.Num() < MaxRecentDirections)
    {
        return false;
    }
    
    // 检测方向反复变化的次数
    // 振荡定义：方向在左右之间反复切换
    int32 DirectionChanges = 0;
    
    for (int32 i = 1; i < RecentDirections.Num(); ++i)
    {
        const FVector& PrevDir = RecentDirections[i - 1];
        const FVector& CurrDir = RecentDirections[i];
        
        // 计算方向变化的角度
        float DotProduct = FVector::DotProduct(PrevDir, CurrDir);
        
        // 如果方向变化超过 60 度，认为是一次显著变化
        if (DotProduct < 0.5f)  // cos(60°) ≈ 0.5
        {
            DirectionChanges++;
        }
    }
    
    // 如果在最近的历史中方向变化次数超过阈值，认为是振荡
    // 例如：6 个方向中有 3 次以上的显著变化
    return DirectionChanges >= OscillationThreshold;
}


//=============================================================================
