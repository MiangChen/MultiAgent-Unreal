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
};
