// MASkillGeometryUtils.h
// 技能层几何计算辅助工具
// 注意: 基础几何函数已统一到 MAGeometryUtils，本文件仅提供技能特定的高级功能

#pragma once

#include "CoreMinimal.h"

/**
 * 技能层几何工具类
 * 仅提供技能执行所需的高级几何计算功能
 * 
 * 注意: 基础几何函数请直接使用 FMAGeometryUtils:
 * - IsPointInPolygon2D() - 点在多边形内判断
 * - GetPolygonBounds() - 多边形边界框
 * - GetPolygonCenter() - 多边形中心
 */
class MULTIAGENT_API FMASkillGeometryUtils
{
public:
    /** 
     * 生成割草机式搜索路径 (技能特定功能)
     * 
     * @param BoundaryVertices 边界多边形顶点
     * @param ScanWidth 扫描宽度
     * @return 搜索路径点数组
     */
    static TArray<FVector> GenerateLawnmowerPath(const TArray<FVector>& BoundaryVertices, float ScanWidth);

    /**
     * 生成巡逻航点 (从边界多边形生成)
     * 包含所有边界顶点和多边形中心点
     * 
     * @param BoundaryVertices 边界多边形顶点
     * @return 巡逻航点数组 (顶点 + 中心)
     */
    static TArray<FVector> GeneratePatrolWaypoints(const TArray<FVector>& BoundaryVertices);

    /**
     * 过滤地面安全航点 (移除障碍物内的点)
     * 用于地面机器人路径规划，确保航点不在障碍物内部
     * 
     * @param Waypoints 原始航点数组
     * @param ObstaclePolygons 障碍物多边形数组 (每个障碍物是一个顶点数组)
     * @return 过滤后的安全航点数组
     */
    static TArray<FVector> FilterGroundSafeWaypoints(
        const TArray<FVector>& Waypoints,
        const TArray<TArray<FVector>>& ObstaclePolygons);

    /**
     * 查找最近的安全位置 (在障碍物外)
     * 如果给定位置在障碍物内，返回最近的障碍物外位置
     * 
     * @param Position 原始位置
     * @param ObstaclePolygons 障碍物多边形数组
     * @param SafetyMargin 安全边距 (默认 100cm)
     * @return 最近的安全位置
     */
    static FVector FindNearestSafePosition(
        const FVector& Position,
        const TArray<TArray<FVector>>& ObstaclePolygons,
        float SafetyMargin = 100.f);

    /**
     * 最近邻排序 (优化巡逻路径)
     * 从起始位置开始，贪心选择最近的未访问航点
     * 
     * @param Waypoints 原始航点数组
     * @param StartPosition 起始位置
     * @return 排序后的航点数组
     */
    static TArray<FVector> SortWaypointsByNearestNeighbor(
        const TArray<FVector>& Waypoints,
        const FVector& StartPosition);
};
