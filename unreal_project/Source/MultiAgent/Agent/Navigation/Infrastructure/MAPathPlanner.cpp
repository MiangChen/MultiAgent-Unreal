// MAPathPlanner.cpp
// 路径规划器工具类实现

#include "MAPathPlanner.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"

//=============================================================================
// 射线检测高度配置
//=============================================================================

namespace MAPathPlannerConstants
{
    // 多高度射线检测的高度配置（脚踝、膝盖、腰部）
    static const TArray<float> RaycastHeights = {30.f, 60.f, 90.f};
    
    // 悬崖检测参数
    static constexpr float CliffCheckHeight = 100.f;      // 向上偏移
    static constexpr float CliffCheckDepth = 300.f;       // 向下检测深度
    static constexpr float MaxAllowedHeightDiff = 150.f;  // 最大允许高度差
    
    // 高程图构建参数
    static constexpr float ElevationRayStartOffset = 1000.f;   // 射线起始高度偏移
    static constexpr float ElevationRayEndOffset = 1000.f;     // 射线结束高度偏移
    static constexpr float ObstacleCheckHeight = 60.f;         // 障碍物检测高度（腰部）
}

//=============================================================================
// FMAElevationMap 实现
//=============================================================================

void FMAElevationMap::Build(
    UWorld* World,
    const FVector& Center,
    float Radius,
    float InCellSize,
    AActor* IgnoreActor,
    float AgentRadius)
{
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("FMAElevationMap::Build - World is null"));
        return;
    }
    
    // 保存栅格参数
    CellSize = FMath::Max(InCellSize, 50.f);
    
    // 计算栅格尺寸
    Width = FMath::CeilToInt((Radius * 2.f) / CellSize);
    Height = Width;  // 正方形栅格
    
    // 限制栅格尺寸
    Width = FMath::Clamp(Width, 10, 200);
    Height = FMath::Clamp(Height, 10, 200);
    
    // 计算栅格原点（左下角）
    Origin.X = Center.X - (Width * CellSize * 0.5f);
    Origin.Y = Center.Y - (Height * CellSize * 0.5f);
    Origin.Z = Center.Z;
    
    // 初始化栅格数组
    Cells.Empty();
    Cells.SetNum(Width);
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(IgnoreActor);
    QueryParams.bTraceComplex = false;
    
    int32 TraversableCount = 0;
    int32 ObstacleCount = 0;
    int32 NoGroundCount = 0;
    
    for (int32 X = 0; X < Width; ++X)
    {
        Cells[X].SetNum(Height);
        
        for (int32 Y = 0; Y < Height; ++Y)
        {
            FMAElevationCell& Cell = Cells[X][Y];
            Cell.ResetSearchState();
            
            // 计算单元格中心的世界坐标
            FVector CellCenter;
            CellCenter.X = Origin.X + (X + 0.5f) * CellSize;
            CellCenter.Y = Origin.Y + (Y + 0.5f) * CellSize;
            CellCenter.Z = Center.Z;
            
            // 从高处向下发射垂直射线检测地面
            FVector RayStart = CellCenter + FVector(0.f, 0.f, MAPathPlannerConstants::ElevationRayStartOffset);
            FVector RayEnd = CellCenter - FVector(0.f, 0.f, MAPathPlannerConstants::ElevationRayEndOffset);
            
            FHitResult HitResult;
            bool bFoundGround = World->LineTraceSingleByChannel(
                HitResult,
                RayStart,
                RayEnd,
                ECC_WorldStatic,
                QueryParams
            );
            
            if (bFoundGround)
            {
                // 记录地面高度
                Cell.GroundZ = HitResult.ImpactPoint.Z;
                
                // 使用球形重叠检测障碍物
                FVector CheckCenter = FVector(CellCenter.X, CellCenter.Y, Cell.GroundZ + MAPathPlannerConstants::ObstacleCheckHeight);
                FCollisionShape SphereShape = FCollisionShape::MakeSphere(AgentRadius);
                
                TArray<FOverlapResult> Overlaps;
                bool bHasObstacle = World->OverlapMultiByChannel(
                    Overlaps,
                    CheckCenter,
                    FQuat::Identity,
                    ECC_WorldDynamic,
                    SphereShape,
                    QueryParams
                );
                
                Cell.bHasObstacle = bHasObstacle && Overlaps.Num() > 0;
                
                // 初步设置可通行性（有地面且无障碍物）
                // 详细的坡度评估在 EvaluateTraversability 中进行
                Cell.bTraversable = !Cell.bHasObstacle;
                
                if (Cell.bHasObstacle)
                {
                    ObstacleCount++;
                }
                else
                {
                    TraversableCount++;
                }
            }
            else
            {
                // 无地面（悬崖、空洞）
                Cell.GroundZ = FLT_MAX;
                Cell.bTraversable = false;
                Cell.bHasObstacle = false;
                NoGroundCount++;
            }
        }
    }
    
    int32 TotalCells = Width * Height;
    UE_LOG(LogTemp, Log, TEXT("FMAElevationMap: Built %dx%d grid (%d cells), traversable: %d, obstacles: %d, no ground: %d"),
           Width, Height, TotalCells, TraversableCount, ObstacleCount, NoGroundCount);
}

bool FMAElevationMap::EvaluateTraversability(
    int32 FromX, int32 FromY,
    int32 ToX, int32 ToY,
    float MaxSlopeAngle,
    float MaxStepHeight) const
{
    // 检查坐标有效性
    if (!IsValidCell(FromX, FromY) || !IsValidCell(ToX, ToY))
    {
        return false;
    }
    
    const FMAElevationCell& FromCell = Cells[FromX][FromY];
    const FMAElevationCell& ToCell = Cells[ToX][ToY];
    
    // 规则 1: 目标格必须有地面
    if (!ToCell.HasGround())
    {
        return false;
    }
    
    // 规则 2: 目标格不能有障碍物
    if (ToCell.bHasObstacle)
    {
        return false;
    }
    
    // 规则 3: 起始格必须有地面
    if (!FromCell.HasGround())
    {
        return false;
    }
    
    // 规则 4: 高度差检查（台阶检测）
    float HeightDiff = FMath::Abs(ToCell.GroundZ - FromCell.GroundZ);
    if (HeightDiff > MaxStepHeight)
    {
        return false;
    }
    
    // 规则 5: 坡度检查
    float HorizontalDist = CellSize;
    // 对角线移动时水平距离更长
    if (FromX != ToX && FromY != ToY)
    {
        HorizontalDist = CellSize * 1.414f;  // sqrt(2)
    }
    
    float SlopeAngle = FMath::RadiansToDegrees(FMath::Atan2(HeightDiff, HorizontalDist));
    if (SlopeAngle > MaxSlopeAngle)
    {
        return false;
    }
    
    return true;
}

float FMAElevationMap::GetGroundHeight(int32 X, int32 Y) const
{
    if (!IsValidCell(X, Y))
    {
        return FLT_MAX;
    }
    return Cells[X][Y].GroundZ;
}

bool FMAElevationMap::IsTraversable(int32 X, int32 Y) const
{
    if (!IsValidCell(X, Y))
    {
        return false;
    }
    return Cells[X][Y].bTraversable;
}

bool FMAElevationMap::HasGround(int32 X, int32 Y) const
{
    if (!IsValidCell(X, Y))
    {
        return false;
    }
    return Cells[X][Y].HasGround();
}

bool FMAElevationMap::IsValidCell(int32 X, int32 Y) const
{
    return X >= 0 && X < Width && Y >= 0 && Y < Height;
}

FIntPoint FMAElevationMap::WorldToGrid(const FVector& WorldPos) const
{
    float RelativeX = WorldPos.X - Origin.X;
    float RelativeY = WorldPos.Y - Origin.Y;
    
    int32 GridX = FMath::FloorToInt(RelativeX / CellSize);
    int32 GridY = FMath::FloorToInt(RelativeY / CellSize);
    
    return FIntPoint(GridX, GridY);
}

FVector FMAElevationMap::GridToWorld(int32 X, int32 Y) const
{
    FVector WorldPos;
    WorldPos.X = Origin.X + (X + 0.5f) * CellSize;
    WorldPos.Y = Origin.Y + (Y + 0.5f) * CellSize;
    
    // 如果有有效地面高度，使用它；否则使用原点高度
    if (IsValidCell(X, Y) && Cells[X][Y].HasGround())
    {
        WorldPos.Z = Cells[X][Y].GroundZ;
    }
    else
    {
        WorldPos.Z = Origin.Z;
    }
    
    return WorldPos;
}

TArray<FIntPoint> FMAElevationMap::GetNeighbors(int32 X, int32 Y) const
{
    TArray<FIntPoint> Neighbors;
    Neighbors.Reserve(8);
    
    // 8 方向邻居
    static const int32 DX[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const int32 DY[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    for (int32 i = 0; i < 8; ++i)
    {
        int32 NX = X + DX[i];
        int32 NY = Y + DY[i];
        
        if (IsValidCell(NX, NY))
        {
            Neighbors.Add(FIntPoint(NX, NY));
        }
    }
    
    return Neighbors;
}

void FMAElevationMap::ResetSearchState()
{
    for (int32 X = 0; X < Width; ++X)
    {
        for (int32 Y = 0; Y < Height; ++Y)
        {
            Cells[X][Y].ResetSearchState();
        }
    }
}

FMAElevationCell& FMAElevationMap::GetCell(int32 X, int32 Y)
{
    check(IsValidCell(X, Y));
    return Cells[X][Y];
}

const FMAElevationCell& FMAElevationMap::GetCell(int32 X, int32 Y) const
{
    check(IsValidCell(X, Y));
    return Cells[X][Y];
}

//=============================================================================
// FMAMultiLayerRaycast 实现
//=============================================================================

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
    for (float Height : MAPathPlannerConstants::RaycastHeights)
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
    FVector RayStart = CheckPoint + FVector(0.f, 0.f, MAPathPlannerConstants::CliffCheckHeight);
    FVector RayEnd = CheckPoint - FVector(0.f, 0.f, MAPathPlannerConstants::CliffCheckDepth);
    
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
    if (HeightDiff > MAPathPlannerConstants::MaxAllowedHeightDiff)
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
// FMAElevationMapPathfinder 实现
//=============================================================================

FMAElevationMapPathfinder::FMAElevationMapPathfinder(const FMAPathPlannerConfig& InConfig)
    : Config(InConfig)
    , CurrentWaypointIndex(0)
    , bHasValidPath(false)
    , LastTargetLocation(FVector::ZeroVector)
    , LastStartLocation(FVector::ZeroVector)
    , SmoothedDirection(FVector::ZeroVector)
{
}

FVector FMAElevationMapPathfinder::CalculateDirection(
    UWorld* World,
    const FVector& CurrentLocation,
    const FVector& TargetLocation,
    AActor* IgnoreActor)
{
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("FMAElevationMapPathfinder::CalculateDirection - World is null"));
        return FVector::ZeroVector;
    }
    
    // 缓存世界和忽略 Actor
    CachedWorld = World;
    CachedIgnoreActor = IgnoreActor;
    
    // 计算到目标的方向
    FVector ToTarget = TargetLocation - CurrentLocation;
    ToTarget.Z = 0.f;
    
    if (ToTarget.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }
    
    // 检查是否需要重新规划路径
    if (NeedsReplan(CurrentLocation, TargetLocation))
    {
        // 构建高程图
        ElevationMap.Build(
            World,
            CurrentLocation,
            Config.ElevationSearchRadius,
            Config.ElevationCellSize,
            IgnoreActor,
            Config.AgentRadius
        );
        
        // 执行 A* 搜索
        bool bFoundPath = FindPath(CurrentLocation, TargetLocation);
        
        if (!bFoundPath)
        {
            UE_LOG(LogTemp, Warning, TEXT("FMAElevationMapPathfinder: No path found, returning direct direction"));
            bHasValidPath = false;
            return ToTarget.GetSafeNormal();
        }
        
        // 更新缓存的位置
        LastStartLocation = CurrentLocation;
        LastTargetLocation = TargetLocation;
    }
    
    // 路径跟踪：推进路径点
    while (CurrentWaypointIndex < CachedPath.Num())
    {
        FVector CurrentWaypoint = CachedPath[CurrentWaypointIndex];
        float DistanceToWaypoint = FVector::Dist2D(CurrentLocation, CurrentWaypoint);
        
        if (DistanceToWaypoint < Config.ElevationWaypointReachThreshold)
        {
            // 到达当前路径点，移动到下一个
            CurrentWaypointIndex++;
            UE_LOG(LogTemp, Verbose, TEXT("FMAElevationMapPathfinder: Reached waypoint %d/%d"),
                   CurrentWaypointIndex, CachedPath.Num());
        }
        else
        {
            break;
        }
    }
    
    // 检查是否已经走完所有路径点
    if (CurrentWaypointIndex >= CachedPath.Num())
    {
        // 所有路径点都已到达，直接朝向目标
        FVector DirectDirection = ToTarget.GetSafeNormal();
        
        // 方向平滑
        if (SmoothedDirection.IsNearlyZero())
        {
            SmoothedDirection = DirectDirection;
        }
        else
        {
            SmoothedDirection = FMath::Lerp(SmoothedDirection, DirectDirection, Config.PathSmoothingFactor);
            SmoothedDirection.Normalize();
        }
        
        return SmoothedDirection;
    }
    
    // 计算朝向当前路径点的方向
    FVector ToWaypoint = CachedPath[CurrentWaypointIndex] - CurrentLocation;
    ToWaypoint.Z = 0.f;  // 水平方向移动，高度由物理引擎处理
    
    if (ToWaypoint.IsNearlyZero())
    {
        // 如果方向为零，尝试下一个路径点
        if (CurrentWaypointIndex + 1 < CachedPath.Num())
        {
            ToWaypoint = CachedPath[CurrentWaypointIndex + 1] - CurrentLocation;
            ToWaypoint.Z = 0.f;
        }
        else
        {
            return ToTarget.GetSafeNormal();
        }
    }
    
    FVector DesiredDirection = ToWaypoint.GetSafeNormal();
    
    // 方向平滑（指数移动平均）
    if (SmoothedDirection.IsNearlyZero())
    {
        SmoothedDirection = DesiredDirection;
    }
    else
    {
        SmoothedDirection = FMath::Lerp(SmoothedDirection, DesiredDirection, Config.PathSmoothingFactor);
        SmoothedDirection.Normalize();
    }
    
    return SmoothedDirection;
}

bool FMAElevationMapPathfinder::HasValidPath() const
{
    return bHasValidPath;
}

void FMAElevationMapPathfinder::Reset()
{
    CachedPath.Empty();
    CurrentWaypointIndex = 0;
    bHasValidPath = false;
    LastTargetLocation = FVector::ZeroVector;
    LastStartLocation = FVector::ZeroVector;
    SmoothedDirection = FVector::ZeroVector;
    CachedWorld.Reset();
    CachedIgnoreActor.Reset();
}

void FMAElevationMapPathfinder::SetConfig(const FMAPathPlannerConfig& NewConfig)
{
    Config = NewConfig;
    
    // 验证参数范围
    Config.ElevationCellSize = FMath::Clamp(Config.ElevationCellSize, 50.f, 200.f);
    Config.ElevationSearchRadius = FMath::Clamp(Config.ElevationSearchRadius, 1000.f, 10000.f);
    Config.ElevationMaxSlopeAngle = FMath::Clamp(Config.ElevationMaxSlopeAngle, 10.f, 45.f);
    Config.ElevationMaxStepHeight = FMath::Clamp(Config.ElevationMaxStepHeight, 20.f, 100.f);
    Config.PathSmoothingFactor = FMath::Clamp(Config.PathSmoothingFactor, 0.1f, 0.5f);
    Config.ElevationWaypointReachThreshold = FMath::Max(Config.ElevationWaypointReachThreshold, 50.f);
    Config.ElevationReplanThreshold = FMath::Max(Config.ElevationReplanThreshold, 100.f);
    Config.AgentRadius = FMath::Max(Config.AgentRadius, 10.f);
}

bool FMAElevationMapPathfinder::NeedsReplan(const FVector& CurrentLocation, const FVector& TargetLocation) const
{
    // 情况 1: 没有有效路径
    if (!bHasValidPath || CachedPath.Num() == 0)
    {
        return true;
    }
    
    // 情况 2: 目标位置发生显著变化
    float TargetChange = FVector::Dist2D(TargetLocation, LastTargetLocation);
    if (TargetChange > Config.ElevationReplanThreshold)
    {
        UE_LOG(LogTemp, Verbose, TEXT("FMAElevationMapPathfinder: Target changed (%.1f), replanning"), TargetChange);
        return true;
    }
    
    // 情况 3: 偏离路径超过阈值
    if (CurrentWaypointIndex < CachedPath.Num())
    {
        // 检查是否偏离整条路径
        float MinDistanceToPath = FLT_MAX;
        for (int32 i = CurrentWaypointIndex; i < CachedPath.Num(); ++i)
        {
            float Dist = FVector::Dist2D(CurrentLocation, CachedPath[i]);
            MinDistanceToPath = FMath::Min(MinDistanceToPath, Dist);
        }
        
        if (MinDistanceToPath > Config.ElevationReplanThreshold)
        {
            UE_LOG(LogTemp, Verbose, TEXT("FMAElevationMapPathfinder: Deviation detected (%.1f), replanning"), MinDistanceToPath);
            return true;
        }
    }
    
    // 情况 4: 已经走完所有路径点但还没到达目标
    if (CurrentWaypointIndex >= CachedPath.Num())
    {
        float DistanceToTarget = FVector::Dist2D(CurrentLocation, TargetLocation);
        if (DistanceToTarget > Config.ElevationWaypointReachThreshold)
        {
            return true;
        }
    }
    
    return false;
}

bool FMAElevationMapPathfinder::FindPath(const FVector& Start, const FVector& Target)
{
    // 清空之前的路径
    CachedPath.Empty();
    CurrentWaypointIndex = 0;
    bHasValidPath = false;
    
    // 检查高程图是否已构建
    if (!ElevationMap.IsBuilt())
    {
        UE_LOG(LogTemp, Warning, TEXT("FMAElevationMapPathfinder::FindPath - Elevation map not built"));
        return false;
    }
    
    // 转换起点和终点到栅格坐标
    FIntPoint StartGrid = ElevationMap.WorldToGrid(Start);
    FIntPoint TargetGrid = ElevationMap.WorldToGrid(Target);
    
    // 检查起点和终点是否在栅格范围内
    if (!ElevationMap.IsValidCell(StartGrid.X, StartGrid.Y))
    {
        UE_LOG(LogTemp, Warning, TEXT("FMAElevationMapPathfinder::FindPath - Start position out of grid bounds"));
        return false;
    }
    
    if (!ElevationMap.IsValidCell(TargetGrid.X, TargetGrid.Y))
    {
        UE_LOG(LogTemp, Warning, TEXT("FMAElevationMapPathfinder::FindPath - Target position out of grid bounds"));
        return false;
    }
    
    // 检查起点是否可通行
    if (!ElevationMap.IsTraversable(StartGrid.X, StartGrid.Y))
    {
        UE_LOG(LogTemp, Warning, TEXT("FMAElevationMapPathfinder::FindPath - Start cell (%d,%d) is not traversable"),
               StartGrid.X, StartGrid.Y);
        return false;
    }
    
    // 检查终点是否可通行
    if (!ElevationMap.IsTraversable(TargetGrid.X, TargetGrid.Y))
    {
        UE_LOG(LogTemp, Warning, TEXT("FMAElevationMapPathfinder::FindPath - Target cell (%d,%d) is not traversable"),
               TargetGrid.X, TargetGrid.Y);
        return false;
    }
    
    // 重置所有格子的 A* 搜索状态
    ElevationMap.ResetSearchState();
    
    // 使用优先队列（开放列表）
    TArray<TPair<float, FIntPoint>> OpenList;
    
    // Lambda 函数：维护最小堆 - 入堆
    auto HeapPush = [&OpenList](float F, FIntPoint Point)
    {
        OpenList.Add(TPair<float, FIntPoint>(F, Point));
        // 上浮操作
        int32 Index = OpenList.Num() - 1;
        while (Index > 0)
        {
            int32 Parent = (Index - 1) / 2;
            if (OpenList[Index].Key < OpenList[Parent].Key)
            {
                OpenList.Swap(Index, Parent);
                Index = Parent;
            }
            else
            {
                break;
            }
        }
    };
    
    // Lambda 函数：维护最小堆 - 出堆
    auto HeapPop = [&OpenList]() -> FIntPoint
    {
        if (OpenList.Num() == 0)
        {
            return FIntPoint(-1, -1);
        }
        
        FIntPoint Result = OpenList[0].Value;
        OpenList[0] = OpenList.Last();
        OpenList.RemoveAt(OpenList.Num() - 1);
        
        // 下沉操作
        int32 Index = 0;
        while (true)
        {
            int32 Left = 2 * Index + 1;
            int32 Right = 2 * Index + 2;
            int32 Smallest = Index;
            
            if (Left < OpenList.Num() && OpenList[Left].Key < OpenList[Smallest].Key)
            {
                Smallest = Left;
            }
            if (Right < OpenList.Num() && OpenList[Right].Key < OpenList[Smallest].Key)
            {
                Smallest = Right;
            }
            
            if (Smallest != Index)
            {
                OpenList.Swap(Index, Smallest);
                Index = Smallest;
            }
            else
            {
                break;
            }
        }
        
        return Result;
    };
    
    // 初始化起点
    FMAElevationCell& StartCell = ElevationMap.GetCell(StartGrid.X, StartGrid.Y);
    StartCell.G = 0.f;
    StartCell.H = CalculateHeuristic(StartGrid.X, StartGrid.Y, TargetGrid.X, TargetGrid.Y);
    
    HeapPush(StartCell.F(), StartGrid);
    
    // A* 主循环
    while (OpenList.Num() > 0)
    {
        // 取出 F 值最小的节点
        FIntPoint Current = HeapPop();
        
        if (Current.X < 0 || Current.Y < 0)
        {
            break;
        }
        
        FMAElevationCell& CurrentCell = ElevationMap.GetCell(Current.X, Current.Y);
        
        // 如果已访问，跳过
        if (CurrentCell.bVisited)
        {
            continue;
        }
        
        CurrentCell.bVisited = true;
        
        // 检查是否到达目标
        if (Current.X == TargetGrid.X && Current.Y == TargetGrid.Y)
        {
            // 找到路径，回溯构建路径
            ReconstructPath(TargetGrid);
            bHasValidPath = true;
            UE_LOG(LogTemp, Log, TEXT("FMAElevationMapPathfinder: Found path with %d waypoints"), CachedPath.Num());
            return true;
        }
        
        // 遍历 8 方向邻居
        TArray<FIntPoint> Neighbors = ElevationMap.GetNeighbors(Current.X, Current.Y);
        
        for (const FIntPoint& Neighbor : Neighbors)
        {
            // 检查可通行性（考虑坡度和台阶高度）
            if (!ElevationMap.EvaluateTraversability(
                Current.X, Current.Y,
                Neighbor.X, Neighbor.Y,
                Config.ElevationMaxSlopeAngle,
                Config.ElevationMaxStepHeight))
            {
                continue;
            }
            
            FMAElevationCell& NeighborCell = ElevationMap.GetCell(Neighbor.X, Neighbor.Y);
            
            // 跳过已访问的节点
            if (NeighborCell.bVisited)
            {
                continue;
            }
            
            // 计算移动代价
            float MoveCost = CalculateMoveCost(Current.X, Current.Y, Neighbor.X, Neighbor.Y);
            float NewG = CurrentCell.G + MoveCost;
            
            // 如果找到更短的路径
            if (NewG < NeighborCell.G)
            {
                NeighborCell.G = NewG;
                NeighborCell.H = CalculateHeuristic(Neighbor.X, Neighbor.Y, TargetGrid.X, TargetGrid.Y);
                NeighborCell.ParentX = Current.X;
                NeighborCell.ParentY = Current.Y;
                
                HeapPush(NeighborCell.F(), Neighbor);
            }
        }
    }
    
    // 没有找到路径
    UE_LOG(LogTemp, Warning, TEXT("FMAElevationMapPathfinder::FindPath - No path found"));
    bHasValidPath = false;
    return false;
}

float FMAElevationMapPathfinder::CalculateMoveCost(int32 FromX, int32 FromY, int32 ToX, int32 ToY) const
{
    float BaseCost = Config.ElevationCellSize;
    
    // 对角线移动代价更高
    if (FromX != ToX && FromY != ToY)
    {
        BaseCost *= 1.414f;  // sqrt(2)
    }
    
    // 获取高度差
    float FromZ = ElevationMap.GetGroundHeight(FromX, FromY);
    float ToZ = ElevationMap.GetGroundHeight(ToX, ToY);
    
    // 检查高度是否有效
    if (FromZ >= FLT_MAX * 0.5f || ToZ >= FLT_MAX * 0.5f)
    {
        return FLT_MAX;  // 无效高度，返回极大代价
    }
    
    float HeightDiff = ToZ - FromZ;
    
    // 坡度惩罚（上坡代价更高）
    if (HeightDiff > 0)
    {
        // 上坡: 每 100cm 高度差增加 100% 代价
        BaseCost *= (1.0f + HeightDiff / 100.f);
    }
    
    return BaseCost;
}

float FMAElevationMapPathfinder::CalculateHeuristic(int32 FromX, int32 FromY, int32 ToX, int32 ToY) const
{
    // 计算水平距离
    float DX = static_cast<float>(ToX - FromX) * Config.ElevationCellSize;
    float DY = static_cast<float>(ToY - FromY) * Config.ElevationCellSize;
    
    // 获取高度差
    float FromZ = ElevationMap.GetGroundHeight(FromX, FromY);
    float ToZ = ElevationMap.GetGroundHeight(ToX, ToY);
    
    float DZ = 0.f;
    if (FromZ < FLT_MAX * 0.5f && ToZ < FLT_MAX * 0.5f)
    {
        DZ = ToZ - FromZ;
    }
    
    // 欧几里得距离（考虑高度差）
    return FMath::Sqrt(DX * DX + DY * DY + DZ * DZ);
}

void FMAElevationMapPathfinder::ReconstructPath(const FIntPoint& TargetGrid)
{
    CachedPath.Empty();
    
    TArray<FVector> ReversePath;
    
    int32 X = TargetGrid.X;
    int32 Y = TargetGrid.Y;
    
    while (X >= 0 && Y >= 0)
    {
        // 生成带高度的路径点
        FVector WorldPos = ElevationMap.GridToWorld(X, Y);
        // GridToWorld 已经设置了正确的 Z 值（地面高度）
        ReversePath.Add(WorldPos);
        
        const FMAElevationCell& Cell = ElevationMap.GetCell(X, Y);
        int32 ParentX = Cell.ParentX;
        int32 ParentY = Cell.ParentY;
        X = ParentX;
        Y = ParentY;
    }
    
    // 反转路径（从起点到终点）
    for (int32 i = ReversePath.Num() - 1; i >= 0; --i)
    {
        CachedPath.Add(ReversePath[i]);
    }
    
    CurrentWaypointIndex = 0;
}

//=============================================================================
// FMAElevationMapPathfinder 调试可视化
//=============================================================================

void FMAElevationMapPathfinder::DrawDebugElevationMap(
    UWorld* World,
    bool bDrawTraversable,
    bool bDrawObstacles,
    bool bDrawNoGround,
    float Duration) const
{
    if (!World || !ElevationMap.IsBuilt())
    {
        return;
    }
    
    const float CellSize = ElevationMap.GetCellSize();
    const float HalfCellSize = CellSize * 0.5f;
    const float BoxHeight = 10.f;  // 绘制的方块高度
    
    for (int32 X = 0; X < ElevationMap.GetWidth(); ++X)
    {
        for (int32 Y = 0; Y < ElevationMap.GetHeight(); ++Y)
        {
            const FMAElevationCell& Cell = ElevationMap.GetCell(X, Y);
            FVector WorldPos = ElevationMap.GridToWorld(X, Y);
            
            FColor DrawColor;
            bool bShouldDraw = false;
            
            if (!Cell.HasGround())
            {
                // 无地面区域 - 黄色
                if (bDrawNoGround)
                {
                    DrawColor = FColor::Yellow;
                    bShouldDraw = true;
                    // 无地面时使用原点高度
                    WorldPos.Z = ElevationMap.GetOrigin().Z;
                }
            }
            else if (Cell.bHasObstacle)
            {
                // 障碍物 - 红色
                if (bDrawObstacles)
                {
                    DrawColor = FColor::Red;
                    bShouldDraw = true;
                }
            }
            else if (Cell.bTraversable)
            {
                // 可通行区域 - 绿色
                if (bDrawTraversable)
                {
                    DrawColor = FColor::Green;
                    bShouldDraw = true;
                }
            }
            
            if (bShouldDraw)
            {
                // 绘制方块表示栅格
                FVector BoxExtent(HalfCellSize * 0.8f, HalfCellSize * 0.8f, BoxHeight);
                DrawDebugBox(
                    World,
                    WorldPos + FVector(0.f, 0.f, BoxHeight),
                    BoxExtent,
                    DrawColor,
                    false,
                    Duration,
                    0,
                    2.f
                );
            }
        }
    }
}

void FMAElevationMapPathfinder::DrawDebugPath(UWorld* World, float Duration) const
{
    if (!World || CachedPath.Num() == 0)
    {
        return;
    }
    
    // 绘制路径线段
    for (int32 i = 0; i < CachedPath.Num() - 1; ++i)
    {
        FVector Start = CachedPath[i] + FVector(0.f, 0.f, 50.f);  // 稍微抬高以便可见
        FVector End = CachedPath[i + 1] + FVector(0.f, 0.f, 50.f);
        
        // 已经走过的路径 - 灰色，未走过的 - 蓝色
        FColor LineColor = (i < CurrentWaypointIndex) ? FColor::Silver : FColor::Blue;
        
        DrawDebugLine(
            World,
            Start,
            End,
            LineColor,
            false,
            Duration,
            0,
            4.f
        );
    }
    
    // 绘制路径点
    for (int32 i = 0; i < CachedPath.Num(); ++i)
    {
        FVector Point = CachedPath[i] + FVector(0.f, 0.f, 50.f);
        
        FColor PointColor;
        float PointSize;
        
        if (i == CurrentWaypointIndex)
        {
            // 当前目标路径点 - 橙色，较大
            PointColor = FColor::Orange;
            PointSize = 20.f;
        }
        else if (i < CurrentWaypointIndex)
        {
            // 已经走过的路径点 - 灰色，较小
            PointColor = FColor::Silver;
            PointSize = 10.f;
        }
        else
        {
            // 未走过的路径点 - 青色
            PointColor = FColor::Cyan;
            PointSize = 15.f;
        }
        
        DrawDebugSphere(
            World,
            Point,
            PointSize,
            8,
            PointColor,
            false,
            Duration,
            0,
            2.f
        );
    }
    
    // 绘制起点和终点标记
    if (CachedPath.Num() > 0)
    {
        // 起点 - 绿色菱形
        FVector StartPoint = CachedPath[0] + FVector(0.f, 0.f, 80.f);
        DrawDebugPoint(World, StartPoint, 25.f, FColor::Green, false, Duration);
        
        // 终点 - 红色菱形
        FVector EndPoint = CachedPath.Last() + FVector(0.f, 0.f, 80.f);
        DrawDebugPoint(World, EndPoint, 25.f, FColor::Red, false, Duration);
    }
}
