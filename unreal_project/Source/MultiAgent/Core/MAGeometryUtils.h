// MAGeometryUtils.h
// 几何计算工具类
// 用于多选模式的凸包计算和点排序
// Requirements: 6.1, 6.2, 7.1

#pragma once

#include "CoreMinimal.h"

/**
 * 几何计算工具类
 * 提供凸包计算、边界框角点收集、最近邻排序等静态方法
 * 用于多选模式下的 Polygon 和 LineString 生成
 */
class MULTIAGENT_API FMAGeometryUtils
{
public:
    //=========================================================================
    // 凸包计算 (Requirements: 6.2, 6.3)
    //=========================================================================

    /**
     * 计算 2D 凸包 (Graham Scan 算法)
     * @param Points 输入点集 (XY 平面)
     * @return 凸包顶点 (逆时针顺序)
     * 
     * 算法步骤:
     * 1. 找到最低点 (Y 最小，相同 Y 时 X 最小) 作为起点
     * 2. 按极角排序其他点
     * 3. 使用栈构建凸包
     * 4. 返回逆时针顺序的顶点
     */
    static TArray<FVector2D> ComputeConvexHull2D(const TArray<FVector2D>& Points);

    //=========================================================================
    // 边界框角点收集 (Requirements: 6.1)
    //=========================================================================

    /**
     * 从 Actor 列表收集边界框角点
     * @param Actors Actor 列表
     * @return 所有角点的 2D 投影 (XY 平面)
     * 
     * 每个 Actor 的 3D 边界框有 8 个角点，投影到 XY 平面后
     * 可能产生重复点，但不影响凸包计算
     */
    static TArray<FVector2D> CollectBoundingBoxCorners(const TArray<AActor*>& Actors);

    //=========================================================================
    // 最近邻排序 (Requirements: 7.1, 7.2, 7.3)
    //=========================================================================

    /**
     * 最近邻排序 (用于 LineString)
     * @param Points 输入点集
     * @return 排序后的点序列
     * 
     * 算法步骤:
     * 1. 找到最小 X 坐标点作为起点 (相同 X 时选择较小 Y)
     * 2. 贪心选择最近未访问点
     * 3. 等距时选择较小 Y 坐标点
     */
    static TArray<FVector2D> SortByNearestNeighbor(const TArray<FVector2D>& Points);

    //=========================================================================
    // 辅助方法
    //=========================================================================

    /**
     * 计算 2D 叉积 (用于判断点的方向)
     * @param O 原点
     * @param A 点 A
     * @param B 点 B
     * @return 叉积值: >0 表示逆时针, <0 表示顺时针, =0 表示共线
     * 
     * 计算向量 OA 和 OB 的叉积: (A-O) × (B-O)
     */
    static float CrossProduct2D(const FVector2D& O, const FVector2D& A, const FVector2D& B);

private:
    /** 用于极角排序的比较函数 */
    static bool CompareByPolarAngle(const FVector2D& Pivot, const FVector2D& A, const FVector2D& B);
};
