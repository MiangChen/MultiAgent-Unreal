// MASkillGeometryUtils.cpp
// 技能层几何计算辅助工具
// 注意: 基础几何函数已统一到 MAGeometryUtils，本文件仅提供技能特定的高级功能

#include "MASkillGeometryUtils.h"
#include "../../../Utils/MAGeometryUtils.h"

TArray<FVector> FMASkillGeometryUtils::GenerateLawnmowerPath(const TArray<FVector>& BoundaryVertices, float ScanWidth)
{
    TArray<FVector> Path;
    if (BoundaryVertices.Num() < 3 || ScanWidth <= 0.f) return Path;
    
    // 最小航点间距，避免导航服务频繁启停
    const float MinWaypointSpacing = 2000.f;
    
    FVector MinBound, MaxBound;
    FMAGeometryUtils::GetPolygonBounds(BoundaryVertices, MinBound, MaxBound);
    
    float Z = FMAGeometryUtils::GetPolygonCenter(BoundaryVertices).Z;
    bool bGoingRight = true;
    
    // Y方向使用扫描宽度（传感器覆盖宽度）
    for (float Y = MinBound.Y; Y <= MaxBound.Y; Y += ScanWidth)
    {
        TArray<FVector> RowPoints;
        
        // X方向使用最小航点间距
        if (bGoingRight)
        {
            for (float X = MinBound.X; X <= MaxBound.X; X += MinWaypointSpacing)
            {
                FVector Point(X, Y, Z);
                if (FMAGeometryUtils::IsPointInPolygon2D(Point, BoundaryVertices))
                {
                    RowPoints.Add(Point);
                }
            }
        }
        else
        {
            for (float X = MaxBound.X; X >= MinBound.X; X -= MinWaypointSpacing)
            {
                FVector Point(X, Y, Z);
                if (FMAGeometryUtils::IsPointInPolygon2D(Point, BoundaryVertices))
                {
                    RowPoints.Add(Point);
                }
            }
        }
        
        Path.Append(RowPoints);
        bGoingRight = !bGoingRight;
    }
    
    return Path;
}

//=============================================================================
// GeneratePatrolWaypoints - 生成巡逻航点 (顶点 + 中心)
//=============================================================================

TArray<FVector> FMASkillGeometryUtils::GeneratePatrolWaypoints(const TArray<FVector>& BoundaryVertices)
{
    TArray<FVector> Waypoints;
    
    if (BoundaryVertices.Num() < 3)
    {
        return Waypoints;
    }
    
    // 添加所有边界顶点作为巡逻点
    Waypoints.Append(BoundaryVertices);
    
    // 添加多边形中心作为额外巡逻点
    FVector Center = FMAGeometryUtils::GetPolygonCenter(BoundaryVertices);
    Waypoints.Add(Center);
    
    return Waypoints;
}

//=============================================================================
// FilterGroundSafeWaypoints - 过滤障碍物内的航点
//=============================================================================

TArray<FVector> FMASkillGeometryUtils::FilterGroundSafeWaypoints(
    const TArray<FVector>& Waypoints,
    const TArray<TArray<FVector>>& ObstaclePolygons)
{
    TArray<FVector> SafeWaypoints;
    
    for (const FVector& Waypoint : Waypoints)
    {
        bool bInsideObstacle = false;
        
        // 检查航点是否在任何障碍物内
        for (const TArray<FVector>& ObstaclePolygon : ObstaclePolygons)
        {
            if (ObstaclePolygon.Num() >= 3 && 
                FMAGeometryUtils::IsPointInPolygon2D(Waypoint, ObstaclePolygon))
            {
                bInsideObstacle = true;
                break;
            }
        }
        
        // 只保留不在障碍物内的航点
        if (!bInsideObstacle)
        {
            SafeWaypoints.Add(Waypoint);
        }
    }
    
    return SafeWaypoints;
}

//=============================================================================
// FindNearestSafePosition - 查找最近安全位置
//=============================================================================

FVector FMASkillGeometryUtils::FindNearestSafePosition(
    const FVector& Position,
    const TArray<TArray<FVector>>& ObstaclePolygons,
    float SafetyMargin)
{
    // 首先检查位置是否已经安全
    bool bInsideAnyObstacle = false;
    int32 ContainingObstacleIndex = -1;
    
    for (int32 i = 0; i < ObstaclePolygons.Num(); ++i)
    {
        const TArray<FVector>& ObstaclePolygon = ObstaclePolygons[i];
        if (ObstaclePolygon.Num() >= 3 && 
            FMAGeometryUtils::IsPointInPolygon2D(Position, ObstaclePolygon))
        {
            bInsideAnyObstacle = true;
            ContainingObstacleIndex = i;
            break;
        }
    }
    
    // 如果位置已经安全，直接返回
    if (!bInsideAnyObstacle)
    {
        return Position;
    }
    
    // 位置在障碍物内，需要找到最近的安全位置
    // 策略: 找到包含该点的障碍物多边形的最近边，然后向外偏移
    const TArray<FVector>& ContainingObstacle = ObstaclePolygons[ContainingObstacleIndex];
    
    FVector NearestPointOnEdge = Position;
    float MinDistSq = MAX_FLT;
    FVector OutwardDirection = FVector::ZeroVector;
    
    // 遍历障碍物的所有边，找到最近的边
    const int32 NumVertices = ContainingObstacle.Num();
    for (int32 i = 0; i < NumVertices; ++i)
    {
        const FVector& V1 = ContainingObstacle[i];
        const FVector& V2 = ContainingObstacle[(i + 1) % NumVertices];
        
        // 计算点到线段的最近点 (2D)
        FVector2D P2D(Position.X, Position.Y);
        FVector2D A2D(V1.X, V1.Y);
        FVector2D B2D(V2.X, V2.Y);
        
        FVector2D AB = B2D - A2D;
        float ABLengthSq = AB.SizeSquared();
        
        FVector2D ClosestPoint2D;
        if (ABLengthSq < KINDA_SMALL_NUMBER)
        {
            // 线段退化为点
            ClosestPoint2D = A2D;
        }
        else
        {
            // 计算投影参数 t
            float t = FVector2D::DotProduct(P2D - A2D, AB) / ABLengthSq;
            t = FMath::Clamp(t, 0.f, 1.f);
            ClosestPoint2D = A2D + AB * t;
        }
        
        float DistSq = FVector2D::DistSquared(P2D, ClosestPoint2D);
        
        if (DistSq < MinDistSq)
        {
            MinDistSq = DistSq;
            NearestPointOnEdge = FVector(ClosestPoint2D.X, ClosestPoint2D.Y, Position.Z);
            
            // 计算向外的方向 (垂直于边，指向多边形外部)
            FVector2D EdgeNormal(-AB.Y, AB.X);
            EdgeNormal.Normalize();
            
            // 确定哪个方向是向外的 (远离多边形中心)
            FVector ObstacleCenter = FMAGeometryUtils::GetPolygonCenter(ContainingObstacle);
            FVector2D ToCenter(ObstacleCenter.X - ClosestPoint2D.X, ObstacleCenter.Y - ClosestPoint2D.Y);
            
            if (FVector2D::DotProduct(EdgeNormal, ToCenter) > 0)
            {
                EdgeNormal = -EdgeNormal; // 反转方向，使其指向外部
            }
            
            OutwardDirection = FVector(EdgeNormal.X, EdgeNormal.Y, 0.f);
        }
    }
    
    // 从最近边上的点向外偏移安全边距
    FVector SafePosition = NearestPointOnEdge + OutwardDirection * SafetyMargin;
    
    // 验证新位置是否真的安全
    bool bStillInsideObstacle = false;
    for (const TArray<FVector>& ObstaclePolygon : ObstaclePolygons)
    {
        if (ObstaclePolygon.Num() >= 3 && 
            FMAGeometryUtils::IsPointInPolygon2D(SafePosition, ObstaclePolygon))
        {
            bStillInsideObstacle = true;
            break;
        }
    }
    
    // 如果仍在障碍物内，增加偏移距离
    if (bStillInsideObstacle)
    {
        SafePosition = NearestPointOnEdge + OutwardDirection * (SafetyMargin * 2.f);
    }
    
    return SafePosition;
}

//=============================================================================
// SortWaypointsByNearestNeighbor - 最近邻排序
//=============================================================================

TArray<FVector> FMASkillGeometryUtils::SortWaypointsByNearestNeighbor(
    const TArray<FVector>& Waypoints,
    const FVector& StartPosition)
{
    TArray<FVector> SortedWaypoints;
    
    if (Waypoints.Num() == 0)
    {
        return SortedWaypoints;
    }
    
    if (Waypoints.Num() == 1)
    {
        SortedWaypoints.Add(Waypoints[0]);
        return SortedWaypoints;
    }
    
    // 复制航点列表用于追踪未访问的航点
    TArray<FVector> Unvisited = Waypoints;
    FVector CurrentPosition = StartPosition;
    
    while (Unvisited.Num() > 0)
    {
        // 找到距离当前位置最近的未访问航点
        int32 NearestIndex = 0;
        float NearestDistSq = FVector::DistSquared(CurrentPosition, Unvisited[0]);
        
        for (int32 i = 1; i < Unvisited.Num(); ++i)
        {
            float DistSq = FVector::DistSquared(CurrentPosition, Unvisited[i]);
            
            // 选择更近的点，距离相同时选择 Y 坐标较小的
            if (DistSq < NearestDistSq ||
                (FMath::IsNearlyEqual(DistSq, NearestDistSq) && 
                 Unvisited[i].Y < Unvisited[NearestIndex].Y))
            {
                NearestIndex = i;
                NearestDistSq = DistSq;
            }
        }
        
        // 将最近的航点添加到结果中
        CurrentPosition = Unvisited[NearestIndex];
        SortedWaypoints.Add(CurrentPosition);
        Unvisited.RemoveAt(NearestIndex);
    }
    
    return SortedWaypoints;
}
