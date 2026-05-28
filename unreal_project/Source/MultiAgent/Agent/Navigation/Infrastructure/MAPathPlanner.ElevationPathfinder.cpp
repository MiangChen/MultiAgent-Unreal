// MAPathPlanner.ElevationPathfinder.cpp
// Elevation map A* pathfinder implementation and debug drawing

#include "MAPathPlanner.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

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
