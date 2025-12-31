// MAGeometryUtils.h
// 几何计算工具类
// 用于多选模式的凸包计算和点排序
// Requirements: 6.1, 6.2, 7.1, 3.2, 4.2, 5.2, 5.3, 5.4

#pragma once

#include "CoreMinimal.h"
#include "OrientedBoxTypes.h"

/**
 * Prism 几何数据结构
 * 用于存储棱柱几何信息 (底面多边形 + 高度)
 * Requirements: 6.1, 2.3, 2.4
 */
struct MULTIAGENT_API FMAPrismGeometry
{
    /** 底面顶点 (逆时针顺序) */
    TArray<FVector> BottomVertices;
    
    /** 棱柱高度 (maxZ - minZ) */
    float Height;
    
    /** 是否有效 */
    bool bIsValid;
    
    FMAPrismGeometry()
        : Height(0.0f)
        , bIsValid(false)
    {
    }
};

/**
 * OBB (有向包围盒) 几何数据结构
 * 用于存储 OBB 几何信息
 * Requirements: 3.2, 5.2, 5.3, 5.4
 */
struct MULTIAGENT_API FMAOBBGeometry
{
    /** 底面四个角点 (逆时针顺序) */
    TArray<FVector> CornerPoints;
    
    /** OBB 中心点 */
    FVector Center;
    
    /** OBB 半边长 (Extent) */
    FVector Extent;
    
    /** OBB 方向 (四元数) */
    FQuat Orientation;
    
    /** 是否有效 */
    bool bIsValid;
    
    FMAOBBGeometry()
        : Center(FVector::ZeroVector)
        , Extent(FVector::ZeroVector)
        , Orientation(FQuat::Identity)
        , bIsValid(false)
    {
    }
};

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

    //=========================================================================
    // Prism 几何计算 (Requirements: 6.1, 2.3, 2.4, 6.2, 6.3, 6.4)
    //=========================================================================

    /**
     * 从 Actor 列表计算 Prism 几何
     * @param Actors Actor 列表
     * @return Prism 几何数据 (底面顶点 + 高度)
     * 
     * 算法步骤:
     * 1. 收集所有 Actor 边界框的 3D 顶点
     * 2. 计算高度 (maxZ - minZ)
     * 3. 将顶点投影到 XY 平面
     * 4. 计算凸包作为底面多边形
     * 5. 确保顶点为逆时针顺序
     * 
     * Requirements: 2.3, 2.4, 6.2, 6.3, 6.4
     */
    static FMAPrismGeometry ComputePrismFromActors(const TArray<AActor*>& Actors);

    //=========================================================================
    // OBB 几何计算 (Requirements: 3.2, 5.2, 5.3, 5.4)
    //=========================================================================

    /**
     * 从 Actor 列表计算 OBB (有向包围盒)
     * @param Actors Actor 列表
     * @return OBB 几何数据 (四个角点 + 中心 + 方向)
     * 
     * 算法步骤:
     * 1. 收集所有 Actor 边界框的 3D 顶点
     * 2. 使用 FOrientedBox3d 拟合最小 OBB
     * 3. 提取底面四个角点
     * 
     * Requirements: 3.2, 5.2, 5.3
     */
    static FMAOBBGeometry ComputeOBBFromActors(const TArray<AActor*>& Actors);

    /**
     * 从 FOrientedBox3d 提取底面四个角点
     * @param OBB 有向包围盒
     * @return 底面四个角点 (逆时针顺序)
     * 
     * 算法步骤:
     * 1. 获取 OBB 的 8 个角点
     * 2. 筛选 Z 坐标最小的 4 个点
     * 3. 按逆时针顺序排列
     * 
     * Requirements: 5.4
     */
    static TArray<FVector> ExtractBottomCorners(const UE::Geometry::TOrientedBox3<double>& OBB);

    /**
     * 计算矩形短边的两个中点
     * @param CornerPoints 矩形的四个角点 (逆时针顺序)
     * @return 短边的两个中点
     * 
     * 算法步骤:
     * 1. 计算相邻边的长度
     * 2. 识别短边 (较短的两条对边)
     * 3. 计算短边的中点
     * 
     * 用于 trans_facility 类型的 linestring points 计算
     */
    static TArray<FVector> ComputeShortEdgeMidpoints(const TArray<FVector>& CornerPoints);

    //=========================================================================
    // 几何中心计算 (Requirements: 4.2)
    //=========================================================================

    /**
     * 从 Actor 列表计算几何中心
     * @param Actors Actor 列表
     * @return 所有 Actor 位置的算术平均值
     * 
     * Requirements: 4.2
     */
    static FVector ComputeCenterFromActors(const TArray<AActor*>& Actors);

    //=========================================================================
    // 3D 边界框顶点收集 (Requirements: 3.3, 6.2)
    //=========================================================================

    /**
     * 从 Actor 列表收集 3D 边界框顶点
     * @param Actors Actor 列表
     * @return 所有边界框的 3D 顶点
     * 
     * 每个 Actor 的边界框有 8 个角点
     * 
     * Requirements: 3.3, 6.2
     */
    static TArray<FVector> CollectBoundingBoxVertices3D(const TArray<AActor*>& Actors);

private:
    /** 用于极角排序的比较函数 */
    static bool CompareByPolarAngle(const FVector2D& Pivot, const FVector2D& A, const FVector2D& B);
};
